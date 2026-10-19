#include "TacticalGridSystem.hpp"
#include <algorithm>
#include <limits>
#include <unordered_set>
#include <Config.hpp>
#include "ecs/Scene.h"
#include "systems/transformSystem/componets/TransformComponent.hpp"
#include "systems/selectionSystem/SelectionSystem.hpp"
#include "AssetTypes.hpp"
#include "assetDatas/ModelData.h"
#include "../../../assetManager/src/assets/meshAsset/MeshAsset.h"

namespace engine::ecs {

    TacticalGridSystem::TacticalGridSystem(Scene* scene) : System(scene) {
        Initialize();
    }

    void TacticalGridSystem::Initialize() {
        if (!scene || !scene->engine.assetManagerInterface) {
            return;
        }

        auto planeUuidOpt = scene->engine.assetManagerInterface->getAssetUuid("internalTacticalPlaneModel");
        if (!planeUuidOpt.has_value())
        {
            auto createRes = scene->engine.assetManagerInterface->createAsset(am::AssetType::Model, std::string(RESOURCES_DIR) + ".internal\\internalTacticalPlaneModel", "internalTacticalPlaneModel");
            if (createRes.has_value()) {
                planeAssetUuid = createRes.value();
                auto planeData = scene->engine.assetManagerInterface->getAssetData<am::ModelData>(planeAssetUuid);
                if (planeData && !planeData->rootNode.mChildren.empty() && !planeData->rootNode.mChildren[0].meshes.empty()) {
                    planeMeshUuid = planeData->rootNode.mChildren[0].meshes[0].get()->id;
                    auto meshData = planeData->rootNode.mChildren[0].meshes[0].get()->getAsset()->getAssetDataAs<am::MeshData>();
                    if (meshData) {
                        am::VertexAsset v0{}, v1{}, v2{}, v3{};
                        v0.Position = glm::vec3(-1.0f, 0.0f, -1.0f); v0.Normal = glm::vec3(0, 1, 0); v0.TexCoords = glm::vec2(0.0f, 0.0f); v0.Color = glm::vec4(1.0f);
                        v1.Position = glm::vec3( 1.0f, 0.0f, -1.0f); v1.Normal = glm::vec3(0, 1, 0); v1.TexCoords = glm::vec2(1.0f, 0.0f); v1.Color = glm::vec4(1.0f);
                        v2.Position = glm::vec3( 1.0f, 0.0f,  1.0f); v2.Normal = glm::vec3(0, 1, 0); v2.TexCoords = glm::vec2(1.0f, 1.0f); v2.Color = glm::vec4(1.0f);
                        v3.Position = glm::vec3(-1.0f, 0.0f,  1.0f); v3.Normal = glm::vec3(0, 1, 0); v3.TexCoords = glm::vec2(0.0f, 1.0f); v3.Color = glm::vec4(1.0f);

                        meshData->vertices = {v0, v1, v2, v3};
                        meshData->indices = {0, 1, 2, 0, 2, 3};
                        meshData->boundingBoxMin = glm::vec3(-1.0f, 0.0f, -1.0f);
                        meshData->boundingBoxMax = glm::vec3( 1.0f, 0.0f,  1.0f);
                        planeData->boundingBoxMin = glm::vec3(-1.0f, 0.0f, -1.0f);
                        planeData->boundingBoxMax = glm::vec3( 1.0f, 0.0f,  1.0f);
                        scene->engine.assetManagerInterface->saveAsset(planeMeshUuid);
                    }
                }
            }
        }
        else
        {
            planeAssetUuid = planeUuidOpt.value();
            auto planeData = scene->engine.assetManagerInterface->getAssetData<am::ModelData>(planeAssetUuid);
            if (planeData && !planeData->rootNode.mChildren.empty() && !planeData->rootNode.mChildren[0].meshes.empty()) {
                planeMeshUuid = planeData->rootNode.mChildren[0].meshes[0].get()->id;
            }
        }

        auto shaderOpt = scene->engine.assetManagerInterface->getAssetUuid("gridOverlayShader");
        if (shaderOpt.has_value()) {
            gridShaderUuid = shaderOpt.value();
        }
    }

