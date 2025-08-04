#ifndef COMPONENTINTEGRALARRAY_H
#define COMPONENTINTEGRALARRAY_H

#include "../Types.h"
#include <array>
#include <bitset>
#include <cassert>
#include "IComponentArray.h"

namespace engine::ecs {
    template<typename T>
    class IntegralComponentArray : public IComponentArray {
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

        std::array<T, MAX_ENTITIES> componentArray{};
    private:
        std::bitset<MAX_ENTITIES> activeComponents{};
    };
}

#include "IntegralComponentArray.tpp"

#endif // COMPONENTARRAY_H
