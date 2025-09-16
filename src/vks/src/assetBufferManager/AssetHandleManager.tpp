#pragma once
#include "AssetHandleManager.h"

template <typename T>
T* vks::AssetHandleManager::getOrLoadResource(const boost::uuids::uuid& assetId)
{
    if (isResourceLoaded(assetId))
        return dynamic_cast<T*>(loadedResources[assetId].get());

    return dynamic_cast<T*>(loadResource(assetId));
}
