#include "TacticalSceneManagerSystem.hpp"

#include <limits>
#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

#ifdef ENABLE_IMGUI
#include <imgui.h>
#endif

#include "UUIDManager.hpp"
#include "ecs/Scene.h"
#include "systems/transformSystem/componets/TransformComponent.hpp"
#include "systems/renderingSystem/componets/CameraComponent.hpp"
#include "systems/collisionSystem/CollisionSystem.hpp"
#include "systems/pathfindingSystem/PathfindingSystem.hpp"
#include "systems/pathfindingSystem/componets/GridFragment.hpp"
#include "systems/tacticalGridSystem/TacticalGridSystem.hpp"
#include "systems/gizmoSystem/GizmoSystem.hpp"

namespace engine::ecs {

    TacticalSceneManagerSystem::TacticalSceneManagerSystem(Scene* scene) : System(scene) {
    }

    void TacticalSceneManagerSystem::SetSelectedEntity(Entity entity) {
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
        UpdatePawnHUD();
    }

    void TacticalSceneManagerSystem::ClearSelection() {
        selectedEntity = MAX_ENTITIES;
        UpdatePawnHUD();
    }

    bool TacticalSceneManagerSystem::HasSelectedEntity() const {
        return selectedEntity != MAX_ENTITIES && scene && scene->IsEntityActive(selectedEntity);
    }

    std::string TacticalSceneManagerSystem::GetSelectedEntityName() const {
        if (!HasSelectedEntity()) {
            return "";
        }
        if (scene->HasComponent<PawnComponent>(selectedEntity)) {
            return scene->GetComponent<PawnComponent>(selectedEntity).name;
        }
        return scene->GetEntityName(selectedEntity);
    }

    std::optional<float> TacticalSceneManagerSystem::GetSelectedEntityHealth() const {
        if (!HasSelectedEntity() || !scene->HasComponent<PawnComponent>(selectedEntity)) {
            return std::nullopt;
        }
        return scene->GetComponent<PawnComponent>(selectedEntity).currentHealth;
    }

    std::optional<float> TacticalSceneManagerSystem::GetSelectedEntityMaxHealth() const {
        if (!HasSelectedEntity() || !scene->HasComponent<PawnComponent>(selectedEntity)) {
            return std::nullopt;
        }
        return scene->GetComponent<PawnComponent>(selectedEntity).maxHealth;
    }

    std::optional<PawnComponent> TacticalSceneManagerSystem::GetSelectedPawnComponent() const {
        if (!HasSelectedEntity() || !scene->HasComponent<PawnComponent>(selectedEntity)) {
            return std::nullopt;
        }
        return scene->GetComponent<PawnComponent>(selectedEntity);
    }

    bool TacticalSceneManagerSystem::GetSelectedAgentInfo(std::string& outName, float& outCurrentHealth, float& outMaxHealth) const {
        if (!HasSelectedEntity()) {
            return false;
        }
        if (scene->HasComponent<PawnComponent>(selectedEntity)) {
            const auto& pawn = scene->GetComponent<PawnComponent>(selectedEntity);
            outName = pawn.name;
            outCurrentHealth = pawn.currentHealth;
            outMaxHealth = pawn.maxHealth;
        } else {
            outName = GetSelectedEntityName();
            outCurrentHealth = 0.0f;
            outMaxHealth = 0.0f;
        }
        return true;
    }

    void TacticalSceneManagerSystem::OnEntityRemoved(ComponentID componentID, std::type_index type) {
        if (selectedEntity != MAX_ENTITIES && scene && !scene->IsEntityActive(selectedEntity)) {
            ClearSelection();
        }
        if (hudHealthBackgroundEntity != MAX_ENTITIES && scene && !scene->IsEntityActive(hudHealthBackgroundEntity)) {
            hudHealthBackgroundEntity = MAX_ENTITIES;
        }
        if (hudHealthFillEntity != MAX_ENTITIES && scene && !scene->IsEntityActive(hudHealthFillEntity)) {
            hudHealthFillEntity = MAX_ENTITIES;
        }
        if (hudNameTextEntity != MAX_ENTITIES && scene && !scene->IsEntityActive(hudNameTextEntity)) {
            hudNameTextEntity = MAX_ENTITIES;
        }
    }

