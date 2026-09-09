//
// Created by redkc on 15/10/2025.
//
#include <imgui.h>
#include "RendererComponent.hpp"

#include "Asset.hpp"
#include "AssetTypes.hpp"
#include "ecs/Scene.h"


void engine::ecs::RendererComponent::CustomDrawImGui(Scene* scene)
{
    std::string buttonText = (shaderUuid.is_nil() ? "Select Shader Program" : boost::uuids::to_string(shaderUuid)) + "##RendererShader";
    if (ImGui::Button(buttonText.c_str()))
    {
        ImGui::OpenPopup("Shader Program List");
    }

    if (scene && ImGui::BeginPopup("Shader Program List"))
    {
        for (const auto& lookUpName : scene->engine.assetManagerInterface->getRegisteredAssetsNames(am::AssetType::ShaderProgram))
        {
            if (ImGui::MenuItem(lookUpName.c_str()))
            {
                shaderUuid = scene->engine.assetManagerInterface->getAssetUuid(lookUpName).value();
            }
        }
        ImGui::EndPopup();
    }
}
