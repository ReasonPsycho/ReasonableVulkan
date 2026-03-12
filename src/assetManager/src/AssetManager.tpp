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
    auto asset_opt = getAsset(id);
    if (!asset_opt) {
        return nullptr;
    }
    return std::dynamic_pointer_cast<T>(std::shared_ptr<Asset>(*asset_opt, [](Asset *) {
    }));
}

template <typename T>
void AssetManager::RegisterAssetType()
{
    auto type = std::type_index(typeid(T));

    factories[type] = [](am::AssetFactoryData& data)
    {
        return std::make_unique<T>(data);
    };

    savers[type] = [](am::Asset& asset, rapidjson::Document& doc)
    {
        static_cast<T&>(asset).SaveAssetMetadata(doc);
    };

    loaders[type] = [](am::Asset& asset, rapidjson::Document& doc)
    {
        static_cast<T&>(asset).LoadAssetMetadata(doc);
    };
}