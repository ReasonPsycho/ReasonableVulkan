//
// Created by redkc on 14/10/2026.
//

#include "PathfindingSystem.hpp"
#include "ecs/Scene.h"
#include "systems/transformSystem/componets/TransformComponent.hpp"
#include <queue>
#include <limits>
#include <cmath>
#include <algorithm>

namespace engine::ecs {

PathfindingSystem::PathfindingSystem(Scene* scene) : System(scene) {}

void PathfindingSystem::OnStart() {
    BuildNavGrid();
}

void PathfindingSystem::SetCellSize(float size) {
    cellSize = size > 0.001f ? size : 1.0f;
    MarkGridDirty();
}

float PathfindingSystem::GetCellSize() const {
    return cellSize;
}

void PathfindingSystem::SetDefaultEmptyWalkable(bool walkable) {
    defaultEmptyWalkable = walkable;
    MarkGridDirty();
}

bool PathfindingSystem::GetDefaultEmptyWalkable() const {
    return defaultEmptyWalkable;
}

void PathfindingSystem::MarkGridDirty() {
    isGridDirty = true;
}

bool PathfindingSystem::IsGridDirty() const {
    return isGridDirty;
}

int PathfindingSystem::GetGridWidth() const {
    return gridWidth;
}

int PathfindingSystem::GetGridHeight() const {
    return gridHeight;
}

glm::ivec2 PathfindingSystem::GetGridMin() const {
    return glm::ivec2(minGridX, minGridY);
}

glm::ivec2 PathfindingSystem::GetGridMax() const {
    return glm::ivec2(maxGridX, maxGridY);
}

glm::ivec2 PathfindingSystem::WorldToGrid(const glm::vec3& worldPos, int agentSize) const {
    if (agentSize <= 0) agentSize = 1;
    float offset = static_cast<float>(agentSize - 1) * 0.5f * cellSize;
    return glm::ivec2(
        static_cast<int>(std::floor((worldPos.x - offset) / cellSize)),
        static_cast<int>(std::floor((worldPos.z - offset) / cellSize))
    );
}

glm::vec3 PathfindingSystem::GridToWorld(const glm::ivec2& gridPos, float height, int agentSize) const {
    if (agentSize <= 0) agentSize = 1;
    float halfSizeOffset = static_cast<float>(agentSize) * 0.5f * cellSize;
    return glm::vec3(
        static_cast<float>(gridPos.x) * cellSize + halfSizeOffset,
        height,
        static_cast<float>(gridPos.y) * cellSize + halfSizeOffset
    );
}

void PathfindingSystem::SnapEntityToGrid(Entity entity) {
    if (!scene) return;
    if (scene->HasComponent<TransformComponent>(entity)) {
        auto& transform = scene->GetComponent<TransformComponent>(entity);
        if (scene->HasComponent<GridFragment>(entity)) {
            const auto& frag = scene->GetComponent<GridFragment>(entity);
            transform.position = frag.SnapPosition(transform.position, cellSize);
            transform.isDirty = true;
        } else if (scene->HasComponent<Obstacle>(entity)) {
            const auto& obs = scene->GetComponent<Obstacle>(entity);
            transform.position = obs.SnapPosition(transform.position, cellSize);
            transform.isDirty = true;
        } else if (scene->HasComponent<Barrier>(entity)) {
            const auto& bar = scene->GetComponent<Barrier>(entity);
            transform.position = bar.SnapPosition(transform.position, cellSize);
            transform.isDirty = true;
        }
    }
}

void PathfindingSystem::SnapAllFragmentsToGrid() {
    if (!scene) return;
    for (auto [entity, frag] : scene->GetActiveView<GridFragment>()) {
        if (frag.snapToGrid && scene->HasComponent<TransformComponent>(entity)) {
            auto& transform = scene->GetComponent<TransformComponent>(entity);
            transform.position = frag.SnapPosition(transform.position, cellSize);
            transform.isDirty = true;
        }
    }
}

void PathfindingSystem::SnapAllEntitiesToGrid() {
    if (!scene) return;
    for (auto [entity, frag] : scene->GetActiveView<GridFragment>()) {
        if (frag.snapToGrid && scene->HasComponent<TransformComponent>(entity)) {
            auto& transform = scene->GetComponent<TransformComponent>(entity);
            transform.position = frag.SnapPosition(transform.position, cellSize);
            transform.isDirty = true;
        }
    }
    for (auto [entity, obs] : scene->GetActiveView<Obstacle>()) {
        if (obs.snapToGrid && scene->HasComponent<TransformComponent>(entity)) {
            auto& transform = scene->GetComponent<TransformComponent>(entity);
            transform.position = obs.SnapPosition(transform.position, cellSize);
            transform.isDirty = true;
        }
    }
    for (auto [entity, bar] : scene->GetActiveView<Barrier>()) {
        if (bar.snapToGrid && scene->HasComponent<TransformComponent>(entity)) {
            auto& transform = scene->GetComponent<TransformComponent>(entity);
            transform.position = bar.SnapPosition(transform.position, cellSize);
            transform.isDirty = true;
        }
    }
}

void PathfindingSystem::BuildNavGrid() {
    if (!scene) {
        gridWalkable.clear();
        gridClearance.clear();
        horizontalBarrierCosts.clear();
        verticalBarrierCosts.clear();
        gridWidth = 0;
        gridHeight = 0;
        isGridDirty = false;
        return;
    }

    std::vector<std::pair<Entity, GridFragment>> fragments;
    for (auto [entity, frag] : scene->GetActiveView<GridFragment>()) {
        fragments.push_back({entity, frag});
    }

    std::vector<std::pair<Entity, Obstacle>> obstacles;
    for (auto [entity, obs] : scene->GetActiveView<Obstacle>()) {
        obstacles.push_back({entity, obs});
    }

    std::vector<std::pair<Entity, Barrier>> barriers;
    for (auto [entity, bar] : scene->GetActiveView<Barrier>()) {
        barriers.push_back({entity, bar});
    }

    if (fragments.empty() && obstacles.empty() && barriers.empty()) {
        minGridX = 0;
        minGridY = 0;
        maxGridX = 0;
        maxGridY = 0;
        gridWidth = 1;
        gridHeight = 1;
        gridWalkable.assign(1, defaultEmptyWalkable ? 1 : 0);
        gridClearance.assign(1, defaultEmptyWalkable ? 1 : 0);
        horizontalBarrierCosts.clear();
        verticalBarrierCosts.clear();
        isGridDirty = false;
        return;
    }

    bool hasBounds = false;
    int computedMinX = 0, computedMinY = 0, computedMaxX = 0, computedMaxY = 0;

    for (auto& [entity, frag] : fragments) {
        glm::vec3 pos{0.0f};
        if (scene->HasComponent<TransformComponent>(entity)) {
            auto& transform = scene->GetComponent<TransformComponent>(entity);
            if (frag.snapToGrid) {
                transform.position = frag.SnapPosition(transform.position, cellSize);
                transform.isDirty = true;
            }
            pos = transform.position;
        }

        float halfW = static_cast<float>(frag.width) * 0.5f;
        float halfH = static_cast<float>(frag.height) * 0.5f;
        int ox = static_cast<int>(std::floor((pos.x / cellSize) - halfW + 0.5f));
        int oy = static_cast<int>(std::floor((pos.z / cellSize) - halfH + 0.5f));
        int ex = ox + std::max(1, frag.width) - 1;
        int ey = oy + std::max(1, frag.height) - 1;

        if (!hasBounds) {
            computedMinX = ox;
            computedMinY = oy;
            computedMaxX = ex;
            computedMaxY = ey;
            hasBounds = true;
        } else {
            computedMinX = std::min(computedMinX, ox);
            computedMinY = std::min(computedMinY, oy);
            computedMaxX = std::max(computedMaxX, ex);
            computedMaxY = std::max(computedMaxY, ey);
        }
    }

    for (auto& [entity, obs] : obstacles) {
        glm::vec3 pos{0.0f};
        if (scene->HasComponent<TransformComponent>(entity)) {
            auto& transform = scene->GetComponent<TransformComponent>(entity);
            if (obs.snapToGrid) {
                transform.position = obs.SnapPosition(transform.position, cellSize);
                transform.isDirty = true;
            }
            pos = transform.position;
        }

        float halfW = static_cast<float>(obs.width) * 0.5f;
        float halfH = static_cast<float>(obs.height) * 0.5f;
        int ox = static_cast<int>(std::floor((pos.x / cellSize) - halfW + 0.5f));
        int oy = static_cast<int>(std::floor((pos.z / cellSize) - halfH + 0.5f));
        int ex = ox + std::max(1, obs.width) - 1;
        int ey = oy + std::max(1, obs.height) - 1;

        if (!hasBounds) {
            computedMinX = ox;
            computedMinY = oy;
            computedMaxX = ex;
            computedMaxY = ey;
            hasBounds = true;
        } else {
            computedMinX = std::min(computedMinX, ox);
            computedMinY = std::min(computedMinY, oy);
            computedMaxX = std::max(computedMaxX, ex);
            computedMaxY = std::max(computedMaxY, ey);
        }
    }

    for (auto& [entity, bar] : barriers) {
        glm::vec3 pos{0.0f};
        if (scene->HasComponent<TransformComponent>(entity)) {
            auto& transform = scene->GetComponent<TransformComponent>(entity);
            if (bar.snapToGrid) {
                transform.position = bar.SnapPosition(transform.position, cellSize);
                transform.isDirty = true;
            }
            pos = transform.position;
        }

        int ox = 0, ex = 0, oy = 0, ey = 0;
        if (bar.orientation == BarrierOrientation::Horizontal) {
            int gridY = static_cast<int>(std::round(pos.z / cellSize));
            float halfLen = static_cast<float>(bar.length) * 0.5f;
            ox = static_cast<int>(std::floor((pos.x / cellSize) - halfLen + 0.5f));
            ex = ox + std::max(1, bar.length) - 1;
            oy = gridY - 1;
            ey = gridY;
        } else {
            int gridX = static_cast<int>(std::round(pos.x / cellSize));
            float halfLen = static_cast<float>(bar.length) * 0.5f;
            oy = static_cast<int>(std::floor((pos.z / cellSize) - halfLen + 0.5f));
            ey = oy + std::max(1, bar.length) - 1;
            ox = gridX - 1;
            ex = gridX;
        }

        if (!hasBounds) {
            computedMinX = ox;
            computedMinY = oy;
            computedMaxX = ex;
            computedMaxY = ey;
            hasBounds = true;
        } else {
            computedMinX = std::min(computedMinX, ox);
            computedMinY = std::min(computedMinY, oy);
            computedMaxX = std::max(computedMaxX, ex);
            computedMaxY = std::max(computedMaxY, ey);
        }
    }

    minGridX = computedMinX;
    minGridY = computedMinY;
    maxGridX = computedMaxX;
    maxGridY = computedMaxY;
    gridWidth = maxGridX - minGridX + 1;
    gridHeight = maxGridY - minGridY + 1;

    gridWalkable.assign(static_cast<size_t>(gridWidth * gridHeight), defaultEmptyWalkable ? 1 : 0);

    for (auto& [entity, frag] : fragments) {
        glm::vec3 pos{0.0f};
        if (scene->HasComponent<TransformComponent>(entity)) {
            pos = scene->GetComponent<TransformComponent>(entity).position;
        }

        float halfW = static_cast<float>(frag.width) * 0.5f;
        float halfH = static_cast<float>(frag.height) * 0.5f;
        int ox = static_cast<int>(std::floor((pos.x / cellSize) - halfW + 0.5f));
        int oy = static_cast<int>(std::floor((pos.z / cellSize) - halfH + 0.5f));

        for (int ly = 0; ly < frag.height; ++ly) {
            for (int lx = 0; lx < frag.width; ++lx) {
                int gx = ox + lx;
                int gy = oy + ly;
                if (IsInBounds(gx, gy)) {
                    gridWalkable[ToGridIndex(gx, gy)] = frag.IsCellWalkable(lx, ly) ? 1 : 0;
                }
            }
        }
    }

    for (auto& [entity, obs] : obstacles) {
        if (!obs.enabled) continue;

        glm::vec3 pos{0.0f};
        if (scene->HasComponent<TransformComponent>(entity)) {
            pos = scene->GetComponent<TransformComponent>(entity).position;
        }

        float halfW = static_cast<float>(obs.width) * 0.5f;
        float halfH = static_cast<float>(obs.height) * 0.5f;
        int ox = static_cast<int>(std::floor((pos.x / cellSize) - halfW + 0.5f));
        int oy = static_cast<int>(std::floor((pos.z / cellSize) - halfH + 0.5f));

        for (int ly = 0; ly < obs.height; ++ly) {
            for (int lx = 0; lx < obs.width; ++lx) {
                int gx = ox + lx;
                int gy = oy + ly;
                if (IsInBounds(gx, gy)) {
                    gridWalkable[ToGridIndex(gx, gy)] = 0;
                }
            }
        }
    }

    CalculateClearanceMap();

    size_t barrierArraySize = static_cast<size_t>((gridWidth + 2) * (gridHeight + 2));
    horizontalBarrierCosts.assign(barrierArraySize, 0.0f);
    verticalBarrierCosts.assign(barrierArraySize, 0.0f);

    for (auto& [entity, bar] : barriers) {
        if (!bar.enabled) continue;

        glm::vec3 pos{0.0f};
        if (scene->HasComponent<TransformComponent>(entity)) {
            pos = scene->GetComponent<TransformComponent>(entity).position;
        }

        if (bar.orientation == BarrierOrientation::Horizontal) {
            int gridY = static_cast<int>(std::round(pos.z / cellSize));
            float halfLen = static_cast<float>(bar.length) * 0.5f;
            int ox = static_cast<int>(std::floor((pos.x / cellSize) - halfLen + 0.5f));
            for (int i = 0; i < bar.length; ++i) {
                int x = ox + i;
                int y = gridY - 1;
                AddHorizontalBarrierCost(x, y, bar.extraCost);
            }
        } else {
            int gridX = static_cast<int>(std::round(pos.x / cellSize));
            float halfLen = static_cast<float>(bar.length) * 0.5f;
            int oy = static_cast<int>(std::floor((pos.z / cellSize) - halfLen + 0.5f));
            for (int i = 0; i < bar.length; ++i) {
                int y = oy + i;
                int x = gridX - 1;
                AddVerticalBarrierCost(x, y, bar.extraCost);
            }
        }
    }

    isGridDirty = false;
}

void PathfindingSystem::CalculateClearanceMap() {
    if (gridWidth <= 0 || gridHeight <= 0 || gridWalkable.empty()) {
        gridClearance.clear();
        return;
    }

    gridClearance.assign(static_cast<size_t>(gridWidth * gridHeight), 0);

    for (int y = maxGridY; y >= minGridY; --y) {
        for (int x = maxGridX; x >= minGridX; --x) {
            size_t idx = ToGridIndex(x, y);
            if (gridWalkable[idx] == 0) {
                gridClearance[idx] = 0;
            } else {
                if (x == maxGridX || y == maxGridY) {
                    gridClearance[idx] = 1;
                } else {
                    uint8_t right = gridClearance[ToGridIndex(x + 1, y)];
                    uint8_t down = gridClearance[ToGridIndex(x, y + 1)];
                    uint8_t diag = gridClearance[ToGridIndex(x + 1, y + 1)];
                    uint8_t minNeighbor = std::min({right, down, diag});
                    gridClearance[idx] = static_cast<uint8_t>(std::min(255, 1 + minNeighbor));
                }
            }
        }
    }
}

bool PathfindingSystem::IsInBounds(int x, int y) const {
    return x >= minGridX && x <= maxGridX && y >= minGridY && y <= maxGridY && gridWidth > 0 && gridHeight > 0;
}

uint8_t PathfindingSystem::GetClearance(int x, int y) const {
    if (!IsInBounds(x, y) || gridClearance.empty()) return 0;
    return gridClearance[ToGridIndex(x, y)];
}

bool PathfindingSystem::IsWalkable(int x, int y, int agentSize) const {
    if (agentSize <= 0) agentSize = 1;
    if (!IsInBounds(x, y)) return false;
    return GetClearance(x, y) >= static_cast<uint8_t>(agentSize);
}

void PathfindingSystem::SetCellWalkable(int x, int y, bool walkable) {
    if (!IsInBounds(x, y)) return;
    gridWalkable[ToGridIndex(x, y)] = walkable ? 1 : 0;
    CalculateClearanceMap();
}

float PathfindingSystem::GetHorizontalBarrierCost(int x, int y) const {
    if (horizontalBarrierCosts.empty()) return 0.0f;
    int ix = x - minGridX + 1;
    int iy = y - minGridY + 1;
    if (ix < 0 || ix >= gridWidth + 2 || iy < 0 || iy >= gridHeight + 2) return 0.0f;
    return horizontalBarrierCosts[static_cast<size_t>(iy * (gridWidth + 2) + ix)];
}

void PathfindingSystem::SetHorizontalBarrierCost(int x, int y, float cost) {
    if (horizontalBarrierCosts.empty()) return;
    int ix = x - minGridX + 1;
    int iy = y - minGridY + 1;
    if (ix < 0 || ix >= gridWidth + 2 || iy < 0 || iy >= gridHeight + 2) return;
    horizontalBarrierCosts[static_cast<size_t>(iy * (gridWidth + 2) + ix)] = cost;
}

void PathfindingSystem::AddHorizontalBarrierCost(int x, int y, float cost) {
    if (horizontalBarrierCosts.empty()) return;
    int ix = x - minGridX + 1;
    int iy = y - minGridY + 1;
    if (ix < 0 || ix >= gridWidth + 2 || iy < 0 || iy >= gridHeight + 2) return;
    horizontalBarrierCosts[static_cast<size_t>(iy * (gridWidth + 2) + ix)] += cost;
}

float PathfindingSystem::GetVerticalBarrierCost(int x, int y) const {
    if (verticalBarrierCosts.empty()) return 0.0f;
    int ix = x - minGridX + 1;
    int iy = y - minGridY + 1;
    if (ix < 0 || ix >= gridWidth + 2 || iy < 0 || iy >= gridHeight + 2) return 0.0f;
    return verticalBarrierCosts[static_cast<size_t>(iy * (gridWidth + 2) + ix)];
}

void PathfindingSystem::SetVerticalBarrierCost(int x, int y, float cost) {
    if (verticalBarrierCosts.empty()) return;
    int ix = x - minGridX + 1;
    int iy = y - minGridY + 1;
    if (ix < 0 || ix >= gridWidth + 2 || iy < 0 || iy >= gridHeight + 2) return;
    verticalBarrierCosts[static_cast<size_t>(iy * (gridWidth + 2) + ix)] = cost;
}

void PathfindingSystem::AddVerticalBarrierCost(int x, int y, float cost) {
    if (verticalBarrierCosts.empty()) return;
    int ix = x - minGridX + 1;
    int iy = y - minGridY + 1;
    if (ix < 0 || ix >= gridWidth + 2 || iy < 0 || iy >= gridHeight + 2) return;
    verticalBarrierCosts[static_cast<size_t>(iy * (gridWidth + 2) + ix)] += cost;
}

float PathfindingSystem::GetEdgeBarrierCost(int x1, int y1, int x2, int y2) const {
    if (x1 == x2 && y2 == y1 + 1) {
        return GetHorizontalBarrierCost(x1, y1);
    } else if (x1 == x2 && y2 == y1 - 1) {
        return GetHorizontalBarrierCost(x1, y2);
    } else if (y1 == y2 && x2 == x1 + 1) {
        return GetVerticalBarrierCost(x1, y1);
    } else if (y1 == y2 && x2 == x1 - 1) {
        return GetVerticalBarrierCost(x2, y1);
    }
    return 0.0f;
}

float PathfindingSystem::GetMovementBarrierCost(const glm::ivec2& from, const glm::ivec2& to, int agentSize) const {
    if (agentSize <= 0) agentSize = 1;
    glm::ivec2 dir = to - from;
    float cost = 0.0f;

    if (dir.x != 0 && dir.y == 0) {
        int xEdge = (dir.x > 0) ? (from.x + agentSize - 1) : (from.x - 1);
        for (int k = 0; k < agentSize; ++k) {
            cost += GetVerticalBarrierCost(xEdge, from.y + k);
        }
    } else if (dir.x == 0 && dir.y != 0) {
        int yEdge = (dir.y > 0) ? (from.y + agentSize - 1) : (from.y - 1);
        for (int k = 0; k < agentSize; ++k) {
            cost += GetHorizontalBarrierCost(from.x + k, yEdge);
        }
    } else if (dir.x != 0 && dir.y != 0) {
        float route1 = 0.0f;
        float route2 = 0.0f;
        int xEdge = (dir.x > 0) ? (from.x + agentSize - 1) : (from.x - 1);
        int yEdge = (dir.y > 0) ? (from.y + agentSize - 1) : (from.y - 1);

        for (int k = 0; k < agentSize; ++k) {
            route1 += GetVerticalBarrierCost(xEdge, from.y + k);
            route1 += GetHorizontalBarrierCost(from.x + dir.x + k, yEdge);

            route2 += GetHorizontalBarrierCost(from.x + k, yEdge);
            route2 += GetVerticalBarrierCost(xEdge, from.y + dir.y + k);
        }
        cost = std::max(route1, route2);
    }
    return cost;
}

std::vector<glm::ivec2> PathfindingSystem::FindPathGrid(const glm::ivec2& start, const glm::ivec2& target, int agentSize, Entity ignoreEntity) {
    if (isGridDirty) {
        BuildNavGrid();
    }

    if (agentSize <= 0) agentSize = 1;

    if (!IsInBounds(start.x, start.y) || !IsInBounds(target.x, target.y)) {
        return {};
    }

    // Collect cells occupied by other active agents in the scene
    std::vector<uint8_t> agentOccupied;
    if (scene && gridWidth > 0 && gridHeight > 0) {
        agentOccupied.assign(static_cast<size_t>(gridWidth * gridHeight), 0);
        for (auto [otherEnt, otherAgent] : scene->GetActiveView<PathfindingAgent>()) {
            if (otherEnt == ignoreEntity) continue;
            if (!scene->HasComponent<TransformComponent>(otherEnt)) continue;

            const auto& trans = scene->GetComponent<TransformComponent>(otherEnt);
            int sz = std::max(1, otherAgent.agentSize);
            glm::ivec2 agentGrid = WorldToGrid(trans.position, sz);

            if (ignoreEntity == MAX_ENTITIES && agentGrid == start) {
                continue;
            }

            for (int dy = 0; dy < sz; ++dy) {
                for (int dx = 0; dx < sz; ++dx) {
                    int gx = agentGrid.x + dx;
                    int gy = agentGrid.y + dy;
                    if (IsInBounds(gx, gy)) {
                        agentOccupied[ToGridIndex(gx, gy)] = 1;
                    }
                }
            }
        }
    }

    auto isCellFree = [&](int x, int y) -> bool {
        if (!IsWalkable(x, y, agentSize)) return false;
        if (agentOccupied.empty()) return true;
        for (int dy = 0; dy < agentSize; ++dy) {
            for (int dx = 0; dx < agentSize; ++dx) {
                int gx = x + dx;
                int gy = y + dy;
                if (!IsInBounds(gx, gy) || agentOccupied[ToGridIndex(gx, gy)] != 0) {
                    return false;
                }
            }
        }
        return true;
    };

    if (!isCellFree(start.x, start.y) || !isCellFree(target.x, target.y)) {
        return {};
    }

    if (start == target) {
        return {start};
    }

    struct Node {
        glm::ivec2 pos;
        float gCost;
        float fCost;

        bool operator>(const Node& other) const {
            return fCost > other.fCost;
        }
    };

    size_t totalCells = static_cast<size_t>(gridWidth * gridHeight);
    const float INF = std::numeric_limits<float>::infinity();
    std::vector<float> gScore(totalCells, INF);
    std::vector<int> cameFrom(totalCells, -1);
    std::vector<bool> closedSet(totalCells, false);

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;

    auto Heuristic = [](const glm::ivec2& a, const glm::ivec2& b) -> float {
        int dx = std::abs(a.x - b.x);
        int dy = std::abs(a.y - b.y);
        return static_cast<float>(std::max(dx, dy)) + 0.41421356f * static_cast<float>(std::min(dx, dy));
    };

    size_t startIdx = ToGridIndex(start.x, start.y);
    gScore[startIdx] = 0.0f;
    openSet.push({start, 0.0f, Heuristic(start, target)});

    static const glm::ivec2 directions[8] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };
    static const float moveCosts[8] = {
        1.0f, 1.0f, 1.0f, 1.0f,
        1.41421356f, 1.41421356f, 1.41421356f, 1.41421356f
    };

