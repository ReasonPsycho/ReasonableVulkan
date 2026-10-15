#include "ScriptRegistry.hpp"
#include "ScriptHotReloadManager.hpp"

namespace engine::ecs {

    void ScriptRegistry::RegisterScriptRaw(std::string_view name,
                                           ScriptCreateFn create,
                                           ScriptSerializeFn serialize,
                                           ScriptDeserializeFn deserialize
#ifdef ENABLE_IMGUI
                                           , ScriptDrawInspectorFn drawInspector
#endif
    ) {
        std::string nameStr(name);
        ScriptTypeInfo info;
        info.name = nameStr;
        info.create = create;
        info.serialize = serialize;
        info.deserialize = deserialize;
#ifdef ENABLE_IMGUI
        info.drawInspector = drawInspector;
#endif
        typeInfos[nameStr] = std::move(info);
    }

    void InitializeScriptRegistry() {
        static bool initialized = false;
        if (initialized) return;
        initialized = true;

        ScriptHotReloadManager::Instance().LoadScriptLibrary();
    }

} // namespace engine::ecs
