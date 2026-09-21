//
// Created by redkc on 15/10/2025.
//
#include <imgui.h>
#include "MeshComponent.hpp"

#include "Asset.hpp"
#include "AssetTypes.hpp"
#include "assetDatas/ModelData.h"
#include "ecs/Scene.h"

void engine::ecs::MeshComponent::CustomDrawImGui(Scene* scene)
{
    std::string buttonText = (modelUuid.is_nil() ? "Select Model" : boost::uuids::to_string(modelUuid)) + "##MeshModel";
    if (ImGui::Button(buttonText.c_str()))
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
                runtimeModelHandle = gfx::ModelHandle::invalid();
            }
        }
        ImGui::EndPopup();
    }

    if (scene && !modelUuid.is_nil())
    {
        if (scene->engine.assetManagerInterface->getAsset(modelUuid).has_value())
        {
            auto modelData = scene->engine.assetManagerInterface->getAssetData<am::ModelData>(modelUuid);
            if (modelData)
            {
                ImGui::DragVec3("Min bounding box", modelData->boundingBoxMin);
                ImGui::DragVec3("Max bounding box", modelData->boundingBoxMax);
            }
        }
    }
}
