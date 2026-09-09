
//
// Created by redkc on 22/12/2025.
//

#include <imgui.h>
#include "LightComponent.hpp"
#include "ecs/Scene.h"

void engine::ecs::LightComponent::CustomDrawImGui(Scene* scene)
{
    int typeInt = static_cast<int>(type);
    const char* items[] = {"Directional", "Point", "Spot"};

    if (ImGui::Combo("Light Type", &typeInt, items, IM_ARRAYSIZE(items)))
    {
        setType(static_cast<Type>(typeInt));
    }

    ImGui::ColorEdit3("Color", &color.x);
    ImGui::SliderFloat("Intensity", &intensity, 0.0f, 10.0f);
    ImGui::Checkbox("Has Shadow", &hasShadow);

    std::visit([scene](auto& specificData) {
        DrawComponentFields(specificData, scene);
    }, data);
}