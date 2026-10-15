#include "EditorCommands.hpp"
#include "ecs/Scene.h"
#include "EditorSystem.hpp"
#include "systems/transformSystem/componets/TransformComponent.hpp"

namespace engine::editor
{
    // ==========================================
    // EntityTransformCommand
    // ==========================================
    EntityTransformCommand::EntityTransformCommand(ecs::Scene* scene, ecs::Entity entity,
                                                   const glm::mat4& oldLocal, const glm::mat4& newLocal,
                                                   const glm::mat4& oldGlobal, const glm::mat4& newGlobal)
        : scene(scene), entity(entity),
          oldLocalMatrix(oldLocal), newLocalMatrix(newLocal),
          oldGlobalMatrix(oldGlobal), newGlobalMatrix(newGlobal)
    {
    }

    void EntityTransformCommand::Execute()
    {
        if (!scene || entity == ecs::MAX_ENTITIES) return;
        if (!scene->HasComponent<ecs::TransformComponent>(entity)) return;

        auto transformArray = scene->GetIntegralComponentArray<ecs::TransformComponent>();
        if (!transformArray) return;

        auto& transform = transformArray->GetComponentFromEntity(entity);
        setLocalMatrix(transform, newLocalMatrix);
    }

    void EntityTransformCommand::Undo()
    {
        if (!scene || entity == ecs::MAX_ENTITIES) return;
        if (!scene->HasComponent<ecs::TransformComponent>(entity)) return;

        auto transformArray = scene->GetIntegralComponentArray<ecs::TransformComponent>();
        if (!transformArray) return;

        auto& transform = transformArray->GetComponentFromEntity(entity);
        setLocalMatrix(transform, oldLocalMatrix);
    }

    std::string EntityTransformCommand::GetName() const
    {
        return "Transform Entity #" + std::to_string(entity);
    }

    // ==========================================
    // EntitiesTransformCommand
    // ==========================================
    EntitiesTransformCommand::EntitiesTransformCommand(ecs::Scene* scene, const std::vector<EntityTransformRecord>& records)
        : scene(scene), records(records)
    {
    }

    void EntitiesTransformCommand::Execute()
    {
        if (!scene) return;
        auto transformArray = scene->GetIntegralComponentArray<ecs::TransformComponent>();
        if (!transformArray) return;

        for (const auto& rec : records)
        {
            if (rec.entity == ecs::MAX_ENTITIES) continue;
            if (!scene->HasComponent<ecs::TransformComponent>(rec.entity)) continue;

            auto& transform = transformArray->GetComponentFromEntity(rec.entity);
            setLocalMatrix(transform, rec.newLocalMatrix);
        }
    }

    void EntitiesTransformCommand::Undo()
    {
        if (!scene) return;
        auto transformArray = scene->GetIntegralComponentArray<ecs::TransformComponent>();
        if (!transformArray) return;

        for (const auto& rec : records)
        {
            if (rec.entity == ecs::MAX_ENTITIES) continue;
            if (!scene->HasComponent<ecs::TransformComponent>(rec.entity)) continue;

            auto& transform = transformArray->GetComponentFromEntity(rec.entity);
            setLocalMatrix(transform, rec.oldLocalMatrix);
        }
    }

    std::string EntitiesTransformCommand::GetName() const
    {
        if (records.size() == 1)
        {
            return "Transform Entity #" + std::to_string(records[0].entity);
        }
        return "Transform " + std::to_string(records.size()) + " Entities";
    }

    // ==========================================
    // CreateEntityCommand
    // ==========================================
    CreateEntityCommand::CreateEntityCommand(ecs::Scene* scene, ecs::Entity entity, ecs::Entity parent, const std::string& name)
        : scene(scene), entity(entity), parent(parent), name(name)
    {
    }

    void CreateEntityCommand::Execute()
    {
        if (!scene) return;
        if (hasSnapshot && snapshotDoc)
        {
            entity = scene->DeserializeObjectFromJson(*snapshotDoc, parent);
        }
        else
        {
            scene->SetEntityActive(entity, true);
        }
    }

    void CreateEntityCommand::Undo()
    {
        if (!scene || entity == ecs::MAX_ENTITIES) return;

        if (!hasSnapshot)
        {
            snapshotDoc = std::make_shared<rapidjson::Document>();
            scene->SerializeObjectToJson(entity, *snapshotDoc);
            hasSnapshot = true;
        }

        scene->DestroyEntity(entity);
    }

    std::string CreateEntityCommand::GetName() const
    {
        return "Create Entity " + name;
    }

    // ==========================================
    // DeleteEntitiesCommand
    // ==========================================
    DeleteEntitiesCommand::DeleteEntitiesCommand(ecs::Scene* scene, const std::vector<ecs::EntityID>& entities, ecs::EditorSystem* editor)
        : scene(scene), editor(editor), targetEntities(entities)
    {
        if (scene)
        {
            for (ecs::EntityID e : targetEntities)
            {
                if (!scene->IsEntityActive(e) && !scene->HasComponent<ecs::TransformComponent>(e)) continue;

                EntityDeleteSnapshot snap;
                snap.originalEntity = e;
                snap.parent = scene->GetParent(e);
                snap.name = editor ? editor->GetEntityRawName(e, scene) : scene->GetEntityName(e);
                snap.wasActive = scene->IsEntityActive(e);
                snap.doc = std::make_shared<rapidjson::Document>();
                scene->SerializeObjectToJson(e, *snap.doc);
                snapshots.push_back(snap);
            }
        }
    }

