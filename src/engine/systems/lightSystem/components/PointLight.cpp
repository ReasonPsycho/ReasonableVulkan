
//
// Created by redkc on 24/10/2025.
//

#include "PointLight.hpp"
#include <imgui.h>

namespace engine::ecs
{
    inline void PointLight::ShowImGui(Scene* scene, Component* component) const
    {

        auto typed = dynamic_cast<PointLight*>(component);

        ImGui::ColorEdit3("Color", &typed->color.x);
        ImGui::DragFloat("Intensity", &typed->intensity, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Constant", &typed->constant, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Linear", &typed->linear, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Quadratic", &typed->quadratic, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("Radius", &typed->radius, 0.1f, 0.0f, 100.0f);
    }

    inline void PointLight::SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const
    {
        obj.AddMember("type", "PointLight", allocator);
        
        rapidjson::Value colorArray(rapidjson::kArrayType);
        colorArray.PushBack(color.r, allocator);
        colorArray.PushBack(color.g, allocator);
        colorArray.PushBack(color.b, allocator);
        obj.AddMember("color", colorArray, allocator);
        
        obj.AddMember("intensity", intensity, allocator);
        obj.AddMember("constant", constant, allocator);
        obj.AddMember("linear", linear, allocator);
        obj.AddMember("quadratic", quadratic, allocator);
        obj.AddMember("radius", radius, allocator);
    }

    inline void PointLight::DeserializeFromJson(const rapidjson::Value& obj)
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
        
        if (obj.HasMember("constant")) {
            constant = obj["constant"].GetFloat();
        }
        
        if (obj.HasMember("linear")) {
            linear = obj["linear"].GetFloat();
        }
        
        if (obj.HasMember("quadratic")) {
            quadratic = obj["quadratic"].GetFloat();
        }
        
        if (obj.HasMember("radius")) {
            radius = obj["radius"].GetFloat();
        }
    }
}