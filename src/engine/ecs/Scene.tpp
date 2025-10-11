#pragma once
#include "../systems/transformSystem/componets/Transform.h"
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
void Scene::RegisterComponent()
{
    assert(componentArrays.find( typeid(T)) == componentArrays.end() && "Component already registered.");

    std::type_index typeIdx = typeid(T);
    componentArrays[typeIdx] = std::make_unique<ComponentArray<T>>();

    ComponentTypeID componentTypeId = GetComponentTypeID<T>();
    indexToType.insert_or_assign(componentTypeId, typeIdx);


    #ifdef EDITOR_ENABLED

    GetSystem<EditorSystem>().get()->RegisterComponentType<T>();

    #endif
}

template <typename T>
void Scene::AddComponent(Entity entity, T component)
{
    if constexpr (std::is_same<T, Transform>::value)
    {
        GetIntegralComponentArray<T>()->AddComponentToEntity(entity, component);
    }
    else
    {
        GetComponentArray<T>()->AddComponentToEntity(entity, component);
    }

    Signature& signature = entitySignatures[entity];
    signature.set(GetComponentTypeID<T>(), true);

    // 🔍 Check each system
    for (auto& [_, system] : systems) {
        if ((signature & system->signature) == system->signature) {
            system->AddEntity(entity);
        }
    }
}

template <typename T>
void Scene::RemoveComponent(Entity entity)
{
    if constexpr (std::is_same<T, Transform>::value)
    {
        GetIntegralComponentArray<T>()->RemoveComponentFronEntity(entity);
    }
    else
    {
        GetComponentArray<T>()->RemoveComponentFronEntity(entity);
    }
    
    Signature& signature = entitySignatures[entity];
    signature.set(GetComponentTypeID<T>(), false);

    for (auto& [_, system] : systems) {
        if ((signature & system->signature) != system->signature) {
            system->RemoveEntity(entity);
        }
    }
}

template <typename T>
bool Scene::HasComponent(Entity entity)
{
    if constexpr (std::is_same<T, Transform>::value)
    {
    return GetIntegralComponentArray<T>()->HasComponent(entity);
    }
    else
    {
     return GetComponentArray<T>()->HasComponent(entity);
    }
}

template <typename T>
void Scene::SetComponentActive(Entity entity,bool active )
{
    if constexpr (std::is_same<T, Transform>::value)
    {
        GetIntegralComponentArray<T>()->SetComponentActive(entity,active);
    }
    else
    {
        GetComponentArray<T>()->SetComponentActive(entity,active);
    }
}

template <typename T>
bool Scene::IsComponentActive(Entity entity)
{
    if constexpr (std::is_same<T, Transform>::value)
    {
        return GetIntegralComponentArray<T>()->IsComponentActive(entity);
    }
    else
    {
        return GetComponentArray<T>()->IsComponentActive(entity);
    }
}

template <typename T>
auto Scene::GetComponent(Entity entity) -> T&
{
    if constexpr (std::is_same<T, Transform>::value)
    {
        return GetIntegralComponentArray<T>()->GetComponent(entity);
    }
    else
    {
        return GetComponentArray<T>()->GetComponent(entity);
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
    assert(componentArrays.find( typeid(T)) == componentArrays.end() && "Component already registered.");

    std::type_index typeIdx = typeid(T);
    componentArrays[typeIdx] = std::make_unique<IntegralComponentArray<T>>();

    ComponentTypeID componentTypeId = GetComponentTypeID<T>();
    indexToType.insert_or_assign(componentTypeId, typeIdx);
}

template <typename T>
std::shared_ptr<ComponentArray<T>> Scene::GetComponentArray()
{
    // Cast from IComponentArray to ComponentArray<T>
    auto basePtr = componentArrays[ typeid(T)].get();
    return std::shared_ptr<ComponentArray<T>>(static_cast<ComponentArray<T>*>(basePtr),
                                              [](ComponentArray<T>*){}); // do-nothing deleter
}

template <typename T>
std::shared_ptr<IntegralComponentArray<T>> Scene::GetIntegralComponentArray()
{
    // Cast from IComponentArray to ComponentArray<T>
    auto basePtr = componentArrays[ typeid(T)].get();
    return std::shared_ptr<IntegralComponentArray<T>>(static_cast<IntegralComponentArray<T>*>(basePtr),
                                              [](IntegralComponentArray<T>*){}); // do-nothing deleter
}

template<typename T>
std::shared_ptr<T> Scene::GetSystem()
{
    auto typeIndex = std::type_index(typeid(T));
    return std::static_pointer_cast<T>(systems.at(typeIndex));
}