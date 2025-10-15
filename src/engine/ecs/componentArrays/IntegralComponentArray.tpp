#pragma once
#include "IntegralComponentArray.h"

using namespace engine::ecs;

template <typename T>
void IntegralComponentArray<T>::AddComponentToEntity(Entity entity, T component) {
    assert(entity < MAX_ENTITIES);
    componentArray[entity] = component;
    activeComponents[entity] = true;
}

template <typename T>
void IntegralComponentArray<T>::RemoveComponentFronEntity(Entity entity) {
    assert(entity < MAX_ENTITIES);
    activeComponents[entity] = false;
}

template <typename T>
T& IntegralComponentArray<T>::GetComponent(Entity entity) {
    assert(entity < MAX_ENTITIES);
    return componentArray[entity];
}

template <typename T>
bool IntegralComponentArray<T>::HasComponent(Entity entity) const {
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

template <typename T>
void IntegralComponentArray<T>::AddComponentUntyped(Entity entity)
{
    AddComponentToEntity(entity, T());
}

template <typename T>
Component& IntegralComponentArray<T>::GetComponentUntyped(Entity entity)
{
    assert(entity < MAX_ENTITIES);
    return componentArray[entity];
}

// Untyped overrides
template <typename T>
void IntegralComponentArray<T>::RemoveComponentUntyped(Entity entity) {
    RemoveComponentFronEntity(entity);
}

template <typename T>
bool IntegralComponentArray<T>::HasComponentUntyped(Entity entity) const {
    return HasComponent(entity);
}

template <typename T>
void IntegralComponentArray<T>::SetComponentActiveUntyped(Entity entity, bool active) {
    SetComponentActive(entity, active);
}

template <typename T>
bool IntegralComponentArray<T>::IsComponentActiveUntyped(Entity entity) const {
    return IsComponentActive(entity);
}


template <typename T>
void IntegralComponentArray<T>::SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
    rapidjson::Value components(rapidjson::kArrayType);

    // Store active components and their data
    for (Entity entity = 0; entity < MAX_ENTITIES; ++entity) {
        if (activeComponents[entity]) {
            rapidjson::Value componentObj(rapidjson::kObjectType);

            // Store entity ID
            componentObj.AddMember("entity", static_cast<uint64_t>(entity), allocator);

            // Store component data
            rapidjson::Value componentData(rapidjson::kObjectType);
            componentArray[entity].SerializeToJson(componentData, allocator);
            componentObj.AddMember("data", componentData, allocator);

            components.PushBack(componentObj, allocator);
        }
    }

    obj.AddMember("components", components, allocator);
}

template <typename T>
void IntegralComponentArray<T>::DeserializeFromJson(const rapidjson::Value& obj) {
    // Reset active components
    activeComponents.reset();

    // Read components
    if (obj.HasMember("components") && obj["components"].IsArray()) {
        const auto& components = obj["components"];
        for (rapidjson::SizeType i = 0; i < components.Size(); i++) {
            const auto& componentObj = components[i];

            if (componentObj.HasMember("entity") && componentObj.HasMember("data")) {
                Entity entity = componentObj["entity"].GetUint64();

                // Create and deserialize component
                T component;
                component.DeserializeFromJson(componentObj["data"]);

                // Store component and mark as active
                componentArray[entity] = component;
                activeComponents[entity] = true;
            }
        }
    }
}