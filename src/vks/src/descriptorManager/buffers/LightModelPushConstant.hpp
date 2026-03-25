#ifndef LIGHT_MODEL_PUSH_CONSTANT_HPP
#define LIGHT_MODEL_PUSH_CONSTANT_HPP

#include <glm/glm.hpp>

namespace vks {
    struct LightModelPushConstant {
        glm::mat4 model;
        int lightIndex;
        int lightType; // 0: Directional, 1: Point, 2: Spot
    };
}

#endif // LIGHT_MODEL_PUSH_CONSTANT_HPP
