#pragma once

#include <string>
#include <filesystem>
#include <functional>
#include <atomic>
#include <memory>

namespace engine::ecs {

    class Scene;

    class ScriptHotReloadManager {
    public:
        static ScriptHotReloadManager& Instance();

        bool LoadScriptLibrary(const std::filesystem::path& dllPath = {});
        void UnloadScriptLibrary();
        bool ReloadScriptLibrary();

        bool IsLoaded() const { return libraryHandle != nullptr; }
        const std::filesystem::path& GetActiveDllPath() const { return activeDllPath; }
        std::filesystem::path FindScriptDllPath() const;

        bool AreScriptsOutdated(const std::filesystem::path& scriptsDir = {}) const;
        bool RebuildScriptLibrary(std::string* outBuildOutput = nullptr);
        std::filesystem::path FindCMakeBuildDir() const;
        bool CheckAndRebuildIfOutdated(const std::filesystem::path& scriptsDir = {}, std::string* outBuildOutput = nullptr);

        void CleanupTempFiles();

    private:
        ScriptHotReloadManager();
        ~ScriptHotReloadManager();

        void* libraryHandle{nullptr};
        std::filesystem::path activeDllPath;
        std::filesystem::path cachedOriginalDllPath;
    };

} // namespace engine::ecs
