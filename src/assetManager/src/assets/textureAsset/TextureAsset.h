#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>
#include <vector>
#include <cstdint>
#include "../../../include/Asset.hpp"
#include "spdlog/spdlog.h"
#include <functional>

#include "assetDatas/TextureData.h"


namespace am {
    
    class TextureAsset : public Asset {
    public:

        explicit TextureAsset(const boost::uuids::uuid& id, ImportContext assetFactoryData);
        explicit TextureAsset(const boost::uuids::uuid& id, const std::string& path, AssetFormat format);

        ~TextureAsset() override;

        void SaveAssetToJson(rapidjson::Document& document) override;
        void SaveAssetToBin(std::string& path) override;

        void loadFromFile(const std::string &path);

        [[nodiscard]] size_t calculateContentHash() const override;
        [[nodiscard]] AssetType getType() const override;

        [[nodiscard]] int getWidth() const { return data.width ; }
        [[nodiscard]] int getHeight() const { return data.height ; }
        [[nodiscard]] int getChannels() const { return data.channels ; }
        [[nodiscard]] bool hasAlpha() const { return data.hasAlpha; }

        [[nodiscard]] const unsigned* getData() const {
            return data.pixels.data();
        }

        [[nodiscard]] size_t getDataSize() const {
            return data.pixels.size() ;
        }

        void SaveAssetMetadata(rapidjson::Document& document) override;
        void LoadAssetMetadata(rapidjson::Document& document) override;

        std::any getAssetData() override {
            return &data;
        }

        bool shouldSaveToBin() const override { return saveToBinInsteadOfJson; }
    private:
        TextureData data;

        bool saveToBinInsteadOfJson = true;

    };
}

#endif