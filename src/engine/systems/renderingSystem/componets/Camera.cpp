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

void Camera::SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const
{
    obj.AddMember("fov", fov, allocator);
    obj.AddMember("aspectRatio", aspectRatio, allocator);
    obj.AddMember("nearPlane", nearPlane, allocator);
    obj.AddMember("farPlane", farPlane, allocator);

    rapidjson::Value lightPosArray(rapidjson::kArrayType);
    lightPosArray.PushBack(lightpos.x, allocator);
    lightPosArray.PushBack(lightpos.y, allocator);
    lightPosArray.PushBack(lightpos.z, allocator);
    lightPosArray.PushBack(lightpos.w, allocator);
    obj.AddMember("lightPosition", lightPosArray, allocator);
}

void Camera::DeserializeFromJson(const rapidjson::Value& obj)
{
    if (obj.HasMember("fov") && obj["fov"].IsFloat()) fov = obj["fov"].GetFloat();
    if (obj.HasMember("aspectRatio") && obj["aspectRatio"].IsFloat()) aspectRatio = obj["aspectRatio"].GetFloat();
    if (obj.HasMember("nearPlane") && obj["nearPlane"].IsFloat()) nearPlane = obj["nearPlane"].GetFloat();
    if (obj.HasMember("farPlane") && obj["farPlane"].IsFloat()) farPlane = obj["farPlane"].GetFloat();

    if (obj.HasMember("lightPosition") && obj["lightPosition"].IsArray()) {
        const auto& lightPosArray = obj["lightPosition"];
        if (lightPosArray.Size() == 4) {
            lightpos = glm::vec4(
                lightPosArray[0].GetFloat(),
                lightPosArray[1].GetFloat(),
                lightPosArray[2].GetFloat(),
                lightPosArray[3].GetFloat()
            );
        }
    }
    isDirty = true;
}