    gfx::MeshHandle TacticalGridSystem::GetDecalMeshHandle() {
        if (!planeMeshHandle.isValid() && scene && scene->engine.graphicsEngine) {
            if (planeMeshUuid == boost::uuids::nil_uuid()) {
                Initialize();
            }
            if (planeMeshUuid != boost::uuids::nil_uuid()) {
                planeMeshHandle = scene->engine.graphicsEngine->loadMesh(planeMeshUuid);
            }
        }
        return planeMeshHandle;
    }

    gfx::ShaderProgramHandle TacticalGridSystem::GetGridShaderHandle() {
        if (!gridShaderHandle.isValid() && scene && scene->engine.graphicsEngine) {
            if (gridShaderUuid == boost::uuids::nil_uuid()) {
                Initialize();
            }
            if (gridShaderUuid != boost::uuids::nil_uuid()) {
                gridShaderHandle = scene->engine.graphicsEngine->loadShader(gridShaderUuid);
            }
        }
        return gridShaderHandle;
    }

    void TacticalGridSystem::SetSelectedEntity(Entity entity) {
        if (entity != MAX_ENTITIES && scene && scene->IsEntityActive(entity)) {
            if (scene->HasComponent<PathfindingAgent>(entity)) {
                activeSelectedEntity = entity;
                isOverlayActive = true;
                RefreshReachableRange();
                return;
            }
        }
        ClearSelectedEntity();
    }

    void TacticalGridSystem::ClearSelectedEntity() {
        activeSelectedEntity = MAX_ENTITIES;
        isOverlayActive = false;
        reachableCells.clear();
        reachableCostMap.clear();
        reachableCellTransforms.clear();
        ClearHover();
    }

    void TacticalGridSystem::OnEntityRemoved(ComponentID componentID, std::type_index type) {
        if (activeSelectedEntity != MAX_ENTITIES && scene && !scene->IsEntityActive(activeSelectedEntity)) {
            ClearSelectedEntity();
        }
    }

    void TacticalGridSystem::RefreshReachableRange() {
        if (activeSelectedEntity == MAX_ENTITIES || !scene || !scene->IsEntityActive(activeSelectedEntity)) {
            ClearSelectedEntity();
            return;
        }

        if (!scene->HasComponent<PathfindingAgent>(activeSelectedEntity)) {
            ClearSelectedEntity();
            return;
        }

        auto& agent = scene->GetComponent<PathfindingAgent>(activeSelectedEntity);
        selectedAgentSize = std::max(1, agent.agentSize);
        selectedMovementAllowance = agent.movementAllowance;

        glm::vec3 agentPos{0.0f};
        if (scene->HasComponent<TransformComponent>(activeSelectedEntity)) {
            agentPos = scene->GetComponent<TransformComponent>(activeSelectedEntity).position;
        }
        groundHeight = agentPos.y;

        auto pathSys = scene->GetSystem<PathfindingSystem>();
        if (!pathSys) {
            return;
        }

        selectedEntityGridPos = pathSys->WorldToGrid(agentPos, selectedAgentSize);
        reachableCells = pathSys->CalculateReachableCells(selectedEntityGridPos, selectedAgentSize, selectedMovementAllowance, activeSelectedEntity);

        reachableCostMap.clear();
        for (const auto& r : reachableCells) {
            reachableCostMap[r.gridPos] = r.traversalCost;
        }

        UpdateDecalTransform();
        UpdatePreviewPath();
    }

    bool TacticalGridSystem::IsCellReachable(const glm::ivec2& gridPos) const {
        return reachableCostMap.find(gridPos) != reachableCostMap.end();
    }

