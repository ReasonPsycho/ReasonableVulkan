//
// Created by redkc on 17.05.2025.
//

#include "AssetInfo.hpp"
#include "AssetManager.hpp"
#include "Asset.hpp"

am::Asset *am::AssetInfo::getAsset() {
    if (!isLoaded && !loadedAsset) {
        // Use the asset manager to load the asset
        auto &assetManager = assetFactoryData.assetManager;
        auto factory = assetManager.getFactory(type);
        if (factory) {
            loadedAsset = factory(assetFactoryData).release();
            isLoaded = true;
        }
    }
    return loadedAsset;
}

bool am::AssetInfo::isAssetLoaded() const {
    return isLoaded;
}
