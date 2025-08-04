#pragma once
#include "ComponentArray.h"

using namespace engine::ecs;

template <typename T>
void IntegralComponentArray<T>::AddComponentToEntity(std::uint32_t entity, T component) {
    assert(entity < MAX_ENTITIES);
    componentArray[entity] = component;
    activeComponents[entity] = true;
}

template <typename T>
void IntegralComponentArray<T>::RemoveComponentFronEntity(std::uint32_t entity) {
    assert(entity < MAX_ENTITIES);
    activeComponents[entity] = false;
}

template <typename T>
T& IntegralComponentArray<T>::GetComponent(std::uint32_t entity) {
    assert(entity < MAX_ENTITIES);
    return componentArray[entity];
}

template <typename T>
bool IntegralComponentArray<T>::HasComponent(std::uint32_t entity) const {
    assert(entity < MAX_ENTITIES);
    return true;
}

template <typename T>
void IntegralComponentArray<T>::SetComponentActive(Entity entity, bool active) {
    assert(entity < MAX_ENTITIES);
    activeComponents[entity] = active;
}

template <typename T>
bool IntegralComponentArray<T>::IsComponentActive(Entity entity) const{
    assert(entity < MAX_ENTITIES);
    return activeComponents[entity];
}

template <typename T>
std::array<T, MAX_ENTITIES>& IntegralComponentArray<T>::GetComponents() {
    return componentArray;
}

// Untyped overrides
template <typename T>
void IntegralComponentArray<T>::RemoveComponentUntyped(std::uint32_t entity) {
    RemoveComponentFronEntity(entity);
}

template <typename T>
bool IntegralComponentArray<T>::HasComponentUntyped(std::uint32_t entity) const {
    return HasComponent(entity);
}

template <typename T>
void IntegralComponentArray<T>::SetComponentActiveUntyped(std::uint32_t entity, bool active) {
    SetComponentActive(entity, active);
}

template <typename T>
bool IntegralComponentArray<T>::IsComponentActiveUntyped(std::uint32_t entity) const {
    return IsComponentActive(entity);
}
