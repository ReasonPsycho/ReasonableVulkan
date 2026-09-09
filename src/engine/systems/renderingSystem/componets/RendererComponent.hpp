//
// Created by redkc on 01/10/2025.
//

#ifndef REASONABLEVULKAN_RENDERERCOMPONENT_HPP
#define REASONABLEVULKAN_RENDERERCOMPONENT_HPP

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "ecs/Component.hpp"

namespace engine::ecs
{
    class Scene;

    struct RendererComponent : public Component
    {
        [[=Tooltip{"Shader program asset UUID"}]]
        boost::uuids::uuid shaderUuid{boost::uuids::nil_uuid()};

        RendererComponent() = default;
        explicit RendererComponent(boost::uuids::uuid shaderId)
            : shaderUuid(shaderId) {}

        void CustomDrawImGui(Scene* scene);
    };
}

#endif //REASONABLEVULKAN_RENDERERCOMPONENT_HPP