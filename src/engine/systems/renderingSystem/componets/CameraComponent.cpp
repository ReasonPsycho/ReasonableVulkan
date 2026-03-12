//
// Created by redkc on 15/10/2025.
//

    // Utility functions

#include <imgui.h>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>
#include "CameraComponent.hpp"
#include "ecs/Scene.h"

    inline void CameraComponent::ShowImGui(Scene* scene,Component* component) const
    {
auto typed = dynamic_cast<CameraComponent*>(component);

        if (ImGui::CollapsingHeader("Camera"))
        {
            bool changed = false;
            changed |= ImGui::SliderFloat("FOV",&typed->fov, 1.0f, 120.0f);
            changed |= ImGui::DragFloat("Aspect Ratio", &typed->aspectRatio, 0.01f, 0.1f, 10.0f);
            changed |= ImGui::DragFloat("Near Plane", &typed->nearPlane, 0.01f, 0.001f, typed->farPlane);
            changed |= ImGui::DragFloat("Far Plane", &typed->farPlane, 1.0f, typed->nearPlane, 10000.0f);
            
            std::string skyboxIdStr = boost::uuids::to_string(typed->skyboxTextureId);
            if (ImGui::InputText("Skybox Texture UUID", &skyboxIdStr[0], skyboxIdStr.size() + 1)) {
                try {
                    typed->skyboxTextureId = boost::uuids::string_generator()(skyboxIdStr);
                    changed = true;
                } catch (...) {}
            }

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

void CameraComponent::SerializeComponentToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const
{
    obj.AddMember("fov", fov, allocator);
    obj.AddMember("aspectRatio", aspectRatio, allocator);
    obj.AddMember("nearPlane", nearPlane, allocator);
    obj.AddMember("farPlane", farPlane, allocator);

    std::string skyboxIdStr = boost::uuids::to_string(skyboxTextureId);
    rapidjson::Value skyboxIdVal;
    skyboxIdVal.SetString(skyboxIdStr.c_str(), allocator);
    obj.AddMember("skyboxTextureId", skyboxIdVal, allocator);
}

void CameraComponent::DeserializeComponentFromJson(const rapidjson::Value& obj)
{
    if (obj.HasMember("fov") && obj["fov"].IsFloat()) fov = obj["fov"].GetFloat();
    if (obj.HasMember("aspectRatio") && obj["aspectRatio"].IsFloat()) aspectRatio = obj["aspectRatio"].GetFloat();
    if (obj.HasMember("nearPlane") && obj["nearPlane"].IsFloat()) nearPlane = obj["nearPlane"].GetFloat();
    if (obj.HasMember("farPlane") && obj["farPlane"].IsFloat()) farPlane = obj["farPlane"].GetFloat();



    if (obj.HasMember("skyboxTextureId") && obj["skyboxTextureId"].IsString()) {
        try {
            skyboxTextureId = boost::uuids::string_generator()(obj["skyboxTextureId"].GetString());
        } catch (...) {
            skyboxTextureId = boost::uuids::nil_uuid();
        }
    }
    isDirty = true;
}

