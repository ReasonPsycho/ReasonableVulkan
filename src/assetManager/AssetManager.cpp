//
// Created by redkc on 09.05.2025.
//

#include "AssetManager.hpp"

namespace ae {
    boost::uuids::uuid AssetManager::registerAsset(const std::string &path, AssetType type) {
        auto metaDataInfo = lookupAssetInfoByPath(path);
        if (metaDataInfo) {
            return metaDataInfo->id;
        }

        AssetInfo info;
        info.id = boost::uuids::random_generator()(); // Generate new UUID
        info.path = path;
        info.type = type;


        // Find factory for this asset type
        auto factoryIt = factories.find(info.type);
        if (factoryIt == factories.end()) {
            throw std::runtime_error("No factory registered for asset type");
        }

        // Store metadata
        metadata[info.id] = info;

        // Create and load the asset
        std::shared_ptr<Asset> asset = factoryIt->second();
        asset->loadFromFile(info.path);
        assets[info.id] = asset;
    }


    void AssetManager::registerFactory(AssetType type, AssetFactory factory) {
        factories[type] = factory;
    }

    AssetManager &AssetManager::getInstance() {
        static AssetManager instance;
        return instance;
    }

    std::optional<AssetInfo> AssetManager::lookupAssetInfo(const boost::uuids::uuid &id) const {
        auto it = metadata.find(id);
        if (it != metadata.end()) return it->second;
        return std::nullopt;
    }
}


