#include "ScriptRegistry.hpp"
#include "ScriptHotReloadManager.hpp"

namespace engine::ecs {

    void InitializeScriptRegistry() {
        static bool initialized = false;
        if (initialized) return;
        initialized = true;

        ScriptHotReloadManager::Instance().LoadScriptLibrary();
    }

} // namespace engine::ecs
