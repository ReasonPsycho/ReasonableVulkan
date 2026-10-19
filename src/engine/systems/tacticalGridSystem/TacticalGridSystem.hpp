#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "components/TacticalGridComponent.hpp"
#include "systems/pathfindingSystem/PathfindingSystem.hpp"
#include "systems/pathfindingSystem/componets/PathfindingAgent.hpp"
#include "ecs/System.h"
#include "ecs/Types.h"

namespace engine::ecs {
    class Scene;

    struct GridPosHash {
        std::size_t operator()(const glm::ivec2& pos) const noexcept {
            return std::hash<int>()(pos.x) ^ (std::hash<int>()(pos.y) << 16);
        }
    };

    class TacticalGridSystem : public System<TacticalGridSystem, TacticalGridComponent> {
    public:
        explicit TacticalGridSystem(Scene* scene);

        void Update(float deltaTime) override;

        // Overlay State Control
        bool IsOverlayActive() const { return isOverlayActive; }
        void SetOverlayActive(bool active) { isOverlayActive = active; }

        Entity GetSelectedEntity() const { return activeSelectedEntity; }
        void SetSelectedEntity(Entity entity);
        void ClearSelectedEntity();

        // Reachability Queries & Refresh
        void RefreshReachableRange();
        bool IsCellReachable(const glm::ivec2& gridPos) const;
        float GetCellTraversalCost(const glm::ivec2& gridPos) const;
        const std::vector<ReachableCellInfo>& GetReachableCells() const { return reachableCells; }

        // Hover & Path Preview
        void SetHoverPosition(const glm::vec3& worldPos);
        void SetHoverGrid(const glm::ivec2& gridPos);
        void ClearHover();
        bool IsHoverValid() const { return isHoverValid; }
        glm::ivec2 GetHoverGridPos() const { return hoverGridPos; }

        const std::vector<glm::vec3>& GetPreviewPath() const { return previewPath; }
        const std::vector<glm::ivec2>& GetPreviewPathGrid() const { return previewPathGrid; }
        float GetPreviewPathCost() const { return previewPathCost; }

        // Grid & Decal Geometry Metrics
        float GetCellSize() const;
        glm::vec2 GetGridDimensions() const;
        glm::vec3 GetGridOriginWorld() const;
        glm::mat4 GetDecalModelMatrix() const { return decalModelMatrix; }
        const std::vector<glm::mat4>& GetReachableCellTransforms() const { return reachableCellTransforms; }
        float GetAnimationTime() const { return animationTime; }

        gfx::MeshHandle GetDecalMeshHandle();
        gfx::ShaderProgramHandle GetGridShaderHandle();
        boost::uuids::uuid GetPlaneMeshUuid() const { return planeMeshUuid; }
        boost::uuids::uuid GetPlaneAssetUuid() const { return planeAssetUuid; }

        void Initialize();

        TacticalGridComponent GetActiveVisualSettings() const;

    protected:
        void OnComponentAdded(ComponentID componentID, std::type_index type) override {}
        void OnEntityRemoved(ComponentID componentID, std::type_index type) override;

    private:
        void UpdateDecalTransform();
        void UpdatePreviewPath();

        bool isOverlayActive{false};
        Entity activeSelectedEntity{MAX_ENTITIES};
        glm::ivec2 selectedEntityGridPos{0, 0};
        int selectedAgentSize{1};
        float selectedMovementAllowance{0.0f};

        std::vector<ReachableCellInfo> reachableCells{};
        std::unordered_map<glm::ivec2, float, GridPosHash> reachableCostMap{};

        bool isHoverValid{false};
        glm::ivec2 hoverGridPos{0, 0};
        std::vector<glm::vec3> previewPath{};
        std::vector<glm::ivec2> previewPathGrid{};
        float previewPathCost{0.0f};

        float animationTime{0.0f};
        glm::mat4 decalModelMatrix{1.0f};
        std::vector<glm::mat4> reachableCellTransforms{};
        glm::vec3 decalCenter{0.0f};
        glm::vec2 decalSize{0.0f};
        float groundHeight{0.0f};

        boost::uuids::uuid planeAssetUuid = boost::uuids::nil_uuid();
        boost::uuids::uuid planeMeshUuid = boost::uuids::nil_uuid();
        boost::uuids::uuid gridShaderUuid = boost::uuids::nil_uuid();

        gfx::MeshHandle planeMeshHandle = gfx::MeshHandle::invalid();
        gfx::ShaderProgramHandle gridShaderHandle = gfx::ShaderProgramHandle::invalid();
    };
}
