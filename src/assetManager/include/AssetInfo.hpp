//
// Created by redkc on 17.05.2025.
//

#ifndef ASSETINFO_HPP
#define ASSETINFO_HPP
#include <boost/uuid/uuid.hpp>


namespace am {
    class Asset; // Forward declaration
    class AssetManager;
    enum class AssetType;

    struct AssetFactoryData {
        std::string path;
        AssetType assetType;
        int assimpIndex;

        // Constructor to initialize the reference and other members
        AssetFactoryData(std::string p, AssetType type, int assimpIndex = 0)
            :
               path(std::move(p))
              , assetType(type)
              , assimpIndex(assimpIndex) {
        }

        bool operator==(const AssetFactoryData& factory_context) const;
    };

    class AssetInfo {
    public:
        boost::uuids::uuid id;
        std::string path;
        AssetType type;
        size_t contentHash;
        AssetFactoryData assetFactoryData;
        bool isLoaded = false;
        Asset *loadedAsset = nullptr;

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
              , assetFactoryData(std::move(factoryData)) {
        }

        // Method to get or load asset
        Asset *getAsset();

        bool isAssetLoaded() const;

        AssetInfo(AssetInfo &&other) noexcept
            : id(other.id)
              , path(std::move(other.path))
              , type(other.type)
              , contentHash(other.contentHash)
              , assetFactoryData(std::move(other.assetFactoryData))
              , loadedAsset(other.loadedAsset)
              , isLoaded(other.isLoaded) {
            other.loadedAsset = nullptr;
            other.isLoaded = false;
        }
    };
}


#endif //ASSETINFO_HPP