    bool reached = false;

    while (!openSet.empty()) {
        Node current = openSet.top();
        openSet.pop();

        if (current.pos == target) {
            reached = true;
            break;
        }

        size_t currentIdx = ToGridIndex(current.pos.x, current.pos.y);
        if (closedSet[currentIdx]) continue;
        closedSet[currentIdx] = true;

        for (int i = 0; i < 8; ++i) {
            glm::ivec2 dir = directions[i];
            glm::ivec2 neighbor = current.pos + dir;

            if (!IsInBounds(neighbor.x, neighbor.y)) continue;
            if (!isCellFree(neighbor.x, neighbor.y)) continue;

            if (i >= 4) {
                if (!isCellFree(current.pos.x + dir.x, current.pos.y) ||
                    !isCellFree(current.pos.x, current.pos.y + dir.y)) {
                    continue;
                }
            }

            size_t neighborIdx = ToGridIndex(neighbor.x, neighbor.y);
            if (closedSet[neighborIdx]) continue;

            float barrierCost = GetMovementBarrierCost(current.pos, neighbor, agentSize);
            float tentativeG = gScore[currentIdx] + moveCosts[i] + barrierCost;
            if (tentativeG < gScore[neighborIdx]) {
                cameFrom[neighborIdx] = static_cast<int>(currentIdx);
                gScore[neighborIdx] = tentativeG;
                float f = tentativeG + Heuristic(neighbor, target);
                openSet.push({neighbor, tentativeG, f});
            }
        }
    }

