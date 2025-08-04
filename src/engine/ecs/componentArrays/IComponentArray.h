#ifndef COMPONENTARRAYBASE_H
#define COMPONENTARRAYBASE_H

#include "../Types.h"

namespace engine::ecs
{
    class IComponentArray {
    public:
        virtual ~IComponentArray() = default;

        // Untyped interface
        virtual void RemoveComponentUntyped(Entity entity) = 0;
        virtual bool HasComponentUntyped(Entity entity) const = 0;
        virtual void SetComponentActiveUntyped(Entity entity, bool active) = 0;
        virtual bool IsComponentActiveUntyped(Entity entity) const = 0;
    };
}
#endif // COMPONENTARRAYBASE_H