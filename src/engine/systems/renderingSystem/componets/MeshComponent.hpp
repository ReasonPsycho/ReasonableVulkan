//
// Created by redkc on 01/10/2025.
//

#ifndef REASONABLEVULKAN_MESHCOMPONENT_HPP
#define REASONABLEVULKAN_MESHCOMPONENT_HPP

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "Handle.hpp"
#include "ecs/Component.hpp"

namespace engine::ecs
{
    class Scene;

    struct MeshComponent : public Component
    {
        [[=Tooltip{"Model asset UUID"}]]
        boost::uuids::uuid modelUuid{boost::uuids::nil_uuid()};

        [[=NonSerialized{}]]
        gfx::ModelHandle runtimeModelHandle = gfx::ModelHandle::invalid();

        MeshComponent() = default;
        explicit MeshComponent(boost::uuids::uuid modelId)
            : modelUuid(modelId) {}
        explicit MeshComponent(boost::uuids::uuid modelId, gfx::ModelHandle handle)
            : modelUuid(modelId), runtimeModelHandle(handle) {}

        void CustomDrawImGui(Scene* scene);
    };
}

#endif //REASONABLEVULKAN_MESHCOMPONENT_HPP
