//
// Created by redkc on 15/10/2025.
//
#include <imgui.h>
#include "Model.hpp"
#include "AssetTypes.hpp"
#include "ecs/Scene.h"


void Model::ShowImGui(Scene* scene, Component* component) const
{
    auto typed = dynamic_cast<Model*>(component);
    if (ImGui::CollapsingHeader("Model"))
    {
        if (ImGui::Button(typed->modelUuid.is_nil() ? "Select Model" : boost::uuids::to_string(typed->modelUuid).c_str()))
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
                    typed->modelUuid = assetInfo.get()->id;
                }
            }
            ImGui::EndPopup();
        }
    }

}
