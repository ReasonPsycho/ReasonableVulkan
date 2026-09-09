//
// Created by redkc on 01/10/2025.
//

#ifndef REASONABLEVULKAN_MODEL_HPP
#define REASONABLEVULKAN_MODEL_HPP

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <glm/vec3.hpp>

#include "ecs/Component.hpp"

namespace engine::ecs
{
    class Scene;

    struct RendererComponent : public Component
    {
        [[=Tooltip{"Model asset UUID"}]]
        boost::uuids::uuid modelUuid{boost::uuids::nil_uuid()};

        [[=Tooltip{"Shader program asset UUID"}]]
        boost::uuids::uuid shaderUuid{boost::uuids::nil_uuid()};

        RendererComponent() = default;
        explicit RendererComponent(boost::uuids::uuid modelId, boost::uuids::uuid shaderId = boost::uuids::nil_uuid())
            : modelUuid(modelId), shaderUuid(shaderId) {}

        void CustomDrawImGui(Scene* scene);
    };
}

#endif //REASONABLEVULKAN_MODEL_HPP