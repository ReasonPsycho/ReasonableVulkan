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
#include "Asset.hpp"  // Add this include
#include "AssetInfo.hpp"

namespace am {
    class AssetManager {
        using AssetFactory = std::function<std::unique_ptr<am::Asset>(am::AssetFactoryData &)>;

    public:
        AssetManager(const AssetManager&) = delete;
        AssetManager& operator=(const AssetManager&) = delete;
        
        static AssetManager &getInstance();

        std::shared_ptr<AssetInfo> registerAsset(AssetFactoryData *factoryContext);

        void registerFactory(AssetType type, AssetFactory factory);

        template<typename T>
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


        [[nodiscard]] std::optional<std::shared_ptr<AssetInfo> > lookupAssetInfoByPath(const std::string &path) const {
            auto it = std::find_if(metadata.begin(), metadata.end(),
                                   [&path](const auto &pair) {
                                       return pair.second->path == path;
                                   });

            if (it != metadata.end()) {
                return it->second;
            }
            return std::nullopt;
        }

        AssetFactory getFactory(AssetType type) const {
            auto it = factories.find(type);
            BOOST_ASSERT(it != factories.end());
            return it->second;
        }

    private:
        AssetManager();
        ~AssetManager();
        [[nodiscard]] std::optional<std::shared_ptr<AssetInfo> > lookupAssetInfo(const boost::uuids::uuid &id) const;

        [[nodiscard]] std::optional<Asset *> lookupAsset(const boost::uuids::uuid &id) const;

        std::unordered_map<boost::uuids::uuid, std::unique_ptr<Asset>, boost::hash<boost::uuids::uuid> > assets;
        std::unordered_map<boost::uuids::uuid, std::shared_ptr<AssetInfo>, boost::hash<boost::uuids::uuid> > metadata;
        std::unordered_map<AssetType, AssetFactory> factories;
    };
} // am

#endif //ASSETMANAGER_HPP
