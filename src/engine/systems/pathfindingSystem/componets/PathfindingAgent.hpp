//
// Created by redkc on 14/10/2026.
//

#ifndef REASONABLEVULKAN_PATHFINDINGAGENT_HPP
#define REASONABLEVULKAN_PATHFINDINGAGENT_HPP

#include <vector>
#include <glm/glm.hpp>
#include "ecs/Component.hpp"

namespace engine::ecs
{
    struct PathfindingAgent : public Component
    {
        [[=Range{1.0f, 16.0f, 1.0f}, =Tooltip{"Agent box size on grid (e.g. 1 for 1x1, 2 for 2x2, 3 for 3x3)"}]]
        int agentSize{1};

        [[=Range{0.1f, 100.0f, 0.5f}, =Tooltip{"Movement speed in world units per second"}]]
        float speed{5.0f};

        [[=Range{0.01f, 10.0f, 0.05f}, =Tooltip{"Distance threshold to reach waypoint"}]]
        float waypointThreshold{0.1f};

        [[=Range{0.0f, 10000.0f, 0.5f}, =Tooltip{"Maximum movement traversal cost allowance per move (0 for unlimited)"}]]
        float movementAllowance{0.0f};

        [[=Tooltip{"Target world position"}]]
        glm::vec3 targetPosition{0.0f};

        [[=Tooltip{"Whether agent has an active target destination"}]]
        bool hasTarget{false};

        [[=Tooltip{"Whether agent is currently moving along a path"}]]
        bool isMoving{false};

        [[=Tooltip{"Whether agent has reached its target"}]]
        bool reachedTarget{false};

        [[=Tooltip{"Whether system automatically updates transform along path"}]]
        bool autoMove{true};

        [[=NonSerialized{}, =HidenInInspector{}]]
        std::vector<glm::vec3> waypoints{};

        [[=NonSerialized{}, =HidenInInspector{}]]
        size_t currentWaypointIndex{0};

        PathfindingAgent() = default;
        explicit PathfindingAgent(int size, float moveSpeed = 5.0f, float allowance = 0.0f)
            : agentSize(size), speed(moveSpeed), movementAllowance(allowance) {}
        explicit PathfindingAgent(float allowance)
            : movementAllowance(allowance) {}

        float GetMovementAllowance() const { return movementAllowance; }
        void SetMovementAllowance(float allowance) { movementAllowance = allowance; }
        bool HasMovementAllowance() const { return movementAllowance > 0.0f; }

        void SetTarget(const glm::vec3& target) {
            targetPosition = target;
            hasTarget = true;
            reachedTarget = false;
            isMoving = false;
            waypoints.clear();
            currentWaypointIndex = 0;
        }

        void Stop() {
            hasTarget = false;
            isMoving = false;
            waypoints.clear();
            currentWaypointIndex = 0;
        }
    };
}

#endif //REASONABLEVULKAN_PATHFINDINGAGENT_HPP
