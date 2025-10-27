//
// Created by redkc on 18/02/2024.
//

#ifndef REASONABLEGL_SCENE_H
#define REASONABLEGL_SCENE_H

#include <memory>
#include <queue>
#include <typeindex>
#include <unordered_map>

#include "systems/transformSystem/componets/Transform.hpp"
#include "Engine.h"
#include "componentArrays/IComponentArray.h"
#include "componentArrays/ComponentArray.h"
#include "Types.h"
#include "System.h"
#include "TransformNode.h"
#include "componentArrays/IntegralComponentArray.h"
#include "systems/renderingSystem/componets/Camera.hpp"

namespace engine::ecs
{
    struct Transform;

    struct CameraObject
    {
        Camera* camera;
        Transform* transform;
    };

    class Scene {
    public:

        explicit Scene(Engine& engine);

        void Update(float deltaTime);

        //Entity
        Entity CreateEntity(Entity parentEntity = -1);
        Entity CreateEntity(Transform transform,Entity parentEntity = -1);
        Entity CreateEntity(std::string entityName,Entity parentEntity = -1);
        Entity CreateEntity(std::string entityName,Transform transform,Entity parentEntity = -1);


        void DestroyEntity(Entity entity);

        void SetEntityActive(Entity entity, bool active);

        bool IsEntityActive(Entity entity) const;

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

        //Scene Graph
        void SetParent(Entity child, Entity parent);
        void RemoveParent(Entity child);
        Entity GetParent(Entity entity) const;
        const std::vector<Entity>& GetChildren(Entity entity) const;
        bool HasParent(Entity entity) const;
        bool IsAncestor(Entity potentialAncestor, Entity entity) const;

        void SerializeToJson(rapidjson::Document& doc) const;
        void DeserializeFromJson(const rapidjson::Document& doc);
        void AddComponent(const std::type_index& type);

        std::unordered_map<Entity, TransformNode> sceneGraph;
        std::vector<Entity> rootEntities;

        CameraObject GetActiveCamera();

        //Engine
        engine::Engine& engine;

        uint32_t maxEntityIndex = 0;

    private:

        //Entities
        std::queue<Entity> freeEntities;  // recycled IDs
        std::unordered_map<Entity, Signature> entitySignatures;
        std::bitset<MAX_ENTITIES> activeEntities;

        //Components
        template<typename T>
        void RegisterIntegralComponent();

        std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> componentArrays;
        std::unordered_map<ComponentTypeID, std::type_index> indexToType;

        //Systems
        std::unordered_map<std::type_index, std::shared_ptr<SystemBase>> systems;

        void SerializeEntities(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const;
        void SerializeComponents(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const;
        void SerializeSystems(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const;
        void SerializeSceneGraph(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const;

        void RegisterSystem(const std::type_index& type);
        void DeserializeEntities(const rapidjson::Value& obj);
        void DeserializeComponents(const rapidjson::Value& obj);
        void DeserializeSystems(const rapidjson::Value& obj);
        void DeserializeSceneGraph(const rapidjson::Value& obj);
    };


}

#include "Scene.tpp"

#endif //REASONABLEGL_SCENE_H
