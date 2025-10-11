//
// Created by redkc on 05/08/2025.
//

#ifndef CAMERA_H
#define CAMERA_H
#include <glm/glm.hpp>
#include <imgui.h>
#include "ecs/Component.hpp"

namespace engine::ecs
{
    struct Camera : public Component
    {
        glm::mat4 projection;
        glm::mat4 view;
        glm::vec4 lightpos;

        Camera()
    : projection(glm::mat4(1.0f))  // Identity matrix
    , view(glm::mat4(1.0f))        // Identity matrix
    , lightpos(glm::vec4(0.0f))       // Zero vector
{}
        Camera(const glm::mat4& projection, const glm::mat4& view, const glm::vec4& light)
            : projection(projection), view(view), lightpos(light) {}
        void ImGuiComponent() override {
            ImGui::Text("Camera");
        }
    };
}
#endif //CAMERA_H
