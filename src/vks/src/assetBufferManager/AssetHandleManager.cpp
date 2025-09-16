//
// Created by redkc on 12/08/2025.
//

#include "AssetHandleManager.h"

#include "Asset.hpp"
#include "../../VulkanRenderer.h"
#include "../../../assetManager/src/assets/ModelAsset.h"


bool vks::AssetHandleManager::isResourceLoaded(const boost::uuids::uuid& assetId)
{
    return loadedResources.find(assetId) != loadedResources.end();
}
