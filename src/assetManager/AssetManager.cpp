//
// Created by redkc on 09.05.2025.
//

#include "Asset.hpp"
#include "AssetManager.hpp"

namespace am {

    AssetManager::AssetManager() {
    }

    AssetManager::~AssetManager() {
    }


    std::optional<std::shared_ptr<AssetInfo>> AssetManager::registerAsset(AssetFactoryData *factoryContext) {
        try
        {
            // First check if we already have this factoryContext
            auto uuids = getUUIDsByPath(factoryContext->path);
            for (auto uuid : uuids)
            {
                auto info = lookupAssetInfo(uuid);
                if (info.value()->assetFactoryData == *factoryContext ) {
                    return info.value();
                }
            }

            // Create and load the asset to calculate its hash
            auto factoryIt = factories.find(factoryContext->assetType);
            if (factoryIt == factories.end()) {
                spdlog::error("No factory registered for asset type");
                throw std::runtime_error("No factory registered for asset type");
            }

            std::unique_ptr<Asset> newAsset = factoryIt->second(*factoryContext);
            size_t contentHash = newAsset->calculateContentHash();

            // Check if we have an asset with the same content hash
            auto existingAsset = std::find_if(metadata.begin(), metadata.end(),
                                              [contentHash](const auto &pair) {
                                                  return pair.second->contentHash == contentHash;
                                              });

            if (existingAsset != metadata.end()) {
                // We found an asset with the same content
                factoryContext->scene = nullptr;
                return existingAsset->second;
            }

            auto id = boost::uuids::random_generator()();
            // If we get here, this is a new unique asset
            auto info = std::make_shared<AssetInfo>(id, factoryContext->path, factoryContext->assetType, contentHash,
                                                    *factoryContext);
            info->isLoaded = true;

            metadata.insert(std::make_pair(id, info));
            assets[id] = std::move(newAsset);
            info->loadedAsset = assets[id].get();
            pathToUUIDs[factoryContext->path].push_back(id);
            factoryContext->scene = nullptr;

            return info;
        }
            catch (const std::exception& e) {
                return std::nullopt;
            }
        }


    void AssetManager::registerFactory(AssetType type, AssetFactory factory) {
        factories[type] = std::move(factory);
    }

    AssetManager &AssetManager::getInstance() {
        static AssetManager instance;
        return instance;
    }

    std::optional<std::shared_ptr<AssetInfo> > AssetManager::lookupAssetInfo(const boost::uuids::uuid &id) const {
        auto it = metadata.find(id);
        if (it != metadata.end()) return it->second;
        spdlog::error("No asset found!");
        return std::nullopt;
    }

    std::vector<boost::uuids::uuid> AssetManager::getUUIDsByPath(const std::string &path) const {
        auto it = pathToUUIDs.find(path);
        if (it != pathToUUIDs.end()) {
            return it->second;
        }
        return {};
    }

    std::optional<Asset *> AssetManager::lookupAsset(const boost::uuids::uuid &id) const {
        auto it = assets.find(id);
        if (it != assets.end()) return it->second.get();
        spdlog::error("No asset found !");
        return std::nullopt;
    }
}