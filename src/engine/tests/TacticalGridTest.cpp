#include <boost/test/unit_test.hpp>
#include "../Engine.h"
#include "ecs/Scene.h"
#include "TestHelper.hpp"
#include "systems/transformSystem/componets/TransformComponent.hpp"
#include "systems/renderingSystem/componets/CameraComponent.hpp"
#include "systems/collisionSystem/CollisionSystem.hpp"
#include "systems/pathfindingSystem/PathfindingSystem.hpp"
#include "systems/pathfindingSystem/componets/PathfindingAgent.hpp"
#include "systems/pathfindingSystem/componets/GridFragment.hpp"
#include "systems/pathfindingSystem/componets/Obstacle.hpp"
#include "systems/pathfindingSystem/componets/Barrier.hpp"
#include "systems/selectionSystem/SelectionSystem.hpp"
#include "systems/tacticalGridSystem/TacticalGridSystem.hpp"
#include "systems/tacticalGridSystem/components/TacticalGridComponent.hpp"

using namespace engine;
using namespace engine::ecs;
using namespace engine::test;

BOOST_AUTO_TEST_SUITE(TacticalGridTestSuite)

BOOST_AUTO_TEST_CASE(TacticalGridSystemRegistrationAndStateLifecycleTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalLifecycleScene");
    BOOST_REQUIRE(scene != nullptr);

    auto tacticalSystem = scene->GetSystem<TacticalGridSystem>();
    BOOST_REQUIRE(tacticalSystem != nullptr);
    BOOST_CHECK(!tacticalSystem->IsOverlayActive());
    BOOST_CHECK_EQUAL(tacticalSystem->GetSelectedEntity(), MAX_ENTITIES);
    BOOST_CHECK(tacticalSystem->GetReachableCells().empty());

    // 1. Setup Grid and Agent
    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(pathfindingSystem != nullptr);
    pathfindingSystem->SetCellSize(1.0f);

    Entity floor = scene->CreateEntity("Floor");
    TransformComponent tFloor;
    tFloor.position = glm::vec3(0.0f, 0.0f, 0.0f);
    scene->AddComponent<TransformComponent>(floor, tFloor);
    GridFragment fFloor(9, 9);
    scene->AddComponent<GridFragment>(floor, fFloor);
    pathfindingSystem->BuildNavGrid();

    Entity agentEntity = scene->CreateEntity("TacticalAgent");
    TransformComponent tAgent;
    tAgent.position = pathfindingSystem->GridToWorld(glm::ivec2(0, 0), 0.0f, 1);
    scene->AddComponent<TransformComponent>(agentEntity, tAgent);
    PathfindingAgent agentComp(1, 5.0f, 3.0f); // Size 1, speed 5, allowance 3.0
    scene->AddComponent<PathfindingAgent>(agentEntity, agentComp);

    // 2. Select Agent directly via TacticalGridSystem
    tacticalSystem->SetSelectedEntity(agentEntity);
    BOOST_CHECK(tacticalSystem->IsOverlayActive());
    BOOST_CHECK_EQUAL(tacticalSystem->GetSelectedEntity(), agentEntity);
    BOOST_CHECK(!tacticalSystem->GetReachableCells().empty());
    BOOST_CHECK(tacticalSystem->IsCellReachable(glm::ivec2(0, 0)));
    BOOST_CHECK_CLOSE(tacticalSystem->GetCellTraversalCost(glm::ivec2(0, 0)), 0.0f, 0.001f);

    // 3. Deselect
    tacticalSystem->ClearSelectedEntity();
    BOOST_CHECK(!tacticalSystem->IsOverlayActive());
    BOOST_CHECK_EQUAL(tacticalSystem->GetSelectedEntity(), MAX_ENTITIES);
    BOOST_CHECK(tacticalSystem->GetReachableCells().empty());

    // 4. Synchronize via SelectionSystem
    auto selectionSystem = scene->GetSystem<SelectionSystem>();
    BOOST_REQUIRE(selectionSystem != nullptr);

    selectionSystem->SetSelectedEntity(agentEntity);
    tacticalSystem->Update(0.016f);
    BOOST_CHECK(tacticalSystem->IsOverlayActive());
    BOOST_CHECK_EQUAL(tacticalSystem->GetSelectedEntity(), agentEntity);

    // Deselect from SelectionSystem
    selectionSystem->ClearSelection();
    tacticalSystem->Update(0.016f);
    BOOST_CHECK(!tacticalSystem->IsOverlayActive());
    BOOST_CHECK_EQUAL(tacticalSystem->GetSelectedEntity(), MAX_ENTITIES);

    engine.RemoveScene("TacticalLifecycleScene");
}

