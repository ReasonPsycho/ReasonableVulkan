//
// Created by redkc on 14/10/2025.
//

#ifndef REASONABLEVULKAN_SHOWIMGUIMODEL_HPP
#define REASONABLEVULKAN_SHOWIMGUIMODEL_HPP

#include "AssetTypes.hpp"
#include "ecs/Scene.h"
#include "systems/renderingSystem/componets/Model.hpp"

inline void  ShowImGuiModel(Scene* scene,Model* model)
{
    if (ImGui::CollapsingHeader("Model"))
    {
        if (ImGui::Button(model->modelUuid.is_nil() ? "Select Model" : boost::uuids::to_string(model->modelUuid).c_str()))
        {
            ImGui::OpenPopup("Model List");
        }

        if (ImGui::BeginPopup("Model List"))
        {
            for (const auto& assetInfo : scene->engine.assetManagerInterface->getRegisteredAssets(am::AssetType::Model))
            {
                const std::string idStr = boost::uuids::to_string(assetInfo.get()->id);
                if (ImGui::MenuItem(idStr.c_str()))
                {
                    model->modelUuid = assetInfo.get()->id;
                }
            }
            ImGui::EndPopup();
        }
    }
}


#endif //REASONABLEVULKAN_SHOWIMGUIMODEL_HPP