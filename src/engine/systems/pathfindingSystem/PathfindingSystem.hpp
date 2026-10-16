//
// Created by redkc on 14/10/2026.
//

#ifndef REASONABLEVULKAN_PATHFINDINGSYSTEM_HPP
#define REASONABLEVULKAN_PATHFINDINGSYSTEM_HPP

#include <typeindex>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

#include "componets/PathfindingAgent.hpp"
#include "componets/GridFragment.hpp"
#include "ecs/System.h"
#include "ecs/Types.h"

namespace engine::ecs
{
    class Scene;

    class PathfindingSystem : public System<PathfindingSystem, PathfindingAgent, GridFragment>
    {
    public:
        [[=Range{0.01f, 100.0f, 0.1f}, =Tooltip{"Size of each grid cell in world units"}]]
        float cellSize{1.0f};

        explicit PathfindingSystem(Scene* scene);

        void OnStart() override;

        void Update(float deltaTime) override;

        // Grid Configuration
        void SetCellSize(float size);
        float GetCellSize() const;

        void SetDefaultEmptyWalkable(bool walkable);
        bool GetDefaultEmptyWalkable() const;

        void MarkGridDirty();
        bool IsGridDirty() const;

        // Grid Bounds & Metrics
        int GetGridWidth() const;
        int GetGridHeight() const;
        glm::ivec2 GetGridMin() const;
        glm::ivec2 GetGridMax() const;

        // Coordinate Transforms
        glm::ivec2 WorldToGrid(const glm::vec3& worldPos, int agentSize = 1) const;
        glm::vec3 GridToWorld(const glm::ivec2& gridPos, float height = 0.0f, int agentSize = 1) const;

        // Grid Construction & Clearance
        void BuildNavGrid();
        void CalculateClearanceMap();

        // Spatial / Navigation Queries
        bool IsInBounds(int x, int y) const;
        bool IsWalkable(int x, int y, int agentSize = 1) const;
        uint8_t GetClearance(int x, int y) const;
        void SetCellWalkable(int x, int y, bool walkable);

        // Pathfinding
        std::vector<glm::ivec2> FindPathGrid(const glm::ivec2& start, const glm::ivec2& target, int agentSize = 1, Entity ignoreEntity = MAX_ENTITIES);
        std::vector<glm::vec3> FindPath(const glm::vec3& startWorld, const glm::vec3& targetWorld, int agentSize = 1, float height = 0.0f, Entity ignoreEntity = MAX_ENTITIES);
        bool RequestPath(Entity entity, const glm::vec3& targetWorld);

        // Grid Alignment & Snapping
        void SnapAllFragmentsToGrid();
        void SnapEntityToGrid(Entity entity);


    protected:
        void OnComponentAdded(ComponentID componentID, std::type_index type) override;
        void OnEntityRemoved(ComponentID componentID, std::type_index type) override;

    private:
        bool defaultEmptyWalkable{false};

        int minGridX{0};
        int minGridY{0};
        int maxGridX{0};
        int maxGridY{0};
        int gridWidth{0};
        int gridHeight{0};

        std::vector<uint8_t> gridWalkable{};
        std::vector<uint8_t> gridClearance{};
        bool isGridDirty{true};

        inline size_t ToGridIndex(int x, int y) const {
            return static_cast<size_t>((y - minGridY) * gridWidth + (x - minGridX));
        }
    };
}

#endif //REASONABLEVULKAN_PATHFINDINGSYSTEM_HPP