BOOST_AUTO_TEST_CASE(TacticalGridDijkstraReachabilityAccuracyTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalDijkstraScene");
    BOOST_REQUIRE(scene != nullptr);

    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(pathfindingSystem != nullptr);
    pathfindingSystem->SetCellSize(1.0f);

    // Create 11x11 Grid centered at (0, 0, 0)
    Entity floor = scene->CreateEntity("Floor");
    TransformComponent tFloor;
    tFloor.position = glm::vec3(0.0f, 0.0f, 0.0f);
    scene->AddComponent<TransformComponent>(floor, tFloor);
    GridFragment fFloor(11, 11);
    scene->AddComponent<GridFragment>(floor, fFloor);
    pathfindingSystem->BuildNavGrid();

    // 1. Calculate reachable cells with maxCost = 2.0f
    auto reachable2 = pathfindingSystem->CalculateReachableCells(glm::ivec2(0, 0), 1, 2.0f);
    BOOST_REQUIRE(!reachable2.empty());

    for (const auto& cell : reachable2) {
        BOOST_CHECK(cell.traversalCost <= 2.0f + 1e-4f);
        BOOST_CHECK(cell.isReachable);
    }

    // Direct reachability checks for specific coordinates:
    // (0, 0) cost 0.0
    // (1, 0) cost 1.0, (2, 0) cost 2.0, (3, 0) cost 3.0 (beyond range)
    // (1, 1) diagonal cost 1.414, (2, 1) cost 2.414 (beyond range)
    auto tacticalSystem = scene->GetSystem<TacticalGridSystem>();
    BOOST_REQUIRE(tacticalSystem != nullptr);

    Entity agentEntity = scene->CreateEntity("Agent");
    TransformComponent tAgent;
    tAgent.position = pathfindingSystem->GridToWorld(glm::ivec2(0, 0), 0.0f, 1);
    scene->AddComponent<TransformComponent>(agentEntity, tAgent);
    PathfindingAgent agentComp(1, 5.0f, 2.0f);
    scene->AddComponent<PathfindingAgent>(agentEntity, agentComp);

    tacticalSystem->SetSelectedEntity(agentEntity);
    BOOST_CHECK(tacticalSystem->IsCellReachable(glm::ivec2(0, 0)));
    BOOST_CHECK(tacticalSystem->IsCellReachable(glm::ivec2(1, 0)));
    BOOST_CHECK(tacticalSystem->IsCellReachable(glm::ivec2(2, 0)));
    BOOST_CHECK(!tacticalSystem->IsCellReachable(glm::ivec2(3, 0)));
    BOOST_CHECK(tacticalSystem->IsCellReachable(glm::ivec2(1, 1)));
    BOOST_CHECK(!tacticalSystem->IsCellReachable(glm::ivec2(2, 1)));

    BOOST_CHECK_CLOSE(tacticalSystem->GetCellTraversalCost(glm::ivec2(1, 0)), 1.0f, 0.001f);
    BOOST_CHECK_CLOSE(tacticalSystem->GetCellTraversalCost(glm::ivec2(2, 0)), 2.0f, 0.001f);
    BOOST_CHECK_CLOSE(tacticalSystem->GetCellTraversalCost(glm::ivec2(1, 1)), 1.41421356f, 0.001f);

    engine.RemoveScene("TacticalDijkstraScene");
}

