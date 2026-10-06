//
// Created by redkc on 15/10/2025.
//
#include <imgui.h>
#include "MeshComponent.hpp"

#include "Asset.hpp"
#include "AssetTypes.hpp"
#include "assetDatas/MeshData.h"
#include "ecs/Scene.h"

void engine::ecs::MeshComponent::CustomDrawImGui(Scene* scene)
{
    std::string buttonText = (meshUuid.is_nil() ? "Select Mesh" : boost::uuids::to_string(meshUuid)) + "##MeshModel";
    if (ImGui::Button(buttonText.c_str()))
    {
        ImGui::OpenPopup("Mesh List");
    }
    
    if (scene && ImGui::BeginPopup("Mesh List"))
    {
        for (const auto& assetLookUpName : scene->engine.assetManagerInterface->getRegisteredAssetsNames(am::AssetType::Mesh))
        {
            if (ImGui::MenuItem(assetLookUpName.c_str()))
            {
                meshUuid = scene->engine.assetManagerInterface->getAssetUuid(assetLookUpName).value();
                runtimeMeshHandle = gfx::MeshHandle::invalid();
            }
        }
        ImGui::EndPopup();
    }

    if (scene && !meshUuid.is_nil())
    {
        if (scene->engine.assetManagerInterface->getAsset(meshUuid).has_value())
        {
            auto meshData = scene->engine.assetManagerInterface->getAssetData<am::MeshData>(meshUuid);
            if (meshData)
            {
                ImGui::DragVec3("Min bounding box", meshData->boundingBoxMin);
                ImGui::DragVec3("Max bounding box", meshData->boundingBoxMax);
            }
        }
    }
}
