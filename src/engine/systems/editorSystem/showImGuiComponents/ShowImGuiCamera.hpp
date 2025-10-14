//
// Created by redkc on 14/10/2025.
//

#ifndef REASONABLEVULKAN_SHOWIMGUICAMERA_HPP
#define REASONABLEVULKAN_SHOWIMGUICAMERA_HPP
#include "ecs/Scene.h"
#include "systems/renderingSystem/componets/camera.hpp"

inline void  ShowImGuiCamera(Scene* scene,Camera* camera) {
    if (ImGui::CollapsingHeader("Camera"))
    {
        bool changed = false;
        changed |= ImGui::SliderFloat("FOV", &camera->fov, 1.0f, 120.0f);
        changed |= ImGui::DragFloat("Aspect Ratio", &camera->aspectRatio, 0.01f, 0.1f, 10.0f);
        changed |= ImGui::DragFloat("Near Plane", &camera->nearPlane, 0.01f, 0.001f, camera->farPlane);
        changed |= ImGui::DragFloat("Far Plane", &camera->farPlane, 1.0f, camera->nearPlane, 10000.0f);

        if (changed) {
            camera->isDirty = true;
        }

        if (ImGui::TreeNode("Matrices"))
        {
            ImGui::Text("View Matrix:");
            for (int i = 0; i < 4; i++) {
                ImGui::Text("%.2f %.2f %.2f %.2f",
                    camera->view[i][0], camera->view[i][1], camera->view[i][2], camera->view[i][3]);
            }

            ImGui::Text("\nProjection Matrix:");
            for (int i = 0; i < 4; i++) {
                ImGui::Text("%.2f %.2f %.2f %.2f",
                    camera->projection[i][0], camera->projection[i][1], camera->projection[i][2], camera->projection[i][3]);
            }
            ImGui::TreePop();
        }
    }
}


#endif //REASONABLEVULKAN_SHOWIMGUICAMERA_HPP