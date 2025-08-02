//
// Created by redkc on 02/08/2025.
//

#ifndef COMPONENTARRAYBASE_H
#define COMPONENTARRAYBASE_H
namespace engine::ecs
{
    class ComponentArrayBase {

    public:
        virtual ~ComponentArrayBase() = default;

        // Add a component to entity - generic interface takes entity ID and raw void pointer to component data
        virtual void AddComponentToEntity(std::uint32_t entity, const void* component) = 0;

        // Remove component from entity
        virtual void RemoveComponentFronEntity(std::uint32_t entity) = 0;

        // Check if entity has component
        virtual bool HasComponent(std::uint32_t entity) const = 0;

        // Set component active/inactive
        virtual void SetComponentActive(std::uint32_t entity, bool active) = 0;

        // Check if component is active
        virtual bool IsComponentActive(std::uint32_t entity) const = 0;
    };
}
#endif //COMPONENTARRAYBASE_H