    void TacticalSceneManagerSystem::CreateSelectedPawnHUDEntities(const glm::vec2& size, const glm::vec2& offset, UIAnchor anchor) {
        if (!scene) return;

        DestroySelectedPawnHUDEntities();

        hudHealthBackgroundEntity = scene->CreateEntity("HUD_SelectedPawn_HP_Background");
        HUDElementComponent bgComp;
        bgComp.anchor = static_cast<int>(anchor);
        bgComp.offset = offset;
        bgComp.size = size;
        bgComp.color = glm::vec4(0.15f, 0.15f, 0.15f, 0.9f);
        bgComp.layerDepth = hudBackgroundLayerDepth;
        bgComp.isVisible = false;
        scene->AddComponent<HUDElementComponent>(hudHealthBackgroundEntity, bgComp);

        hudHealthFillEntity = scene->CreateEntity("HUD_SelectedPawn_HP_Fill");
        HUDElementComponent fillComp;
        fillComp.anchor = static_cast<int>(anchor);
        fillComp.offset = offset;
        fillComp.size = size;
        fillComp.color = glm::vec4(0.2f, 0.8f, 0.2f, 1.0f);
        fillComp.layerDepth = hudFillLayerDepth;
        fillComp.isVisible = false;
        scene->AddComponent<HUDElementComponent>(hudHealthFillEntity, fillComp);

        hudNameTextEntity = scene->CreateEntity("HUD_SelectedPawn_Name_Text");
        HUDTextComponent textComp;
        textComp.anchor = static_cast<int>(anchor);
        textComp.offset = glm::vec2(offset.x, offset.y - 25.0f);
        textComp.fontSize = 16.0f;
        textComp.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        textComp.fontUuid = hudFontUuid;
        textComp.layerDepth = hudTextLayerDepth;
        textComp.isVisible = false;
        textComp.fontUuid = hudFontUuid;
        scene->AddComponent<HUDTextComponent>(hudNameTextEntity, textComp);

        UpdatePawnHUD();
    }

    void TacticalSceneManagerSystem::DestroySelectedPawnHUDEntities() {
        if (!scene) return;

        if (hudHealthBackgroundEntity != MAX_ENTITIES && scene->IsEntityActive(hudHealthBackgroundEntity)) {
            scene->DestroyEntity(hudHealthBackgroundEntity);
        }
        hudHealthBackgroundEntity = MAX_ENTITIES;

        if (hudHealthFillEntity != MAX_ENTITIES && scene->IsEntityActive(hudHealthFillEntity)) {
            scene->DestroyEntity(hudHealthFillEntity);
        }
        hudHealthFillEntity = MAX_ENTITIES;

        if (hudNameTextEntity != MAX_ENTITIES && scene->IsEntityActive(hudNameTextEntity)) {
            scene->DestroyEntity(hudNameTextEntity);
        }
        hudNameTextEntity = MAX_ENTITIES;
    }

    void TacticalSceneManagerSystem::BindHUDEntities(Entity bgEntity, Entity fillEntity, Entity nameEntity) {
        hudHealthBackgroundEntity = bgEntity;
        hudHealthFillEntity = fillEntity;
        hudNameTextEntity = nameEntity;
        UpdatePawnHUD();
    }

