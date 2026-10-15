#pragma once

#include "CommandHistory.hpp"
#include "ecs/Types.h"
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <rapidjson/document.h>
#include <typeindex>
#include <string>
#include <vector>
#include <memory>
#include <utility>

namespace engine::ecs
{
    class Scene;
    class EditorSystem;
}

namespace engine::editor
{
    // Command for Gizmo and Inspector transform changes
    class EntityTransformCommand : public ICommand
    {
    public:
        EntityTransformCommand(ecs::Scene* scene, ecs::Entity entity,
                               const glm::mat4& oldLocal, const glm::mat4& newLocal,
                               const glm::mat4& oldGlobal, const glm::mat4& newGlobal);

        void Execute() override;
        void Undo() override;
        std::string GetName() const override;

    private:
        ecs::Scene* scene = nullptr;
        ecs::Entity entity = ecs::MAX_ENTITIES;
        glm::mat4 oldLocalMatrix{1.0f};
        glm::mat4 newLocalMatrix{1.0f};
        glm::mat4 oldGlobalMatrix{1.0f};
        glm::mat4 newGlobalMatrix{1.0f};
    };

    struct EntityTransformRecord
    {
        ecs::Entity entity = ecs::MAX_ENTITIES;
        glm::mat4 oldLocalMatrix{1.0f};
        glm::mat4 newLocalMatrix{1.0f};
        glm::mat4 oldGlobalMatrix{1.0f};
        glm::mat4 newGlobalMatrix{1.0f};
    };

    // Command for multiple entities transform changes
    class EntitiesTransformCommand : public ICommand
    {
    public:
        EntitiesTransformCommand(ecs::Scene* scene, const std::vector<EntityTransformRecord>& records);

        void Execute() override;
        void Undo() override;
        std::string GetName() const override;

    private:
        ecs::Scene* scene = nullptr;
        std::vector<EntityTransformRecord> records;
    };

    // Command for creating an entity
    class CreateEntityCommand : public ICommand
    {
    public:
        CreateEntityCommand(ecs::Scene* scene, ecs::Entity entity, ecs::Entity parent = ecs::MAX_ENTITIES, const std::string& name = "Entity");

        void Execute() override;
        void Undo() override;
        std::string GetName() const override;

    private:
        ecs::Scene* scene = nullptr;
        ecs::Entity entity = ecs::MAX_ENTITIES;
        ecs::Entity parent = ecs::MAX_ENTITIES;
        std::string name;
        std::shared_ptr<rapidjson::Document> snapshotDoc;
        bool hasSnapshot = false;
    };

    // Structure for entity deletion snapshot
    struct EntityDeleteSnapshot
    {
        ecs::Entity originalEntity = ecs::MAX_ENTITIES;
        ecs::Entity parent = ecs::MAX_ENTITIES;
        std::string name;
        bool wasActive = true;
        std::shared_ptr<rapidjson::Document> doc;
    };

    // Command for deleting entities (preserves hierarchy, components, and active state)
    class DeleteEntitiesCommand : public ICommand
    {
    public:
        DeleteEntitiesCommand(ecs::Scene* scene, const std::vector<ecs::EntityID>& entities, ecs::EditorSystem* editor = nullptr);

        void Execute() override;
        void Undo() override;
        std::string GetName() const override;

    private:
        ecs::Scene* scene = nullptr;
        ecs::EditorSystem* editor = nullptr;
        std::vector<EntityDeleteSnapshot> snapshots;
        std::vector<ecs::EntityID> targetEntities;
    };

    // Command for reparenting an entity
    class ReparentEntityCommand : public ICommand
    {
    public:
        ReparentEntityCommand(ecs::Scene* scene, ecs::Entity child, ecs::Entity oldParent, ecs::Entity newParent);

        void Execute() override;
        void Undo() override;
        std::string GetName() const override;

    private:
        ecs::Scene* scene = nullptr;
        ecs::Entity child = ecs::MAX_ENTITIES;
        ecs::Entity oldParent = ecs::MAX_ENTITIES;
        ecs::Entity newParent = ecs::MAX_ENTITIES;
    };

    // Command for renaming an entity
    class RenameEntityCommand : public ICommand
    {
    public:
        RenameEntityCommand(ecs::EditorSystem* editor, ecs::Scene* scene, ecs::Entity entity, const std::string& oldName, const std::string& newName);

        void Execute() override;
        void Undo() override;
        std::string GetName() const override;

    private:
        ecs::EditorSystem* editor = nullptr;
        ecs::Scene* scene = nullptr;
        ecs::Entity entity = ecs::MAX_ENTITIES;
        std::string oldName;
        std::string newName;
    };

    // Command for toggling active state of entities
    class SetEntityActiveCommand : public ICommand
    {
    public:
        SetEntityActiveCommand(ecs::Scene* scene, const std::vector<std::pair<ecs::Entity, bool>>& entitiesOldStates, bool newActive);

        void Execute() override;
        void Undo() override;
        std::string GetName() const override;

    private:
        ecs::Scene* scene = nullptr;
        std::vector<std::pair<ecs::Entity, bool>> oldStates;
        bool newActive = true;
    };

    // Command for adding a component
    class AddComponentCommand : public ICommand
    {
    public:
        AddComponentCommand(ecs::Scene* scene, ecs::Entity entity, std::type_index typeIdx, const std::string& typeDisplayName);

        void Execute() override;
        void Undo() override;
        std::string GetName() const override;

    private:
        ecs::Scene* scene = nullptr;
        ecs::Entity entity = ecs::MAX_ENTITIES;
        std::type_index typeIdx;
        std::string typeDisplayName;
    };

    // Command for removing a component
    class RemoveComponentCommand : public ICommand
    {
    public:
        RemoveComponentCommand(ecs::Scene* scene, ecs::Entity entity, std::type_index typeIdx, const std::string& typeDisplayName, std::shared_ptr<rapidjson::Document> serializedDoc = nullptr);

        void Execute() override;
        void Undo() override;
        std::string GetName() const override;

    private:
        ecs::Scene* scene = nullptr;
        ecs::Entity entity = ecs::MAX_ENTITIES;
        std::type_index typeIdx;
        std::string typeDisplayName;
        std::shared_ptr<rapidjson::Document> doc;
    };

    // Command for instantiating prefabs or models into scene
    class InstantiateEntitiesCommand : public ICommand
    {
    public:
        InstantiateEntitiesCommand(ecs::Scene* scene, const std::vector<ecs::EntityID>& entities, const std::string& operationName = "Instantiate", ecs::EditorSystem* editor = nullptr);

        void Execute() override;
        void Undo() override;
        std::string GetName() const override;

    private:
        ecs::Scene* scene = nullptr;
        ecs::EditorSystem* editor = nullptr;
        std::vector<ecs::EntityID> entities;
        std::string operationName;
        std::vector<EntityDeleteSnapshot> snapshots;
    };

    // Generic lambda command
    class LambdaCommand : public ICommand
    {
    public:
        LambdaCommand(std::string name, std::function<void()> onExecute, std::function<void()> onUndo);

        void Execute() override;
        void Undo() override;
        std::string GetName() const override;

    private:
        std::string name;
        std::function<void()> onExecute;
        std::function<void()> onUndo;
    };
}
