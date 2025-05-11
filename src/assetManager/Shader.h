#ifndef SHADER_H
#define SHADER_H

#include <cstring>
#include <optional>
#include <vector>
#include <span>
#include <cstdint>

#include "Asset.hpp"
#include "AssetManager.hpp"
#include "spdlog/spdlog.h"

#include <fstream>
#include <filesystem>
#include <functional>

namespace ae {
    enum class ShaderStage : uint32_t {
        Vertex = 0,
        Fragment = 1,
        Compute = 2,
        Geometry = 3,
        TessellationControl = 4,
        TessellationEvaluation = 5
    };

    struct ShaderData {
        std::vector<std::uint32_t> bytecode;
        ShaderStage stage;
    };

    struct ShaderFactoryContext : ae::BaseFactoryContext {
        std::string shaderPath;
    };

    class Shader : public Asset {
    public:
        explicit Shader(ae::ShaderFactoryContext shader_factory_context)
            : Asset(shader_factory_context) {
        }

        void loadFromFile(const std::string &path);

        // Returns a view into the shader bytecode
        [[nodiscard]] std::span<const std::uint32_t> getBytecode() const;

        [[nodiscard]] ShaderStage getStage() const;

    private:
        std::optional<ShaderData> shaderData;

        size_t calculateContentHash() const override;

        [[nodiscard]] AssetType getType() const override;
    };
} // namespace ae

#endif