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

        explicit ShaderAsset(am::AssetFactoryData base_factory_context)
            : Asset(base_factory_context) {
            loadFromFile(base_factory_context.path);
        }

        void loadFromFile(const std::string &path);

        // Returns a view into the shader bytecode
        [[nodiscard]] std::span<const std::uint32_t> getBytecode() const;

        [[nodiscard]] ShaderStage getStage() const;

        void* getAssetData() override { return &data; }
    private:
        ShaderData data;

        size_t calculateContentHash() const override;
        [[nodiscard]] AssetType getType() const override;
    };
} // namespace am

#endif