BOOST_AUTO_TEST_CASE(TacticalGridReachabilityWithObstaclesAndBarriersTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalBarrierScene");
    BOOST_REQUIRE(scene != nullptr);

    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(pathfindingSystem != nullptr);
    pathfindingSystem->SetCellSize(1.0f);

    // Create 9x9 Grid centered at (0, 0, 0)
    Entity floor = scene->CreateEntity("Floor");
    TransformComponent tFloor;
    tFloor.position = glm::vec3(0.0f, 0.0f, 0.0f);
    scene->AddComponent<TransformComponent>(floor, tFloor);
    GridFragment fFloor(9, 9);
    scene->AddComponent<GridFragment>(floor, fFloor);

    // Create obstacle at (1, 0)
    Entity obsEntity = scene->CreateEntity("Obstacle");
    TransformComponent tObs;
    tObs.position = pathfindingSystem->GridToWorld(glm::ivec2(1, 0), 0.0f, 1);
    scene->AddComponent<TransformComponent>(obsEntity, tObs);
    Obstacle obsComp(1, 1);
    scene->AddComponent<Obstacle>(obsEntity, obsComp);

    // Create vertical barrier across Y in [-4..4] at X=0 (between X=-1 and X=0) with extra cost 4.0
    Entity barEntity = scene->CreateEntity("Barrier");
    TransformComponent tBar;
    tBar.position = glm::vec3(0.0f, 0.0f, 0.0f);
    scene->AddComponent<TransformComponent>(barEntity, tBar);
    Barrier barComp(BarrierOrientation::Vertical, 4.0f, 9);
    scene->AddComponent<Barrier>(barEntity, barComp);

    pathfindingSystem->BuildNavGrid();

    auto tacticalSystem = scene->GetSystem<TacticalGridSystem>();
    BOOST_REQUIRE(tacticalSystem != nullptr);

    // 1. Agent on left side of barrier at (-1, 0) with movement allowance 3.0
    Entity agentEntity = scene->CreateEntity("Agent");
    TransformComponent tAgent;
    tAgent.position = pathfindingSystem->GridToWorld(glm::ivec2(-1, 0), 0.0f, 1);
    scene->AddComponent<TransformComponent>(agentEntity, tAgent);
    PathfindingAgent agentComp(1, 5.0f, 3.0f);
    scene->AddComponent<PathfindingAgent>(agentEntity, agentComp);

    tacticalSystem->SetSelectedEntity(agentEntity);

    // Can move along left side (e.g. (-2, 0), (-1, 1), (-1, -1))
    BOOST_CHECK(tacticalSystem->IsCellReachable(glm::ivec2(-1, 0)));
    BOOST_CHECK(tacticalSystem->IsCellReachable(glm::ivec2(-2, 0)));
    BOOST_CHECK(tacticalSystem->IsCellReachable(glm::ivec2(-1, 1)));

    // Cannot cross barrier to (0, 0) because crossing cost is 1.0 + 4.0 = 5.0 > 3.0
    BOOST_CHECK(!tacticalSystem->IsCellReachable(glm::ivec2(0, 0)));
    BOOST_CHECK(!tacticalSystem->IsCellReachable(glm::ivec2(1, 0))); // Also blocked by obstacle

    // 2. Increase movement allowance to 6.0
    agentComp.SetMovementAllowance(6.0f);
    scene->GetComponent<PathfindingAgent>(agentEntity).SetMovementAllowance(6.0f);
    tacticalSystem->RefreshReachableRange();

    // Now can cross barrier to (0, 0) (cost 5.0)
    BOOST_CHECK(tacticalSystem->IsCellReachable(glm::ivec2(0, 0)));
    BOOST_CHECK_CLOSE(tacticalSystem->GetCellTraversalCost(glm::ivec2(0, 0)), 5.0f, 0.001f);
    // (1, 0) remains unreachable because it has an Obstacle
    BOOST_CHECK(!tacticalSystem->IsCellReachable(glm::ivec2(1, 0)));

    engine.RemoveScene("TacticalBarrierScene");
}