    if (!reached) {
        return {};
    }

    std::vector<glm::ivec2> path;
    int curr = static_cast<int>(ToGridIndex(target.x, target.y));
    int startI = static_cast<int>(startIdx);

    while (curr != -1) {
        int cx = (curr % gridWidth) + minGridX;
        int cy = (curr / gridWidth) + minGridY;
        path.push_back({cx, cy});
        if (curr == startI) break;
        curr = cameFrom[static_cast<size_t>(curr)];
    }

    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<glm::vec3> PathfindingSystem::FindPath(const glm::vec3& startWorld, const glm::vec3& targetWorld, int agentSize, float height, Entity ignoreEntity) {
    glm::ivec2 startGrid = WorldToGrid(startWorld, agentSize);
    glm::ivec2 targetGrid = WorldToGrid(targetWorld, agentSize);

    auto gridPath = FindPathGrid(startGrid, targetGrid, agentSize, ignoreEntity);
    if (gridPath.empty()) {
        return {};
    }

    std::vector<glm::vec3> worldPath;
    worldPath.reserve(gridPath.size());

    for (const auto& gp : gridPath) {
        worldPath.push_back(GridToWorld(gp, height, agentSize));
    }

    if (!worldPath.empty()) {
        worldPath.front() = glm::vec3(startWorld.x, height, startWorld.z);
        worldPath.back() = glm::vec3(targetWorld.x, height, targetWorld.z);
    }

    return worldPath;
}

bool PathfindingSystem::RequestPath(Entity entity, const glm::vec3& targetWorld) {
    if (!scene || !scene->HasComponent<PathfindingAgent>(entity)) return false;
    auto& agent = scene->GetComponent<PathfindingAgent>(entity);
    glm::vec3 startPos{0.0f};
    if (scene->HasComponent<TransformComponent>(entity)) {
        startPos = scene->GetComponent<TransformComponent>(entity).position;
    }
    agent.SetTarget(targetWorld);
    agent.waypoints = FindPath(startPos, targetWorld, agent.agentSize, startPos.y, entity);
    agent.currentWaypointIndex = 0;
    agent.isMoving = !agent.waypoints.empty();
    agent.reachedTarget = agent.waypoints.empty() && (WorldToGrid(startPos, agent.agentSize) == WorldToGrid(targetWorld, agent.agentSize));
    if (agent.reachedTarget && scene->HasComponent<TransformComponent>(entity)) {
        auto& transform = scene->GetComponent<TransformComponent>(entity);
        transform.position.x = targetWorld.x;
        transform.position.z = targetWorld.z;
        transform.isDirty = true;
    }
    return agent.isMoving || agent.reachedTarget;
}

void PathfindingSystem::Update(float deltaTime) {
    if (!scene) return;

    if (isGridDirty) {
        BuildNavGrid();
    }

    for (auto [entity, agent] : scene->GetActiveView<PathfindingAgent>()) {
        if (agent.hasTarget && !agent.isMoving && !agent.reachedTarget) {
            if (scene->HasComponent<TransformComponent>(entity)) {
                auto& transform = scene->GetComponent<TransformComponent>(entity);
                agent.waypoints = FindPath(transform.position, agent.targetPosition, agent.agentSize, transform.position.y, entity);
                agent.currentWaypointIndex = 0;
                agent.isMoving = !agent.waypoints.empty();
                if (agent.waypoints.empty()) {
                    glm::ivec2 curGrid = WorldToGrid(transform.position, agent.agentSize);
                    glm::ivec2 tgtGrid = WorldToGrid(agent.targetPosition, agent.agentSize);
                    agent.reachedTarget = (curGrid == tgtGrid);
                    if (agent.reachedTarget) {
                        transform.position.x = agent.targetPosition.x;
                        transform.position.z = agent.targetPosition.z;
                        transform.isDirty = true;
                    }
                    agent.hasTarget = false;
                }
            }
        }

        if (agent.isMoving && agent.autoMove && scene->HasComponent<TransformComponent>(entity)) {
            auto& transform = scene->GetComponent<TransformComponent>(entity);
            if (agent.currentWaypointIndex < agent.waypoints.size()) {
                glm::vec3 targetWaypoint = agent.waypoints[agent.currentWaypointIndex];
                glm::vec3 diff = targetWaypoint - transform.position;
                glm::vec2 diffXZ(diff.x, diff.z);
                float dist = glm::length(diffXZ);

                if (dist <= agent.waypointThreshold) {
                    agent.currentWaypointIndex++;
                    if (agent.currentWaypointIndex >= agent.waypoints.size()) {
                        transform.position.x = targetWaypoint.x;
                        transform.position.z = targetWaypoint.z;
                        transform.isDirty = true;
                        agent.isMoving = false;
                        agent.hasTarget = false;
                        agent.reachedTarget = true;
                    }
                } else {
                    glm::vec2 dir2D = diffXZ / dist;
                    float step = agent.speed * deltaTime;
                    if (step >= dist) {
                        transform.position.x = targetWaypoint.x;
                        transform.position.z = targetWaypoint.z;
                        transform.isDirty = true;
                        agent.currentWaypointIndex++;
                        if (agent.currentWaypointIndex >= agent.waypoints.size()) {
                            agent.isMoving = false;
                            agent.hasTarget = false;
                            agent.reachedTarget = true;
                        }
                    } else {
                        transform.position.x += dir2D.x * step;
                        transform.position.z += dir2D.y * step;
                        transform.isDirty = true;
                    }

                    if (glm::length(dir2D) > 0.001f) {
                        float angle = std::atan2(dir2D.x, dir2D.y);
                        transform.rotation = glm::angleAxis(angle, glm::vec3(0.0f, 1.0f, 0.0f));
                    }
                }
            } else {
                if (!agent.waypoints.empty()) {
                    transform.position.x = agent.waypoints.back().x;
                    transform.position.z = agent.waypoints.back().z;
                    transform.isDirty = true;
                } else if (agent.hasTarget) {
                    transform.position.x = agent.targetPosition.x;
                    transform.position.z = agent.targetPosition.z;
                    transform.isDirty = true;
                }
                agent.isMoving = false;
                agent.hasTarget = false;
                agent.reachedTarget = true;
            }
        }
    }
}

void PathfindingSystem::OnComponentAdded(ComponentID componentID, std::type_index type) {
    if (type == std::type_index(typeid(GridFragment)) ||
        type == std::type_index(typeid(Obstacle)) ||
        type == std::type_index(typeid(Barrier))) {
        MarkGridDirty();
    }
}

void PathfindingSystem::OnEntityRemoved(ComponentID componentID, std::type_index type) {
    if (type == std::type_index(typeid(GridFragment)) ||
        type == std::type_index(typeid(Obstacle)) ||
        type == std::type_index(typeid(Barrier))) {
        MarkGridDirty();
    }
}

} // namespace engine::ecs

