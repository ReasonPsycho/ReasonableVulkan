#ifndef COMPONENTARRAY_H
#define COMPONENTARRAY_H

#include "Types.h"
#include <array>
#include <bitset>
#include <cassert>
#include "IComponentArray.h"

namespace engine::ecs {
    template<typename T>
    class ComponentArray : public IComponentArray {
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

    private:
        std::array<T, MAX_COMPONENTS_ARRAY> componentArray{};
        std::bitset<MAX_COMPONENTS_ARRAY> activeComponents{};
        std::unordered_map<Entity, std::uint32_t> entityToIndex{};
        std::queue<std::uint32_t> freeComponents;
        uint32_t maxComponentIndex = 0;

    };
}

#include "ComponentArray.tpp"

#endif // COMPONENTARRAY_H
