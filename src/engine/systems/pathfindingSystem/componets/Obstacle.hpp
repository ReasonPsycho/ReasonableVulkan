#ifndef REASONABLEVULKAN_OBSTACLE_HPP
#define REASONABLEVULKAN_OBSTACLE_HPP

#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <glm/glm.hpp>
#include "ecs/Component.hpp"

namespace engine::ecs
{
    struct Obstacle : public Component
    {
        [[=Range{1.0f, 1000.0f, 1.0f}, =Tooltip{"Obstacle width in cells"}]]
        int width{1};

        [[=Range{1.0f, 1000.0f, 1.0f}, =Tooltip{"Obstacle height in cells"}]]
        int height{1};

        [[=Tooltip{"Snap entity position to grid alignment"}]]
        bool snapToGrid{true};

        [[=Tooltip{"Show gizmo in editor"}]]
        bool showGizmo{true};

        [[=Tooltip{"Whether obstacle is actively blocking navigation"}]]
        bool enabled{true};

        Obstacle() = default;
        Obstacle(int w, int h, bool gizmo = true)
            : width(w), height(h), showGizmo(gizmo) {}
        Obstacle(int w, int h, bool snap, bool gizmo, bool isEnabled = true)
            : width(w), height(h), snapToGrid(snap), showGizmo(gizmo), enabled(isEnabled) {}

        static glm::vec3 Snap(const glm::vec3& worldPos, float cellSz = 1.0f) {
            return Snap(worldPos, 1, 1, cellSz);
        }

        static glm::vec3 Snap(const glm::vec3& worldPos, int w, int h, float cellSz = 1.0f) {
            if (cellSz <= 0.0f) return worldPos;
            float halfW = static_cast<float>(w) * 0.5f;
            float halfH = static_cast<float>(h) * 0.5f;
            float ox = std::floor((worldPos.x / cellSz) - halfW + 0.5f);
            float oz = std::floor((worldPos.z / cellSz) - halfH + 0.5f);
            return glm::vec3(
                (ox + halfW) * cellSz,
                worldPos.y,
                (oz + halfH) * cellSz
            );
        }

        glm::vec3 SnapPosition(const glm::vec3& worldPos, float cellSz = 1.0f) const {
            return Snap(worldPos, width, height, cellSz);
        }
    };

    using ObstacleComponent = Obstacle;
}

#endif //REASONABLEVULKAN_OBSTACLE_HPP
