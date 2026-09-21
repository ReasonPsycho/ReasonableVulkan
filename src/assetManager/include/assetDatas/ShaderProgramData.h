#ifndef SHADERPROGRAMDATA_H
#define SHADERPROGRAMDATA_H

#include <vector>
#include <memory>
#include "AssetTypes.hpp"
#include "AssetReflection.hpp"

namespace am {
    class AssetInfo;

    struct ShaderProgramData {
        [[=JsonName{"vertex"}]]                 std::shared_ptr<am::AssetInfo> vertexShader;
        [[=JsonName{"fragment"}]]               std::shared_ptr<am::AssetInfo> fragmentShader;
        [[=JsonName{"compute"}]]                std::shared_ptr<am::AssetInfo> computeShader;
        [[=JsonName{"geometry"}]]               std::shared_ptr<am::AssetInfo> geometryShader;
        [[=JsonName{"tessellationControl"}]]    std::shared_ptr<am::AssetInfo> tessellationControlShader;
        [[=JsonName{"tessellationEvaluation"}]] std::shared_ptr<am::AssetInfo> tessellationEvaluationShader;
    };
}

#endif // SHADERPROGRAMDATA_H
