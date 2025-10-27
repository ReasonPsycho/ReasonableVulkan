
//
// Created by redkc on 24/10/2025.
//

#include "SpotLight.hpp"
#include <imgui.h>

namespace engine::ecs
{
    inline void SpotLight::ShowImGui(Scene* scene, Component* component) const
    {
        auto typed = dynamic_cast<SpotLight*>(component);

        ImGui::ColorEdit3("Color", &typed->color.x);
        ImGui::DragFloat("Intensity", &typed->intensity, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat3("Direction", &typed->direction.x, 0.1f);
    }

    inline void SpotLight::SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const
    {
        obj.AddMember("type", "SpotLight", allocator);
        
        rapidjson::Value colorArray(rapidjson::kArrayType);
        colorArray.PushBack(color.r, allocator);
        colorArray.PushBack(color.g, allocator);
        colorArray.PushBack(color.b, allocator);
        obj.AddMember("color", colorArray, allocator);
        
        obj.AddMember("intensity", intensity, allocator);
        
        rapidjson::Value directionArray(rapidjson::kArrayType);
        directionArray.PushBack(direction.x, allocator);
        directionArray.PushBack(direction.y, allocator);
        directionArray.PushBack(direction.z, allocator);
        obj.AddMember("direction", directionArray, allocator);
    }

    inline void SpotLight::DeserializeFromJson(const rapidjson::Value& obj)
    {
        if (obj.HasMember("color") && obj["color"].IsArray()) {
            const auto& colorArray = obj["color"];
            color.r = colorArray[0].GetFloat();
            color.g = colorArray[1].GetFloat();
            color.b = colorArray[2].GetFloat();
        }
        
        if (obj.HasMember("intensity")) {
            intensity = obj["intensity"].GetFloat();
        }
        
        if (obj.HasMember("direction") && obj["direction"].IsArray()) {
            const auto& directionArray = obj["direction"];
            direction.x = directionArray[0].GetFloat();
            direction.y = directionArray[1].GetFloat();
            direction.z = directionArray[2].GetFloat();
        }
    }
}