    void DeleteEntitiesCommand::Execute()
    {
        if (!scene) return;

        for (ecs::EntityID e : targetEntities)
        {
            scene->DestroyEntity(e);
        }

        if (editor)
        {
            editor->ClearSelectedEntities();
        }
    }

    void DeleteEntitiesCommand::Undo()
    {
        if (!scene) return;

        std::vector<ecs::EntityID> restoredEntities;
        for (auto& snap : snapshots)
        {
            if (snap.doc)
            {
                ecs::Entity restored = scene->DeserializeObjectFromJson(*snap.doc, snap.parent);
                if (restored != ecs::MAX_ENTITIES)
                {
                    if (editor && !snap.name.empty())
                    {
                        editor->SetEntityName(restored.GetID(), snap.name, scene);
                    }
                    scene->SetEntityActive(restored.GetID(), snap.wasActive);
                    restoredEntities.push_back(restored.GetID());
                }
            }
        }

        targetEntities = restoredEntities;

        if (editor && !restoredEntities.empty())
        {
            editor->SetSelectedEntities(restoredEntities);
        }
    }

    std::string DeleteEntitiesCommand::GetName() const
    {
        if (snapshots.size() == 1)
        {
            return "Delete Entity '" + snapshots[0].name + "'";
        }
        return "Delete " + std::to_string(snapshots.size()) + " Entities";
    }

    // ==========================================
    // ReparentEntityCommand
    // ==========================================
    ReparentEntityCommand::ReparentEntityCommand(ecs::Scene* scene, ecs::Entity child, ecs::Entity oldParent, ecs::Entity newParent)
        : scene(scene), child(child), oldParent(oldParent), newParent(newParent)
    {
    }

    void ReparentEntityCommand::Execute()
    {
        if (!scene || child == ecs::MAX_ENTITIES) return;

        if (newParent != ecs::MAX_ENTITIES && newParent != (ecs::Entity)-1)
        {
            scene->SetParent(child, newParent);
        }
        else
        {
            scene->RemoveParent(child);
        }
    }

    void ReparentEntityCommand::Undo()
    {
        if (!scene || child == ecs::MAX_ENTITIES) return;

        if (oldParent != ecs::MAX_ENTITIES && oldParent != (ecs::Entity)-1)
        {
            scene->SetParent(child, oldParent);
        }
        else
        {
            scene->RemoveParent(child);
        }
    }

    std::string ReparentEntityCommand::GetName() const
    {
        return "Reparent Entity #" + std::to_string(child);
    }

    // ==========================================
    // RenameEntityCommand
    // ==========================================
    RenameEntityCommand::RenameEntityCommand(ecs::EditorSystem* editor, ecs::Scene* scene, ecs::Entity entity, const std::string& oldName, const std::string& newName)
        : editor(editor), scene(scene), entity(entity), oldName(oldName), newName(newName)
    {
    }

    void RenameEntityCommand::Execute()
    {
        if (editor)
        {
            editor->SetEntityName(entity, newName, scene);
        }
        else if (scene)
        {
            scene->SetEntityName(entity, newName);
        }
    }

    void RenameEntityCommand::Undo()
    {
        if (editor)
        {
            editor->SetEntityName(entity, oldName, scene);
        }
        else if (scene)
        {
            scene->SetEntityName(entity, oldName);
        }
    }

    std::string RenameEntityCommand::GetName() const
    {
        return "Rename Entity to '" + newName + "'";
    }

    // ==========================================
    // SetEntityActiveCommand
    // ==========================================
    SetEntityActiveCommand::SetEntityActiveCommand(ecs::Scene* scene, const std::vector<std::pair<ecs::Entity, bool>>& entitiesOldStates, bool newActive)
        : scene(scene), oldStates(entitiesOldStates), newActive(newActive)
    {
    }

    void SetEntityActiveCommand::Execute()
    {
        if (!scene) return;
        for (const auto& [e, _] : oldStates)
        {
            scene->SetEntityActive(e, newActive);
        }
    }

    void SetEntityActiveCommand::Undo()
    {
        if (!scene) return;
        for (const auto& [e, oldVal] : oldStates)
        {
            scene->SetEntityActive(e, oldVal);
        }
    }

    std::string SetEntityActiveCommand::GetName() const
    {
        return std::string(newActive ? "Activate " : "Deactivate ") + (oldStates.size() > 1 ? (std::to_string(oldStates.size()) + " Entities") : "Entity");
    }

    // ==========================================
    // AddComponentCommand
    // ==========================================
    AddComponentCommand::AddComponentCommand(ecs::Scene* scene, ecs::Entity entity, std::type_index typeIdx, const std::string& typeDisplayName)
        : scene(scene), entity(entity), typeIdx(typeIdx), typeDisplayName(typeDisplayName)
    {
    }

