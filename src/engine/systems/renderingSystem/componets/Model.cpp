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

void Model::SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const
{
    rapidjson::Value uuidStr;
    std::string uuidString = boost::uuids::to_string(modelUuid);
    uuidStr.SetString(uuidString.c_str(), allocator);
    obj.AddMember("modelUuid", uuidStr, allocator);
}

void Model::DeserializeFromJson(const rapidjson::Value& obj)
{
    if (obj.HasMember("modelUuid") && obj["modelUuid"].IsString()) {
        std::string uuidStr = obj["modelUuid"].GetString();
        boost::uuids::string_generator gen;
        modelUuid = gen(uuidStr);
    }
}
