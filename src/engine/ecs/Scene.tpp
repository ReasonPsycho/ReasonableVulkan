
#pragma once
#include "Scene.h"



template <typename ... Components>
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


template <typename T>
void Scene::AddComponent(Entity entity, T component)
{
    ComponentID componentId;

    if constexpr (has_annotation<Integral>(^^T))
    {
       componentId = GetIntegralComponentArray<T>()->AddComponentToEntity(entity, component);
    }
    else
    {
       componentId = GetComponentArray<T>()->AddComponentToEntity(entity, component);
    }

    Signature& signature = entitySignatures[entity];
    signature.set(GetComponentTypeID<T>(), true);

    std::type_index componentType(typeid(T));
    for (auto& [_, system] : systems) {
        for (auto type : system->registeredComponentTypes)
        {
            if (type == componentType)
            {
                system->AddComponent(componentId, componentType);
            }
        }
    }
}


template <typename T>
void Scene::RemoveComponent(Entity entity)
{
    auto typeIdx = std::type_index(typeid(T));
    auto it = componentArrays.find(typeIdx);
    if (it == componentArrays.end()) {
        return;
    }

    ComponentID componentId;
    if constexpr (has_annotation<Integral>(^^T))
    {
       componentId = static_cast<IntegralComponentArray<T>*>(it->second.get())->RemoveComponentFronEntity(entity);
    }
    else
    {
       componentId = static_cast<ComponentArray<T>*>(it->second.get())->RemoveComponentFronEntity(entity);
    }

    Signature& signature = entitySignatures[entity];
    signature.set(GetComponentTypeID<T>(), false);

    // Check each system
    for (auto& [_, system] : systems) {
        for (auto type : system->registeredComponentTypes)
        {
            if (type == typeid(T))
            {
                system->RemoveComponent(componentId, type);
            }
        }
    }
}

template <typename T>
bool Scene::HasComponent(Entity entity)
{
    auto typeIdx = std::type_index(typeid(T));
    auto it = componentArrays.find(typeIdx);
    if (it == componentArrays.end()) {
        return false;
    }
    if constexpr (has_annotation<Integral>(^^T))
    {
        return static_cast<IntegralComponentArray<T>*>(it->second.get())->HasComponent(entity);
    }
    else
    {
        return static_cast<ComponentArray<T>*>(it->second.get())->HasComponent(entity);
    }
}

template <typename T>
void Scene::SetComponentActive(Entity entity,bool active )
{
    auto typeIdx = std::type_index(typeid(T));
    auto it = componentArrays.find(typeIdx);
    if (it != componentArrays.end()) {
        if constexpr (has_annotation<Integral>(^^T))
        {
            static_cast<IntegralComponentArray<T>*>(it->second.get())->SetComponentActive(entity, active);
        }
        else
        {
            static_cast<ComponentArray<T>*>(it->second.get())->SetComponentActive(entity, active);
        }
    }
}

template <typename T>
bool Scene::IsComponentActive(Entity entity)
{
    auto typeIdx = std::type_index(typeid(T));
    auto it = componentArrays.find(typeIdx);
    if (it == componentArrays.end()) {
        return false;
    }
    if constexpr (has_annotation<Integral>(^^T))
    {
        return static_cast<IntegralComponentArray<T>*>(it->second.get())->IsComponentActive(entity);
    }
    else
    {
        return static_cast<ComponentArray<T>*>(it->second.get())->IsComponentActive(entity);
    }
}

template <typename T>
auto Scene::GetComponent(Entity entity) -> T&
{
    if constexpr (has_annotation<Integral>(^^T))
    {
        return GetIntegralComponentArray<T>()->GetComponentFromEntity(entity);
    }
    else
    {
        return GetComponentArray<T>()->GetComponentFromEntity(entity);
    }
}

template <typename T, typename ... Args>
std::shared_ptr<T> Scene::RegisterSystem(Args&&... args)
{
    static_assert(std::is_base_of<SystemBase, T>::value, "T must inherit from SystemBase");

    auto typeIndex = std::type_index(typeid(T));
    auto system = std::make_shared<T>(this,std::forward<Args>(args)...);
    systems[typeIndex] = system;
    return system;
}

template <typename T>
void  Scene::RegisterIntegralComponent()
{
    std::type_index typeIdx = typeid(T);
    if (componentArrays.find(typeIdx) == componentArrays.end()) {
        componentArrays[typeIdx] = std::make_shared<IntegralComponentArray<T>>();
    }
}

template <typename T>
void Scene::RegisterComponent()
{
    std::type_index typeIdx = typeid(T);
    if (componentArrays.find(typeIdx) == componentArrays.end()) {
        componentArrays[typeIdx] = std::make_shared<ComponentArray<T>>();
    }
}

template <typename T>
std::shared_ptr<ComponentArray<T>> Scene::GetComponentArray()
{
    auto typeIdx = std::type_index(typeid(T));
    auto it = componentArrays.find(typeIdx);
    if (it == componentArrays.end()) {
        AddComponent(typeIdx);
        it = componentArrays.find(typeIdx);
    }
    auto basePtr = it->second.get();
    return std::shared_ptr<ComponentArray<T>>(static_cast<ComponentArray<T>*>(basePtr),
                                              [](ComponentArray<T>*){}); // do-nothing deleter
}

template <typename T>
std::shared_ptr<IntegralComponentArray<T>> Scene::GetIntegralComponentArray()
{
    auto typeIdx = std::type_index(typeid(T));
    auto it = componentArrays.find(typeIdx);
    if (it == componentArrays.end()) {
        AddComponent(typeIdx);
        it = componentArrays.find(typeIdx);
    }
    auto basePtr = it->second.get();
    return std::shared_ptr<IntegralComponentArray<T>>(static_cast<IntegralComponentArray<T>*>(basePtr),
                                              [](IntegralComponentArray<T>*){}); // do-nothing deleter
}

template<typename T>
std::shared_ptr<T> Scene::GetSystem()
{
    auto typeIndex = std::type_index(typeid(T));
    auto it = systems.find(typeIndex);

    if (it != systems.end()) {
        return std::static_pointer_cast<T>(it->second);
    }

    return nullptr;
}