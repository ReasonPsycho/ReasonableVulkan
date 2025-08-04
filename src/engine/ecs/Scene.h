//
// Created by redkc on 18/02/2024.
//

#ifndef REASONABLEGL_SCENE_H
#define REASONABLEGL_SCENE_H

#include <memory>
#include <queue>
#include <typeindex>
#include <unordered_map>

#include "IComponentArray.h"
#include "ComponentArray.h"
#include "IntegralComponentArray.h"

#include "Types.h"
#include "System.h"
#include "TransformNode.h"
#include "systems/TransformSystem.h"

namespace engine::ecs
{

    class Scene {
    public:

        Scene()
        {
            RegisterIntegralComponent<Transform>();
            RegisterSystem<TransformSystem>();
        }

        void Update(float deltaTime);

        //Entity
        Entity CreateEntity();

        void DestroyEntity(Entity entity);

        void SetEntityActive(Entity entity, bool active);

        bool IsEntityActive(Entity entity) const;

        template<typename... Components>
        std::vector<Entity> GetEntitiesWith();

        //Components
        template<typename T>
        void RegisterComponent();

        template<typename T>
        std::shared_ptr<ComponentArray<T>> GetComponentArray();

        template <class T>
        std::shared_ptr<IntegralComponentArray<T>> GetIntegralComponentArray();

        template<typename T>
        void AddComponent(Entity entity, T component);

        template<typename T>
        void RemoveComponent(Entity entity);

        template<typename T>
        auto GetComponent(Entity entity) -> T&;

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


        //Scene Graph
        void SetParent(Entity child, Entity parent);
        void RemoveParent(Entity child);
        Entity GetParent(Entity entity) const;
        const std::vector<Entity>& GetChildren(Entity entity) const;
        bool HasParent(Entity entity) const;

        std::unordered_map<Entity, TransformNode> sceneGraph;
        std::vector<Entity> rootEntities;
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
        std::unordered_map<ComponentTypeID, std::type_index> indexToType;

        //Systems
        std::unordered_map<std::type_index, std::shared_ptr<SystemBase>> systems;
    };


}

#include "Scene.tpp"

#endif //REASONABLEGL_SCENE_H
