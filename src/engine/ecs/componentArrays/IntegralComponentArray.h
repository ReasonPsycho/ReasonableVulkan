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
        ComponentID AddComponentToEntity(Entity entity, T component);
        ComponentID RemoveComponentFronEntity(Entity entity);
        T& GetComponentFromEntity(Entity entity);
        T& GetComponent(ComponentID componentId);
        bool HasComponent(Entity entity) const;
        void SetComponentActive(Entity entity, bool active);
        bool IsComponentActive(Entity entity) const;
        std::array<T, MAX_ENTITIES>& GetComponents();

        // Untyped interface overrides
        ComponentID AddComponentUntyped(Entity entity) override;
        Component& GetComponentUntyped(Entity entity) override;
        void RemoveComponentUntyped(Entity entity) override;
        bool HasComponentUntyped(Entity entity) const override;
        void SetComponentActiveUntyped(Entity entity, bool active) override;
        bool IsComponentActiveUntyped(Entity entity) const override;

        void SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
        void DeserializeFromJson(const rapidjson::Value& obj) override;

        std::array<T, MAX_ENTITIES> componentArray{};
    private:
        std::bitset<MAX_ENTITIES> activeComponents{};
    };
}

#include "IntegralComponentArray.tpp"

#endif // COMPONENTARRAY_H
