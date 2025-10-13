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
        void AddComponentToEntity(Entity entity, T component);
        void RemoveComponentFronEntity(Entity entity);
        T& GetComponent(Entity entity);
        bool HasComponent(Entity entity) const;
        void SetComponentActive(Entity entity, bool active);
        bool IsComponentActive(Entity entity) const;
        Entity ComponentIndexToEntity(ComponentIndex index) const;
        std::size_t GetArraySize() const;
        std::array<T, MAX_COMPONENTS_ARRAY>& GetComponents();

        //Untyped interface overrides
        void AddComponentUntyped(Entity entity) override;
        Component& GetComponentUntyped(Entity entity)  override;
        void RemoveComponentUntyped(Entity entity) override;
        bool HasComponentUntyped(Entity entity) const override;
        void SetComponentActiveUntyped(Entity entity, bool active) override;
        bool IsComponentActiveUntyped(Entity entity) const override;

    private:
        std::array<T, MAX_COMPONENTS_ARRAY> componentArray;
        std::bitset<MAX_COMPONENTS_ARRAY> activeComponents;
        std::unordered_map<Entity, ComponentIndex> entityToIndexMap;
        std::unordered_map<ComponentIndex, Entity> indexToEntityMap;
        std::size_t size = 0;
    };
};

#include "ComponentArray.tpp"

#endif //COMPONENTARRAY_H
