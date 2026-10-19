#pragma once
#include <glm/vec2.hpp>
#include "ecs/Component.hpp"
#include "ecs/utility/Reflection.hpp"

namespace engine::ecs {
    struct RectTransformComponent : public Component {
        [[=Tooltip{"Local position offset in pixels"}]]
        glm::vec2 position{0.0f, 0.0f};

        [[=Tooltip{"Size in pixels (Width, Height)"}]]
        glm::vec2 size{100.0f, 100.0f};

        [[=Tooltip{"Normalized anchor min (0 = Left/Top, 1 = Right/Bottom)"}]]
        glm::vec2 anchorMin{0.0f, 0.0f};

        [[=Tooltip{"Normalized anchor max (0 = Left/Top, 1 = Right/Bottom)"}]]
        glm::vec2 anchorMax{0.0f, 0.0f};

        [[=Tooltip{"Pivot point (0.5, 0.5 is center, 0,0 is top-left)"}]]
        glm::vec2 pivot{0.5f, 0.5f};

        [[=Tooltip{"Rotation in degrees"}]]
        float rotation{0.0f};

        [[=Tooltip{"Sorting layer / Z-Index"}]]
        int zIndex{0};

        RectTransformComponent() = default;
        RectTransformComponent(glm::vec2 pos, glm::vec2 sz) : position(pos), size(sz) {}
        RectTransformComponent(glm::vec2 pos, glm::vec2 sz, glm::vec2 aMin, glm::vec2 aMax, glm::vec2 piv = glm::vec2(0.5f, 0.5f), int z = 0)
            : position(pos), size(sz), anchorMin(aMin), anchorMax(aMax), pivot(piv), zIndex(z) {}
    };
}
