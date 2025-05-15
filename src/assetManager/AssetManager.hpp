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
#include <boost/test/tools/assertion.hpp>

namespace ae {
    class Asset; // Forward declaration
    class AssetManager;
    
    struct AssetFactoryData {
        AssetManager &assetManager;
        std::string path;
        AssetType assetType;
        int assimpIndex;

        // Constructor to initialize the reference and other members
        AssetFactoryData(AssetManager& manager, std::string p, AssetType type,int assimpIndex = 0)
            : assetManager(manager)
            , path(std::move(p))
            , assetType(type)
            , assimpIndex(assimpIndex)
        {}
    };

    struct AssetInfo {
        boost::uuids::uuid id;
        std::string path;
        AssetType type;
        size_t contentHash;
        AssetFactoryData assetFactoryData;

        // Constructor
        AssetInfo(boost::uuids::uuid uuid, 
                  std::string p, 
                  AssetType t, 
                  size_t hash,
                  AssetFactoryData factoryData)
            : id(uuid)
            , path(std::move(p))
            , type(t)
            , contentHash(hash)
            , assetFactoryData(std::move(factoryData))
        {}
    };



    class AssetManager {
        using AssetFactory = std::function<std::shared_ptr<ae::Asset>(ae::AssetFactoryData &)>;

    public:
        static AssetManager &getInstance();

        boost::uuids::uuid registerAsset(AssetFactoryData factoryContext);

        void registerFactory(AssetType type, AssetFactory factory);

        template<typename T>
        std::shared_ptr<T> getByUUID(const boost::uuids::uuid &id) {
            auto it = assets.find(id);
            if (it != assets.end()) {
                return std::static_pointer_cast<T>(it->second);
            }
            auto asset_opt = lookupAsset(id);
            if (!asset_opt) {
                return nullptr;
            }
            return std::static_pointer_cast<T>(*asset_opt);
        }

        [[nodiscard]] std::optional<AssetInfo> lookupAssetInfoByPath(const std::string &path) const {
            auto it = std::find_if(metadata.begin(), metadata.end(),
                                   [&path](const auto &pair) {
                                       return pair.second.path == path;
                                   });

            if (it != metadata.end()) {
                return it->second;
            }
            return std::nullopt;
        }

    private:
        AssetManager() = default;

        [[nodiscard]] std::optional<AssetInfo> lookupAssetInfo(const boost::uuids::uuid &id) const;
        [[nodiscard]] std::optional<std::shared_ptr<Asset> > lookupAsset(const boost::uuids::uuid &id) const;

        std::unordered_map<boost::uuids::uuid, std::shared_ptr<Asset>, boost::hash<boost::uuids::uuid> > assets;
        std::unordered_map<boost::uuids::uuid, AssetInfo, boost::hash<boost::uuids::uuid> > metadata;
        std::unordered_map<AssetType, AssetFactory> factories;
    };
} // ae

#endif //ASSETMANAGER_HPP