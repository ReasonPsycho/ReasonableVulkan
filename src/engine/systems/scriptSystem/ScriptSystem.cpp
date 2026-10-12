#include "ScriptSystem.hpp"
#include "ScriptRegistry.hpp"
#include "ecs/Scene.h"

using namespace engine::ecs;

ScriptSystem::ScriptSystem(Scene* scene) : System(scene) {
    InitializeScriptRegistry();
}

void ScriptSystem::Update(float deltaTime) {
    auto scriptCompArray = scene->GetComponentArray<ScriptComponent>();
    if (!scriptCompArray) return;

    auto& scriptComponents = scriptCompArray->GetComponents();
    size_t count = scriptCompArray->GetArraySize();

    for (size_t i = 0; i < count; ++i) {
        if (!scriptCompArray->IsComponentActive(i)) continue;
        Entity entity = scriptCompArray->ComponentIndexToEntity(i);
        if (!scene->IsEntityActive(entity)) continue;

        auto& comp = scriptComponents[i];
        for (auto& script : comp.scripts) {
            if (!script || !script->IsEnabled()) continue;

            if (!script->scene) {
                script->scene = scene;
                script->entity = entity;
                script->OnAwake();
            }

            if (!script->started) {
                script->started = true;
                script->OnStart();
            }

            script->OnUpdate(deltaTime);
        }
    }
}

void ScriptSystem::OnEntityRemoved(ComponentID componentID, std::type_index /*type*/) {
    auto scriptCompArray = scene->GetComponentArray<ScriptComponent>();
    if (!scriptCompArray) return;

    if (scriptCompArray->HasComponent(componentID)) {
        auto& comp = scriptCompArray->GetComponent(componentID);
        for (auto& script : comp.scripts) {
            if (script) {
                script->OnDestroy();
            }
        }
    }
}
