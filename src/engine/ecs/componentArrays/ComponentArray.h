#ifndef COMPONENTARRAY_H
#define COMPONENTARRAY_H

#include "../Types.h"
#include <array>
#include <bitset>
#include <cassert>
#include "IComponentArray.h"

namespace engine::ecs {
    template<typename T>
    class ComponentArray : public IComponentArray {
    public:
        void AddComponentToEntity(Entity entity, T component);
        void RemoveComponentFronEntity(Entity entity);
        T& GetComponent(Entity entity);
        bool HasComponent(Entity entity) const;
        void SetComponentActive(Entity entity, bool active);
        bool IsComponentActive(Entity entity) const;
        std::array<T, MAX_ENTITIES>& GetComponents();

        // Untyped interface overrides
        void RemoveComponentUntyped(Entity entity) override;
        bool HasComponentUntyped(Entity entity) const override;
        void SetComponentActiveUntyped(Entity entity, bool active) override;
        bool IsComponentActiveUntyped(Entity entity) const override;

    private:
        std::array<T, MAX_COMPONENTS_ARRAY> componentArray{};
        std::bitset<MAX_COMPONENTS_ARRAY> activeComponents{};
        std::unordered_map<Entity, Entity> entityToIndex{};
        std::queue<Entity> freeComponents;
        uint32_t maxComponentIndex = 0;

    };
}

#include "ComponentArray.tpp"

#endif // COMPONENTARRAY_H
