//
// Created by redkc on 02/08/2025.
//

#ifndef COMPONENTARRAY_H
#define COMPONENTARRAY_H
#include <unordered_map>
#include <cassert>

#include "IComponentArray.h"

namespace engine::ecs{
    template<typename T>
    class ComponentArray : public IComponentArray {
    public:
        ComponentID AddComponentToEntity(Entity entity, T component);
        ComponentID RemoveComponentFronEntity(Entity entity);
        T& GetComponentFromEntity(Entity entity);
        T& GetComponent(ComponentID componentID);
        bool HasComponent(Entity entity) const;
        void SetComponentActive(Entity entity, bool active);
        bool IsComponentActive(ComponentID componentIndex) const;
        Entity ComponentIndexToEntity(ComponentID index) const;
        std::size_t GetArraySize() const;
        std::array<T, MAX_COMPONENTS_ARRAY>& GetComponents();

        //Untyped interface overrides
        ComponentID AddComponentUntyped(Entity entity) override;
        Component& GetComponentUntyped(Entity entity)  override;
        void RemoveComponentUntyped(Entity entity) override;
        bool HasComponentUntyped(Entity entity) const override;
        void SetComponentActiveUntyped(Entity entity, bool active) override;
        bool IsComponentActiveUntyped(Entity entity) const override;

        void SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
        void DeserializeFromJson(const rapidjson::Value& obj) override;
    private:
        std::array<T, MAX_COMPONENTS_ARRAY> componentArray;
        std::bitset<MAX_COMPONENTS_ARRAY> activeComponents;
        std::unordered_map<Entity, ComponentID> entityToIndexMap;
        std::unordered_map<ComponentID, Entity> indexToEntityMap;
        std::size_t size = 0;
    };
};

#include "ComponentArray.tpp"

#endif //COMPONENTARRAY_H