    void TacticalSceneManagerSystem::UpdatePawnHUD() {
        if (!scene) return;

        bool hasBg = (hudHealthBackgroundEntity != MAX_ENTITIES && scene->IsEntityActive(hudHealthBackgroundEntity) && scene->HasComponent<HUDElementComponent>(hudHealthBackgroundEntity));
        bool hasFill = (hudHealthFillEntity != MAX_ENTITIES && scene->IsEntityActive(hudHealthFillEntity) && scene->HasComponent<HUDElementComponent>(hudHealthFillEntity));
        bool hasName = (hudNameTextEntity != MAX_ENTITIES && scene->IsEntityActive(hudNameTextEntity) && scene->HasComponent<HUDTextComponent>(hudNameTextEntity));

        if (!hasBg && !hasFill && !hasName) return;

        if (showPawnHUD && HasSelectedEntity() && scene->HasComponent<PawnComponent>(selectedEntity)) {
            const auto& pawn = scene->GetComponent<PawnComponent>(selectedEntity);
            float healthRatio = pawn.maxHealth > 0.0f ? std::clamp(pawn.currentHealth / pawn.maxHealth, 0.0f, 1.0f) : 0.0f;

            if (hasBg) {
                auto& bgComp = scene->GetComponent<HUDElementComponent>(hudHealthBackgroundEntity);
                bgComp.isVisible = true;

                if (hasFill) {
                    auto& fillComp = scene->GetComponent<HUDElementComponent>(hudHealthFillEntity);
                    fillComp.isVisible = true;
                    fillComp.anchor = bgComp.anchor;
                    fillComp.offset = bgComp.offset;
                    fillComp.size.x = bgComp.size.x * healthRatio;
                    fillComp.size.y = bgComp.size.y;
                    fillComp.color = glm::vec4(1.0f - healthRatio, healthRatio, 0.1f, 1.0f);
                }
            } else if (hasFill) {
                auto& fillComp = scene->GetComponent<HUDElementComponent>(hudHealthFillEntity);
                fillComp.isVisible = true;
            }

            if (hasName) {
                auto& textComp = scene->GetComponent<HUDTextComponent>(hudNameTextEntity);
                textComp.isVisible = true;
                if (!pawn.name.empty()) {
                    textComp.text = pawn.name;
                } else if (scene->HasComponent<NameComponent>(selectedEntity)) {
                    textComp.text = scene->GetComponent<NameComponent>(selectedEntity).name;
                } else {
                    textComp.text = "Pawn";
                }
            }
        } else {
            if (hasBg) {
                scene->GetComponent<HUDElementComponent>(hudHealthBackgroundEntity).isVisible = false;
            }
            if (hasFill) {
                scene->GetComponent<HUDElementComponent>(hudHealthFillEntity).isVisible = false;
            }
            if (hasName) {
                scene->GetComponent<HUDTextComponent>(hudNameTextEntity).isVisible = false;
            }
        }
    }

