#include "SelectionSystem.hpp"

#include <limits>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

#include "ecs/Scene.h"
#include "systems/transformSystem/componets/TransformComponent.hpp"
#include "systems/renderingSystem/componets/CameraComponent.hpp"
#include "systems/collisionSystem/CollisionSystem.hpp"
#include "systems/pathfindingSystem/PathfindingSystem.hpp"
#include "systems/pathfindingSystem/componets/GridFragment.hpp"
#include "systems/gizmoSystem/GizmoSystem.hpp"

namespace engine::ecs {

    SelectionSystem::SelectionSystem(Scene* scene) : System(scene) {
    }

    void SelectionSystem::SetSelectedEntity(Entity entity) {
        if (entity != MAX_ENTITIES && scene && scene->IsEntityActive(entity)) {
            if (requirePathfindingAgent) {
                if (scene->HasComponent<PathfindingAgent>(entity)) {
                    selectedEntity = entity;
                } else {
                    selectedEntity = MAX_ENTITIES;
                }
            } else {
                selectedEntity = entity;
            }
        } else {
            selectedEntity = MAX_ENTITIES;
        }
    }

    void SelectionSystem::ClearSelection() {
        selectedEntity = MAX_ENTITIES;
    }

    bool SelectionSystem::HasSelectedEntity() const {
        return selectedEntity != MAX_ENTITIES && scene && scene->IsEntityActive(selectedEntity);
    }

    void SelectionSystem::OnEntityRemoved(ComponentID componentID, std::type_index type) {
        if (selectedEntity != MAX_ENTITIES && scene && !scene->IsEntityActive(selectedEntity)) {
            ClearSelection();
        }
    }

    std::optional<CameraComponent> SelectionSystem::GetActiveGameCamera(TransformComponent* outTransform) const {
        if (!scene) return std::nullopt;

        // 1. Search for active camera in scene
        for (auto [cameraEntity, camera, cameraTransform] : scene->GetActiveView<CameraComponent, TransformComponent>()) {
            if (camera.active) {
                if (outTransform) {
                    *outTransform = cameraTransform;
                }
                CameraComponent cam = camera;
                cam.view = glm::inverse(cameraTransform.globalMatrix);
                return cam;
            }
        }

        // 2. Fallback to any camera in scene
        for (auto [cameraEntity, camera, cameraTransform] : scene->GetActiveView<CameraComponent, TransformComponent>()) {
            if (outTransform) {
                *outTransform = cameraTransform;
            }
            CameraComponent cam = camera;
            cam.view = glm::inverse(cameraTransform.globalMatrix);
            return cam;
        }

        return std::nullopt;
    }

    bool SelectionSystem::HandleLeftClick(float screenX, float screenY, float windowWidth, float windowHeight) {
        return SelectAtScreenPos(screenX, screenY, windowWidth, windowHeight);
    }

    bool SelectionSystem::SelectAtScreenPos(float screenX, float screenY, float windowWidth, float windowHeight) {
        if (!scene || windowWidth <= 0.0f || windowHeight <= 0.0f) {
            return false;
        }

        TransformComponent camTransform{};
        auto camOpt = GetActiveGameCamera(&camTransform);
        if (!camOpt.has_value()) {
            return false;
        }

        CameraComponent cam = *camOpt;
        cam.aspectRatio = windowWidth / windowHeight;
        if (cam.fov > 0.0f && cam.nearPlane > 0.0f && cam.farPlane > cam.nearPlane) {
            cam.projection = glm::perspective(glm::radians(cam.fov), cam.aspectRatio, cam.nearPlane, cam.farPlane);
        }

        auto colSys = scene->GetSystem<CollisionSystem>();
        if (!colSys) {
            return false;
        }

        Ray ray = colSys->ScreenToWorldRay(cam, screenX, screenY, windowWidth, windowHeight);

        std::optional<Entity> hitEntity = std::nullopt;
        float closestDist = std::numeric_limits<float>::max();

        // 1. Try standard mesh collision raycast
        auto meshHit = colSys->RayCastClosest(ray);
        if (meshHit.has_value()) {
            hitEntity = meshHit->entity;
            closestDist = meshHit->distance;
        }

        // 2. Raycast directly against PathfindingAgent entities with bounding box
        for (auto [entity, agent, trans] : scene->GetActiveView<PathfindingAgent, TransformComponent>()) {
            float halfSz = static_cast<float>(agent.agentSize) * 0.5f;
            glm::vec3 boxMin(-halfSz, 0.0f, -halfSz);
            glm::vec3 boxMax(halfSz, static_cast<float>(agent.agentSize) * 1.5f, halfSz);
            float dist = 0.0f;
            if (colSys->RayIntersectsAABB(ray, boxMin, boxMax, trans.globalMatrix, dist)) {
                if (dist < closestDist) {
                    closestDist = dist;
                    hitEntity = entity;
                }
            }
        }

        if (hitEntity.has_value()) {
            Entity e = *hitEntity;
            if (!requirePathfindingAgent || scene->HasComponent<PathfindingAgent>(e)) {
                SetSelectedEntity(e);
                return true;
            }
        }

        ClearSelection();
        return false;
    }

