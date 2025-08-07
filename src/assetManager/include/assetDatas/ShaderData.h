//
// Created by redkc on 07/08/2025.
//

#ifndef SHADERDATA_H
#define SHADERDATA_H
#include <cstdint>
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
    };
}
#endif //SHADERDATA_H