BOOST_AUTO_TEST_CASE(TacticalGridHoverPathPreviewAndTruncationTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalHoverScene");
    BOOST_REQUIRE(scene != nullptr);

    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(pathfindingSystem != nullptr);
    pathfindingSystem->SetCellSize(1.0f);

    Entity floor = scene->CreateEntity("Floor");
    TransformComponent tFloor;
    tFloor.position = glm::vec3(0.0f, 0.0f, 0.0f);
    scene->AddComponent<TransformComponent>(floor, tFloor);
    GridFragment fFloor(15, 15);
    scene->AddComponent<GridFragment>(floor, fFloor);
    pathfindingSystem->BuildNavGrid();

    auto tacticalSystem = scene->GetSystem<TacticalGridSystem>();
    BOOST_REQUIRE(tacticalSystem != nullptr);

    Entity agentEntity = scene->CreateEntity("Agent");
    TransformComponent tAgent;
    tAgent.position = pathfindingSystem->GridToWorld(glm::ivec2(0, 0), 0.0f, 1);
    scene->AddComponent<TransformComponent>(agentEntity, tAgent);
    PathfindingAgent agentComp(1, 5.0f, 4.0f); // Allowance 4.0
    scene->AddComponent<PathfindingAgent>(agentEntity, agentComp);

    tacticalSystem->SetSelectedEntity(agentEntity);
    BOOST_REQUIRE(tacticalSystem->IsOverlayActive());

    // 1. Hover within allowance (target (3, 0), cost 3.0 <= 4.0)
    tacticalSystem->SetHoverGrid(glm::ivec2(3, 0));
    BOOST_CHECK(tacticalSystem->IsHoverValid());
    BOOST_CHECK_EQUAL(tacticalSystem->GetHoverGridPos().x, 3);
    BOOST_CHECK_EQUAL(tacticalSystem->GetHoverGridPos().y, 0);

    const auto& pathWithin = tacticalSystem->GetPreviewPathGrid();
    BOOST_REQUIRE(!pathWithin.empty());
    BOOST_CHECK_EQUAL(pathWithin.size(), 4); // (0,0), (1,0), (2,0), (3,0)
    BOOST_CHECK_EQUAL(pathWithin.back().x, 3);
    BOOST_CHECK_CLOSE(tacticalSystem->GetPreviewPathCost(), 3.0f, 0.001f);

    // 2. Hover beyond allowance (target (6, 0), clamped/truncated to maxCost = 4.0 -> stops at (4, 0))
    tacticalSystem->SetHoverGrid(glm::ivec2(6, 0));
    const auto& pathBeyond = tacticalSystem->GetPreviewPathGrid();
    BOOST_REQUIRE(!pathBeyond.empty());
    BOOST_CHECK_EQUAL(pathBeyond.size(), 5); // (0,0), (1,0), (2,0), (3,0), (4,0)
    BOOST_CHECK_EQUAL(pathBeyond.back().x, 4);
    BOOST_CHECK_CLOSE(tacticalSystem->GetPreviewPathCost(), 4.0f, 0.001f);

    // 3. Clear Hover
    tacticalSystem->ClearHover();
    BOOST_CHECK(!tacticalSystem->IsHoverValid());
    BOOST_CHECK(tacticalSystem->GetPreviewPath().empty());
    BOOST_CHECK(tacticalSystem->GetPreviewPathGrid().empty());
    BOOST_CHECK_CLOSE(tacticalSystem->GetPreviewPathCost(), 0.0f, 0.001f);

    engine.RemoveScene("TacticalHoverScene");
}

