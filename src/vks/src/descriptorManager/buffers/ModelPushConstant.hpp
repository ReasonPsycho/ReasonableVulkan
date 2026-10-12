#ifndef MODEL_PUSH_CONSTANT_HPP
#define MODEL_PUSH_CONSTANT_HPP

#include <glm/glm.hpp>
#include <cstdint>

namespace vks {
    struct alignas(16) ModelPushConstant {
        glm::mat4 model{1.0f};
        uint32_t materialIndex{0};
    };
}

#endif // MODEL_PUSH_CONSTANT_HPP
