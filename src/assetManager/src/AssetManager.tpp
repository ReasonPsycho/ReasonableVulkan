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