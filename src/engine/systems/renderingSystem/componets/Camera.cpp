//
// Created by redkc on 15/10/2025.
//

    // Utility functions

#include <imgui.h>
#include "Camera.hpp"
#include "ecs/Scene.h"

    inline void Camera::ShowImGui(Scene* scene,Component* component) const
    {
auto typed = dynamic_cast<Camera*>(component);

        if (ImGui::CollapsingHeader("Camera"))
        {
            bool changed = false;
            changed |= ImGui::SliderFloat("FOV",&typed->fov, 1.0f, 120.0f);
            changed |= ImGui::DragFloat("Aspect Ratio", &typed->aspectRatio, 0.01f, 0.1f, 10.0f);
            changed |= ImGui::DragFloat("Near Plane", &typed->nearPlane, 0.01f, 0.001f, typed->farPlane);
            changed |= ImGui::DragFloat("Far Plane", &typed->farPlane, 1.0f, typed->nearPlane, 10000.0f);

            if (changed) {
                typed->isDirty = true;
            }

            if (ImGui::TreeNode("Matrices"))
            {
                ImGui::Text("View Matrix:");
                for (int i = 0; i < 4; i++) {
                    ImGui::Text("%.2f %.2f %.2f %.2f",
                                typed->view[i][0], typed->view[i][1], typed->view[i][2], typed->view[i][3]);
                }

                ImGui::Text("\nProjection Matrix:");
                for (int i = 0; i < 4; i++) {
                    ImGui::Text("%.2f %.2f %.2f %.2f",
                                typed->projection[i][0], typed->projection[i][1], typed->projection[i][2], typed->projection[i][3]);
                }
                ImGui::TreePop();
            }
        }
    }

