//
// Created by redkc on 05/08/2025.
//

#ifndef ASSETMANAGERINTERFACE_H
#define ASSETMANAGERINTERFACE_H

#ifndef ASSETMANAGER_HPP
#define ASSETMANAGER_HPP

#include "stb_image.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/functional/hash.hpp>
#include "AssetTypes.hpp"
#include "UUIDManager.hpp"
#include <memory>
#include <unordered_map>
#include <functional>
#include <optional>
#include <assimp/scene.h>
#include <boost/test/tools/assertion.hpp>
#include <spdlog/spdlog.h>

#include "Asset.hpp"  // Add this include
#include "AssetInfo.hpp"



namespace am {
    class AssetManagerInterface {
    public:
        static AssetManager &getInstance();

        virtual  std::optional<std::shared_ptr<AssetInfo>> registerAsset(AssetFactoryData *factoryContext);
        virtual  [[nodiscard]] std::optional<std::shared_ptr<AssetInfo> > lookupAssetInfo(const boost::uuids::uuid &id) const;
        virtual  [[nodiscard]] std::optional<Asset *> lookupAsset(const boost::uuids::uuid &id) const;

        virtual template <typename T>
             std::shared_ptr<T> getByUUID(const boost::uuids::uuid &id) {
            auto it = assets.find(id);
            if (it != assets.end()) {
                return std::dynamic_pointer_cast<T>(std::shared_ptr<Asset>(it->second.get(), [](Asset *) {
                }));
            }
            auto asset_opt = lookupAsset(id);
            if (!asset_opt) {
                return nullptr;
            }
            return std::dynamic_pointer_cast<T>(std::shared_ptr<Asset>(*asset_opt, [](Asset *) {
            }));
        }
        [[nodiscard]] std::vector<boost::uuids::uuid> getUUIDsByPath(const std::string &path) const;

    };

} // am

#endif //ASSETMANAGER_HPP

#endif //ASSETMANAGERINTERFACE_H