    float TacticalGridSystem::GetCellTraversalCost(const glm::ivec2& gridPos) const {
        auto it = reachableCostMap.find(gridPos);
        if (it != reachableCostMap.end()) {
            return it->second;
        }
        return -1.0f;
    }

    void TacticalGridSystem::SetHoverPosition(const glm::vec3& worldPos) {
        if (!scene) return;
        auto pathSys = scene->GetSystem<PathfindingSystem>();
        if (!pathSys) return;

        glm::ivec2 grid = pathSys->WorldToGrid(worldPos, selectedAgentSize);
        SetHoverGrid(grid);
    }

    void TacticalGridSystem::SetHoverGrid(const glm::ivec2& gridPos) {
        hoverGridPos = gridPos;
        isHoverValid = true;
        UpdatePreviewPath();
    }

    void TacticalGridSystem::ClearHover() {
        isHoverValid = false;
        previewPath.clear();
        previewPathGrid.clear();
        previewPathCost = 0.0f;
    }

    void TacticalGridSystem::UpdatePreviewPath() {
        if (!isOverlayActive || !isHoverValid || activeSelectedEntity == MAX_ENTITIES || !scene) {
            previewPath.clear();
            previewPathGrid.clear();
            previewPathCost = 0.0f;
            return;
        }

        auto pathSys = scene->GetSystem<PathfindingSystem>();
        if (!pathSys) {
            return;
        }

        previewPathGrid = pathSys->FindPathGrid(selectedEntityGridPos, hoverGridPos, selectedAgentSize, activeSelectedEntity, selectedMovementAllowance);
        previewPathCost = pathSys->GetPathCost(previewPathGrid, selectedAgentSize);

        if (scene->HasComponent<TransformComponent>(activeSelectedEntity)) {
            const auto& trans = scene->GetComponent<TransformComponent>(activeSelectedEntity);
            glm::vec3 hoverWorld = pathSys->GridToWorld(hoverGridPos, groundHeight, selectedAgentSize);
            previewPath = pathSys->FindPath(trans.position, hoverWorld, selectedAgentSize, groundHeight, activeSelectedEntity, selectedMovementAllowance);
        }
    }

    float TacticalGridSystem::GetCellSize() const {
        if (scene) {
            if (auto pathSys = scene->GetSystem<PathfindingSystem>()) {
                return pathSys->GetCellSize();
            }
        }
        return 1.0f;
    }

    glm::vec2 TacticalGridSystem::GetGridDimensions() const {
        return decalSize;
    }

    glm::vec3 TacticalGridSystem::GetGridOriginWorld() const {
        return decalCenter;
    }

    TacticalGridComponent TacticalGridSystem::GetActiveVisualSettings() const {
        if (scene) {
            for (auto [entity, comp] : scene->GetActiveView<TacticalGridComponent>()) {
                return comp;
            }
        }
        return TacticalGridComponent{};
    }

