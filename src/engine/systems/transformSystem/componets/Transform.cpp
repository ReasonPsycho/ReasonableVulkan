//
// Created by redkc on 15/10/2025.
//

#include <imgui.h>
#include "Transform.hpp"
#include "ecs/Scene.h"


void Transform::ShowImGui(Scene* scene, Component* component) const
{
    auto typed = dynamic_cast<Transform*>(component);
    if (ImGui::CollapsingHeader("Transform"))
    {
        if (ImGui::DragVec3("Position", typed->position, 0.1f))
        {
            typed->isDirty = true;
        }

        // For rotation, we'll show it as Euler angles in degrees for easier editing
        glm::vec3 eulerDegrees = glm::degrees(glm::eulerAngles(typed->rotation));
        if (ImGui::DragVec3("Rotation", eulerDegrees, 1.0f))
        {
            setLocalRotationFromEulerDegrees(*typed, eulerDegrees);
        }

        if (ImGui::DragVec3("Scale", typed->scale, 0.1f))
        {
            typed->isDirty = true;
        }

        if (ImGui::TreeNode("Matrices"))
        {
            ImGui::DisplayMat4("Local Matrix", typed->localMatrix);
            ImGui::DisplayMat4("Global Matrix", typed->globalMatrix);
            ImGui::TreePop();
        }
    }
}
