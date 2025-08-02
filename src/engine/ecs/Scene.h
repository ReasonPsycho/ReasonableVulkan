//
// Created by redkc on 18/02/2024.
//

#ifndef REASONABLEGL_SCENE_H
#define REASONABLEGL_SCENE_H

#include <memory>
#include <typeindex>

#include "ComponentArray.h"
#include "Types.h"
#include "System.h"

namespace engine::ecs
{

    class Scene {
    public:
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
        void AddComponent(Entity entity, T component);

        template<typename T>
        void RemoveComponent(Entity entity);

        template<typename T>
        auto GetComponent(Entity entity) -> T&;

        template<typename T>
        bool HasComponent(Entity entity);

        //Systems
        template<typename T, typename... Args>
        std::shared_ptr<T> RegisterSystem(Args&&... args);

        void Update(float deltaTime);

        //Scene Graph
        void SetParent(Entity child, Entity parent);
        void RemoveParent(Entity child);
        Entity GetParent(Entity entity) const;
        const std::vector<Entity>& GetChildren(Entity entity) const;
        bool HasParent(Entity entity) const;

        std::unordered_map<Entity, TransformNode> sceneGraph;
        std::vector<Entity> rootEntities;
    private:

        //Components
        std::unordered_map<const char*, std::unique_ptr<void>> componentArrays;
        uint32_t livingEntityCount = 0;

        template<typename T>
        std::unique_ptr<ComponentArray<T>> GetComponentArray();

        //Entities
        std::unordered_map<Entity, Signature> entitySignatures;
        std::bitset<MAX_ENTITIES> activeEntities;

        //Systems
        std::unordered_map<std::type_index, std::shared_ptr<SystemBase>> systems;

        template<typename T>
        std::shared_ptr<T> GetSystem();
    };


}

#endif //REASONABLEGL_SCENE_H
