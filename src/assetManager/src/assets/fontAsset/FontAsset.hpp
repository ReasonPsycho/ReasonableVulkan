#pragma once

#include "../../../include/Asset.hpp"
#include "assetDatas/FontData.h"

namespace am {

    class FontAsset : public Asset {
    public:
        static constexpr AssetType StaticType = AssetType::Font;
        using DataType = FontData;

        FontData data;

        explicit FontAsset(const boost::uuids::uuid& id, std::string path);
        FontAsset(const boost::uuids::uuid& id, ImportContext assetFactoryData);
        explicit FontAsset(const std::string& path, AssetFormat format);

        [[nodiscard]] AssetType getType() const override;
        [[nodiscard]] size_t calculateContentHash() const override;

        std::any getAssetData() override {
            return &data;
        }

        void SaveAssetToJson(rapidjson::Document& document) override;
        void SaveAssetToBin(std::string& path) override {}

        void SaveAssetMetadata(rapidjson::Document& document) override;
        void LoadAssetMetadata(rapidjson::Document& document) override;

        void loadFromFnt(const std::string& path);
        void loadFromJson(const std::string& path);
        bool loadWithFreeType(const std::string& path, float targetFontSize = 48.0f);
    };

} // namespace am
