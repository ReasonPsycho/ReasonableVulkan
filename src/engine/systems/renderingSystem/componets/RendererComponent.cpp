//
// Created by redkc on 15/10/2025.
//
#include <imgui.h>
#include "RendererComponent.hpp"

#include "Asset.hpp"
#include "AssetTypes.hpp"
#include "assetDatas/ModelData.h"
#include "ecs/Scene.h"


void RendererComponent::ShowImGui(Scene* scene, Component* component) const
{
    auto typed = dynamic_cast<RendererComponent*>(component);
    if (ImGui::CollapsingHeader("Model"))
    {
        if (ImGui::Button(typed->modelUuid.is_nil() ? "Select Model" : boost::uuids::to_string(typed->modelUuid).c_str()))
        {
            ImGui::OpenPopup("Model List");
        }
        
        if (ImGui::BeginPopup("Model List"))
        {
            for (const auto& assetLookUpName : scene->engine.assetManagerInterface->getRegisteredAssetsNames(am::AssetType::Model))
            {
                if (ImGui::MenuItem(assetLookUpName.c_str()))
                {
                    auto modelData = scene->engine.assetManagerInterface->getAssetData<am::ModelData>(assetLookUpName);
                    typed->boundingBoxMin = modelData->boundingBoxMin;
                    typed->boundingBoxMax = modelData->boundingBoxMax;
                }
            }
            ImGui::EndPopup();
        }

        if (ImGui::Button(typed->shaderUuid.is_nil() ? "Select Shader Program" : boost::uuids::to_string(typed->shaderUuid).c_str()))
        {
            ImGui::OpenPopup("Shader Program List");
        }

        if (ImGui::BeginPopup("Shader Program List"))
        {
            for (const auto& lookUpName : scene->engine.assetManagerInterface->getRegisteredAssetsNames(am::AssetType::ShaderProgram))
            {
                if (ImGui::MenuItem(lookUpName.c_str()))
                {
                    //typed->shaderUuid = assetInfo.get()->id;
                }
            }
            ImGui::EndPopup();
        }

        ImGui::DragVec3("Min bounding box", typed->boundingBoxMin);
        ImGui::DragVec3("Max bounding box", typed->boundingBoxMax);
    }

}

void RendererComponent::SerializeComponentToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const
{
    rapidjson::Value modelUuidStr;
    std::string modelUuidString = boost::uuids::to_string(modelUuid);
    modelUuidStr.SetString(modelUuidString.c_str(), allocator);
    obj.AddMember("modelUuid", modelUuidStr, allocator);

    rapidjson::Value shaderUuidStr;
    std::string shaderUuidString = boost::uuids::to_string(shaderUuid);
    shaderUuidStr.SetString(shaderUuidString.c_str(), allocator);
    obj.AddMember("shaderUuid", shaderUuidStr, allocator);
}

void RendererComponent::DeserializeComponentFromJson(const rapidjson::Value& obj)
{
    if (obj.HasMember("modelUuid") && obj["modelUuid"].IsString()) {
        std::string uuidStr = obj["modelUuid"].GetString();
        boost::uuids::string_generator gen;
        modelUuid = gen(uuidStr);
    }
    if (obj.HasMember("shaderUuid") && obj["shaderUuid"].IsString()) {
        std::string uuidStr = obj["shaderUuid"].GetString();
        boost::uuids::string_generator gen;
        shaderUuid = gen(uuidStr);
    }
}