    bool SelectionSystem::HandleRightClick(float screenX, float screenY, float windowWidth, float windowHeight) {
        return MoveSelectedToScreenPos(screenX, screenY, windowWidth, windowHeight);
    }

    bool SelectionSystem::MoveSelectedToScreenPos(float screenX, float screenY, float windowWidth, float windowHeight) {
        if (!HasSelectedEntity() || !scene->HasComponent<PathfindingAgent>(selectedEntity)) {
            return false;
        }

        if (windowWidth <= 0.0f || windowHeight <= 0.0f) {
            return false;
        }

        TransformComponent camTransform{};
        auto camOpt = GetActiveGameCamera(&camTransform);
        if (!camOpt.has_value()) {
            return false;
        }

        CameraComponent cam = *camOpt;
        cam.aspectRatio = windowWidth / windowHeight;
        if (cam.fov > 0.0f && cam.nearPlane > 0.0f && cam.farPlane > cam.nearPlane) {
            cam.projection = glm::perspective(glm::radians(cam.fov), cam.aspectRatio, cam.nearPlane, cam.farPlane);
        }

        auto colSys = scene->GetSystem<CollisionSystem>();
        if (!colSys) {
            return false;
        }

        Ray ray = colSys->ScreenToWorldRay(cam, screenX, screenY, windowWidth, windowHeight);

        bool foundTarget = false;
        glm::vec3 targetWorldPos{0.0f};

        // 1. Raycast against mesh entities in the scene (excluding selected entity)
        auto meshHit = colSys->RayCastClosest(ray);
        if (meshHit.has_value() && meshHit->entity != selectedEntity) {
            targetWorldPos = meshHit->hitPoint;
            foundTarget = true;
        }

        // 2. Raycast against GridFragment entities in the scene
        if (!foundTarget) {
            float closestGridDist = std::numeric_limits<float>::max();
            for (auto [gridEnt, gridFrag, trans] : scene->GetActiveView<GridFragment, TransformComponent>()) {
                float halfW = static_cast<float>(gridFrag.width) * 0.5f;
                float halfH = static_cast<float>(gridFrag.height) * 0.5f;
                glm::vec3 boxMin(-halfW, -0.2f, -halfH);
                glm::vec3 boxMax(halfW, 0.2f, halfH);
                float dist = 0.0f;
                if (colSys->RayIntersectsAABB(ray, boxMin, boxMax, trans.globalMatrix, dist)) {
                    if (dist < closestGridDist) {
                        closestGridDist = dist;
                        targetWorldPos = ray.origin + ray.direction * dist;
                        foundTarget = true;
                    }
                }
            }
        }

        // 3. Fallback ground plane intersection at y = groundY
        if (!foundTarget) {
            float groundY = defaultGroundY;
            if (scene->HasComponent<TransformComponent>(selectedEntity)) {
                groundY = scene->GetComponent<TransformComponent>(selectedEntity).position.y;
            }
            if (std::abs(ray.direction.y) > 1e-6f) {
                float t = (groundY - ray.origin.y) / ray.direction.y;
                if (t > 0.0f) {
                    targetWorldPos = ray.origin + ray.direction * t;
                    foundTarget = true;
                }
            }
        }

        if (!foundTarget) {
            return false;
        }

        return MoveSelectedToWorldPos(targetWorldPos);
    }

    bool SelectionSystem::MoveSelectedToWorldPos(const glm::vec3& targetWorldPos) {
        if (!HasSelectedEntity() || !scene->HasComponent<PathfindingAgent>(selectedEntity)) {
            return false;
        }

        auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
        if (!pathfindingSystem) {
            return false;
        }

        int agentSize = 1;
        if (scene->HasComponent<PathfindingAgent>(selectedEntity)) {
            agentSize = scene->GetComponent<PathfindingAgent>(selectedEntity).agentSize;
        }

        glm::ivec2 gridCoord = pathfindingSystem->WorldToGrid(targetWorldPos, agentSize);
        glm::vec3 gridPlacement = pathfindingSystem->GridToWorld(gridCoord, targetWorldPos.y, agentSize);
        return pathfindingSystem->RequestPath(selectedEntity, gridPlacement);
    }

    void SelectionSystem::Update(float deltaTime) {
        if (!scene) return;

        if (selectedEntity != MAX_ENTITIES) {
            if (!scene->IsEntityActive(selectedEntity) || (requirePathfindingAgent && !scene->HasComponent<PathfindingAgent>(selectedEntity))) {
                ClearSelection();
                return;
            }

            if (showSelectionGizmo && scene->HasComponent<TransformComponent>(selectedEntity)) {
                auto& trans = scene->GetComponent<TransformComponent>(selectedEntity);
                if (auto gizmoSystem = scene->GetSystem<GizmoSystem>()) {
                    float sz = 1.0f;
                    if (scene->HasComponent<PathfindingAgent>(selectedEntity)) {
                        sz = static_cast<float>(scene->GetComponent<PathfindingAgent>(selectedEntity).agentSize);
                    }
                    gizmoSystem->DrawCube(trans.position, glm::vec3(sz * 1.1f, 0.1f, sz * 1.1f), glm::vec3(0.1f, 0.9f, 0.1f), 0.0f);
                }
            }
        }
    }

} // namespace engine::ecs
