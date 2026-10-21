#ifndef REASONABLEVULKAN_TACTICALSCENEMANAGERSYSTEM_HPP
#define REASONABLEVULKAN_TACTICALSCENEMANAGERSYSTEM_HPP

#include <typeindex>
#include <optional>
#include <string>
#include <glm/glm.hpp>

#include "ecs/System.h"
#include "components/PawnComponent.hpp"
#include "ecs/components/NameComponent.hpp"
#include "systems/pathfindingSystem/componets/PathfindingAgent.hpp"
#include "systems/hudSystem/components/HUDElementComponent.hpp"
#include "systems/hudSystem/components/HUDTextComponent.hpp"

namespace engine::ecs {
    struct TransformComponent;
    struct CameraComponent;

    class TacticalSceneManagerSystem : public System<TacticalSceneManagerSystem, PathfindingAgent> {
    public:
        explicit TacticalSceneManagerSystem(Scene* scene);

        void Update(float deltaTime) override;
        void OnStart() override;
        // Selection queries and controls
        Entity GetSelectedEntity() const { return selectedEntity; }
        void SetSelectedEntity(Entity entity);
        void ClearSelection();
        bool HasSelectedEntity() const;
        bool IsSelected(Entity entity) const { return selectedEntity == entity && entity != MAX_ENTITIES; }

        // Selected entity data queries (health, name, stats)
        std::string GetSelectedEntityName() const;
        std::optional<float> GetSelectedEntityHealth() const;
        std::optional<float> GetSelectedEntityMaxHealth() const;
        std::optional<PawnComponent> GetSelectedPawnComponent() const;
        bool GetSelectedAgentInfo(std::string& outName, float& outCurrentHealth, float& outMaxHealth) const;

        // Click / Screen coordinate interaction (e.g. from game window)
        // Left click to select pathfinding entity at screen coordinates
        bool HandleLeftClick(float screenX, float screenY, float windowWidth, float windowHeight);
        bool SelectAtScreenPos(float screenX, float screenY, float windowWidth, float windowHeight);

        // Right click to send selected entity to move to grid placement at screen coordinates
        bool HandleRightClick(float screenX, float screenY, float windowWidth, float windowHeight);
        bool MoveSelectedToScreenPos(float screenX, float screenY, float windowWidth, float windowHeight);

        // Direct world position command to move selected entity to grid placement
        bool MoveSelectedToWorldPos(const glm::vec3& targetWorldPos);

        // Mouse hover interaction (e.g. from game window or editor viewport)
        bool HandleMouseMove(float screenX, float screenY, float windowWidth, float windowHeight);
        bool HoverAtScreenPos(float screenX, float screenY, float windowWidth, float windowHeight);
        std::optional<glm::vec3> RaycastGroundTarget(float screenX, float screenY, float windowWidth, float windowHeight) const;

        // Tactical Game Camera Movement (XCOM style)
        // Panning: translates target across horizontal ground plane
        void PanCamera(const glm::vec2& movementInput, float deltaTime, bool fast = false);
        void PanCameraByPixels(float deltaX, float deltaY);

        // Rotation: rotates camera around target
        void RotateCameraLeft(float stepDegrees = 45.0f);
        void RotateCameraRight(float stepDegrees = 45.0f);
        void RotateCameraYaw(float deltaYaw);

        // Zoom: distance between camera and target
        void ZoomCamera(float zoomDelta);
        void ZoomIn(float amount = 2.5f);
        void ZoomOut(float amount = 2.5f);

        // Tilt / Pitch
        void SetCameraPitch(float pitch);
        float GetCameraPitch() const { return cameraPitch; }

        // Focus & Center
        void FocusOnPosition(const glm::vec3& targetPos, bool immediate = false);
        void FocusOnEntity(Entity entity, bool immediate = false);
        void FocusOnSelected(bool immediate = false);

        // Getters and Setters
        glm::vec3 GetCameraTarget() const { return cameraTarget; }
        void SetCameraTarget(const glm::vec3& target, bool immediate = false);
        float GetCameraDistance() const { return cameraDistance; }
        void SetCameraDistance(float distance, bool immediate = false);
        float GetCameraYaw() const { return cameraYaw; }
        void SetCameraYaw(float yaw, bool immediate = false);

        // Camera updates and transforms
        void UpdateCamera(float deltaTime);
        void ApplyTacticalCameraToActiveCamera();
        Entity GetActiveGameCameraEntity() const;

        // Full Game Window Input processing (WASD, Q/E, Mouse Drag, Scroll, Edge Pan, Focus)
        bool HandleGameWindowInput(float screenX, float screenY, float windowWidth, float windowHeight, float deltaTime, bool isHovered, bool isFocused);

        // HUD Panel Integration for Selected Pawn
        void CreateSelectedPawnHUDEntities(const glm::vec2& size = glm::vec2(200.0f, 20.0f), const glm::vec2& offset = glm::vec2(-100.0f, -60.0f), UIAnchor anchor = UIAnchor::BottomCenter);
        void DestroySelectedPawnHUDEntities();
        void UpdatePawnHUD();
        void BindHUDEntities(Entity bgEntity, Entity fillEntity, Entity nameEntity = MAX_ENTITIES);

#ifdef ENABLE_IMGUI
        bool CustomDrawSettingsImGui(Scene* scenePtr);
#endif