BOOST_AUTO_TEST_CASE(TacticalGridMultiSizeAgentFootprintAndDecalMetricsTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalMultiSizeScene");
    BOOST_REQUIRE(scene != nullptr);

    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(pathfindingSystem != nullptr);
    pathfindingSystem->SetCellSize(1.0f);

    Entity floor = scene->CreateEntity("Floor");
    TransformComponent tFloor;
    tFloor.position = glm::vec3(0.0f, 0.0f, 0.0f);
    scene->AddComponent<TransformComponent>(floor, tFloor);
    GridFragment fFloor(10, 10);
    scene->AddComponent<GridFragment>(floor, fFloor);
    pathfindingSystem->BuildNavGrid();

    auto tacticalSystem = scene->GetSystem<TacticalGridSystem>();
    BOOST_REQUIRE(tacticalSystem != nullptr);

    // Agent size 2 at grid (0, 0)
    Entity agent2 = scene->CreateEntity("AgentSize2");
    TransformComponent tAgent2;
    tAgent2.position = pathfindingSystem->GridToWorld(glm::ivec2(0, 0), 0.0f, 2);
    // Footprint is at cross-section (1.0, 0.0, 1.0)
    BOOST_CHECK_CLOSE(tAgent2.position.x, 1.0f, 0.001f);
    BOOST_CHECK_CLOSE(tAgent2.position.z, 1.0f, 0.001f);
    scene->AddComponent<TransformComponent>(agent2, tAgent2);

    PathfindingAgent agentComp(2, 5.0f, 3.0f);
    scene->AddComponent<PathfindingAgent>(agent2, agentComp);

    tacticalSystem->SetSelectedEntity(agent2);
    BOOST_REQUIRE(tacticalSystem->IsOverlayActive());

    // Verify reachable cells calculation for size 2 agent
    const auto& reachable = tacticalSystem->GetReachableCells();
    BOOST_REQUIRE(!reachable.empty());
    for (const auto& r : reachable) {
        BOOST_CHECK(pathfindingSystem->IsWalkable(r.gridPos.x, r.gridPos.y, 2));
    }

    // Verify decal model matrix dimensions and position
    glm::mat4 decalMat = tacticalSystem->GetDecalModelMatrix();
    BOOST_CHECK(decalMat[0][0] > 0.0f); // X scale
    BOOST_CHECK(decalMat[2][2] > 0.0f); // Z scale

    engine.RemoveScene("TacticalMultiSizeScene");
}

BOOST_AUTO_TEST_CASE(TacticalGridComponentCustomizationTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalCustomizationScene");
    BOOST_REQUIRE(scene != nullptr);

    auto tacticalSystem = scene->GetSystem<TacticalGridSystem>();
    BOOST_REQUIRE(tacticalSystem != nullptr);

    // Attach custom TacticalGridComponent
    Entity configEntity = scene->CreateEntity("GridConfig");
    TacticalGridComponent customVis;
    customVis.rangeColor = glm::vec4(0.0f, 1.0f, 0.5f, 0.6f);
    customVis.pathColor = glm::vec4(1.0f, 0.0f, 0.5f, 0.9f);
    customVis.pulseSpeed = 4.0f;
    customVis.heightOffset = 0.05f;
    scene->AddComponent<TacticalGridComponent>(configEntity, customVis);

    TacticalGridComponent retrieved = tacticalSystem->GetActiveVisualSettings();
    BOOST_CHECK_CLOSE(retrieved.rangeColor.r, 0.0f, 0.001f);
    BOOST_CHECK_CLOSE(retrieved.rangeColor.g, 1.0f, 0.001f);
    BOOST_CHECK_CLOSE(retrieved.pathColor.r, 1.0f, 0.001f);
    BOOST_CHECK_CLOSE(retrieved.pulseSpeed, 4.0f, 0.001f);
    BOOST_CHECK_CLOSE(retrieved.heightOffset, 0.05f, 0.001f);

    engine.RemoveScene("TacticalCustomizationScene");
}