    std::optional<CameraComponent> TacticalSceneManagerSystem::GetActiveGameCamera(TransformComponent* outTransform) const {
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

    bool TacticalSceneManagerSystem::HandleLeftClick(float screenX, float screenY, float windowWidth, float windowHeight) {
        return SelectAtScreenPos(screenX, screenY, windowWidth, windowHeight);
    }

    bool TacticalSceneManagerSystem::SelectAtScreenPos(float screenX, float screenY, float windowWidth, float windowHeight) {
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

    std::optional<glm::vec3> TacticalSceneManagerSystem::RaycastGroundTarget(float screenX, float screenY, float windowWidth, float windowHeight) const {
        if (!scene || windowWidth <= 0.0f || windowHeight <= 0.0f) {
            return std::nullopt;
        }

        TransformComponent camTransform{};
        auto camOpt = GetActiveGameCamera(&camTransform);
        if (!camOpt.has_value()) {
            return std::nullopt;
        }

        CameraComponent cam = *camOpt;
        cam.aspectRatio = windowWidth / windowHeight;
        if (cam.fov > 0.0f && cam.nearPlane > 0.0f && cam.farPlane > cam.nearPlane) {
            cam.projection = glm::perspective(glm::radians(cam.fov), cam.aspectRatio, cam.nearPlane, cam.farPlane);
        }

        auto colSys = scene->GetSystem<CollisionSystem>();
        if (!colSys) {
            return std::nullopt;
        }

        Ray ray = colSys->ScreenToWorldRay(cam, screenX, screenY, windowWidth, windowHeight);

        // 1. Raycast against mesh entities in the scene (excluding selected entity)
        auto meshHit = colSys->RayCastClosest(ray);
        if (meshHit.has_value() && meshHit->entity != selectedEntity) {
            return meshHit->hitPoint;
        }

        // 2. Raycast against GridFragment entities in the scene
        float closestGridDist = std::numeric_limits<float>::max();
        bool foundGrid = false;
        glm::vec3 gridHitPos{0.0f};

        for (auto [gridEnt, gridFrag, trans] : scene->GetActiveView<GridFragment, TransformComponent>()) {
            float halfW = static_cast<float>(gridFrag.width) * 0.5f;
            float halfH = static_cast<float>(gridFrag.height) * 0.5f;
            glm::vec3 boxMin(-halfW, -0.2f, -halfH);
            glm::vec3 boxMax(halfW, 0.2f, halfH);
            float dist = 0.0f;
            if (colSys->RayIntersectsAABB(ray, boxMin, boxMax, trans.globalMatrix, dist)) {
                if (dist < closestGridDist) {
                    closestGridDist = dist;
                    gridHitPos = ray.origin + ray.direction * dist;
                    foundGrid = true;
                }
            }
        }

        if (foundGrid) {
            return gridHitPos;
        }

        // 3. Fallback ground plane intersection at y = groundY
        float groundY = defaultGroundY;
        if (selectedEntity != MAX_ENTITIES && scene->HasComponent<TransformComponent>(selectedEntity)) {
            groundY = scene->GetComponent<TransformComponent>(selectedEntity).position.y;
        }
        if (std::abs(ray.direction.y) > 1e-6f) {
            float t = (groundY - ray.origin.y) / ray.direction.y;
            if (t > 0.0f) {
                return ray.origin + ray.direction * t;
            }
        }

        return std::nullopt;
    }

    bool TacticalSceneManagerSystem::HandleMouseMove(float screenX, float screenY, float windowWidth, float windowHeight) {
        return HoverAtScreenPos(screenX, screenY, windowWidth, windowHeight);
    }

    bool TacticalSceneManagerSystem::HoverAtScreenPos(float screenX, float screenY, float windowWidth, float windowHeight) {
        if (!HasSelectedEntity()) {
            if (auto tacSys = scene ? scene->GetSystem<TacticalGridSystem>() : nullptr) {
                tacSys->ClearHover();
            }
            return false;
        }

        auto hitPosOpt = RaycastGroundTarget(screenX, screenY, windowWidth, windowHeight);
        if (hitPosOpt.has_value()) {
            if (auto tacSys = scene->GetSystem<TacticalGridSystem>()) {
                tacSys->SetHoverPosition(*hitPosOpt);
            }
            return true;
        } else {
            if (auto tacSys = scene->GetSystem<TacticalGridSystem>()) {
                tacSys->ClearHover();
            }
            return false;
        }
    }

    bool TacticalSceneManagerSystem::HandleRightClick(float screenX, float screenY, float windowWidth, float windowHeight) {
        return MoveSelectedToScreenPos(screenX, screenY, windowWidth, windowHeight);
    }

    bool TacticalSceneManagerSystem::MoveSelectedToScreenPos(float screenX, float screenY, float windowWidth, float windowHeight) {
        if (!HasSelectedEntity() || !scene->HasComponent<PathfindingAgent>(selectedEntity)) {
            return false;
        }

        auto targetWorldPos = RaycastGroundTarget(screenX, screenY, windowWidth, windowHeight);
        if (!targetWorldPos.has_value()) {
            return false;
        }

        return MoveSelectedToWorldPos(*targetWorldPos);
    }

    bool TacticalSceneManagerSystem::MoveSelectedToWorldPos(const glm::vec3& targetWorldPos) {
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
        bool res = pathfindingSystem->RequestPath(selectedEntity, gridPlacement);

        if (auto tacSys = scene->GetSystem<TacticalGridSystem>()) {
            tacSys->ClearHover();
            tacSys->RefreshReachableRange();
        }

        return res;
    }

    void TacticalSceneManagerSystem::Update(float deltaTime) {
        if (!scene) return;

        if (enableTacticalCamera) {
            UpdateCamera(deltaTime);
        }

        if (selectedEntity != MAX_ENTITIES) {
            if (!scene->IsEntityActive(selectedEntity) || (requirePathfindingAgent && !scene->HasComponent<PathfindingAgent>(selectedEntity))) {
                ClearSelection();
                return;
            }

            if (scene->HasComponent<TransformComponent>(selectedEntity)) {
                auto& trans = scene->GetComponent<TransformComponent>(selectedEntity);
                auto gizmoSystem = scene->GetSystem<GizmoSystem>();
                if (gizmoSystem) {
                    float sz = 1.0f;
                    if (scene->HasComponent<PathfindingAgent>(selectedEntity)) {
                        sz = static_cast<float>(scene->GetComponent<PathfindingAgent>(selectedEntity).agentSize);
                    }

                    // Selection box indicator
                    if (showSelectionGizmo) {
                        gizmoSystem->DrawCube(trans.position, glm::vec3(sz * 1.1f, 0.1f, sz * 1.1f), glm::vec3(0.1f, 0.9f, 0.1f), 0.0f);
                    }

                    // Health bar indicator above entity
                    if (showHealthGizmo && scene->HasComponent<PawnComponent>(selectedEntity)) {
                        const auto& pawn = scene->GetComponent<PawnComponent>(selectedEntity);
                        float pct = pawn.maxHealth > 0.0f ? std::clamp(pawn.currentHealth / pawn.maxHealth, 0.0f, 1.0f) : 0.0f;
                        float barWidth = std::max(0.6f, sz * 0.8f);
                        float barHeight = 0.08f;
                        float barDepth = 0.08f;
                        glm::vec3 barCenter = trans.position + glm::vec3(0.0f, sz * 1.4f + 0.3f, 0.0f);

                        // Background bar (dark background)
                        gizmoSystem->DrawCube(barCenter, glm::vec3(barWidth, barHeight, barDepth), glm::vec3(0.2f, 0.2f, 0.2f), 0.0f);

                        // Foreground bar (health color)
                        if (pct > 0.0f) {
                            float fgWidth = barWidth * pct;
                            float xOffset = (fgWidth - barWidth) * 0.5f;
                            glm::vec3 fgCenter = barCenter + glm::vec3(xOffset, 0.0f, 0.01f);
                            glm::vec3 healthColor = glm::vec3(1.0f - pct, pct, 0.0f);
                            gizmoSystem->DrawCube(fgCenter, glm::vec3(fgWidth, barHeight * 0.9f, barDepth * 1.1f), healthColor, 0.0f);
                        }
                    }
                }
            }
        }

        UpdatePawnHUD();
    }

    void TacticalSceneManagerSystem::OnStart() {
        auto hudFontUuidOpt = scene->engine.assetManagerInterface->getAssetUuid("Geist-Black.font");
        if (hudFontUuidOpt.has_value()) {
            hudFontUuid = hudFontUuidOpt.value();
        }
        CreateSelectedPawnHUDEntities();
        targetCameraTarget = cameraTarget;
        targetCameraDistance = cameraDistance;
        targetCameraYaw = cameraYaw;
    }

    void TacticalSceneManagerSystem::PanCamera(const glm::vec2& movementInput, float deltaTime, bool fast) {
        if (glm::length(movementInput) < 1e-4f) return;
        glm::vec2 normInput = glm::normalize(movementInput);
        float speed = panSpeed * (fast ? fastPanMultiplier : 1.0f);
        float yawRad = glm::radians(cameraYaw);
        glm::vec3 forward = glm::vec3(-std::sin(yawRad), 0.0f, -std::cos(yawRad));
        glm::vec3 right = glm::vec3(std::cos(yawRad), 0.0f, -std::sin(yawRad));

        glm::vec3 delta = (forward * normInput.y + right * normInput.x) * speed * deltaTime;
        targetCameraTarget += delta;
        if (!enableSmoothing) {
            cameraTarget = targetCameraTarget;
        }
    }

    void TacticalSceneManagerSystem::PanCameraByPixels(float deltaX, float deltaY) {
        if (std::abs(deltaX) < 1e-4f && std::abs(deltaY) < 1e-4f) return;
        float yawRad = glm::radians(cameraYaw);
        glm::vec3 forward = glm::vec3(-std::sin(yawRad), 0.0f, -std::cos(yawRad));
        glm::vec3 right = glm::vec3(std::cos(yawRad), 0.0f, -std::sin(yawRad));

        float factor = 0.0015f * cameraDistance;
        glm::vec3 delta = (-right * deltaX + forward * deltaY) * factor;
        targetCameraTarget += delta;
        if (!enableSmoothing) {
            cameraTarget = targetCameraTarget;
        }
    }

    void TacticalSceneManagerSystem::RotateCameraLeft(float stepDegrees) {
        targetCameraYaw -= stepDegrees;
        if (!enableSmoothing) {
            cameraYaw = targetCameraYaw;
        }
    }

    void TacticalSceneManagerSystem::RotateCameraRight(float stepDegrees) {
        targetCameraYaw += stepDegrees;
        if (!enableSmoothing) {
            cameraYaw = targetCameraYaw;
        }
    }

    void TacticalSceneManagerSystem::RotateCameraYaw(float deltaYaw) {
        targetCameraYaw += deltaYaw;
        if (!enableSmoothing) {
            cameraYaw = targetCameraYaw;
        }
    }

    void TacticalSceneManagerSystem::ZoomCamera(float zoomDelta) {
        targetCameraDistance = std::clamp(targetCameraDistance + zoomDelta, minCameraDistance, maxCameraDistance);
        if (!enableSmoothing) {
            cameraDistance = targetCameraDistance;
        }
    }

    void TacticalSceneManagerSystem::ZoomIn(float amount) {
        ZoomCamera(-amount);
    }

    void TacticalSceneManagerSystem::ZoomOut(float amount) {
        ZoomCamera(amount);
    }

    void TacticalSceneManagerSystem::SetCameraPitch(float pitch) {
        cameraPitch = std::clamp(pitch, minPitch, maxPitch);
    }

    void TacticalSceneManagerSystem::FocusOnPosition(const glm::vec3& targetPos, bool immediate) {
        targetCameraTarget = targetPos;
        if (immediate || !enableSmoothing) {
            cameraTarget = targetPos;
        }
    }

    void TacticalSceneManagerSystem::FocusOnEntity(Entity entity, bool immediate) {
        if (!scene || entity == MAX_ENTITIES || !scene->IsEntityActive(entity)) return;
        if (scene->HasComponent<TransformComponent>(entity)) {
            FocusOnPosition(scene->GetComponent<TransformComponent>(entity).position, immediate);
        }
    }

    void TacticalSceneManagerSystem::FocusOnSelected(bool immediate) {
        if (HasSelectedEntity()) {
            FocusOnEntity(selectedEntity, immediate);
        }
    }

    void TacticalSceneManagerSystem::SetCameraTarget(const glm::vec3& target, bool immediate) {
        targetCameraTarget = target;
        if (immediate || !enableSmoothing) {
            cameraTarget = target;
        }
    }

    void TacticalSceneManagerSystem::SetCameraDistance(float distance, bool immediate) {
        targetCameraDistance = std::clamp(distance, minCameraDistance, maxCameraDistance);
        if (immediate || !enableSmoothing) {
            cameraDistance = targetCameraDistance;
        }
    }

    void TacticalSceneManagerSystem::SetCameraYaw(float yaw, bool immediate) {
        targetCameraYaw = yaw;
        if (immediate || !enableSmoothing) {
            cameraYaw = yaw;
        }
    }

    Entity TacticalSceneManagerSystem::GetActiveGameCameraEntity() const {
        if (!scene) return MAX_ENTITIES;
        for (auto [cameraEntity, camera, cameraTransform] : scene->GetActiveView<CameraComponent, TransformComponent>()) {
            if (camera.active) {
                return cameraEntity;
            }
        }
        for (auto [cameraEntity, camera, cameraTransform] : scene->GetActiveView<CameraComponent, TransformComponent>()) {
            return cameraEntity;
        }
        return MAX_ENTITIES;
    }

    void TacticalSceneManagerSystem::ApplyTacticalCameraToActiveCamera() {
        if (!scene) return;
        Entity camEnt = GetActiveGameCameraEntity();
        if (camEnt == MAX_ENTITIES || !scene->HasComponent<TransformComponent>(camEnt)) {
            return;
        }

        auto& trans = scene->GetComponent<TransformComponent>(camEnt);
        float yawRad = glm::radians(cameraYaw);
        float pitchRad = glm::radians(cameraPitch);

        trans.position.x = cameraTarget.x + cameraDistance * std::cos(pitchRad) * std::sin(yawRad);
        trans.position.y = cameraTarget.y + cameraDistance * std::sin(pitchRad);
        trans.position.z = cameraTarget.z + cameraDistance * std::cos(pitchRad) * std::cos(yawRad);

        glm::vec3 lookDir = glm::normalize(cameraTarget - trans.position);
        trans.rotation = glm::quatLookAt(lookDir, glm::vec3(0.0f, 1.0f, 0.0f));
        trans.isDirty = true;
        computeLocalMatrix(trans);
        trans.globalMatrix = trans.localMatrix;

        if (scene->HasComponent<CameraComponent>(camEnt)) {
            auto& cam = scene->GetComponent<CameraComponent>(camEnt);
            updateViewMatrix(cam, trans.globalMatrix);
        }
    }

    void TacticalSceneManagerSystem::UpdateCamera(float deltaTime) {
        if (!enableTacticalCamera || !scene) return;

        Entity camEnt = GetActiveGameCameraEntity();
        if (camEnt != MAX_ENTITIES && scene->HasComponent<TransformComponent>(camEnt)) {
            if (!cameraInitialized) {
                auto& trans = scene->GetComponent<TransformComponent>(camEnt);
                if (glm::length(cameraTarget) == 0.0f && (trans.position.x != 0.0f || trans.position.y != 0.0f || trans.position.z != 0.0f)) {
                    glm::vec3 fwd = getForward(trans);
                    if (std::abs(fwd.y) > 0.01f && trans.position.y > 0.0f) {
                        float distToGround = trans.position.y / (-fwd.y);
                        if (distToGround > 0.0f) {
                            cameraTarget = trans.position + fwd * distToGround;
                            cameraDistance = std::clamp(glm::length(trans.position - cameraTarget), minCameraDistance, maxCameraDistance);
                            targetCameraTarget = cameraTarget;
                            targetCameraDistance = cameraDistance;
                        }
                    }
                }
                cameraInitialized = true;
            }
        }

        if (enableSmoothing && deltaTime > 0.0f) {
            float t = std::clamp(smoothingSpeed * deltaTime, 0.0f, 1.0f);
            cameraTarget = glm::mix(cameraTarget, targetCameraTarget, t);
            cameraDistance = glm::mix(cameraDistance, targetCameraDistance, t);

            float diffYaw = targetCameraYaw - cameraYaw;
            while (diffYaw > 180.0f) diffYaw -= 360.0f;
            while (diffYaw < -180.0f) diffYaw += 360.0f;
            cameraYaw += diffYaw * t;
        } else {
            cameraTarget = targetCameraTarget;
            cameraDistance = targetCameraDistance;
            cameraYaw = targetCameraYaw;
        }

        cameraPitch = std::clamp(cameraPitch, minPitch, maxPitch);
        cameraDistance = std::clamp(cameraDistance, minCameraDistance, maxCameraDistance);

        while (cameraYaw >= 360.0f) { cameraYaw -= 360.0f; targetCameraYaw -= 360.0f; }
        while (cameraYaw < 0.0f) { cameraYaw += 360.0f; targetCameraYaw += 360.0f; }

        ApplyTacticalCameraToActiveCamera();
    }

    bool TacticalSceneManagerSystem::HandleGameWindowInput(float screenX, float screenY, float windowWidth, float windowHeight, float deltaTime, bool isHovered, bool isFocused) {
        if (!enableTacticalCamera || !scene) return false;

#ifdef ENABLE_IMGUI
        ImGuiIO& io = ImGui::GetIO();
        bool fast = io.KeyShift;

        // 1. Keyboard Panning, Rotation, Zoom, Focus (when window is focused or hovered, and not typing text)
        if ((isHovered || isFocused) && !io.WantTextInput) {
            glm::vec2 moveDir{0.0f};
            if (ImGui::IsKeyDown(ImGuiKey_W) || ImGui::IsKeyDown(ImGuiKey_UpArrow)) moveDir.y += 1.0f;
            if (ImGui::IsKeyDown(ImGuiKey_S) || ImGui::IsKeyDown(ImGuiKey_DownArrow)) moveDir.y -= 1.0f;
            if (ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_LeftArrow)) moveDir.x -= 1.0f;
            if (ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow)) moveDir.x += 1.0f;

            if (glm::length(moveDir) > 0.0f) {
                PanCamera(moveDir, deltaTime, fast);
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Q)) {
                RotateCameraLeft(rotationStepAngle);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_E)) {
                RotateCameraRight(rotationStepAngle);
            }

