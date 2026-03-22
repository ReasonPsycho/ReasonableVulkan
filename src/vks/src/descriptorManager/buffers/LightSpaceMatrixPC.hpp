//
// Created by Junie on 22/03/2026.
//

#ifndef REASONABLEVULKAN_LIGHTSPACEMATRIXPC_HPP
#define REASONABLEVULKAN_LIGHTSPACEMATRIXPC_HPP

#include <glm/glm.hpp>

namespace vks {
    struct LightSpaceMatrixPC {
        alignas(16) glm::mat4 lightSpaceMatrix;
        alignas(16) glm::vec3 lightPos;
    };
}

#endif //REASONABLEVULKAN_LIGHTSPACEMATRIXPC_HPP
