//
// Created by redkc on 18/02/2024.
//

#ifndef REASONABLEGL_SCENE_H
#define REASONABLEGL_SCENE_H

#include <memory>
#include <queue>
#include <typeindex>
#include <unordered_map>

#include "systems/transformSystem/componets/TransformComponent.hpp"
#include "Engine.h"
#include "componentArrays/IComponentArray.h"
#include "componentArrays/ComponentArray.h"
#include "Types.h"
#include "System.h"
#include "TransformNode.h"
#include "componentArrays/IntegralComponentArray.h"
#include "systems/renderingSystem/componets/CameraComponent.hpp"

namespace engine::ecs
{
    struct TransformComponent;

    struct CameraObject
    {
        CameraComponent* camera;
        TransformComponent* transform;
    };

    class Scene {
    public:

        explicit Scene(::engine::Engine& engine);

        void Update(float deltaTime);

        //Entity
        Entity CreateEntity(Entity parentEntity = -1);
        Entity CreateEntity(TransformComponent transform,Entity parentEntity = -1);
        Entity CreateEntity(std::string entityName,Entity parentEntity = -1);
        Entity CreateEntity(std::string entityName,TransformComponent transform,Entity parentEntity = -1);


        void DestroyEntity(Entity entity);

        void SetEntityActive(Entity entity, bool active);

        bool IsEntityActive(Entity entity) const;

        std::string GetEntityName(Entity entity) const;
        void SetEntityName(Entity entity, const std::string& name);

        template<typename... Components>
        std::vector<Entity> GetEntitiesWith();

        //Components
        template<typename T>
        void RegisterComponent();

       std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> GetComponentArrays();


        template<typename T>
        std::shared_ptr<ComponentArray<T>> GetComponentArray();

        template <class T>
        std::shared_ptr<IntegralComponentArray<T>> GetIntegralComponentArray();

        template<typename T>
        void AddComponent(Entity entity, T component = T());

        void AddComponent(Entity entity,std::type_index typeIdx);


        template<typename T>
        void RemoveComponent(Entity entity);

        template<typename T>
        auto GetComponent(Entity entity) -> T&;

        size_t RegisteredComponentsSize() const;

        template<typename T>
        bool HasComponent(Entity entity);

        template<typename T>
        void SetComponentActive(Entity entity,bool active);

        template<typename T>
        bool IsComponentActive(Entity entity);

        std::type_index GetTypeFromIndex(std::size_t index) const;
        //Systems
        template<typename T, typename... Args>
        std::shared_ptr<T> RegisterSystem(Args&&... args);

        template<typename T>
        std::shared_ptr<T> GetSystem();

        const      std::unordered_map<std::type_index, std::shared_ptr<SystemBase>>GetSystems();

        // Scene Graph
        void SetParent(Entity child, Entity parent);
        void RemoveParent(Entity child);
        Entity GetParent(Entity entity) const;
        const std::vector<Entity>& GetChildren(Entity entity) const;
        bool HasParent(Entity entity) const;
        bool IsAncestor(Entity potentialAncestor, Entity entity) const;

        // Active state
        void SetActive(bool active);
        bool IsActive() const;
        bool active = false;

        // Editable state
        bool IsEditable() const { return isEditable; }
        void SetEditable(bool editable) { isEditable = editable; }
        bool isEditable = true;

        const std::string& GetName() const { return name; }
        void SetName(const std::string& sceneName) { name = sceneName; }
        std::string name;

        void SerializeToJson(rapidjson::Document& doc) const;
        void DeserializeFromJson(const rapidjson::Document& doc);
        void SerializeObjectToJson(Entity entity, rapidjson::Document& doc) const;
        void SerializePrefabToJson(Entity entity, rapidjson::Document& doc) const;
        Entity DeserializeObjectFromJson(const rapidjson::Document& doc, Entity parentEntity = -1);
        Entity InstantiatePrefab(const rapidjson::Document& doc, Entity parentEntity = -1);
        Entity InstantiatePrefab(boost::uuids::uuid prefabUuid, Entity parentEntity = -1);
        Entity InstantiateModel(boost::uuids::uuid modelOrMeshUuid, Entity parentEntity = -1);
        void AddComponent(const std::type_index& type);

        std::unordered_map<Entity, TransformNode> sceneGraph;
        std::vector<Entity> rootEntities;

        CameraObject GetActiveCamera();

        //Engine
        ::engine::Engine& engine;

        boost::uuids::uuid sceneId{boost::uuids::nil_uuid()};
    private:

        //Entities
        uint32_t maxEntityIndex = 0;
        std::queue<Entity> freeEntities;  // recycled IDs
        std::unordered_map<Entity, Signature> entitySignatures;
        std::bitset<MAX_ENTITIES> activeEntities;

        //Components
        template<typename T>
        void RegisterIntegralComponent();

        std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> componentArrays;

        //Systems
        std::unordered_map<std::type_index, std::shared_ptr<SystemBase>> systems;

        void SerializeEntities(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const;
        void SerializeComponents(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const;
        void SerializeSystems(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const;
        void SerializeSceneGraph(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const;

        void CollectSubtreeEntities(Entity root, std::vector<Entity>& outSubtree) const;
        void SerializeObjectEntities(Entity rootEntity, const std::vector<Entity>& subtree, rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const;
        void SerializeObjectComponents(const std::vector<Entity>& subtree, rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const;
        void SerializeObjectSceneGraph(Entity rootEntity, const std::vector<Entity>& subtree, rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const;

        void RegisterSystem(const std::type_index& type);
        void DeserializeEntities(const rapidjson::Value& obj);
        void DeserializeComponents(const rapidjson::Value& obj);
        void DeserializeSystems(const rapidjson::Value& obj);
        void DeserializeSceneGraph(const rapidjson::Value& obj);
    };


}

#include "Scene.tpp"

#endif //REASONABLEGL_SCENE_H
