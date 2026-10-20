#ifndef HUD_PUSH_CONSTANT_HPP
#define HUD_PUSH_CONSTANT_HPP

#include <glm/glm.hpp>
#include <cstdint>

namespace vks {
    struct alignas(16) HUDPushConstant {
        glm::vec2 screenPosition{0.0f, 0.0f};
        glm::vec2 size{0.0f, 0.0f};
        glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec4 uvBounds{0.0f, 0.0f, 1.0f, 1.0f};
        glm::vec2 screenSize{1.0f, 1.0f};
        uint32_t textureIndex{0};
        uint32_t hasTexture{0};
    };
}

#endif // HUD_PUSH_CONSTANT_HPP
