//
// Created by redkc on 07/08/2025.
//

#ifndef SHADERDATA_H
#define SHADERDATA_H
#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include "AssetReflection.hpp"

namespace am
{
    enum class ShaderStage : uint32_t {
        Vertex [[=ShaderSuffix{"vs"}]] = 0,
        Fragment [[=ShaderSuffix{"fs"}]] = 1,
        Compute [[=ShaderSuffix{"cs"}]] = 2,
        Geometry [[=ShaderSuffix{"gs"}]] = 3,
        TessellationControl [[=ShaderSuffix{"tcs"}]] = 4,
        TessellationEvaluation [[=ShaderSuffix{"tes"}]] = 5
    };

    struct ShaderData {
        std::vector<std::uint32_t> bytecode;
        ShaderStage stage;
        std::map<std::string, std::string> defines;
        std::string originalSource; // Path to the original GLSL source file
    };
}
#endif //SHADERDATA_H
