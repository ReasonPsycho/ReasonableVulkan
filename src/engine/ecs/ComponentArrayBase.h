#ifndef COMPONENTARRAYBASE_H
#define COMPONENTARRAYBASE_H

#include <cstdint>

namespace engine::ecs
{
    class ComponentArrayBase {
    public:
        virtual ~ComponentArrayBase() = default;

        // Untyped interface
        virtual void RemoveComponentUntyped(std::uint32_t entity) = 0;
        virtual bool HasComponentUntyped(std::uint32_t entity) const = 0;
        virtual void SetComponentActiveUntyped(std::uint32_t entity, bool active) = 0;
        virtual bool IsComponentActiveUntyped(std::uint32_t entity) const = 0;
    };
}
#endif // COMPONENTARRAYBASE_H