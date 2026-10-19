#ifndef UI_PUSH_CONSTANT_HPP
#define UI_PUSH_CONSTANT_HPP

#include <glm/glm.hpp>
#include <cstdint>

namespace vks {
    struct alignas(16) UIPushConstant {
        glm::vec2 screenResolution{1.0f, 1.0f};
        glm::vec2 quadPosition{0.0f, 0.0f};
        glm::vec2 quadSize{100.0f, 100.0f};
        glm::vec2 pad0{0.0f, 0.0f};
        glm::vec4 uvRect{0.0f, 0.0f, 1.0f, 1.0f};
        glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
        uint32_t textureIndex{0xFFFFFFFF};
        uint32_t pad1[3]{0, 0, 0};
    };
}

#endif // UI_PUSH_CONSTANT_HPP
