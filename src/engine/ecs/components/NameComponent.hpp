#ifndef REASONABLEVULKAN_NAMECOMPONENT_HPP
#define REASONABLEVULKAN_NAMECOMPONENT_HPP

#include <string>
#include <utility>
#include "ecs/Component.hpp"

namespace engine::ecs
{
    struct [[=Integral{}]] NameComponent : public Component
    {
        [[=Tooltip{"Name of the entity"}]]
        std::string name{"Entity"};

        NameComponent() = default;
        explicit NameComponent(std::string name) : name(std::move(name)) {}
    };
}

#endif //REASONABLEVULKAN_NAMECOMPONENT_HPP
