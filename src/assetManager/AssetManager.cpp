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


    AssetManager::AssetResult AssetManager::registerAsset(AssetFactoryData *factoryContext) {
        try {
            if (!factoryContext) {
                spdlog::error("Attempted to register asset with null factory context");
                return std::unexpected(std::make_exception_ptr(
                    AssetException("Null factory context provided")));
            }

            // First check if we already have this path
            auto metaDataInfo = lookupAssetInfoByPath(factoryContext->path);
            if (metaDataInfo) {
                spdlog::debug("Asset already registered for path: {}", factoryContext->path);
                factoryContext->scene = nullptr;
                return metaDataInfo.value();
            }

            // Get the factory for this asset type
            auto factoryIt = factories.find(factoryContext->assetType);
            if (factoryIt == factories.end()) {
                spdlog::error("No factory found for asset type: {}", static_cast<int>(factoryContext->assetType));
                return std::unexpected(std::make_exception_ptr(
                    AssetFactoryNotFoundException(std::to_string(static_cast<int>(factoryContext->assetType)))));
            }

            spdlog::debug("Creating new asset for path: {}", factoryContext->path);
            auto result = factoryIt->second(*factoryContext);
            if (!result) {
                spdlog::error("Factory failed to create asset for: {}", factoryContext->path);
                return std::unexpected(std::current_exception());
            }
            std::unique_ptr<Asset> newAsset = std::move(result.value());

            size_t contentHash = newAsset->calculateContentHash();
            if (contentHash == 0) {
                spdlog::error("Failed to calculate content hash for asset: {}", factoryContext->path);
                return std::unexpected(std::make_exception_ptr(
                    AssetException("Failed to calculate content hash for asset: " + factoryContext->path)));
            }
            
            spdlog::debug("Calculated content hash: {} for asset: {}", contentHash, factoryContext->path);

            // Check if we have an asset with the same content hash
            auto existingAsset = std::find_if(metadata.begin(), metadata.end(),
                                            [contentHash](const auto &pair) {
                                                return pair.second->contentHash == contentHash;
                                            });

            if (existingAsset != metadata.end()) {
                spdlog::debug("Found existing asset with same content hash: {}", factoryContext->path);
                factoryContext->scene = nullptr;
                return existingAsset->second;
            }

            auto id = boost::uuids::random_generator()();
            spdlog::debug("Registering new asset. Path: {}, UUID: {}", 
                         factoryContext->path, boost::uuids::to_string(id));

            auto info = std::make_shared<AssetInfo>(id, factoryContext->path, 
                                                  factoryContext->assetType, 
                                                  contentHash, *factoryContext);
            info->isLoaded = true;

            metadata.insert(std::make_pair(id, info));
            assets[id] = std::move(newAsset);
            info->loadedAsset = assets[id].get();
            factoryContext->scene = nullptr;

            spdlog::info("Successfully registered new asset: {}", factoryContext->path);
            return info;

        } catch (const std::exception& e) {
            spdlog::error("Failed to register asset: {} - Error: {}", 
                         factoryContext ? factoryContext->path : "unknown", e.what());
            return std::unexpected(std::current_exception());
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
        return std::nullopt;
    }

    std::optional<Asset *> AssetManager::lookupAsset(const boost::uuids::uuid &id) const {
        auto it = assets.find(id);
        if (it != assets.end()) return it->second.get();
        return std::nullopt;
    }
}
