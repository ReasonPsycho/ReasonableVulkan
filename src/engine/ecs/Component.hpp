//
// Created by redkc on 10/10/2025.
//

#ifndef REASONABLEVULKAN_COMPONENT_HPP
#define REASONABLEVULKAN_COMPONENT_HPP

namespace engine::ecs
{
    class Scene;

    struct Component
    {
    public:
        virtual ~Component() = 0;;
    };

    // Provide implementation for the pure virtual destructor in the header
    inline Component::~Component() = default;
}
#endif //REASONABLEVULKAN_COMPONENT_HPP
