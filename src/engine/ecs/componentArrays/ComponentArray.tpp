//
// Created by redkc on 02/08/2025.
//
#pragma once
#include "ComponentArray.h"

using namespace engine::ecs;
template <typename T>
  void ComponentArray<T>::AddComponentToEntity(Entity entity, T component)
{
    assert(entityToIndexMap.find(entity) == entityToIndexMap.end());
    std::size_t newIndex = size;
    entityToIndexMap[entity] = newIndex;
    indexToEntityMap[newIndex] = entity;
    componentArray[newIndex] = component;
    activeComponents[newIndex] = true;
    ++size;
}

template <typename T>
void ComponentArray<T>::RemoveComponentFronEntity(Entity entity)
{
    assert(entityToIndexMap.find(entity) != entityToIndexMap.end());
    std::size_t indexOfRemoved = entityToIndexMap[entity];
    std::size_t indexOfLast = size - 1;
    componentArray[indexOfRemoved] = componentArray[indexOfLast];
    Entity lastEntity = indexToEntityMap[indexOfLast];
    entityToIndexMap[lastEntity] = indexOfRemoved;
    activeComponents[indexOfRemoved] = activeComponents[lastEntity];
    indexToEntityMap[indexOfRemoved] = lastEntity;
    entityToIndexMap.erase(entity);
    indexToEntityMap.erase(indexOfLast);
    --size;
}

template <typename T>
T& ComponentArray<T>::GetComponent(Entity entity)
{
    assert(entityToIndexMap.find(entity) != entityToIndexMap.end());
    return componentArray[entityToIndexMap[entity]];
}

template <typename T>
bool ComponentArray<T>::HasComponent(Entity entity) const
{
    return entityToIndexMap.find(entity) != entityToIndexMap.end();
}

template <typename T>
void ComponentArray<T>::SetComponentActive(Entity entity, bool active)
{
    activeComponents[entity] = active;
}

template <typename T>
bool ComponentArray<T>::IsComponentActive(Entity entity) const
{
    return activeComponents[entity];
}

template <typename T>
const std::array<T, MAX_ENTITIES>& ComponentArray<T>::GetComponents() const
{
    return componentArray;
}

template <typename T>
void ComponentArray<T>::RemoveComponentUntyped(Entity entity)
{
    RemoveComponentFronEntity(entity);
}

template <typename T>
bool ComponentArray<T>::HasComponentUntyped(Entity entity) const
{
    return HasComponent(entity);
}

template <typename T>
void ComponentArray<T>::SetComponentActiveUntyped(Entity entity, bool active)
{
    SetComponentActive(entity, active);
}

template <typename T>
bool ComponentArray<T>::IsComponentActiveUntyped(Entity entity) const
{
    return IsComponentActive(entity);
}