//
// Created by redkc on 15/10/2025.
//
#include <imgui.h>
#include "RendererComponent.hpp"

#include "Asset.hpp"
#include "AssetTypes.hpp"
#include "assetDatas/ModelData.h"
#include "ecs/Scene.h"


void RendererComponent::CustomDrawImGui(Scene* scene)
{
    if (ImGui::Button(modelUuid.is_nil() ? "Select Model" : boost::uuids::to_string(modelUuid).c_str()))
    {
        ImGui::OpenPopup("Model List");
    }
    
    if (scene && ImGui::BeginPopup("Model List"))
    {
        for (const auto& assetLookUpName : scene->engine.assetManagerInterface->getRegisteredAssetsNames(am::AssetType::Model))
        {
            if (ImGui::MenuItem(assetLookUpName.c_str()))
            {
                modelUuid = scene->engine.assetManagerInterface->getAssetUuid(assetLookUpName).value();
            }
        }
        ImGui::EndPopup();
    }

    if (ImGui::Button(shaderUuid.is_nil() ? "Select Shader Program" : boost::uuids::to_string(shaderUuid).c_str()))
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

    if (scene && !modelUuid.is_nil())
    {
        auto modelData = scene->engine.assetManagerInterface->getAssetData<am::ModelData>(modelUuid);
        if (modelData)
        {
            ImGui::DragVec3("Min bounding box", modelData->boundingBoxMin);
            ImGui::DragVec3("Max bounding box", modelData->boundingBoxMax);
        }
    }
}