            if (ImGui::IsKeyPressed(ImGuiKey_T) || ImGui::IsKeyPressed(ImGuiKey_PageUp)) {
                ZoomIn(zoomSpeed);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_G) || ImGui::IsKeyPressed(ImGuiKey_PageDown)) {
                ZoomOut(zoomSpeed);
            }

            if (ImGui::IsKeyPressed(ImGuiKey_F) || ImGui::IsKeyPressed(ImGuiKey_Home)) {
                FocusOnSelected();
            }
        }

        // 2. Screen Edge Panning (when mouse is hovered near viewport edge)
        if (enableEdgePanning && isHovered && windowWidth > 0.0f && windowHeight > 0.0f) {
            if (screenX >= 0.0f && screenX <= windowWidth && screenY >= 0.0f && screenY <= windowHeight) {
                glm::vec2 edgeDir{0.0f};
                if (screenX < edgePanMargin) edgeDir.x -= 1.0f;
                if (screenX > windowWidth - edgePanMargin) edgeDir.x += 1.0f;
                if (screenY < edgePanMargin) edgeDir.y += 1.0f;
                if (screenY > windowHeight - edgePanMargin) edgeDir.y -= 1.0f;

                if (glm::length(edgeDir) > 0.0f) {
                    PanCamera(edgeDir, deltaTime, fast);
                }
            }
        }

        // 3. Mouse Wheel Zooming
        if (isHovered && std::abs(io.MouseWheel) > 0.001f) {
            ZoomCamera(-io.MouseWheel * zoomSpeed * (fast ? fastPanMultiplier : 1.0f));
        }

        // 4. Middle Mouse Button Panning Drag
        if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
            isMiddleMouseDragging = true;
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            isMiddleMouseDragging = false;
        }
        if (isMiddleMouseDragging && (std::abs(io.MouseDelta.x) > 0.0f || std::abs(io.MouseDelta.y) > 0.0f)) {
            PanCameraByPixels(io.MouseDelta.x, io.MouseDelta.y);
        }