    void AddComponentCommand::Execute()
    {
        if (!scene || entity == ecs::MAX_ENTITIES) return;
        scene->AddComponent(entity, typeIdx);
    }

    void AddComponentCommand::Undo()
    {
        if (!scene || entity == ecs::MAX_ENTITIES) return;
        auto compArrays = scene->GetComponentArrays();
        auto it = compArrays.find(typeIdx);
        if (it != compArrays.end())
        {
            it->second->RemoveComponentUntyped(entity);
        }
    }

    std::string AddComponentCommand::GetName() const
    {
        return "Add " + typeDisplayName + " to #" + std::to_string(entity);
    }

    // ==========================================
    // RemoveComponentCommand
    // ==========================================
    RemoveComponentCommand::RemoveComponentCommand(ecs::Scene* scene, ecs::Entity entity, std::type_index typeIdx, const std::string& typeDisplayName, std::shared_ptr<rapidjson::Document> serializedDoc)
        : scene(scene), entity(entity), typeIdx(typeIdx), typeDisplayName(typeDisplayName), doc(serializedDoc)
    {
    }

    void RemoveComponentCommand::Execute()
    {
        if (!scene || entity == ecs::MAX_ENTITIES) return;
        auto compArrays = scene->GetComponentArrays();
        auto it = compArrays.find(typeIdx);
        if (it != compArrays.end())
        {
            it->second->RemoveComponentUntyped(entity);
        }
    }

    void RemoveComponentCommand::Undo()
    {
        if (!scene || entity == ecs::MAX_ENTITIES) return;
        scene->AddComponent(entity, typeIdx);
    }

    std::string RemoveComponentCommand::GetName() const
    {
        return "Remove " + typeDisplayName + " from #" + std::to_string(entity);
    }

    // ==========================================
    // InstantiateEntitiesCommand
    // ==========================================
    InstantiateEntitiesCommand::InstantiateEntitiesCommand(ecs::Scene* scene, const std::vector<ecs::EntityID>& createdEntities, const std::string& sourceName, ecs::EditorSystem* editor)
        : scene(scene), editor(editor), entities(createdEntities), operationName(sourceName)
    {
    }

    void InstantiateEntitiesCommand::Execute()
    {
        if (!scene) return;
        if (!snapshots.empty())
        {
            std::vector<ecs::EntityID> restored;
            for (auto& snap : snapshots)
            {
                if (snap.doc)
                {
                    ecs::Entity r = scene->DeserializeObjectFromJson(*snap.doc, snap.parent);
                    if (r != ecs::MAX_ENTITIES)
                    {
                        if (editor && !snap.name.empty())
                        {
                            editor->SetEntityName(r.GetID(), snap.name, scene);
                        }
                        scene->SetEntityActive(r.GetID(), snap.wasActive);
                        restored.push_back(r.GetID());
                    }
                }
            }
            entities = restored;
            if (editor && !restored.empty())
            {
                editor->SetSelectedEntities(restored);
            }
        }
    }

    void InstantiateEntitiesCommand::Undo()
    {
        if (!scene) return;

        if (snapshots.empty())
        {
            for (ecs::EntityID e : entities)
            {
                if (!scene->IsEntityActive(e) && !scene->HasComponent<ecs::TransformComponent>(e)) continue;

                EntityDeleteSnapshot snap;
                snap.originalEntity = e;
                snap.parent = scene->GetParent(e);
                snap.name = editor ? editor->GetEntityRawName(e, scene) : scene->GetEntityName(e);
                snap.wasActive = scene->IsEntityActive(e);
                snap.doc = std::make_shared<rapidjson::Document>();
                scene->SerializeObjectToJson(e, *snap.doc);
                snapshots.push_back(snap);
            }
        }

        for (ecs::EntityID e : entities)
        {
            scene->DestroyEntity(e);
        }

        if (editor)
        {
            editor->ClearSelectedEntities();
        }
    }

    std::string InstantiateEntitiesCommand::GetName() const
    {
        if (operationName.rfind("Instantiate ", 0) == 0 ||
            operationName.rfind("Paste ", 0) == 0 ||
            operationName.rfind("Duplicate ", 0) == 0 ||
            operationName.rfind("Pasted ", 0) == 0 ||
            operationName.rfind("Duplicated ", 0) == 0)
        {
            return operationName;
        }
        return "Instantiate " + operationName;
    }

    // ==========================================
    // LambdaCommand
    // ==========================================
    LambdaCommand::LambdaCommand(std::string name, std::function<void()> onExecute, std::function<void()> onUndo)
        : name(std::move(name)), onExecute(std::move(onExecute)), onUndo(std::move(onUndo))
    {
    }

    void LambdaCommand::Execute()
    {
        if (onExecute) onExecute();
    }

    void LambdaCommand::Undo()
    {
        if (onUndo) onUndo();
    }

    std::string LambdaCommand::GetName() const
    {
        return name;
    }
}
