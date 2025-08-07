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

#include "../../AssetFactoryRegistry.hpp"
#include "assetDatas/ShaderData.h"

namespace am {


    class Shader : public Asset {
    public:
        static inline AssetFactoryRegistry::Registrar<Shader> registrar{AssetType::Shader};

        explicit Shader(am::AssetFactoryData base_factory_context)
            : Asset(base_factory_context) {
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