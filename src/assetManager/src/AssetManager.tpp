#pragma once
#include "AssetManager.hpp"

template <typename T>
std::shared_ptr<T> AssetManager::getByUUID(const boost::uuids::uuid& id)
{
    auto it = assets.find(id);
    if (it != assets.end()) {
        return std::dynamic_pointer_cast<T>(std::shared_ptr<Asset>(it->second.get(), [](Asset *) {
        }));
    }
    auto asset = getAssetData(id);
    return std::dynamic_pointer_cast<T>(std::shared_ptr<Asset>(asset, [](Asset *) {
    }));
}

template <typename T>
void AssetManager::RegisterAssetType()
{
    auto type = std::type_index(typeid(T));

    importers[type] = [](am::ImportContext& data)
    {
        return std::unique_ptr<am::Asset>(new T(data));
    };

    jsonSavers[type] = [](am::Asset& asset, rapidjson::Document& doc)
    {
        static_cast<T&>(asset).SaveAssetToJson(doc);
    };

    loaders[type] = [](const std::string& path, am::AssetFormat format)
    {
        return std::unique_ptr<am::Asset>(new T(path, format));
    };

    metadataSavers[type] = [](am::Asset& asset, rapidjson::Document& doc)
    {
        static_cast<T&>(asset).SaveAssetMetadata(doc);
    };

    metadataLoaders[type] = [](am::Asset& asset, rapidjson::Document& doc)
    {
        static_cast<T&>(asset).LoadAssetMetadata(doc);
    };
}