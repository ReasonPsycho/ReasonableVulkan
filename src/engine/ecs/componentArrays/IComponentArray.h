#ifndef COMPONENTARRAYBASE_H
#define COMPONENTARRAYBASE_H

#include "../Types.h"
#include "ecs/Component.hpp"

namespace engine::ecs
{
    using ComponentIndex = std::uint32_t;

    class IComponentArray {
    public:
        virtual ~IComponentArray() = default;

        // Untyped interface
        virtual void AddComponentUntyped(Entity entity) = 0;
        virtual void RemoveComponentUntyped(Entity entity) = 0;
        virtual bool HasComponentUntyped(Entity entity) const = 0;
        virtual void SetComponentActiveUntyped(Entity entity, bool active) = 0;
        virtual bool IsComponentActiveUntyped(Entity entity) const = 0;
        virtual Component& GetComponentUntyped(Entity entity) = 0;  // Changed to return const reference
    };
}
#endif // COMPONENTARRAYBASE_H