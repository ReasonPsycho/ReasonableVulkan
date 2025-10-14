//
// Created by redkc on 14/10/2025.
//

#ifndef REASONABLEVULKAN_SHOWIMGUITRANSFORM_HPP
#define REASONABLEVULKAN_SHOWIMGUITRANSFORM_HPP
#include "ecs/Scene.h"
#include "systems/transformSystem/componets/transform.hpp"

inline void ShowImGuiTransform(Scene* scene,Transform* transform)
{
    if (ImGui::CollapsingHeader("Transform"))
    {
        if (ImGui::DragVec3("Position", transform->position, 0.1f))
        {
            transform->isDirty = true;
        }

        // For rotation, we'll show it as Euler angles in degrees for easier editing
        glm::vec3 eulerDegrees = glm::degrees(glm::eulerAngles(transform->rotation));
        if (ImGui::DragVec3("Rotation", eulerDegrees, 1.0f))
        {
            setLocalRotationFromEulerDegrees(*transform, eulerDegrees);
        }

        if (ImGui::DragVec3("Scale", transform->scale, 0.1f))
        {
            transform->isDirty = true;
        }

        if (ImGui::TreeNode("Matrices"))
        {
            ImGui::DisplayMat4("Local Matrix", transform->localMatrix);
            ImGui::DisplayMat4("Global Matrix", transform->globalMatrix);
            ImGui::TreePop();
        }
    }
}


#endif //REASONABLEVULKAN_SHOWIMGUITRANSFORM_HPP