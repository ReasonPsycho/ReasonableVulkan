//
// Created by redkc on 02/08/2025.
//
#pragma once
#include "ComponentArray.h"

using namespace engine::ecs;
template <typename T>
  void ComponentArray<T>::AddComponentToEntity(std::uint32_t entity, T component)
{
    assert(entityToIndexMap.find(entity) == entityToIndexMap.end());
    std::size_t newIndex = size;
    entityToIndexMap[entity] = newIndex;
    indexToEntityMap[newIndex] = entity;
    componentArray[newIndex] = component;
    ++size;
}

template <typename T>
void ComponentArray<T>::RemoveComponentFronEntity(std::uint32_t entity)
{
    assert(entityToIndexMap.find(entity) != entityToIndexMap.end());
    std::size_t indexOfRemoved = entityToIndexMap[entity];
    std::size_t indexOfLast = size - 1;
    componentArray[indexOfRemoved] = componentArray[indexOfLast];
    Entity lastEntity = indexToEntityMap[indexOfLast];
    entityToIndexMap[lastEntity] = indexOfRemoved;
    indexToEntityMap[indexOfRemoved] = lastEntity;
    entityToIndexMap.erase(entity);
    indexToEntityMap.erase(indexOfLast);
    --size;
}

template <typename T>
T& ComponentArray<T>::GetComponent(std::uint32_t entity)
{
    assert(entityToIndexMap.find(entity) != entityToIndexMap.end());
    return componentArray[entityToIndexMap[entity]];
}

template <typename T>
bool ComponentArray<T>::HasComponent(std::uint32_t entity) const
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
void ComponentArray<T>::RemoveComponentUntyped(std::uint32_t entity)
{
    RemoveComponentFronEntity(entity);
}

template <typename T>
bool ComponentArray<T>::HasComponentUntyped(std::uint32_t entity) const
{
    return HasComponent(entity);
}

template <typename T>
void ComponentArray<T>::SetComponentActiveUntyped(std::uint32_t entity, bool active)
{
    SetComponentActive(entity, active);
}

template <typename T>
bool ComponentArray<T>::IsComponentActiveUntyped(std::uint32_t entity) const
{
    return IsComponentActive(entity);
}