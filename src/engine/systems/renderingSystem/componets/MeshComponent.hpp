//
// Created by redkc on 01/10/2025.
//

#ifndef REASONABLEVULKAN_MESHCOMPONENT_HPP
#define REASONABLEVULKAN_MESHCOMPONENT_HPP

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "ecs/Component.hpp"

namespace engine::ecs
{
    class Scene;

    struct MeshComponent : public Component
    {
        [[=Tooltip{"Model asset UUID"}]]
        boost::uuids::uuid modelUuid{boost::uuids::nil_uuid()};

        MeshComponent() = default;
        explicit MeshComponent(boost::uuids::uuid modelId)
            : modelUuid(modelId) {}

        void CustomDrawImGui(Scene* scene);
    };
}

#endif //REASONABLEVULKAN_MESHCOMPONENT_HPP
