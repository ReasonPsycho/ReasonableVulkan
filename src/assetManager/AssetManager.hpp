//
// Created by redkc on 09.05.2025.
//

#ifndef ASSETMANAGER_HPP
#define ASSETMANAGER_HPP

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>#include
#include "Asset.hpp"
#include "UUIDManager.hpp"
#include <memory>
#include <unordered_map>
#include <functional>
#include <optional>
#include <boost/test/tools/assertion.hpp>

namespace ae {
    struct AssetInfo {
        boost::uuids::uuid id;
        std::string path;
        AssetType type;
    };

    class AssetManager {
    public:
        using AssetFactory = std::function<std::shared_ptr<Asset>()>;

        boost::uuids::uuid registerAsset(const std::string &path, AssetType type);

        void registerFactory(AssetType type, AssetFactory factory);

        template<typename T>
        std::shared_ptr<T> loadByUUID(const boost::uuids::uuid &id) {
            auto it = assets.find(id);
            if (it != assets.end()) {
                return std::static_pointer_cast<T>(it->second);
            }

            auto infoOpt = lookupAssetInfo(id);
            if (!infoOpt) return nullptr;

            auto &info = *infoOpt;
            auto factoryIt = factories.find(info.type);
            if (factoryIt == factories.end()) return nullptr;

            std::shared_ptr<Asset> asset = factoryIt->second();
            asset->loadFromFile(info.path);
            assets[id] = asset;

            return std::static_pointer_cast<T>(asset);
        }

        std::optional<AssetInfo> AssetManager::lookupAssetInfoByPath(const std::string &path) const {
            // Find the first metadata entry with matching path
            auto it = std::find_if(metadata.begin(), metadata.end(),
                                   [&path](const auto &pair) {
                                       return pair.second.path == path;
                                   });

            if (it != metadata.end()) {
                return it->second;
            }
            return std::nullopt;
        }

        static AssetManager &getInstance();

    private:
        AssetManager() = default; // Private constructor for singleton
        std::optional<AssetInfo> lookupAssetInfo(const boost::uuids::uuid &id) const;

        std::unordered_map<boost::uuids::uuid, std::shared_ptr<Asset>, boost::hash<boost::uuids::uuid> > assets;
        std::unordered_map<boost::uuids::uuid, AssetInfo, boost::hash<boost::uuids::uuid> > metadata;
        std::unordered_map<AssetType, AssetFactory> factories;
    };
} // ae

#endif //ASSETMANAGER_HPP
