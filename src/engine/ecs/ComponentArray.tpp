#pragma once
#include "ComponentArray.h"

using namespace engine::ecs;

template <typename T>
void ComponentArray<T>::AddComponentToEntity(std::uint32_t entity, T component) {
    assert(entity < MAX_ENTITIES);
    assert(!componentExists[entity]);
    componentArray[entity] = component;
    componentExists[entity] = true;
    activeComponents[entity] = true;
}

template <typename T>
void ComponentArray<T>::RemoveComponentFronEntity(std::uint32_t entity) {
    assert(entity < MAX_ENTITIES);
    assert(componentExists[entity]);
    componentExists[entity] = false;
    activeComponents[entity] = false;
}

template <typename T>
T& ComponentArray<T>::GetComponent(std::uint32_t entity) {
    assert(entity < MAX_ENTITIES);
    assert(componentExists[entity]);
    return componentArray[entity];
}

template <typename T>
bool ComponentArray<T>::HasComponent(std::uint32_t entity) const {
    assert(entity < MAX_ENTITIES);
    return componentExists[entity];
}

template <typename T>
void ComponentArray<T>::SetComponentActive(Entity entity, bool active) {
    assert(entity < MAX_ENTITIES);
    activeComponents[entity] = active;
}

template <typename T>
bool ComponentArray<T>::IsComponentActive(Entity entity) const {
    assert(entity < MAX_ENTITIES);
    return activeComponents[entity];
}

template <typename T>
const std::array<T, MAX_ENTITIES>& ComponentArray<T>::GetComponents() const {
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
