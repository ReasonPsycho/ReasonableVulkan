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
                runtimeShaderHandle = gfx::ShaderProgramHandle::invalid();
            }
        }
        ImGui::EndPopup();
    }

    std::string materialButtonText = (materialUuid.is_nil() ? "Select Material" : boost::uuids::to_string(materialUuid)) + "##RendererMaterial";
    if (ImGui::Button(materialButtonText.c_str()))
    {
        ImGui::OpenPopup("Material List");
    }

    if (scene && ImGui::BeginPopup("Material List"))
    {
        for (const auto& lookUpName : scene->engine.assetManagerInterface->getRegisteredAssetsNames(am::AssetType::Material))
        {
            if (ImGui::MenuItem(lookUpName.c_str()))
            {
                materialUuid = scene->engine.assetManagerInterface->getAssetUuid(lookUpName).value();
                runtimeMaterialHandle = gfx::MaterialHandle::invalid();
            }
        }
        ImGui::EndPopup();
    }
}
