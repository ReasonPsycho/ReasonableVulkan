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

    std::filesystem::path ScriptHotReloadManager::FindCMakeBuildDir() const {
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
            exeDir,
            std::filesystem::current_path(),
            std::filesystem::current_path() / "out" / "build" / "tests",
            std::filesystem::current_path() / "out" / "build" / "debug",
            std::filesystem::current_path() / "out" / "build" / "release",
            std::filesystem::current_path() / "build",
            exeDir.parent_path(),
            exeDir.parent_path().parent_path(),
            exeDir.parent_path().parent_path().parent_path()
        };

        for (const auto& candidate : candidates) {
            std::error_code ec;
            if (std::filesystem::exists(candidate / "CMakeCache.txt", ec)) {
                return std::filesystem::absolute(candidate);
            }
        }

        return exeDir;
    }

    bool ScriptHotReloadManager::AreScriptsOutdated(const std::filesystem::path& scriptsDir) const {
        std::filesystem::path targetDir = scriptsDir;
        if (targetDir.empty()) {
            if (std::filesystem::exists("res/scripts")) {
                targetDir = "res/scripts";
            } else if (std::filesystem::exists("C:\\Users\\redkc\\CLionProjects\\ReasonableVulkanPublic\\res\\scripts")) {
                targetDir = "C:\\Users\\redkc\\CLionProjects\\ReasonableVulkanPublic\\res\\scripts";
            }
        }

        std::error_code ec;
        if (!std::filesystem::exists(targetDir, ec)) {
            return false;
        }

        std::filesystem::path dllPath = FindScriptDllPath();
        if (dllPath.empty() || !std::filesystem::exists(dllPath, ec)) {
            return true;
        }

        auto dllTime = std::filesystem::last_write_time(dllPath, ec);
        if (ec) return true;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(targetDir, ec)) {
            if (entry.is_regular_file(ec)) {
                auto ext = entry.path().extension().string();
                if (ext == ".hpp" || ext == ".cpp" || ext == ".h" || ext == ".c" || ext == ".cxx" || ext == ".cc") {
                    auto srcTime = std::filesystem::last_write_time(entry.path(), ec);
                    if (!ec && srcTime > dllTime) {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    bool ScriptHotReloadManager::RebuildScriptLibrary(std::string* outBuildOutput) {
        std::filesystem::path buildDir = FindCMakeBuildDir();
        std::string cmd;
        if (!buildDir.empty()) {
            cmd = "cmake --build \"" + buildDir.string() + "\" --target gameplay_scripts 2>&1";
        } else {
            cmd = "cmake --build . --target gameplay_scripts 2>&1";
        }

        spdlog::info("ScriptHotReloadManager: Rebuilding gameplay_scripts: {}", cmd);

        std::string output;
        int exitCode = -1;

#if defined(_WIN32)
        FILE* pipe = _popen(cmd.c_str(), "r");
#else
        FILE* pipe = popen(cmd.c_str(), "r");
#endif
        if (pipe) {
            char buffer[512];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                output += buffer;
            }
#if defined(_WIN32)
            exitCode = _pclose(pipe);
#else
            exitCode = pclose(pipe);
#endif
        } else {
            output = "Failed to spawn cmake build process.";
        }

        if (outBuildOutput) {
            *outBuildOutput = output;
        }

        if (exitCode == 0) {
            spdlog::info("ScriptHotReloadManager: Script compilation succeeded");
            return true;
        } else {
            spdlog::error("ScriptHotReloadManager: Script compilation failed with exit code {}:\n{}", exitCode, output);
            return false;
        }
    }

    bool ScriptHotReloadManager::CheckAndRebuildIfOutdated(const std::filesystem::path& scriptsDir, std::string* outBuildOutput) {
        if (AreScriptsOutdated(scriptsDir)) {
            spdlog::info("ScriptHotReloadManager: Scripts are outdated or missing, rebuilding...");
            bool success = RebuildScriptLibrary(outBuildOutput);
            if (success) {
                ReloadScriptLibrary();
            }
            return success;
        }
        return false;
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

        using GetCountFn = size_t (*)();
        using GetEntriesFn = const ScriptExportEntry* (*)();

        auto getCount = reinterpret_cast<GetCountFn>(GetProcAddress(hModule, "GetDynamicScriptCount"));
        auto getEntries = reinterpret_cast<GetEntriesFn>(GetProcAddress(hModule, "GetDynamicScriptEntries"));

        if (!getCount || !getEntries) {
            spdlog::error("ScriptHotReloadManager: Could not find script export entry points in {}",
                          shadowPath.string());
            FreeLibrary(hModule);
            std::filesystem::remove(shadowPath, ec);
            return false;
        }

        UnloadScriptLibrary();

        libraryHandle = hModule;
        activeDllPath = shadowPath;

        size_t count = getCount();
        const ScriptExportEntry* entries = getEntries();
        for (size_t i = 0; i < count; ++i) {
            if (entries[i].name) {
                ScriptRegistry::Instance().RegisterScriptRaw(
                    entries[i].name,
                    entries[i].create,
                    entries[i].serialize,
                    entries[i].deserialize
#ifdef ENABLE_IMGUI
                    , entries[i].drawInspector
#endif
                );
            }
        }

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
