#ifndef REASONABLEVULKAN_BARRIER_HPP
#define REASONABLEVULKAN_BARRIER_HPP

#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <glm/glm.hpp>
#include "ecs/Component.hpp"

namespace engine::ecs
{
    enum class BarrierOrientation
    {
        Horizontal = 0, // Placed along X-axis (between (x, y) and (x, y+1))
        Vertical = 1    // Placed along Z-axis (between (x, y) and (x+1, y))
    };

    struct Barrier : public Component
    {
        [[=Range{0.0f, 10000.0f, 0.5f}, =Tooltip{"Extra movement cost when crossing this barrier"}]]
        float extraCost{5.0f};

        [[=Range{1.0f, 1000.0f, 1.0f}, =Tooltip{"Barrier length in grid units along its orientation"}]]
        int length{1};

        [[=Tooltip{"Barrier orientation (Horizontal along X, Vertical along Z)"}]]
        BarrierOrientation orientation{BarrierOrientation::Horizontal};

        [[=Tooltip{"Snap entity position to grid alignment"}]]
        bool snapToGrid{true};

        [[=Tooltip{"Show gizmo in editor"}]]
        bool showGizmo{true};

        [[=Tooltip{"Whether barrier is actively increasing traversal cost"}]]
        bool enabled{true};

        Barrier() = default;
        explicit Barrier(BarrierOrientation orient, float cost = 5.0f, int len = 1, bool gizmo = true)
            : extraCost(cost), length(len), orientation(orient), showGizmo(gizmo) {}
        explicit Barrier(float cost, BarrierOrientation orient = BarrierOrientation::Horizontal, int len = 1, bool gizmo = true)
            : extraCost(cost), length(len), orientation(orient), showGizmo(gizmo) {}
        explicit Barrier(int len, float cost = 5.0f, BarrierOrientation orient = BarrierOrientation::Horizontal)
            : extraCost(cost), length(len), orientation(orient) {}

        float GetCost() const { return extraCost; }
        void SetCost(float cost) { extraCost = cost; }

        static glm::vec3 Snap(const glm::vec3& worldPos, BarrierOrientation orient, int len = 1, float cellSz = 1.0f) {
            if (cellSz <= 0.0f) return worldPos;
            if (orient == BarrierOrientation::Horizontal) {
                float gridY = std::round(worldPos.z / cellSz);
                float halfLen = static_cast<float>(len) * 0.5f;
                float ox = std::floor((worldPos.x / cellSz) - halfLen + 0.5f);
                return glm::vec3((ox + halfLen) * cellSz, worldPos.y, gridY * cellSz);
            } else {
                float gridX = std::round(worldPos.x / cellSz);
                float halfLen = static_cast<float>(len) * 0.5f;
                float oy = std::floor((worldPos.z / cellSz) - halfLen + 0.5f);
                return glm::vec3(gridX * cellSz, worldPos.y, (oy + halfLen) * cellSz);
            }
        }

        glm::vec3 SnapPosition(const glm::vec3& worldPos, float cellSz = 1.0f) const {
            return Snap(worldPos, orientation, length, cellSz);
        }
    };

    using BarrierComponent = Barrier;
}

#endif //REASONABLEVULKAN_BARRIER_HPP