#endif

        return true;
    }

#ifdef ENABLE_IMGUI
    bool TacticalSceneManagerSystem::CustomDrawSettingsImGui(Scene* scenePtr) {
        bool changed = DrawComponentFields(*this, scenePtr);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (hudHealthBackgroundEntity != MAX_ENTITIES || hudHealthFillEntity != MAX_ENTITIES || hudNameTextEntity != MAX_ENTITIES) {
            ImGui::BulletText("HUD Health Panel: Bound");
            if (ImGui::Button("Destroy HUD Panel Entities")) {
                DestroySelectedPawnHUDEntities();
            }
        } else {
            if (ImGui::Button("Create Selected Pawn HUD Panel")) {
                CreateSelectedPawnHUDEntities();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (HasSelectedEntity()) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "Selected Tactical Agent");
            ImGui::BulletText("Entity ID: %llu", static_cast<unsigned long long>(selectedEntity));

            std::string agentName = GetSelectedEntityName();
            ImGui::BulletText("Name: %s", agentName.empty() ? "(Unnamed)" : agentName.c_str());

            if (scenePtr && scenePtr->HasComponent<PawnComponent>(selectedEntity)) {
                const auto& pawn = scenePtr->GetComponent<PawnComponent>(selectedEntity);
                float pct = pawn.maxHealth > 0.0f ? std::clamp(pawn.currentHealth / pawn.maxHealth, 0.0f, 1.0f) : 0.0f;
                char overlay[64];
                snprintf(overlay, sizeof(overlay), "%.0f / %.0f HP (%.0f%%)", pawn.currentHealth, pawn.maxHealth, pct * 100.0f);
                ImVec4 hpColor = pct > 0.5f ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) : (pct > 0.25f ? ImVec4(0.9f, 0.8f, 0.1f, 1.0f) : ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, hpColor);
                ImGui::ProgressBar(pct, ImVec2(-1.0f, 0.0f), overlay);
                ImGui::PopStyleColor();
            } else {
                ImGui::TextDisabled("No PawnComponent attached.");
            }

            if (scenePtr && scenePtr->HasComponent<PathfindingAgent>(selectedEntity)) {
                const auto& agent = scenePtr->GetComponent<PathfindingAgent>(selectedEntity);
                ImGui::BulletText("Size: %d | Speed: %.1f | Allowance: %.1f", agent.agentSize, agent.speed, agent.movementAllowance);
                ImGui::BulletText("Status: %s", agent.isMoving ? "Moving" : (agent.hasTarget ? "Pathing" : "Idle"));
            }

            if (ImGui::Button("Focus Camera On Selected (F)")) {
                FocusOnSelected();
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear Selection")) {
                ClearSelection();
            }
        } else {
            ImGui::TextDisabled("No agent currently selected.");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "Tactical Camera Quick Actions");
        if (ImGui::Button("Rotate Left (Q)")) {
            RotateCameraLeft(rotationStepAngle);
        }
        ImGui::SameLine();
        if (ImGui::Button("Rotate Right (E)")) {
            RotateCameraRight(rotationStepAngle);
        }
        ImGui::SameLine();
        if (ImGui::Button("Zoom In (T)")) {
            ZoomIn(zoomSpeed);
        }
        ImGui::SameLine();
        if (ImGui::Button("Zoom Out (G)")) {
            ZoomOut(zoomSpeed);
        }

        if (ImGui::Button("Reset Camera Target to Origin")) {
            SetCameraTarget(glm::vec3(0.0f), true);
        }

        return changed;
    }
#endif

} // namespace engine::ecs
