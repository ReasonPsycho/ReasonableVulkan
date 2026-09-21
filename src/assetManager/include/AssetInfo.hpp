//
// Created by redkc on 17.05.2025.
//

#ifndef ASSETINFO_HPP
#define ASSETINFO_HPP
#include <boost/uuid/uuid.hpp>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>  // For to_string
#include "AssetTypes.hpp"
#include "AssetReflection.hpp"

namespace am {
    class Asset; // Forward declaration
    class AssetManager;

    struct ImportContext {
        [[=JsonName{"importPath"}]] std::string importPath;
        [[=JsonName{"assetType"}]]  AssetType assetType = AssetType::Other;
        [[=JsonName{"assimpIndex"}]] int assimpIndex = 0;

        ImportContext() = default;
        ImportContext(std::string p, AssetType type, int assimpIndex = 0)
            :
               importPath(std::move(p))
              , assetType(type)
              , assimpIndex(assimpIndex) {
        }

        bool operator==(const ImportContext& factory_context) const;
    };

    class AssetInfo {
    public:
        boost::uuids::uuid id;
        std::string path;
        AssetType type;
        std::string lookUpName;
        size_t contentHash;
        ImportContext importContext;
        bool isLoaded = false;
        Asset *loadedAsset = nullptr;

        // Constructor
        AssetInfo(boost::uuids::uuid uuid,
                  std::string p,
                  AssetType t,
                  size_t hash,
                  ImportContext factoryData,
                  std::string lookUpName)
            : id(uuid)
              , path(std::move(p))
              , type(t)
              , contentHash(hash)
              , importContext(std::move(factoryData))
              , lookUpName(std::move(lookUpName)) {
        }

        // Method to get or load asset
        Asset *getAsset();

        bool isAssetLoaded() const;

        AssetInfo(AssetInfo &&other) noexcept
            : id(other.id)
              , path(std::move(other.path))
              , type(other.type)
              , contentHash(other.contentHash)
              , importContext(std::move(other.importContext))
              , loadedAsset(other.loadedAsset)
              , lookUpName(other.lookUpName)
              , isLoaded(other.isLoaded) {
            other.loadedAsset = nullptr;
            other.isLoaded = false;
        }

        void SerializeAssetInfoToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const;

        // Add this static method for deserialization
        static AssetInfo DeserializeAssetInfoFromJson(const rapidjson::Value& obj);
    };

    inline boost::uuids::uuid GetAssetInfoId(const std::shared_ptr<AssetInfo>& info) {
        return info ? info->id : boost::uuids::uuid{};
    }

    inline size_t GetAssetInfoContentHash(const std::shared_ptr<AssetInfo>& info) {
        return info ? info->contentHash : 0;
    }
}


#endif //ASSETINFO_HPP
