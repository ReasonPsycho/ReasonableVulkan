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

void Transform::SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const
{
    // Position
    rapidjson::Value posArray(rapidjson::kArrayType);
    posArray.PushBack(position.x, allocator);
    posArray.PushBack(position.y, allocator);
    posArray.PushBack(position.z, allocator);
    obj.AddMember("position", posArray, allocator);

    // Rotation (quaternion)
    rapidjson::Value rotArray(rapidjson::kArrayType);
    rotArray.PushBack(rotation.w, allocator);
    rotArray.PushBack(rotation.x, allocator);
    rotArray.PushBack(rotation.y, allocator);
    rotArray.PushBack(rotation.z, allocator);
    obj.AddMember("rotation", rotArray, allocator);

    // Scale
    rapidjson::Value scaleArray(rapidjson::kArrayType);
    scaleArray.PushBack(scale.x, allocator);
    scaleArray.PushBack(scale.y, allocator);
    scaleArray.PushBack(scale.z, allocator);
    obj.AddMember("scale", scaleArray, allocator);
}

void Transform::DeserializeFromJson(const rapidjson::Value& obj)
{
    if (obj.HasMember("position") && obj["position"].IsArray()) {
        const auto& posArray = obj["position"];
        if (posArray.Size() == 3) {
            position = glm::vec3(
                posArray[0].GetFloat(),
                posArray[1].GetFloat(),
                posArray[2].GetFloat()
            );
        }
    }

    if (obj.HasMember("rotation") && obj["rotation"].IsArray()) {
        const auto& rotArray = obj["rotation"];
        if (rotArray.Size() == 4) {
            rotation = glm::quat(
                rotArray[0].GetFloat(),  // w
                rotArray[1].GetFloat(),  // x
                rotArray[2].GetFloat(),  // y
                rotArray[3].GetFloat()   // z
            );
        }
    }

    if (obj.HasMember("scale") && obj["scale"].IsArray()) {
        const auto& scaleArray = obj["scale"];
        if (scaleArray.Size() == 3) {
            scale = glm::vec3(
                scaleArray[0].GetFloat(),
                scaleArray[1].GetFloat(),
                scaleArray[2].GetFloat()
            );
        }
    }

    isDirty = true;
}