BOOST_AUTO_TEST_CASE(TacticalGridCustomCellSizeAndScalingTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalCustomCellSizeScene");
    BOOST_REQUIRE(scene != nullptr);

    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(pathfindingSystem != nullptr);
    pathfindingSystem->SetCellSize(0.25f);
    BOOST_CHECK_CLOSE(pathfindingSystem->GetCellSize(), 0.25f, 0.001f);

    auto tacticalSystem = scene->GetSystem<TacticalGridSystem>();
    BOOST_REQUIRE(tacticalSystem != nullptr);
    BOOST_CHECK_CLOSE(tacticalSystem->GetCellSize(), 0.25f, 0.001f);

    Entity floor = scene->CreateEntity("Floor");
    TransformComponent tFloor;
    tFloor.position = glm::vec3(0.0f, 0.0f, 0.0f);
    scene->AddComponent<TransformComponent>(floor, tFloor);
    GridFragment fFloor(8, 8);
    scene->AddComponent<GridFragment>(floor, fFloor);
    pathfindingSystem->BuildNavGrid();

    Entity agent = scene->CreateEntity("AgentQuarterCell");
    TransformComponent tAgent;
    tAgent.position = pathfindingSystem->GridToWorld(glm::ivec2(0, 0), 0.0f, 1);
    // Grid (0, 0) center with cellSize 0.25 should be at (0.125, 0.0, 0.125)
    BOOST_CHECK_CLOSE(tAgent.position.x, 0.125f, 0.001f);
    BOOST_CHECK_CLOSE(tAgent.position.z, 0.125f, 0.001f);
    scene->AddComponent<TransformComponent>(agent, tAgent);

    PathfindingAgent agentComp(1, 5.0f, 2.0f);
    scene->AddComponent<PathfindingAgent>(agent, agentComp);

    tacticalSystem->SetSelectedEntity(agent);
    BOOST_REQUIRE(tacticalSystem->IsOverlayActive());

    glm::mat4 decalMat = tacticalSystem->GetDecalModelMatrix();
    BOOST_CHECK(decalMat[0][0] > 0.0f);
    BOOST_CHECK(decalMat[2][2] > 0.0f);

    const auto& cellTransforms = tacticalSystem->GetReachableCellTransforms();
    BOOST_CHECK(!cellTransforms.empty());
    BOOST_CHECK_EQUAL(cellTransforms.size(), tacticalSystem->GetReachableCells().size());

    engine.RemoveScene("TacticalCustomCellSizeScene");
}

BOOST_AUTO_TEST_CASE(TacticalGridReachableCellTransformsExactMatchTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalExactMatchScene");
    BOOST_REQUIRE(scene != nullptr);

    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(pathfindingSystem != nullptr);
    pathfindingSystem->SetCellSize(1.0f);

    Entity floor = scene->CreateEntity("Floor");
    TransformComponent tFloor;
    tFloor.position = glm::vec3(0.0f, 0.0f, 0.0f);
    scene->AddComponent<TransformComponent>(floor, tFloor);
    GridFragment fFloor(5, 5);
    scene->AddComponent<GridFragment>(floor, fFloor);

    // Place obstacle at (1, 0)
    Entity obstacle = scene->CreateEntity("Obstacle");
    TransformComponent tObstacle;
    tObstacle.position = pathfindingSystem->GridToWorld(glm::ivec2(1, 0), 0.0f, 1);
    scene->AddComponent<TransformComponent>(obstacle, tObstacle);
    Obstacle obsComp;
    scene->AddComponent<Obstacle>(obstacle, obsComp);

    pathfindingSystem->BuildNavGrid();

    auto tacticalSystem = scene->GetSystem<TacticalGridSystem>();
    BOOST_REQUIRE(tacticalSystem != nullptr);

    Entity agent = scene->CreateEntity("Agent");
    TransformComponent tAgent;
    tAgent.position = pathfindingSystem->GridToWorld(glm::ivec2(0, 0), 0.0f, 1);
    scene->AddComponent<TransformComponent>(agent, tAgent);
    PathfindingAgent agentComp(1, 1.5f, 2.0f); // 1.5 movement allowance allows distance 1 orthogonal moves
    scene->AddComponent<PathfindingAgent>(agent, agentComp);

    tacticalSystem->SetSelectedEntity(agent);
    BOOST_REQUIRE(tacticalSystem->IsOverlayActive());

    // From (0,0) with cost 1.5, reachable: (0,0), (0,1), (1,1) if diagonal allowed (cost 1.414). (1,0) is blocked by obstacle.
    BOOST_CHECK(!tacticalSystem->IsCellReachable(glm::ivec2(1, 0)));
    const auto& reachable = tacticalSystem->GetReachableCells();
    const auto& cellTransforms = tacticalSystem->GetReachableCellTransforms();
    BOOST_CHECK_EQUAL(cellTransforms.size(), reachable.size());

    // Verify each transform center matches the reachable grid cell world position
    for (size_t i = 0; i < reachable.size(); ++i) {
        glm::vec3 expectedWorld = pathfindingSystem->GridToWorld(reachable[i].gridPos, 0.0f, 1);
        glm::vec4 center = cellTransforms[i] * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        BOOST_CHECK_CLOSE(center.x, expectedWorld.x, 0.01f);
        BOOST_CHECK_CLOSE(center.z, expectedWorld.z, 0.01f);
    }

    engine.RemoveScene("TacticalExactMatchScene");
}

