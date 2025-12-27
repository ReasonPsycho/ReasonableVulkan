//
// Created by redkc on 07/08/2025.
//

#ifndef SHADERDATA_H
#define SHADERDATA_H
#include <cstdint>
#include <map>
#include <vector>


namespace am
{
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
        std::map<std::string, std::string> defines;
        std::string originalSource;

    };
}
#endif //SHADERDATA_H
