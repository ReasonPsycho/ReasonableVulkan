//
// Created by redkc on 18/02/2024.
//
#include "Types.h"
#include "systems/transformSystem/Transform.h"
#include "Scene.h"

#include "tracy/Tracy.hpp"


using namespace engine::ecs;


std::type_index Scene::GetTypeFromIndex(std::size_t index) const
{
    auto it = indexToType.find(index);
    assert(it != indexToType.end() && "Index not registered.");
    return it->second;
}

void Scene::Update(float deltaTime) {
    for (auto& [_, system] : systems) {

    ZoneTransientN(zoneName,(system->name).c_str(),true);
        system->Update(deltaTime);
    }
}



void Scene::SetParent(Entity child, Entity parent) {
    assert(child < maxEntityIndex && parent < maxEntityIndex);

    // Remove child from previous parent and from rootEntities if needed
    RemoveParent(child);

    // Set new parent
    sceneGraph[child].parent = parent;
    sceneGraph[parent].children.push_back(child);

    // Remove child from rootEntities because it now has a parent
    rootEntities.erase(std::remove(rootEntities.begin(), rootEntities.end(), child), rootEntities.end());
}


void Scene::RemoveParent(Entity child) {
    assert(child < maxEntityIndex);

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


Entity Scene::CreateEntity(Transform transform ,Entity parentEntity ) {
    Entity entity;
    if (!freeEntities.empty()) {
        entity = freeEntities.front();
        freeEntities.pop();
    } else {
        entity = maxEntityIndex++;
    }

    AddComponent<Transform>(entity,transform);


    if (parentEntity == -1)
    {
        rootEntities.push_back(entity);
    }else
    {
        auto parentNode = sceneGraph.find(parentEntity);
        parentNode->second.children.push_back(entity);
    }

    auto signature = Signature{};
    signature.set(GetComponentTypeID<Transform>());
    entitySignatures[entity] = signature;

    activeEntities.set(entity, true);
    return entity;
}

void Scene::DestroyEntity(Entity entity) {
    Signature signature = entitySignatures[entity]; // Get entity signature
    for (size_t i = 0; i < signature.size(); ++i) {
        if (signature.test(i)) {
            auto componentIndex = GetTypeFromIndex(i);
            auto& array = componentArrays[componentIndex];
            array->RemoveComponentUntyped(entity);
        }
    }
    entitySignatures.erase(entity);
    activeEntities.reset(entity);

    for (auto& [_, system] : systems) {
        system->RemoveEntity(entity);
    }

    freeEntities.push(entity);  // add ID back for reuse
}


void Scene::SetEntityActive(Entity entity, bool active)
{
    activeEntities[entity] = active;
}

bool Scene::IsEntityActive(Entity entity) const
{
    return activeEntities[entity];
}