#ifndef REASONABLEVULKAN_GRIDFRAGMENT_HPP
#define REASONABLEVULKAN_GRIDFRAGMENT_HPP

#include <vector>
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include "ecs/Component.hpp"

namespace engine::ecs
{
    struct GridFragment : public Component
    {
        [[=Range{1.0f, 1000.0f, 1.0f}, =Tooltip{"Grid fragment width in cells"}]]
        int width{1};

        [[=Range{1.0f, 1000.0f, 1.0f}, =Tooltip{"Grid fragment height in cells"}]]
        int height{1};

        [[=Tooltip{"Snap entity position to grid alignment"}]]
        bool snapToGrid{true};

        [[=Tooltip{"Show gizmo in editor"}]]
        bool showGizmo{true};

        [[=NonSerialized{}, =HidenInInspector{}]]
        std::vector<uint8_t> localWalkable{};

        GridFragment() = default;
        GridFragment(int w, int h, bool gizmo = true)
            : width(w), height(h), showGizmo(gizmo) {}

        bool IsCellWalkable(int x, int y) const {
            if (x < 0 || x >= width || y < 0 || y >= height) return false;
            if (localWalkable.empty()) return true;
            size_t idx = static_cast<size_t>(y * width + x);
            return idx < localWalkable.size() ? (localWalkable[idx] != 0) : true;
        }

        void SetCellWalkable(int x, int y, bool walkable) {
            if (x < 0 || x >= width || y < 0 || y >= height) return;
            if (localWalkable.empty()) {
                localWalkable.resize(static_cast<size_t>(width * height), 1);
            }
            localWalkable[static_cast<size_t>(y * width + x)] = walkable ? 1 : 0;
        }

        void SetAllWalkable(bool walkable) {
            localWalkable.assign(static_cast<size_t>(width * height), walkable ? 1 : 0);
        }

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
}

#endif //REASONABLEVULKAN_GRIDFRAGMENT_HPP
