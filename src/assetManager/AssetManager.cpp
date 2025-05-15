//
// Created by redkc on 09.05.2025.
//

#include "Asset.hpp"
#include "AssetManager.hpp"

namespace ae {
boost::uuids::uuid AssetManager::registerAsset(AssetFactoryData factoryContext) {
    // First check if we already have this path
    auto metaDataInfo = lookupAssetInfoByPath(factoryContext.path);
    if (metaDataInfo) {
        return metaDataInfo->id;
    }

    // Create and load the asset to calculate its hash
    auto factoryIt = factories.find(factoryContext.assetType);
    if (factoryIt == factories.end()) {
        throw std::runtime_error("No factory registered for asset type");
    }

    std::shared_ptr<Asset> newAsset = factoryIt->second(factoryContext);
    size_t contentHash = newAsset->calculateContentHash();

    // Check if we have an asset with the same content hash
    auto existingAsset = std::find_if(metadata.begin(), metadata.end(),
                                      [contentHash](const auto &pair) {
                                          return pair.second.contentHash == contentHash;
                                      });

    if (existingAsset != metadata.end()) {
        // We found an asset with the same content
        return existingAsset->first;
    }

    // If we get here, this is a new unique asset
    AssetInfo info{
        .id = boost::uuids::random_generator()(),
        .path = factoryContext.path,
        .type = factoryContext.assetType,
        .contentHash = contentHash
    };

    metadata.insert({info.id, std::move(info)});
    assets[info.id] = newAsset;

    return info.id;
}


    void AssetManager::registerFactory(AssetType type, AssetFactory factory) {
        factories[type] = std::move(factory);
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

    std::optional<std::shared_ptr<Asset> > AssetManager::lookupAsset(const boost::uuids::uuid &id) const {
        auto it = assets.find(id);
        if (it != assets.end()) return it->second;
        return std::nullopt;
    }
}