//
// Created by redkc on 18/02/2024.
//
#include "Types.h"

#include "TransformNode.h"
#include "systems/Transform.h"
#include "tracy/Tracy.hpp"
#include "Scene.h"

using namespace engine::ecs;

template <typename T>
void Scene::AddComponent(Entity entity, T component)
{
    GetComponentArray<T>()->InsertData(entity, component);

    Signature& signature = entitySignatures[entity];
    signature.set(GetComponentTypeID<T>(), true);

    // 🔍 Check each system
    for (auto& [_, system] : systems) {
        if ((signature & system->signature) == system->signature) {
            system->OnComponentAdded(entity);
        }
    }
}

template <typename T>
  void Scene::RegisterComponent()
{
    const char* typeName = typeid(T).name();
    assert(componentArrays.find(typeName) == componentArrays.end() && "Component already registered.");

    componentArrays[typeName] = std::make_unique<ComponentArray<T>>();

    // Ensure the type gets a ComponentTypeID
    GetComponentTypeID<T>(); // Logs/initializes the ID
}


template <typename T>
auto Scene::GetComponent(Entity entity) -> T&
{
    return GetComponentArray<T>()->GetData(entity);
}

template <typename T>
bool Scene::HasComponent(Entity entity)
{
    return GetComponentArray<T>()->HasData(entity);
}

template <typename T>
std::unique_ptr<ComponentArray<T>> Scene::GetComponentArray()
{
    const char* typeName = typeid(T).name();
    return std::static_pointer_cast<ComponentArray<T>>(componentArrays[typeName]);
}

template<typename T, typename... Args>
std::shared_ptr<T> Scene::RegisterSystem(Args&&... args)
{
    static_assert(std::is_base_of<SystemBase, T>::value, "T must inherit from SystemBase");

    auto typeIndex = std::type_index(typeid(T));
    auto system = std::make_shared<T>(std::forward<Args>(args)...);
    systems[typeIndex] = system;
    return system;
}

template<typename T>
std::shared_ptr<T> Scene::GetSystem()
{
    auto typeIndex = std::type_index(typeid(T));
    return std::static_pointer_cast<T>(systems.at(typeIndex));
}

void Scene::Update(float deltaTime) {
    for (auto& [_, system] : systems) {
    ZoneTransientN(zoneName,(system->name).c_str(),true);
        system->Update(deltaTime);
    }
}

void Scene::SetParent(Entity child, Entity parent) {
    assert(child < livingEntityCount && parent < livingEntityCount);

    // Remove child from previous parent and from rootEntities if needed
    RemoveParent(child);

    // Set new parent
    sceneGraph[child].parent = parent;
    sceneGraph[parent].children.push_back(child);

    // Remove child from rootEntities because it now has a parent
    rootEntities.erase(std::remove(rootEntities.begin(), rootEntities.end(), child), rootEntities.end());
}


void Scene::RemoveParent(Entity child) {
    assert(child < livingEntityCount);

    auto& node = sceneGraph[child];
    if (node.parent != MAX_ENTITIES) {
        auto& siblings = sceneGraph[node.parent].children;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), child), siblings.end());
        node.parent = MAX_ENTITIES;

        // Add child to rootEntities since it lost its parent
        rootEntities.push_back(child);
    }
}

Entity Scene::GetParent(Entity entity) const {
    auto it = sceneGraph.find(entity);
    if (it != sceneGraph.end()) {
        return it->second.parent;
    }
    return MAX_ENTITIES;
}

const std::vector<Entity>& Scene::GetChildren(Entity entity) const {
    static const std::vector<Entity> empty{};
    auto it = sceneGraph.find(entity);
    return it != sceneGraph.end() ? it->second.children : empty;
}

bool Scene::HasParent(Entity entity) const {
    auto it = sceneGraph.find(entity);
    return it != sceneGraph.end() && it->second.parent != MAX_ENTITIES;
}

void Scene::DestroyEntity(Entity entity) {
    assert(entity < livingEntityCount);

    RemoveParent(entity);
    rootEntities.erase(std::remove(rootEntities.begin(), rootEntities.end(), entity), rootEntities.end());

    for (auto& [_, system] : systems) {
        system->OnComponentRemoved(entity);
    }

    entitySignatures.erase(entity);
}

template<typename T>
void Scene::RemoveComponent(Entity entity)
{
    GetComponentArray<T>()->RemoveData(entity);

    Signature& signature = entitySignatures[entity];
    signature.set(GetComponentTypeID<T>(), false);

    for (auto& [_, system] : systems) {
        if ((signature & system->signature) != system->signature) {
            system->OnComponentRemoved(entity);
        }
    }
}

template<typename... Components>
std::vector<Entity> Scene::GetEntitiesWith()
{
    Signature requiredSignature;
    (requiredSignature.set(GetComponentTypeID<Components>()), ...); // Fold expression

    std::vector<Entity> matching;
    for (auto& [entity, signature] : entitySignatures) {
        if ((signature & requiredSignature) == requiredSignature) {
            matching.push_back(entity);
        }
    }
    return matching;
}