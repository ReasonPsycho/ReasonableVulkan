//
// Created by redkc on 01/10/2025.
//

#ifndef REASONABLEVULKAN_MODEL_HPP
#define REASONABLEVULKAN_MODEL_HPP
#include <imgui.h>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include "ecs/Component.hpp"
#include <boost/uuid/uuid_io.hpp>

namespace engine::ecs
{
    struct Model : public Component
    {
        boost::uuids::uuid modelUuid;

        // Add explicit constructor
        explicit Model() : modelUuid(boost::uuids::nil_uuid()) {}
        explicit Model(boost::uuids::uuid id) : modelUuid(id) {}

        void ImGuiComponent() override
        {
            if (ImGui::CollapsingHeader("Model"))
            {
                ImGui::Text("UUID: %s", boost::uuids::to_string(modelUuid).c_str());
            }
        }

    };
}

#endif //REASONABLEVULKAN_MODEL_HPP