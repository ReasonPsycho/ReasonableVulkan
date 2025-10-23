//
// Created by redkc on 17.05.2025.
//

#include "../include/AssetInfo.hpp"
#include <spdlog/spdlog.h>
#include "AssetManager.hpp"
#include "../include/Asset.hpp"

bool am::AssetFactoryData::operator==(const AssetFactoryData& factory_context) const
{
    if (this->path != factory_context.path)
        return false;
    if (this->assetType != factory_context.assetType)
        return false;
    if (this->assimpIndex != factory_context.assimpIndex)
        return false;

    return true;
}


am::Asset *am::AssetInfo::getAsset() {
    return AssetManager::getInstance().getAsset(id).value();
}

am::Asset *am::AssetInfo::getAssetWithRelisingScene() {
    auto* asset = AssetManager::getInstance().getAsset(id).value();
    AssetManager& assetManager = AssetManager::getInstance();
    Assimp::Importer& importer = assetManager.importer;
    importer.FreeScene();
    return asset;
}

bool am::AssetInfo::isAssetLoaded() const {
    return isLoaded;
}

void am::AssetInfo::SerializeToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const
{
    // Convert UUID to string
    std::string uuidStr = boost::uuids::to_string(id);

    obj.AddMember("id", rapidjson::Value(uuidStr.c_str(), allocator), allocator);
    obj.AddMember("path", rapidjson::Value(path.c_str(), allocator), allocator);
    obj.AddMember("type", rapidjson::Value(AssetTypeToString(type).c_str(), allocator), allocator);
    obj.AddMember("contentHash", rapidjson::Value(static_cast<uint64_t>(contentHash)), allocator);

    // Add AssetFactoryData
    rapidjson::Value factoryDataObj(rapidjson::kObjectType);
    factoryDataObj.AddMember("path", rapidjson::Value(assetFactoryData.path.c_str(), allocator), allocator);
    factoryDataObj.AddMember("assetType", rapidjson::Value(AssetTypeToString(assetFactoryData.assetType).c_str(), allocator), allocator);
    factoryDataObj.AddMember("assimpIndex", rapidjson::Value(assetFactoryData.assimpIndex), allocator);

    obj.AddMember("assetFactoryData", factoryDataObj, allocator);
}

am::AssetInfo am::AssetInfo::DeserializeFromJson(const rapidjson::Value& obj)
{
    boost::uuids::string_generator gen;
    boost::uuids::uuid id = gen(obj["id"].GetString());

    std::string path = obj["path"].GetString();
    AssetType type = StringToAssetType(obj["type"].GetString());
    size_t contentHash = obj["contentHash"].GetUint64();

    const auto& factoryData = obj["assetFactoryData"];
    AssetFactoryData assetFactoryData(
        factoryData["path"].GetString(),
        StringToAssetType(factoryData["assetType"].GetString()),
        factoryData["assimpIndex"].GetInt()
    );

    AssetInfo info(id, path, type, contentHash, assetFactoryData);
    info.isLoaded = false;

    return info;
}
