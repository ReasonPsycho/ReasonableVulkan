#pragma once
#include "ComponentArray.h"

using namespace engine::ecs;

template <typename T>
void ComponentArray<T>::AddComponentToEntity(std::uint32_t entity, T component) {
    assert(entity < MAX_ENTITIES);
    assert(!entityToIndex[entity]);

    std::uint32_t componentId;
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
void ComponentArray<T>::RemoveComponentFronEntity(std::uint32_t entity) {
    assert(entity < MAX_ENTITIES);
    assert(entityToIndex[entity]);
    auto componentId = entityToIndex[entity];
    entityToIndex.erase(entity);
    //componentArray.erase(componentId);;
    freeComponents.push(componentId);
}

template <typename T>
T& ComponentArray<T>::GetComponent(std::uint32_t entity) {
    assert(entity < MAX_ENTITIES);
    assert(entityToIndex[entity]);
    return componentArray[entityToIndex[entity]];
}

template <typename T>
bool ComponentArray<T>::HasComponent(std::uint32_t entity) const {
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
void ComponentArray<T>::RemoveComponentUntyped(std::uint32_t entity) {
    RemoveComponentFronEntity(entity);
}

template <typename T>
bool ComponentArray<T>::HasComponentUntyped(std::uint32_t entity) const {
    return HasComponent(entity);
}

template <typename T>
void ComponentArray<T>::SetComponentActiveUntyped(std::uint32_t entity, bool active) {
    SetComponentActive(entity, active);
}

template <typename T>
bool ComponentArray<T>::IsComponentActiveUntyped(std::uint32_t entity) const {
    return IsComponentActive(entity);
}
