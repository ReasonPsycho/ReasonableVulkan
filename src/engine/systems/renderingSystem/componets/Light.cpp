
//
// Created by redkc on 22/12/2025.
//

#include <imgui.h>
#include "Light.hpp"
#include "ecs/Scene.h"

void engine::ecs::Light::ShowImGui(Scene* scene, Component* component) const
{
    auto typed = dynamic_cast<Light*>(component);
    if (ImGui::CollapsingHeader("Light"))
    {
        int typeInt = static_cast<int>(typed->type);
        const char* items[] = {"Directional", "Point", "Spot"};

        if (ImGui::Combo("Light Type", &typeInt, items, IM_ARRAYSIZE(items)))
        {
            Type newType = static_cast<Type>(typeInt);
            if (newType != typed->type)
            {
                typed->type = newType;
                // Update variant to match the new type
                switch (newType)
                {
                    case Type::Directional:
                        typed->data = DirectionalLightData{};
                        break;
                    case Type::Point:
                        typed->data = PointLightData{};
                        break;
                    case Type::Spot:
                        typed->data = SpotLightData{};
                        break;
                }
            }
        }

        ImGui::ColorEdit3("Light Color", &typed->color[0]);
        ImGui::SliderFloat("Intensity##Light", &typed->intensity, 0.0f, 10.0f);

        // Show type-specific controls
        switch (typed->type)
        {
            case Type::Point:
            {
                auto& pointData = std::get<PointLightData>(typed->data);
                ImGui::Separator();
                ImGui::Text("Point Light Settings");
                ImGui::SliderFloat("Radius##PointLight", &pointData.radius, 0.1f, 100.0f);
                ImGui::SliderFloat("Falloff##PointLight", &pointData.falloff, 0.0f, 5.0f);
                break;
            }
            case Type::Spot:
            {
                auto& spotData = std::get<SpotLightData>(typed->data);
                ImGui::Separator();
                ImGui::Text("Spot Light Settings");
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

void engine::ecs::Light::SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const
{
    obj.AddMember("lightType", static_cast<int>(type), allocator);

    rapidjson::Value colorArray(rapidjson::kArrayType);
    colorArray.PushBack(color.x, allocator);
    colorArray.PushBack(color.y, allocator);
    colorArray.PushBack(color.z, allocator);
    obj.AddMember("color", colorArray, allocator);

    obj.AddMember("intensity", intensity, allocator);

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

void engine::ecs::Light::DeserializeFromJson(const rapidjson::Value& obj)
{
    if (obj.HasMember("lightType") && obj["lightType"].IsInt()) {
        type = static_cast<Type>(obj["lightType"].GetInt());

        // Initialize variant based on type
        switch (type)
        {
            case Type::Directional:
                data = DirectionalLightData{};
                break;
            case Type::Point:
                data = PointLightData{};
                break;
            case Type::Spot:
                data = SpotLightData{};
                break;
        }
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