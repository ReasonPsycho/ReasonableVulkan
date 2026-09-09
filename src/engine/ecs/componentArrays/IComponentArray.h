#ifndef COMPONENTARRAYBASE_H
#define COMPONENTARRAYBASE_H

#include "../Types.h"
#include "ecs/Component.hpp"
#include <string_view>

namespace engine::ecs
{

    class IComponentArray {
    public:
        virtual ~IComponentArray() = default;

        virtual std::string_view GetName() const = 0;

        // Untyped interface
        virtual ComponentID AddComponentUntyped(Entity entity) = 0;
        virtual void RemoveComponentUntyped(Entity entity) = 0;
        virtual bool HasComponentUntyped(Entity entity) const = 0;
        virtual void SetComponentActiveUntyped(Entity entity, bool active) = 0;
        virtual bool IsComponentActiveUntyped(ComponentID entity) const = 0;
        virtual void* GetComponentUntyped(Entity entity) = 0;

        virtual void SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const = 0;
        virtual void DeserializeFromJson(const rapidjson::Value& obj) = 0;
    };
}
#endif // COMPONENTARRAYBASE_H