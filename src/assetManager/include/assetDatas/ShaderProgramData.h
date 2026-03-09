#ifndef SHADERPROGRAMDATA_H
#define SHADERPROGRAMDATA_H

#include <vector>
#include <memory>
#include "AssetInfo.hpp"

namespace am {
    struct ShaderProgramData {
        std::shared_ptr<AssetInfo> vertexShader;
        std::shared_ptr<AssetInfo> fragmentShader;
        std::shared_ptr<AssetInfo> computeShader;
        std::shared_ptr<AssetInfo> geometryShader;
        std::shared_ptr<AssetInfo> tessellationControlShader;
        std::shared_ptr<AssetInfo> tessellationEvaluationShader;
    };
}

#endif // SHADERPROGRAMDATA_H
