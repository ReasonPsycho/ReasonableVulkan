#ifndef REASONABLEVULKAN_SELECTIONSYSTEM_HPP
#define REASONABLEVULKAN_SELECTIONSYSTEM_HPP

#include <typeindex>
#include <optional>
#include <glm/glm.hpp>

#include "ecs/System.h"
#include "systems/pathfindingSystem/componets/PathfindingAgent.hpp"

namespace engine::ecs {
    struct TransformComponent;
    struct CameraComponent;

    class SelectionSystem : public System<SelectionSystem, PathfindingAgent> {
    public:
        explicit SelectionSystem(Scene* scene);

        void Update(float deltaTime) override;

        // Selection queries and controls
        Entity GetSelectedEntity() const { return selectedEntity; }
        void SetSelectedEntity(Entity entity);
        void ClearSelection();
        bool HasSelectedEntity() const;
        bool IsSelected(Entity entity) const { return selectedEntity == entity && entity != MAX_ENTITIES; }

        // Click / Screen coordinate interaction (e.g. from game window)
        // Left click to select pathfinding entity at screen coordinates
        bool HandleLeftClick(float screenX, float screenY, float windowWidth, float windowHeight);
        bool SelectAtScreenPos(float screenX, float screenY, float windowWidth, float windowHeight);

        // Right click to send selected entity to move to grid placement at screen coordinates
        bool HandleRightClick(float screenX, float screenY, float windowWidth, float windowHeight);
        bool MoveSelectedToScreenPos(float screenX, float screenY, float windowWidth, float windowHeight);

        // Direct world position command to move selected entity to grid placement
        bool MoveSelectedToWorldPos(const glm::vec3& targetWorldPos);

        // Configuration
        [[=Tooltip{"Whether to show visual gizmo for selected entity"}]]
        bool showSelectionGizmo{true};

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

#endif // REASONABLEVULKAN_SELECTIONSYSTEM_HPP
