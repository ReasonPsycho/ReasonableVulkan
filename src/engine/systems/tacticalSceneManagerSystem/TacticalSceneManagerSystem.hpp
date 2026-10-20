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
