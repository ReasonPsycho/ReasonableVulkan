//
// Created by redkc on 17.05.2025.
//

#include "../include/AssetInfo.hpp"
#include <spdlog/spdlog.h>
#include "AssetManager.hpp"
#include "../include/Asset.hpp"

bool am::ImportContext::operator==(const ImportContext& factory_context) const
{
    if (this->importPath != factory_context.importPath)
        return false;
    if (this->assetType != factory_context.assetType)
        return false;
    if (this->assimpIndex != factory_context.assimpIndex)
        return false;

    return true;
}

am::Asset *am::AssetInfo::getAsset() {
    return loadedAsset;
}

bool am::AssetInfo::isAssetLoaded() const {
    return isLoaded;
}

void am::AssetInfo::SerializeAssetInfoToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const
{
    // Convert UUID to string
    std::string uuidStr = boost::uuids::to_string(id);

    obj.AddMember("id", rapidjson::Value(uuidStr.c_str(), allocator), allocator);
    obj.AddMember("path", rapidjson::Value(importPath.c_str(), allocator), allocator);
    obj.AddMember("type", rapidjson::Value(AssetTypeToString(type).c_str(), allocator), allocator);
    obj.AddMember("contentHash", rapidjson::Value(static_cast<uint64_t>(contentHash)), allocator);

    // Add AssetFactoryData
    rapidjson::Value factoryDataObj(rapidjson::kObjectType);
    factoryDataObj.AddMember("path", rapidjson::Value(importContext.importPath.c_str(), allocator), allocator);
    factoryDataObj.AddMember("assetType", rapidjson::Value(AssetTypeToString(importContext.assetType).c_str(), allocator), allocator);
    factoryDataObj.AddMember("assimpIndex", rapidjson::Value(importContext.assimpIndex), allocator);

    obj.AddMember("assetFactoryData", factoryDataObj, allocator);
}

am::AssetInfo am::AssetInfo::DeserializeAssetInfoFromJson(const rapidjson::Value& obj)
{
    boost::uuids::string_generator gen;
    boost::uuids::uuid id = gen(obj["id"].GetString());

    std::string path = obj["path"].GetString();
    AssetType type = StringToAssetType(obj["type"].GetString());
    size_t contentHash = obj["contentHash"].GetUint64();

    const auto& factoryData = obj["assetFactoryData"];
    ImportContext assetFactoryData(
        factoryData["path"].GetString(),
        StringToAssetType(factoryData["assetType"].GetString()),
        factoryData["assimpIndex"].GetInt()
    );

    AssetInfo info(id, path, type, contentHash, assetFactoryData);
    info.isLoaded = false;

    return info;
}
