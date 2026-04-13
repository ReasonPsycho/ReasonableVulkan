
//
// Created by redkc on 22/12/2025.
//

#include <imgui.h>
#include "LightComponent.hpp"
#include "ecs/Scene.h"

void engine::ecs::LightComponent::ShowImGui(Scene* scene, Component* component) const
{
    auto typed = dynamic_cast<LightComponent*>(component);
    if (ImGui::CollapsingHeader("LightComponent"))
    {
        int typeInt = static_cast<int>(typed->getType());
        const char* items[] = {"Directional", "Point", "Spot"};

        if (ImGui::Combo("LightComponent Type", &typeInt, items, IM_ARRAYSIZE(items)))
        {
            Type newType = static_cast<Type>(typeInt);
            typed->setType(newType);
        }

        ImGui::ColorEdit3("LightComponent Color", &typed->color[0]);
        ImGui::SliderFloat("Intensity##LightComponent", &typed->intensity, 0.0f, 10.0f);
        ImGui::Checkbox("Has Shadow##LightComponent", &typed->hasShadow);

        // Show type-specific controls
        switch (typed->getType())
        {
            case Type::Point:
            {
                auto& pointData = std::get<PointLightData>(typed->data);
                ImGui::Separator();
                ImGui::Text("Point LightComponent Settings");
                ImGui::SliderFloat("Radius##PointLight", &pointData.radius, 0.1f, 100.0f);
                ImGui::SliderFloat("Falloff##PointLight", &pointData.falloff, 0.0f, 5.0f);
                break;
            }
            case Type::Spot:
            {
                auto& spotData = std::get<SpotLightData>(typed->data);
                ImGui::Separator();
                ImGui::Text("Spot LightComponent Settings");
                ImGui::SliderFloat("Inner Angle##SpotLight", &spotData.innerAngle, 0.0f, 90.0f);
                ImGui::SliderFloat("Outer Angle##SpotLight", &spotData.outerAngle, 0.0f, 90.0f);
                ImGui::SliderFloat("Range##SpotLight", &spotData.range, 0.1f, 200.0f);
                break;
            }
            case Type::Directional:
                // No extra settings for directional lights
                break;
        }
    }
}

void engine::ecs::LightComponent::SerializeComponentToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const
{
    obj.AddMember("lightType", static_cast<int>(type), allocator);

    rapidjson::Value colorArray(rapidjson::kArrayType);
    colorArray.PushBack(color.x, allocator);
    colorArray.PushBack(color.y, allocator);
    colorArray.PushBack(color.z, allocator);
    obj.AddMember("color", colorArray, allocator);

    obj.AddMember("intensity", intensity, allocator);
    obj.AddMember("hasShadow", hasShadow, allocator);

    // Serialize type-specific data
    switch (type)
    {
        case Type::Point:
        {
            const auto& pointData = std::get<PointLightData>(data);
            obj.AddMember("radius", pointData.radius, allocator);
            obj.AddMember("falloff", pointData.falloff, allocator);
            break;
        }
        case Type::Spot:
        {
            const auto& spotData = std::get<SpotLightData>(data);
            obj.AddMember("innerAngle", spotData.innerAngle, allocator);
            obj.AddMember("outerAngle", spotData.outerAngle, allocator);
            obj.AddMember("range", spotData.range, allocator);
            break;
        }
        case Type::Directional:
            // No extra data
            break;
    }
}

void engine::ecs::LightComponent::DeserializeComponentFromJson(const rapidjson::Value& obj)
{
    if (obj.HasMember("lightType") && obj["lightType"].IsInt()) {
        setType(static_cast<Type>(obj["lightType"].GetInt()));
    }

    if (obj.HasMember("color") && obj["color"].IsArray()) {
        const auto& colorArray = obj["color"];
        if (colorArray.Size() >= 3) {
            color.x = colorArray[0].GetFloat();
            color.y = colorArray[1].GetFloat();
            color.z = colorArray[2].GetFloat();
        }
    }

    if (obj.HasMember("intensity") && obj["intensity"].IsNumber()) {
        intensity = obj["intensity"].GetFloat();
    }

    if (obj.HasMember("hasShadow") && obj["hasShadow"].IsBool()) {
        hasShadow = obj["hasShadow"].GetBool();
    }

    // Deserialize type-specific data
    switch (type)
    {
        case Type::Point:
        {
            auto& pointData = std::get<PointLightData>(data);
            if (obj.HasMember("radius") && obj["radius"].IsNumber()) {
                pointData.radius = obj["radius"].GetFloat();
            }
            if (obj.HasMember("falloff") && obj["falloff"].IsNumber()) {
                pointData.falloff = obj["falloff"].GetFloat();
            }
            break;
        }
        case Type::Spot:
        {
            auto& spotData = std::get<SpotLightData>(data);
            if (obj.HasMember("innerAngle") && obj["innerAngle"].IsNumber()) {
                spotData.innerAngle = obj["innerAngle"].GetFloat();
            }
            if (obj.HasMember("outerAngle") && obj["outerAngle"].IsNumber()) {
                spotData.outerAngle = obj["outerAngle"].GetFloat();
            }
            if (obj.HasMember("range") && obj["range"].IsNumber()) {
                spotData.range = obj["range"].GetFloat();
            }
            break;
        }
        case Type::Directional:
            // No extra data
            break;
    }
}