BOOST_AUTO_TEST_CASE(TacticalGridMultiCellDeduplicationTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalDeduplicationScene");
    BOOST_REQUIRE(scene != nullptr);

    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(pathfindingSystem != nullptr);
    pathfindingSystem->SetCellSize(1.0f);

    Entity floor = scene->CreateEntity("Floor");
    TransformComponent tFloor;
    tFloor.position = glm::vec3(0.0f, 0.0f, 0.0f);
    scene->AddComponent<TransformComponent>(floor, tFloor);
    GridFragment fFloor(10, 10);
    scene->AddComponent<GridFragment>(floor, fFloor);
    pathfindingSystem->BuildNavGrid();

    auto tacticalSystem = scene->GetSystem<TacticalGridSystem>();
    BOOST_REQUIRE(tacticalSystem != nullptr);

    Entity agent2 = scene->CreateEntity("Agent2");
    TransformComponent tAgent2;
    tAgent2.position = pathfindingSystem->GridToWorld(glm::ivec2(0, 0), 0.0f, 2);
    scene->AddComponent<TransformComponent>(agent2, tAgent2);
    PathfindingAgent agentComp(2, 5.0f, 1.0f); // allowance 1.0 allows cardinal step of distance 1 for size 2
    scene->AddComponent<PathfindingAgent>(agent2, agentComp);

    tacticalSystem->SetSelectedEntity(agent2);
    BOOST_REQUIRE(tacticalSystem->IsOverlayActive());

    // Origin reachable positions from (0,0) with maxCost 1.0 (cardinal directions on symmetric grid):
    // (0,0), (1,0), (-1,0), (0,1), (0,-1) -> 5 origin positions
    const auto& reachable = tacticalSystem->GetReachableCells();
    BOOST_CHECK_EQUAL(reachable.size(), 5);

    // Each origin position covers 2x2 = 4 tiles with overlaps across neighbors:
    // (5 positions * 4 = 20 overlapping tiles).
    // Union contains exactly 12 unique tiles: [-1..2] x [-1..2] minus 4 outer corners
    const auto& cellTransforms = tacticalSystem->GetReachableCellTransforms();
    BOOST_CHECK_EQUAL(cellTransforms.size(), 12);

    // Each transform must be scaled to exactly 1x1 cell size (0.5 * cellSz = 0.5)
    for (const auto& mat : cellTransforms) {
        BOOST_CHECK_CLOSE(mat[0][0], 0.5f, 0.001f);
        BOOST_CHECK_CLOSE(mat[2][2], 0.5f, 0.001f);
    }

    engine.RemoveScene("TacticalDeduplicationScene");
}

BOOST_AUTO_TEST_SUITE_END()
