//
// Created by Junie on 22/03/2026.
//

#ifndef REASONABLEVULKAN_CUBELIGHTSPACEMATRIXPC_HPP
#define REASONABLEVULKAN_CUBELIGHTSPACEMATRIXPC_HPP

#include <glm/glm.hpp>

namespace vks {
    struct CubeLightSpaceMatrixPC {
        alignas(16) glm::mat4 lightSpaceMatrices[6];
        alignas(16) glm::vec3 lightPos;
    };
}

#endif //REASONABLEVULKAN_CUBELIGHTSPACEMATRIXPC_HPP
