#pragma once
#include "ComponentArray.h"

using namespace engine::ecs;

template <typename T>
void ComponentArray<T>::AddComponentToEntity(Entity entity, T component) {
    assert(entity < MAX_ENTITIES);
    assert(!entityToIndex[entity]);

    Entity componentId;
    if (!freeComponents.empty()) {
        entity = freeComponents.front();
        freeComponents.pop();
    } else {
        entity = maxComponentIndex++;
    }

    componentArray[componentId] = component;
    entityToIndex[entity] = componentId;
    activeComponents[componentId] = true;
}

template <typename T>
void ComponentArray<T>::RemoveComponentFronEntity(Entity entity) {
    assert(entity < MAX_ENTITIES);
    assert(entityToIndex[entity]);
    auto componentId = entityToIndex[entity];
    entityToIndex.erase(entity);
    //componentArray.erase(componentId);;
    freeComponents.push(componentId);
}

template <typename T>
T& ComponentArray<T>::GetComponent(Entity entity) {
    assert(entity < MAX_ENTITIES);
    assert(entityToIndex[entity]);
    return componentArray[entityToIndex[entity]];
}

template <typename T>
bool ComponentArray<T>::HasComponent(Entity entity) const {
    assert(entity < MAX_ENTITIES);
    return entityToIndex.contains(entity);
}

template <typename T>
void ComponentArray<T>::SetComponentActive(Entity entity, bool active) {
    assert(entity < MAX_ENTITIES);
    assert(entityToIndex[entity]);
    auto componentId = entityToIndex[entity];
    activeComponents[componentId] = active;
}

template <typename T>
bool ComponentArray<T>::IsComponentActive(Entity entity) const {
    assert(entity < MAX_ENTITIES);
    assert(entityToIndex.at(entity));
    auto componentId = entityToIndex.at(entity);
    return activeComponents[componentId];
}

template <typename T>
 std::array<T, MAX_ENTITIES>& ComponentArray<T>::GetComponents()  {
    return componentArray;
}

// Untyped overrides
template <typename T>
void ComponentArray<T>::RemoveComponentUntyped(Entity entity) {
    RemoveComponentFronEntity(entity);
}

template <typename T>
bool ComponentArray<T>::HasComponentUntyped(Entity entity) const {
    return HasComponent(entity);
}

template <typename T>
void ComponentArray<T>::SetComponentActiveUntyped(Entity entity, bool active) {
    SetComponentActive(entity, active);
}

template <typename T>
bool ComponentArray<T>::IsComponentActiveUntyped(Entity entity) const {
    return IsComponentActive(entity);
}
