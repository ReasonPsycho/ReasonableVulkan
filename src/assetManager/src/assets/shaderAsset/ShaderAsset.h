#ifndef SHADER_H
#define SHADER_H

#include <cstring>
#include <optional>
#include <vector>
#include <span>
#include <cstdint>

#include "../../../include/Asset.hpp"
#include "../../AssetManager.hpp"
#include "spdlog/spdlog.h"

#include <fstream>
#include <filesystem>
#include <functional>

#include "assetDatas/ShaderData.h"

namespace am {

    class ShaderAsset : public Asset {
    public:
        explicit ShaderAsset(const boost::uuids::uuid& id, ImportContext assetFactoryData);
        explicit ShaderAsset(const boost::uuids::uuid& id, const std::string& path, AssetFormat format);

        void SaveAssetToJson(rapidjson::Document& document) override {throw std::runtime_error("Cannot save ShaderAsset to JSON");};
        void SaveAssetToBin(std::string& path) override;

        void loadFromFile(const std::string &path);

        // Recompile with modified defines
        bool recompileWithDefines(const std::map<std::string, std::string>& newDefines);

        // Get current defines
        const std::map<std::string, std::string>& getDefines() const {
            return data.defines;
        }

        // Modify a single define and recompile
        bool setDefineAndRecompile(const std::string& name, const std::string& value);

        // Returns a view into the shader bytecode
        [[nodiscard]] std::span<const std::uint32_t> getBytecode() const;
        [[nodiscard]] ShaderStage getStage() const;

        void SaveAssetMetadata(rapidjson::Document& document) override {}
        void LoadAssetMetadata(rapidjson::Document& document) override {}

        std::any getAssetData() override {
            return &data;
        }

        bool shouldSaveToBin() const override { return saveToBinInsteadOfJson; }

        bool saveToBinInsteadOfJson = true;
    private:
        ShaderData data;

        [[nodiscard]] std::vector<std::uint32_t> compileGLSLToSPIRV(
           const std::string &source,
           ShaderStage stage,
           const std::map<std::string, std::string>& defines = {}
       ) const;

        size_t calculateContentHash() const override;
        [[nodiscard]] AssetType getType() const override;
    };
} // namespace am

#endif