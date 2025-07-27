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
    class AssetManager {
        using AssetFactory = std::function<std::unique_ptr<am::Asset>(am::AssetFactoryData &)>;

    public:
        AssetManager(const AssetManager&) = delete;
        AssetManager& operator=(const AssetManager&) = delete;

        static AssetManager &getInstance();

        void registerFactory(AssetType type, AssetFactory factory);
        std::optional<std::shared_ptr<AssetInfo>> registerAsset(AssetFactoryData *factoryContext);
        [[nodiscard]] std::optional<std::shared_ptr<AssetInfo> > lookupAssetInfo(const boost::uuids::uuid &id) const;
        [[nodiscard]] std::optional<Asset *> lookupAsset(const boost::uuids::uuid &id) const;

        AssetFactory getFactory(AssetType type) const {
            auto it = factories.find(type);
            if(it != factories.end())
            {
                return it->second;
            }
            spdlog::error("Failed to get asset factory!");
            throw std::runtime_error("Failed to get asset factory!");

        }


          template <typename T>
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

    private:
        AssetManager();
        ~AssetManager();


        std::unordered_map<boost::uuids::uuid, std::unique_ptr<Asset>, boost::hash<boost::uuids::uuid> > assets;
        std::unordered_map<boost::uuids::uuid, std::shared_ptr<AssetInfo>, boost::hash<boost::uuids::uuid> > metadata;
        std::unordered_map<std::string, std::vector<boost::uuids::uuid>> pathToUUIDs;
        std::unordered_map<AssetType, AssetFactory> factories;


    #ifdef AM_ENABLE_TESTS
        friend struct AssetManagerTestFixture;
    #endif
    };

} // am

#endif //ASSETMANAGER_HPP