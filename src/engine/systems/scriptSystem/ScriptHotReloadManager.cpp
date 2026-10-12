#include "ScriptHotReloadManager.hpp"
#include "ScriptRegistry.hpp"
#include <spdlog/spdlog.h>
#include <chrono>

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#endif

namespace engine::ecs {

    ScriptHotReloadManager& ScriptHotReloadManager::Instance() {
        static ScriptHotReloadManager instance;
        return instance;
    }

    ScriptHotReloadManager::ScriptHotReloadManager() {
        CleanupTempFiles();
    }

    ScriptHotReloadManager::~ScriptHotReloadManager() {
        UnloadScriptLibrary();
        CleanupTempFiles();
    }

    std::filesystem::path ScriptHotReloadManager::FindScriptDllPath() const {
        if (!cachedOriginalDllPath.empty() && std::filesystem::exists(cachedOriginalDllPath)) {
            return cachedOriginalDllPath;
        }

        std::filesystem::path exeDir;
#if defined(_WIN32)
        char modulePath[MAX_PATH];
        if (GetModuleFileNameA(nullptr, modulePath, MAX_PATH) > 0) {
            exeDir = std::filesystem::path(modulePath).parent_path();
        }
#endif
        if (exeDir.empty()) {
            exeDir = std::filesystem::current_path();
        }

        const std::vector<std::filesystem::path> candidates = {
            exeDir / "gameplay_scripts.dll",
            exeDir / "libgameplay_scripts.dll",
            exeDir / "src" / "engine" / "gameplay_scripts.dll",
            exeDir / "src" / "engine" / "libgameplay_scripts.dll",
            std::filesystem::current_path() / "gameplay_scripts.dll",
            std::filesystem::current_path() / "libgameplay_scripts.dll",
            std::filesystem::current_path() / "src" / "engine" / "gameplay_scripts.dll",
            std::filesystem::current_path() / "src" / "engine" / "libgameplay_scripts.dll",
            std::filesystem::current_path() / "out" / "build" / "debug" / "src" / "engine" / "libgameplay_scripts.dll",
            std::filesystem::current_path() / "out" / "build" / "debug" / "src" / "engine" / "gameplay_scripts.dll",
            std::filesystem::current_path() / "out" / "build" / "debug" / "libgameplay_scripts.dll",
            std::filesystem::current_path() / "out" / "build" / "debug" / "gameplay_scripts.dll",
            std::filesystem::current_path() / "build" / "src" / "engine" / "libgameplay_scripts.dll",
            std::filesystem::current_path() / "build" / "src" / "engine" / "gameplay_scripts.dll"
        };

        for (const auto& candidate : candidates) {
            std::error_code ec;
            if (std::filesystem::exists(candidate, ec) && !std::filesystem::is_directory(candidate, ec)) {
                return std::filesystem::absolute(candidate);
            }
        }

        return {};
    }

    void ScriptHotReloadManager::CleanupTempFiles() {
        std::filesystem::path exeDir;
#if defined(_WIN32)
        char modulePath[MAX_PATH];
        if (GetModuleFileNameA(nullptr, modulePath, MAX_PATH) > 0) {
            exeDir = std::filesystem::path(modulePath).parent_path();
        }
#endif
        if (exeDir.empty()) {
            exeDir = std::filesystem::current_path();
        }

        std::error_code ec;
        if (std::filesystem::exists(exeDir, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(exeDir, ec)) {
                if (entry.is_regular_file(ec)) {
                    std::string filename = entry.path().filename().string();
                    if (filename.rfind("temp_scripts_", 0) == 0 && entry.path().extension() == ".dll") {
                        if (entry.path() != activeDllPath) {
                            std::error_code rmEc;
                            std::filesystem::remove(entry.path(), rmEc);
                        }
                    }
                }
            }
        }
    }

    bool ScriptHotReloadManager::LoadScriptLibrary(const std::filesystem::path& dllPath) {
        std::filesystem::path targetPath = dllPath.empty() ? FindScriptDllPath() : dllPath;
        if (targetPath.empty()) {
            spdlog::warn("ScriptHotReloadManager: Could not find gameplay_scripts.dll binary");
            return false;
        }

        std::error_code ec;
        if (!std::filesystem::exists(targetPath, ec)) {
            spdlog::error("ScriptHotReloadManager: DLL path does not exist: {}", targetPath.string());
            return false;
        }

        cachedOriginalDllPath = targetPath;

        // Create shadow copy to prevent Windows file locking
        std::filesystem::path dir = targetPath.parent_path();
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        std::filesystem::path shadowPath = dir / ("temp_scripts_" + std::to_string(now) + ".dll");

        std::filesystem::copy_file(targetPath, shadowPath, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            spdlog::error("ScriptHotReloadManager: Failed to shadow copy DLL from {} to {}: {}",
                          targetPath.string(), shadowPath.string(), ec.message());
            return false;
        }

#if defined(_WIN32)
        HMODULE hModule = LoadLibraryA(shadowPath.string().c_str());
        if (!hModule) {
            DWORD error = GetLastError();
            spdlog::error("ScriptHotReloadManager: LoadLibraryA failed for {} with error code {}",
                          shadowPath.string(), error);
            std::filesystem::remove(shadowPath, ec);
            return false;
        }

        auto regFn = reinterpret_cast<RegisterDynamicScriptsFn>(GetProcAddress(hModule, "RegisterDynamicScripts"));
        if (!regFn) {
            spdlog::error("ScriptHotReloadManager: Could not find 'RegisterDynamicScripts' entry point in {}",
                          shadowPath.string());
            FreeLibrary(hModule);
            std::filesystem::remove(shadowPath, ec);
            return false;
        }

        UnloadScriptLibrary();

        libraryHandle = hModule;
        activeDllPath = shadowPath;

        regFn(&ScriptRegistry::Instance());
        spdlog::info("ScriptHotReloadManager: Successfully loaded and registered dynamic scripts from {}",
                     shadowPath.string());
        return true;
#else
        spdlog::warn("ScriptHotReloadManager: Dynamic reloading is only implemented for Windows currently");
        return false;
#endif
    }

    void ScriptHotReloadManager::UnloadScriptLibrary() {
        ScriptRegistry::Instance().Clear();

#if defined(_WIN32)
        if (libraryHandle) {
            FreeLibrary(static_cast<HMODULE>(libraryHandle));
            libraryHandle = nullptr;
        }
#endif

        if (!activeDllPath.empty()) {
            std::error_code ec;
            std::filesystem::remove(activeDllPath, ec);
            activeDllPath.clear();
        }
    }

    bool ScriptHotReloadManager::ReloadScriptLibrary() {
        std::filesystem::path target = cachedOriginalDllPath.empty() ? FindScriptDllPath() : cachedOriginalDllPath;
        return LoadScriptLibrary(target);
    }

} // namespace engine::ecs
