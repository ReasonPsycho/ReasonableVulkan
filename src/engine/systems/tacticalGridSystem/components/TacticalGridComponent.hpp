#pragma once
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include "ecs/utility/Reflection.hpp"

namespace engine::ecs {
    struct TacticalGridComponent {
        [[=Tooltip{"Whether tactical grid overlay is enabled"}]]
        bool enabled{true};

        [[=Tooltip{"Reachable movement range tactical glow color"}]]
        glm::vec4 rangeColor{0.15f, 0.75f, 0.95f, 0.45f};

        [[=Tooltip{"Projected path trajectory ribbon color"}]]
        glm::vec4 pathColor{0.95f, 0.85f, 0.25f, 0.9f};

        [[=Tooltip{"Tactical grid line color"}]]
        glm::vec4 gridColor{0.2f, 0.8f, 0.8f, 0.35f};

        [[=Tooltip{"Range boundary glow color"}]]
        glm::vec4 boundaryColor{0.3f, 0.9f, 1.0f, 0.9f};

        [[=Range{0.1f, 10.0f, 0.1f}, =Tooltip{"Holographic pulsing animation speed"}]]
        float pulseSpeed{2.0f};

        [[=Range{0.001f, 0.5f, 0.01f}, =Tooltip{"Grid line thickness"}]]
        float gridLineWidth{0.05f};

        [[=Range{0.001f, 0.5f, 0.01f}, =Tooltip{"Boundary border thickness"}]]
        float cellBorderWidth{0.08f};

        [[=Range{0.0f, 1.0f, 0.01f}, =Tooltip{"Decal plane elevation offset to avoid z-fighting"}]]
        float heightOffset{0.02f};
    };
}
