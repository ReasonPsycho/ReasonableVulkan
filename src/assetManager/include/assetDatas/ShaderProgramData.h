#ifndef SHADERPROGRAMDATA_H
#define SHADERPROGRAMDATA_H

#include <vector>
#include <memory>
#include "AssetTypes.hpp"

namespace am {
    class AssetInfo;

    struct ShaderProgramData {
        std::shared_ptr<am::AssetInfo> vertexShader;
        std::shared_ptr<am::AssetInfo> fragmentShader;
        std::shared_ptr<am::AssetInfo> computeShader;
        std::shared_ptr<am::AssetInfo> geometryShader;
        std::shared_ptr<am::AssetInfo> tessellationControlShader;
        std::shared_ptr<am::AssetInfo> tessellationEvaluationShader;
    };
}

#endif // SHADERPROGRAMDATA_H
