#ifndef COMPONENTINTEGRALARRAY_H
#define COMPONENTINTEGRALARRAY_H

#include "Types.h"
#include <array>
#include <bitset>
#include <cassert>
#include "IComponentArray.h"

namespace engine::ecs {
    template<typename T>
    class IntegralComponentArray : public IComponentArray {
    public:
        void AddComponentToEntity(std::uint32_t entity, T component);
        void RemoveComponentFronEntity(std::uint32_t entity);
        T& GetComponent(std::uint32_t entity);
        bool HasComponent(std::uint32_t entity) const;
        void SetComponentActive(Entity entity, bool active);
        bool IsComponentActive(Entity entity) const;
        std::array<T, MAX_ENTITIES>& GetComponents();

        // Untyped interface overrides
        void RemoveComponentUntyped(std::uint32_t entity) override;
        bool HasComponentUntyped(std::uint32_t entity) const override;
        void SetComponentActiveUntyped(std::uint32_t entity, bool active) override;
        bool IsComponentActiveUntyped(std::uint32_t entity) const override;

        std::array<T, MAX_ENTITIES> componentArray{};
    private:
        std::bitset<MAX_ENTITIES> activeComponents{};
    };
}

#include "IntegralComponentArray.tpp"

#endif // COMPONENTARRAY_H