    void TacticalGridSystem::UpdateDecalTransform() {
        if (!scene) return;
        auto pathSys = scene->GetSystem<PathfindingSystem>();
        if (!pathSys) return;

        float cellSz = pathSys->GetCellSize();
        if (cellSz <= 0.001f) cellSz = 1.0f;

        reachableCellTransforms.clear();
        TacticalGridComponent vis = GetActiveVisualSettings();
        float y = groundHeight + vis.heightOffset;

        std::unordered_set<glm::ivec2, GridPosHash> uniqueCellSet;
        std::vector<glm::ivec2> uniqueCells;
        for (const auto& r : reachableCells) {
            for (int dy = 0; dy < selectedAgentSize; ++dy) {
                for (int dx = 0; dx < selectedAgentSize; ++dx) {
                    glm::ivec2 cellPos(r.gridPos.x + dx, r.gridPos.y + dy);
                    if (uniqueCellSet.insert(cellPos).second) {
                        uniqueCells.push_back(cellPos);
                    }
                }
            }
        }

        for (const auto& cellPos : uniqueCells) {
            float cellCenterX = (static_cast<float>(cellPos.x) + 0.5f) * cellSz;
            float cellCenterZ = (static_cast<float>(cellPos.y) + 0.5f) * cellSz;
            glm::mat4 cellMat = glm::translate(glm::mat4(1.0f), glm::vec3(cellCenterX, y, cellCenterZ));
            cellMat = glm::scale(cellMat, glm::vec3(cellSz * 0.5f, 1.0f, cellSz * 0.5f));
            reachableCellTransforms.push_back(cellMat);
        }

        int minX = 0, minY = 0, maxX = 0, maxY = 0;
        if (!reachableCells.empty()) {
            minX = std::numeric_limits<int>::max();
            minY = std::numeric_limits<int>::max();
            maxX = std::numeric_limits<int>::lowest();
            maxY = std::numeric_limits<int>::lowest();

            for (const auto& r : reachableCells) {
                minX = std::min(minX, r.gridPos.x);
                minY = std::min(minY, r.gridPos.y);
                maxX = std::max(maxX, r.gridPos.x + selectedAgentSize);
                maxY = std::max(maxY, r.gridPos.y + selectedAgentSize);
            }
            // Add 1 cell padding for visual margin
            minX -= 1;
            minY -= 1;
            maxX += 1;
            maxY += 1;
        } else {
            minX = pathSys->GetGridMin().x;
            minY = pathSys->GetGridMin().y;
            maxX = pathSys->GetGridMax().x;
            maxY = pathSys->GetGridMax().y;
        }

        float totalW = static_cast<float>(std::max(1, maxX - minX)) * cellSz;
        float totalH = static_cast<float>(std::max(1, maxY - minY)) * cellSz;
        float centerX = (static_cast<float>(minX) + static_cast<float>(maxX)) * 0.5f * cellSz;
        float centerZ = (static_cast<float>(minY) + static_cast<float>(maxY)) * 0.5f * cellSz;

        decalCenter = glm::vec3(centerX, y, centerZ);
        decalSize = glm::vec2(totalW, totalH);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, decalCenter);
        model = glm::scale(model, glm::vec3(totalW * 0.5f, 1.0f, totalH * 0.5f));
        decalModelMatrix = model;
    }

    void TacticalGridSystem::Update(float deltaTime) {
        if (!scene) return;

        animationTime += deltaTime;

        // Sync with selection system
        if (auto selSys = scene->GetSystem<SelectionSystem>()) {
            Entity curSel = selSys->GetSelectedEntity();
            if (curSel != activeSelectedEntity) {
                SetSelectedEntity(curSel);
            }
        }

        if (activeSelectedEntity != MAX_ENTITIES && scene->IsEntityActive(activeSelectedEntity)) {
            if (scene->HasComponent<TransformComponent>(activeSelectedEntity) && scene->HasComponent<PathfindingAgent>(activeSelectedEntity)) {
                const auto& trans = scene->GetComponent<TransformComponent>(activeSelectedEntity);
                const auto& agent = scene->GetComponent<PathfindingAgent>(activeSelectedEntity);
                auto pathSys = scene->GetSystem<PathfindingSystem>();
                if (pathSys) {
                    glm::ivec2 curGrid = pathSys->WorldToGrid(trans.position, agent.agentSize);
                    if (curGrid != selectedEntityGridPos ||
                        agent.agentSize != selectedAgentSize ||
                        std::abs(agent.movementAllowance - selectedMovementAllowance) > 1e-4f ||
                        pathSys->IsGridDirty()) {
                        RefreshReachableRange();
                    }
                }
            } else {
                ClearSelectedEntity();
            }
        } else if (isOverlayActive) {
            ClearSelectedEntity();
        }
    }

} // namespace engine::ecs
