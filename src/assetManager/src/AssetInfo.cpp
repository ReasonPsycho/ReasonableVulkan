//
// Created by redkc on 17.05.2025.
//

#include "../include/AssetInfo.hpp"
#include <spdlog/spdlog.h>
#include "AssetManager.hpp"
#include "../include/Asset.hpp"

std::string am::NormalizePathToRelative(const std::string& inputPath, const std::string& baseResourceFolder)
{
    if (inputPath.empty()) return "";

    std::string pathStr = inputPath;
    for (char& c : pathStr) {
        if (c == '\\') c = '/';
    }

    // Check if the path contains "/res/" or begins with "res/"
    size_t resPos = pathStr.rfind("/res/");
    if (resPos != std::string::npos) {
        return pathStr.substr(resPos + 1); // "res/..."
    }

    if (pathStr.rfind("res/", 0) == 0) {
        return pathStr;
    }

    // Try relative to baseResourceFolder if provided
    if (!baseResourceFolder.empty()) {
        std::error_code ec;
        std::filesystem::path fullP = std::filesystem::path(inputPath).lexically_normal();
        std::filesystem::path baseP = std::filesystem::path(baseResourceFolder).lexically_normal();
        std::filesystem::path rel = std::filesystem::relative(fullP, baseP, ec);
        if (!ec && !rel.empty()) {
            std::string relStr = rel.generic_string();
            if (relStr.rfind("..", 0) != 0 && relStr != ".") {
                return "res/" + relStr;
            }
        }
    }

    // If it's a relative path (e.g. "fonts/foo.png"), prefix with "res/"
    std::filesystem::path p(pathStr);
    if (p.is_relative() && pathStr.rfind(".", 0) != 0) {
        return "res/" + p.generic_string();
    }

    return std::filesystem::path(inputPath).lexically_normal().generic_string();
}

std::string am::ResolveRelativePath(const std::string& inputPath, const std::string& baseResourceFolder)
{
    if (inputPath.empty()) return "";

    std::string pathStr = inputPath;
    for (char& c : pathStr) {
        if (c == '\\') c = '/';
    }

    std::string baseFolder = baseResourceFolder.empty() ? "res" : baseResourceFolder;

    if (pathStr.rfind("res/", 0) == 0) {
        return (std::filesystem::path(baseFolder) / pathStr.substr(4)).lexically_normal().string();
    }

    std::filesystem::path p(inputPath);
    if (p.is_relative()) {
        return (std::filesystem::path(baseFolder) / p).lexically_normal().string();
    }

    // If absolute path contains "/res/", check if it needs remapping to baseFolder
    size_t resPos = pathStr.rfind("/res/");
    if (resPos != std::string::npos) {
        std::error_code ec;
        if (!std::filesystem::exists(p, ec)) {
            return (std::filesystem::path(baseFolder) / pathStr.substr(resPos + 5)).lexically_normal().string();
        }
    }

    return p.lexically_normal().string();
}

bool am::ImportContext::operator==(const ImportContext& factory_context) const
{
    if (this->assetType != factory_context.assetType)
        return false;
    if (this->assimpIndex != factory_context.assimpIndex)
        return false;
    if (this->importPath != factory_context.importPath) {
        std::filesystem::path p1 = std::filesystem::path(this->importPath).lexically_normal();
        std::filesystem::path p2 = std::filesystem::path(factory_context.importPath).lexically_normal();
        if (p1 != p2) {
            std::string r1 = NormalizePathToRelative(this->importPath);
            std::string r2 = NormalizePathToRelative(factory_context.importPath);
            if (r1 != r2)
                return false;
        }
    }

    return true;
}

am::Asset *am::AssetInfo::getAsset() {
    if (!isLoaded)
    {
        AssetManager &assetManager = AssetManager::getInstance();
        auto asset = assetManager.getAsset(id);
        if (asset.has_value())
        {
            loadedAsset = asset.value();
        }else
        {
            spdlog::error("Failed to load asset with id: {}", boost::uuids::to_string(id));
        }
        return loadedAsset;
    }
    return loadedAsset;
}

bool am::AssetInfo::isAssetLoaded() const {
    return isLoaded;
}

void am::AssetInfo::SerializeAssetInfoToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const
{
    if (!obj.IsObject()) obj.SetObject();

    // Convert UUID to string
    std::string uuidStr = boost::uuids::to_string(id);
    std::string serializedPath = NormalizePathToRelative(path);

    obj.AddMember("id", rapidjson::Value(uuidStr.c_str(), allocator), allocator);
    obj.AddMember("path", rapidjson::Value(serializedPath.c_str(), allocator), allocator);
    obj.AddMember("type", rapidjson::Value(AssetTypeToString(type).c_str(), allocator), allocator);
    obj.AddMember("lookUpName", rapidjson::Value(lookUpName.c_str(), allocator), allocator);
    obj.AddMember("contentHash", rapidjson::Value(static_cast<uint64_t>(contentHash)), allocator);

    if (!thumbnailPath.empty()) {
        std::string serializedThumb = thumbnailPath;
        for (char& c : serializedThumb) {
            if (c == '\\') c = '/';
        }
        obj.AddMember("thumbnailPath", rapidjson::Value(serializedThumb.c_str(), allocator), allocator);
    }

    // Add AssetFactoryData using reflection
    ImportContext contextToSerialize = importContext;
    if (!contextToSerialize.importPath.empty()) {
        contextToSerialize.importPath = NormalizePathToRelative(contextToSerialize.importPath);
    }
    rapidjson::Value factoryDataObj(rapidjson::kObjectType);
    SerializeAssetData(contextToSerialize, factoryDataObj, allocator);
    obj.AddMember("assetFactoryData", factoryDataObj, allocator);
}

am::AssetInfo am::AssetInfo::DeserializeAssetInfoFromJson(const rapidjson::Value& obj)
{
    boost::uuids::string_generator gen;
    boost::uuids::uuid id = gen(obj["id"].GetString());

    std::string path = obj["path"].GetString();
    AssetType type = StringToAssetType(obj["type"].GetString());
    std::string lookUpName = obj["lookUpName"].GetString();
    size_t contentHash = obj["contentHash"].GetUint64();

    ImportContext assetFactoryData("", AssetType::Other, 0);
    if (obj.HasMember("assetFactoryData")) {
        DeserializeAssetData(assetFactoryData, obj["assetFactoryData"]);
    }

    AssetInfo info(id, path, type, contentHash, assetFactoryData, lookUpName);
    if (obj.HasMember("thumbnailPath") && obj["thumbnailPath"].IsString()) {
        info.thumbnailPath = obj["thumbnailPath"].GetString();
    }
    info.isLoaded = false;

    return info;
}
