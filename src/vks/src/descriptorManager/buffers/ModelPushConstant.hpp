#ifndef MODEL_PUSH_CONSTANT_HPP
#define MODEL_PUSH_CONSTANT_HPP

#include <glm/glm.hpp>

namespace vks {
    struct alignas(16) ModelPushConstant {
        glm::mat4 model;
    };
}

#endif // MODEL_PUSH_CONSTANT_HPP
