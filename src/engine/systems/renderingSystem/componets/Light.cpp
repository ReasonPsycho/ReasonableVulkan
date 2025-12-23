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
            typed->type = static_cast<Type>(typeInt);
        }

        ImGui::ColorEdit3("Light Color", &typed->color[0]);
        ImGui::SliderFloat("Intensity##Light", &typed->intensity, 0.0f, 10.0f);
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
}

void engine::ecs::Light::DeserializeFromJson(const rapidjson::Value& obj)
{
    if (obj.HasMember("lightType") && obj["lightType"].IsInt()) {
        type = static_cast<Type>(obj["lightType"].GetInt());
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
}

void engine::ecs::PointLight::ShowImGui(Scene* scene, Component* component) const
{
    auto typed = dynamic_cast<PointLight*>(component);
    if (ImGui::CollapsingHeader("Point Light Data"))
    {
        ImGui::SliderFloat("Radius##PointLight", &typed->radius, 0.1f, 100.0f);
        ImGui::SliderFloat("Falloff##PointLight", &typed->falloff, 0.0f, 5.0f);
    }
}

void engine::ecs::PointLight::SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const
{
    obj.AddMember("radius", radius, allocator);
    obj.AddMember("falloff", falloff, allocator);
}

void engine::ecs::PointLight::DeserializeFromJson(const rapidjson::Value& obj)
{
    if (obj.HasMember("radius") && obj["radius"].IsNumber()) {
        radius = obj["radius"].GetFloat();
    }

    if (obj.HasMember("falloff") && obj["falloff"].IsNumber()) {
        falloff = obj["falloff"].GetFloat();
    }
}

void engine::ecs::SpotLight::ShowImGui(Scene* scene, Component* component) const
{
    auto typed = dynamic_cast<SpotLight*>(component);
    if (ImGui::CollapsingHeader("Spot Light Data"))
    {
        ImGui::SliderFloat("Inner Angle##SpotLight", &typed->innerAngle, 0.0f, 90.0f);
        ImGui::SliderFloat("Outer Angle##SpotLight", &typed->outerAngle, 0.0f, 90.0f);
        ImGui::SliderFloat("Range##SpotLight", &typed->range, 0.1f, 200.0f);
    }
}

void engine::ecs::SpotLight::SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const
{
    obj.AddMember("innerAngle", innerAngle, allocator);
    obj.AddMember("outerAngle", outerAngle, allocator);
    obj.AddMember("range", range, allocator);
}

void engine::ecs::SpotLight::DeserializeFromJson(const rapidjson::Value& obj)
{
    if (obj.HasMember("innerAngle") && obj["innerAngle"].IsNumber()) {
        innerAngle = obj["innerAngle"].GetFloat();
    }

    if (obj.HasMember("outerAngle") && obj["outerAngle"].IsNumber()) {
        outerAngle = obj["outerAngle"].GetFloat();
    }

    if (obj.HasMember("range") && obj["range"].IsNumber()) {
        range = obj["range"].GetFloat();
    }
}