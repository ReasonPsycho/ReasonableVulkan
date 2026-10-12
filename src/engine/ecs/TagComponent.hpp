#ifndef REASONABLEVULKAN_TAGCOMPONENT_HPP
#define REASONABLEVULKAN_TAGCOMPONENT_HPP

#include <string>
#include <utility>
#include "ecs/Component.hpp"

namespace engine::ecs
{
    struct TagComponent : public Component
    {
        [[=Tooltip{"Tag for grouping or categorizing entities"}]]
        std::string tag{"Untagged"};

        TagComponent() = default;
        explicit TagComponent(std::string tag) : tag(std::move(tag)) {}
    };
}

#endif //REASONABLEVULKAN_TAGCOMPONENT_HPP
