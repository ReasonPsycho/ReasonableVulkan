#ifndef ASSET_HPP
#define ASSET_HPP
#include <string>
#include <boost/uuid/uuid.hpp>
#include <filesystem>
#include <any>
#include "AssetTypes.hpp"
#include "AssetInfo.hpp"

namespace am {
    class Asset {
    public:
        explicit Asset(const boost::uuids::uuid& id, const ImportContext& assetFactoryData) : id(id) {}
        explicit Asset(const boost::uuids::uuid& id, const std::string& path, AssetFormat format) : id(id) {}

        virtual ~Asset() = default;

        virtual size_t calculateContentHash() const = 0;

        [[nodiscard]] virtual AssetType getType() const = 0;

        virtual std::any getAssetData() = 0;

        template<typename T>
        T* getAssetDataAs() {
            return std::any_cast<T*>(getAssetData());
        }

        virtual void SaveAssetToJson(rapidjson::Document& document) = 0;
        virtual void SaveAssetToBin(std::string& path) {}

        virtual void SaveAssetMetadata(rapidjson::Document& document) = 0;
        virtual void LoadAssetMetadata(rapidjson::Document& document) = 0;

        virtual bool shouldSaveToBin() const { return saveToBinInsteadOfJson; }

        bool saveToBinInsteadOfJson = false;
        boost::uuids::uuid id;
    };
}

#endif //ASSET_HPP