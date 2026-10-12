#include "ScriptRegistry.hpp"

namespace engine::ecs {

    void AutoRegisterGeneratedScripts();

    void InitializeScriptRegistry() {
        static bool initialized = false;
        if (initialized) return;
        initialized = true;

        AutoRegisterGeneratedScripts();
    }

} // namespace engine::ecs