        // Configuration
        [[=Tooltip{"Whether to show visual gizmo for selected entity"}]]
        bool showSelectionGizmo{true};

        [[=Tooltip{"Whether to show health bar gizmo above selected entity"}]]
        bool showHealthGizmo{true};

        [[=Tooltip{"Whether to show 2D HUD health panel for selected pawn"}]]
        bool showPawnHUD{true};

        [[=Tooltip{"Entity holding the HUD background element"}]]
        Entity hudHealthBackgroundEntity{MAX_ENTITIES};

        [[=Tooltip{"Entity holding the HUD health fill element"}]]
        Entity hudHealthFillEntity{MAX_ENTITIES};

        [[=Tooltip{"Entity holding the HUD name text element"}]]
        Entity hudNameTextEntity{MAX_ENTITIES};

        [[=Tooltip{"Layer depth for HUD panel background"}]]
        int hudBackgroundLayerDepth{10};

        [[=Tooltip{"Layer depth for HUD panel fill"}]]
        int hudFillLayerDepth{11};

        [[=Tooltip{"Layer depth for HUD pawn name text"}]]
        int hudTextLayerDepth{12};

        [[=Tooltip{"Font asset UUID for HUD pawn name text"}, =LookupName{am::AssetType::Font}]]
        boost::uuids::uuid hudFontUuid{boost::uuids::nil_uuid()};

        [[=Tooltip{"Whether selection requires PathfindingAgent component"}]]
        bool requirePathfindingAgent{true};

        [[=Tooltip{"Fallback ground Y coordinate for ray plane intersection"}]]
        float defaultGroundY{0.0f};

        // Tactical Camera Configuration (XCOM Style)
        [[=Tooltip{"Enable tactical camera control on active game camera"}]]
        bool enableTacticalCamera{true};

        [[=Tooltip{"Camera target focus point in world coordinates"}]]
        glm::vec3 cameraTarget{0.0f, 0.0f, 0.0f};

        [[=Range{1.0f, 100.0f, 0.5f}, =Tooltip{"Current camera distance from target"}]]
        float cameraDistance{15.0f};

        [[=Range{10.0f, 89.0f, 1.0f}, =Tooltip{"Camera pitch angle in degrees"}]]
        float cameraPitch{50.0f};

        [[=Range{-360.0f, 360.0f, 1.0f}, =Tooltip{"Camera yaw rotation angle in degrees"}]]
        float cameraYaw{0.0f};

        [[=Range{1.0f, 100.0f, 0.5f}, =Tooltip{"Camera panning speed in units/sec"}]]
        float panSpeed{15.0f};

        [[=Range{1.0f, 5.0f, 0.1f}, =Tooltip{"Multiplier when holding shift for fast pan"}]]
        float fastPanMultiplier{2.0f};

        [[=Range{0.5f, 20.0f, 0.5f}, =Tooltip{"Camera zoom speed per notch"}]]
        float zoomSpeed{2.5f};

        [[=Range{1.0f, 30.0f, 0.5f}, =Tooltip{"Minimum zoom distance"}]]
        float minCameraDistance{3.0f};

        [[=Range{10.0f, 200.0f, 1.0f}, =Tooltip{"Maximum zoom distance"}]]
        float maxCameraDistance{50.0f};

        [[=Range{10.0f, 85.0f, 1.0f}, =Tooltip{"Minimum pitch angle in degrees"}]]
        float minPitch{15.0f};

        [[=Range{10.0f, 89.0f, 1.0f}, =Tooltip{"Maximum pitch angle in degrees"}]]
        float maxPitch{80.0f};

        [[=Range{15.0f, 180.0f, 15.0f}, =Tooltip{"Rotation step angle in degrees for Q/E"}]]
        float rotationStepAngle{45.0f};

        [[=Range{10.0f, 360.0f, 5.0f}, =Tooltip{"Camera rotation speed in deg/s"}]]
        float rotationSpeed{180.0f};

        [[=Tooltip{"Enable screen edge panning"}]]
        bool enableEdgePanning{true};

        [[=Range{1.0f, 50.0f, 1.0f}, =Tooltip{"Screen edge panning trigger margin in pixels"}]]
        float edgePanMargin{15.0f};

        [[=Tooltip{"Enable smooth interpolation for camera movement"}]]
        bool enableSmoothing{true};

        [[=Range{1.0f, 30.0f, 0.5f}, =Tooltip{"Smoothing interpolation factor"}]]
        float smoothingSpeed{10.0f};

        [[=NonSerialized{}]]
        glm::vec3 targetCameraTarget{0.0f, 0.0f, 0.0f};

        [[=NonSerialized{}]]
        float targetCameraDistance{15.0f};

        [[=NonSerialized{}]]
        float targetCameraYaw{0.0f};

        [[=NonSerialized{}]]
        bool isMiddleMouseDragging{false};

        [[=NonSerialized{}]]
        bool cameraInitialized{false};

        [[=NonSerialized{}, =ReadOnly{}]]
        Entity selectedEntity{MAX_ENTITIES};

    protected:
        void OnComponentAdded(ComponentID componentID, std::type_index type) override {}
        void OnEntityRemoved(ComponentID componentID, std::type_index type) override;

    private:
        std::optional<CameraComponent> GetActiveGameCamera(TransformComponent* outTransform = nullptr) const;
    };
} // namespace engine::ecs

#endif // REASONABLEVULKAN_TACTICALSCENEMANAGERSYSTEM_HPP
