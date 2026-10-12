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

        void CleanupTempFiles();

    private:
        ScriptHotReloadManager();
        ~ScriptHotReloadManager();

        void* libraryHandle{nullptr};
        std::filesystem::path activeDllPath;
        std::filesystem::path cachedOriginalDllPath;
    };

} // namespace engine::ecs
