//
// Created by redkc on 02/08/2025.
//

#ifndef COMPONENTARRAY_H
#define COMPONENTARRAY_H
#include "Types.h"
#include <unordered_map>
#include <memory>
#include <cassert>
#include "ecs/systems/Transform.h"
#include "ComponentArrayBase.h"

namespace engine::ecs{
    template<typename T>
    class ComponentArray : public ComponentArrayBase {
    public:
        void AddComponentToEntity(std::uint32_t entity, T component);
        void RemoveComponentFronEntity(std::uint32_t entity);
        T& GetComponent(std::uint32_t entity);
        bool HasComponent(std::uint32_t entity) const;
        void SetComponentActive(Entity entity, bool active);
        bool IsComponentActive(Entity entity) const;
        const std::array<T, MAX_ENTITIES>& GetComponents() const;

        //Untyped interface overrides
        void RemoveComponentUntyped(std::uint32_t entity) override;
        bool HasComponentUntyped(std::uint32_t entity) const override;
        void SetComponentActiveUntyped(std::uint32_t entity, bool active) override;
        bool IsComponentActiveUntyped(std::uint32_t entity) const override;

    private:
        std::array<T, MAX_ENTITIES> componentArray;
        std::bitset<MAX_ENTITIES> activeComponents;
        std::unordered_map<Entity, std::size_t> entityToIndexMap;
        std::unordered_map<std::size_t, Entity> indexToEntityMap;
        std::size_t size = 0;
    };
};

#include "ComponentArray.tpp"

#endif //COMPONENTARRAY_H
