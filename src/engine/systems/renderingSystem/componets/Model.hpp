//
// Created by redkc on 01/10/2025.
//

#ifndef REASONABLEVULKAN_MODEL_HPP
#define REASONABLEVULKAN_MODEL_HPP

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include "ecs/Component.hpp"

namespace engine::ecs
{
    struct Model : public Component
    {
        boost::uuids::uuid modelUuid;

        // Add explicit constructor
        Model() : modelUuid(boost::uuids::nil_uuid()) {}
        explicit Model(boost::uuids::uuid id) : modelUuid(id) {}

        void ShowImGui(Scene* scene,Component* component) const override;
    };
}

#endif //REASONABLEVULKAN_MODEL_HPP