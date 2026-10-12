#include "ScriptSystem.hpp"
#include "ScriptRegistry.hpp"
#include "ScriptHotReloadManager.hpp"
#include "ecs/Scene.h"
#include <spdlog/spdlog.h>

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
            if (!script) continue;

            if (!script->scene) {
                script->scene = scene;
                script->entity = entity;
                script->OnAwake();
            }

            if (!script->IsEnabled()) continue;

            if (!script->started) {
                script->started = true;
                script->OnStart();
            }

            script->OnUpdate(deltaTime);
        }
    }
}

rapidjson::Document ScriptSystem::BeginHotReload() {
    rapidjson::Document snapshot(rapidjson::kObjectType);
    auto& allocator = snapshot.GetAllocator();
    rapidjson::Value entitiesArray(rapidjson::kArrayType);

    auto scriptCompArray = scene->GetComponentArray<ScriptComponent>();
    if (scriptCompArray) {
        size_t count = scriptCompArray->GetArraySize();
        auto& scriptComponents = scriptCompArray->GetComponents();

        for (size_t i = 0; i < count; ++i) {
            if (!scriptCompArray->IsComponentActive(i)) continue;
            Entity entity = scriptCompArray->ComponentIndexToEntity(i);

            auto& comp = scriptComponents[i];
            rapidjson::Value entityObj(rapidjson::kObjectType);
            entityObj.AddMember("entity", static_cast<uint32_t>(entity), allocator);

            rapidjson::Value compObj(rapidjson::kObjectType);
            comp.CaptureStateSnapshot(compObj, allocator);
            entityObj.AddMember("scriptComponent", compObj, allocator);

            entitiesArray.PushBack(entityObj, allocator);

            for (auto& script : comp.scripts) {
                if (script) {
                    script->OnDestroy();
                }
            }
            comp.scripts.clear();
        }
    }

    snapshot.AddMember("entities", entitiesArray, allocator);
    return snapshot;
}

void ScriptSystem::EndHotReload(const rapidjson::Document& snapshot) {
    if (!snapshot.IsObject() || !snapshot.HasMember("entities") || !snapshot["entities"].IsArray()) {
        return;
    }

    const auto& entitiesArray = snapshot["entities"];
    for (rapidjson::SizeType i = 0; i < entitiesArray.Size(); ++i) {
        const auto& entityItem = entitiesArray[i];
        if (!entityItem.IsObject() || !entityItem.HasMember("entity") || !entityItem["entity"].IsUint()) {
            continue;
        }

        Entity entity = static_cast<Entity>(entityItem["entity"].GetUint());
        if (!scene->HasComponent<ScriptComponent>(entity)) {
            continue;
        }

        auto& comp = scene->GetComponent<ScriptComponent>(entity);
        if (entityItem.HasMember("scriptComponent")) {
            comp.RestoreStateSnapshot(entityItem["scriptComponent"], scene);
        }

        for (auto& script : comp.scripts) {
            if (script) {
                script->scene = scene;
                script->entity = entity;
                script->OnAwake();
                if (script->IsEnabled()) {
                    script->started = true;
                    script->OnStart();
                }
            }
        }
    }
}

bool ScriptSystem::PerformHotReload() {
    spdlog::info("ScriptSystem: Beginning hot reload state snapshot...");
    auto snapshot = BeginHotReload();

    bool reloadSuccess = ScriptHotReloadManager::Instance().ReloadScriptLibrary();

    EndHotReload(snapshot);
    if (reloadSuccess) {
        spdlog::info("ScriptSystem: Hot reload completed successfully");
    } else {
        spdlog::warn("ScriptSystem: Hot reload finished with library reload errors");
    }
    return reloadSuccess;
}

bool ScriptSystem::PerformHotReloadForScene(Scene* targetScene) {
    if (!targetScene) return false;
    auto scriptSystem = targetScene->GetSystem<ScriptSystem>();
    if (scriptSystem) {
        return scriptSystem->PerformHotReload();
    } else {
        return ScriptHotReloadManager::Instance().ReloadScriptLibrary();
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
