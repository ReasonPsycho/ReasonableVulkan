#ifndef SHADERPROGRAMASSET_H
#define SHADERPROGRAMASSET_H

#include "../../../include/Asset.hpp"
#include "assetDatas/ShaderProgramData.h"

namespace am {
    class ShaderProgramAsset : public Asset {
    public:
        explicit ShaderProgramAsset(ImportContext assetFactoryData);
        explicit ShaderProgramAsset(const std::string& path, AssetFormat format);

        void SaveAssetToJson(rapidjson::Document& document) override;

        size_t calculateContentHash() const override;
        [[nodiscard]] AssetType getType() const override;

        void SaveAssetMetadata(rapidjson::Document& document) override {}
        void LoadAssetMetadata(rapidjson::Document& document) override {}

        std::any getAssetData() override {
            return &data;
        }

    private:
        ShaderProgramData data;
        void loadFromJson(const std::string& path);
    };
}

#endif // SHADERPROGRAMASSET_H
