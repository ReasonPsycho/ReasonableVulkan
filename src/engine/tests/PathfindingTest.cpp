#include <boost/test/unit_test.hpp>
#include "../Engine.h"
#include "ecs/Scene.h"
#include "TestHelper.hpp"
#include "systems/transformSystem/componets/TransformComponent.hpp"
#include "systems/pathfindingSystem/PathfindingSystem.hpp"
#include "systems/pathfindingSystem/componets/PathfindingAgent.hpp"
#include "systems/pathfindingSystem/componets/GridFragment.hpp"
#include "systems/editorSystem/ComponentGizmoRegistry.hpp"
#include "systems/editorSystem/visualizers/GridFragmentGizmoVisualizer.hpp"

using namespace engine;
using namespace engine::ecs;
using namespace engine::test;

BOOST_AUTO_TEST_SUITE(PathfindingTestSuite)

BOOST_AUTO_TEST_CASE(GridFragmentSnappingAndWalkabilityTest) {
    GridFragment fragment(4, 4);
    BOOST_CHECK_EQUAL(fragment.width, 4);
    BOOST_CHECK_EQUAL(fragment.height, 4);

    // Initial walkability
    BOOST_CHECK(fragment.IsCellWalkable(0, 0));
    BOOST_CHECK(fragment.IsCellWalkable(3, 3));
    BOOST_CHECK(!fragment.IsCellWalkable(4, 4)); // Out of bounds

    // Modify local walkability
    fragment.SetCellWalkable(1, 2, false);
    BOOST_CHECK(!fragment.IsCellWalkable(1, 2));
    BOOST_CHECK(fragment.IsCellWalkable(0, 0));

    // Snapping tests across positive and negative world coordinates (centered)
    glm::vec3 pos1(3.7f, 10.0f, 5.2f);
    glm::vec3 snapped1 = fragment.SnapPosition(pos1, 2.0f);
    BOOST_CHECK_CLOSE(snapped1.x, 4.0f, 0.001f);
    BOOST_CHECK_CLOSE(snapped1.y, 10.0f, 0.001f);
    BOOST_CHECK_CLOSE(snapped1.z, 6.0f, 0.001f);

    glm::vec3 pos2(-1.5f, 0.0f, -3.1f);
    glm::vec3 snapped2 = fragment.SnapPosition(pos2, 2.0f);
    BOOST_CHECK_CLOSE(snapped2.x, -2.0f, 0.001f);
    BOOST_CHECK_CLOSE(snapped2.z, -4.0f, 0.001f);
}

BOOST_AUTO_TEST_CASE(GridFragmentStitchingTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("StitchingTestScene");
    BOOST_REQUIRE(scene);

    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(pathfindingSystem);
    pathfindingSystem->SetCellSize(1.0f);
    BOOST_CHECK_CLOSE(pathfindingSystem->cellSize, 1.0f, 0.001f);

    // Create Tile Fragment 1 at (0, 0, 0) of size 3x3 (bounds [-1..1] x [-1..1])
    Entity tile1 = scene->CreateEntity("Tile1");
    TransformComponent t1;
    t1.position = glm::vec3(0.0f, 0.0f, 0.0f);
    scene->AddComponent<TransformComponent>(tile1, t1);
    GridFragment f1(3, 3);
    scene->AddComponent<GridFragment>(tile1, f1);

    // Create Tile Fragment 2 at (3, 0, 0) of size 3x3 (adjacent along X, bounds [2..4] x [-1..1])
    Entity tile2 = scene->CreateEntity("Tile2");
    TransformComponent t2;
    t2.position = glm::vec3(3.0f, 0.0f, 0.0f);
    scene->AddComponent<TransformComponent>(tile2, t2);
    GridFragment f2(3, 3);
    scene->AddComponent<GridFragment>(tile2, f2);

    pathfindingSystem->BuildNavGrid();

    // Verify unified grid dimensions: bounds [-1..4] x [-1..1] -> width 6, height 3
    BOOST_CHECK_EQUAL(pathfindingSystem->GetGridWidth(), 6);
    BOOST_CHECK_EQUAL(pathfindingSystem->GetGridHeight(), 3);
    BOOST_CHECK(pathfindingSystem->IsWalkable(-1, -1));
    BOOST_CHECK(pathfindingSystem->IsWalkable(4, 1));

    // Pathfinding across the stitched seam (from (-1,-1) to (4,1))
    auto path = pathfindingSystem->FindPathGrid(glm::ivec2(-1, -1), glm::ivec2(4, 1), 1);
    BOOST_REQUIRE(!path.empty());
    BOOST_CHECK_EQUAL(path.front().x, -1);
    BOOST_CHECK_EQUAL(path.front().y, -1);
    BOOST_CHECK_EQUAL(path.back().x, 4);
    BOOST_CHECK_EQUAL(path.back().y, 1);

    engine.RemoveScene("StitchingTestScene");
}

BOOST_AUTO_TEST_CASE(ClearanceAndMultiSizeAgentPathfindingTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("ClearanceTestScene");
    BOOST_REQUIRE(scene);

    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(pathfindingSystem);
    pathfindingSystem->SetCellSize(1.0f);

    // Create a 6x6 floor fragment centered at (0,0,0) (bounds [-3..2] x [-3..2])
    Entity floor = scene->CreateEntity("Floor");
    TransformComponent tFloor;
    tFloor.position = glm::vec3(0.0f, 0.0f, 0.0f);
    scene->AddComponent<TransformComponent>(floor, tFloor);
    GridFragment fFloor(6, 6);
    scene->AddComponent<GridFragment>(floor, fFloor);

    // Create a wall obstacle leaving a narrow corridor of width 1 at local x=3, y=1 (global gx=0, gy=-2)
    // Wall occupies local (3, 0), (3, 2), (3, 3), (3, 4), (3, 5)
    fFloor.SetCellWalkable(3, 0, false);
    fFloor.SetCellWalkable(3, 2, false);
    fFloor.SetCellWalkable(3, 3, false);
    fFloor.SetCellWalkable(3, 4, false);
    fFloor.SetCellWalkable(3, 5, false);
    scene->GetComponent<GridFragment>(floor) = fFloor;

    pathfindingSystem->BuildNavGrid();

    // 1x1 agent can pass through the narrow gap at global (0, -2) (local (3, 1))
    auto path1x1 = pathfindingSystem->FindPathGrid(glm::ivec2(-2, -2), glm::ivec2(2, -2), 1);
    BOOST_REQUIRE(!path1x1.empty());

    // 2x2 agent cannot pass through a 1-cell wide gap
    auto path2x2 = pathfindingSystem->FindPathGrid(glm::ivec2(-3, -3), glm::ivec2(1, -3), 2);
    BOOST_CHECK(path2x2.empty());

    // Now open a 2-cell wide gap at local (3, 2)
    fFloor.SetCellWalkable(3, 2, true);
    scene->GetComponent<GridFragment>(floor) = fFloor;
    pathfindingSystem->BuildNavGrid();

    // Now 2x2 agent can fit and find a path
    auto path2x2_open = pathfindingSystem->FindPathGrid(glm::ivec2(-3, -2), glm::ivec2(1, -2), 2);
    BOOST_REQUIRE(!path2x2_open.empty());

    // 3x3 agent requires a 3x3 footprint and cannot fit in the 2-cell gap
    auto path3x3 = pathfindingSystem->FindPathGrid(glm::ivec2(-3, -3), glm::ivec2(0, -3), 3);
    BOOST_CHECK(path3x3.empty());

    engine.RemoveScene("ClearanceTestScene");
}

BOOST_AUTO_TEST_CASE(PathfindingAgentMovementSystemTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("AgentMovementScene");
    BOOST_REQUIRE(scene);

    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(pathfindingSystem);
    pathfindingSystem->SetCellSize(1.0f);

    // Create 10x10 walkable ground centered at (0, 0, 0)
    Entity ground = scene->CreateEntity("Ground");
    TransformComponent tGround;
    tGround.position = glm::vec3(0.0f, 0.0f, 0.0f);
    scene->AddComponent<TransformComponent>(ground, tGround);
    GridFragment fGround(10, 10);
    scene->AddComponent<GridFragment>(ground, fGround);

    // Create Agent at (0.5, 0, 0.5) (center of cell (0,0))
    Entity agentEntity = scene->CreateEntity("Agent");
    TransformComponent tAgent;
    tAgent.position = glm::vec3(0.5f, 0.0f, 0.5f);
    scene->AddComponent<TransformComponent>(agentEntity, tAgent);

    PathfindingAgent agentComp(1, 10.0f); // Size 1, Speed 10.0 units/sec
    scene->AddComponent<PathfindingAgent>(agentEntity, agentComp);

    pathfindingSystem->BuildNavGrid();

    // Request path to (3.5, 0, 3.5) (cell (3,3))
    glm::vec3 targetPos(3.5f, 0.0f, 3.5f);
    bool requested = pathfindingSystem->RequestPath(agentEntity, targetPos);
    BOOST_REQUIRE(requested);

    auto& agentRef = scene->GetComponent<PathfindingAgent>(agentEntity);
    BOOST_CHECK(agentRef.isMoving);
    BOOST_CHECK(!agentRef.waypoints.empty());

    // Simulate system updates over multiple frames
    float dt = 0.1f;
    for (int frame = 0; frame < 20; ++frame) {
        pathfindingSystem->Update(dt);
        if (scene->GetComponent<PathfindingAgent>(agentEntity).reachedTarget) {
            break;
        }
    }

    auto& finalAgent = scene->GetComponent<PathfindingAgent>(agentEntity);
    auto& finalTransform = scene->GetComponent<TransformComponent>(agentEntity);

    BOOST_CHECK(finalAgent.reachedTarget);
    BOOST_CHECK(!finalAgent.isMoving);
    BOOST_CHECK_CLOSE(finalTransform.position.x, 3.5f, 0.001f);
    BOOST_CHECK_CLOSE(finalTransform.position.z, 3.5f, 0.001f);

    engine.RemoveScene("AgentMovementScene");
}

BOOST_AUTO_TEST_CASE(AgentFinalPlacementAccuracyTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("FinalPlacementScene");
    BOOST_REQUIRE(scene);

    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(pathfindingSystem);
    pathfindingSystem->SetCellSize(1.0f);

    // Create 10x10 walkable ground centered at (0, 0, 0)
    Entity ground = scene->CreateEntity("Ground");
    TransformComponent tGround;
    tGround.position = glm::vec3(0.0f, 0.0f, 0.0f);
    scene->AddComponent<TransformComponent>(ground, tGround);
    GridFragment fGround(10, 10);
    scene->AddComponent<GridFragment>(ground, fGround);

    // Create Agent at (0.5, 0, 0.5) with a large waypoint threshold of 0.8
    Entity agentEntity = scene->CreateEntity("AgentLargeThreshold");
    TransformComponent tAgent;
    tAgent.position = glm::vec3(0.5f, 0.0f, 0.5f);
    scene->AddComponent<TransformComponent>(agentEntity, tAgent);

    PathfindingAgent agentComp(1, 5.0f);
    agentComp.waypointThreshold = 0.8f;
    scene->AddComponent<PathfindingAgent>(agentEntity, agentComp);

    pathfindingSystem->BuildNavGrid();

    glm::vec3 destination(2.5f, 0.0f, 1.5f);
    bool requested = pathfindingSystem->RequestPath(agentEntity, destination);
    BOOST_REQUIRE(requested);

    // Step through updates until agent reaches destination
    for (int frame = 0; frame < 50; ++frame) {
        pathfindingSystem->Update(0.05f);
        if (scene->GetComponent<PathfindingAgent>(agentEntity).reachedTarget) {
            break;
        }
    }

    auto& finalAgent = scene->GetComponent<PathfindingAgent>(agentEntity);
    auto& finalTransform = scene->GetComponent<TransformComponent>(agentEntity);

    BOOST_CHECK(finalAgent.reachedTarget);
    BOOST_CHECK(!finalAgent.isMoving);
    // At the end of traveling, the agent must be placed exactly at the destination transform coordinates
    BOOST_CHECK_CLOSE(finalTransform.position.x, destination.x, 0.0001f);
    BOOST_CHECK_CLOSE(finalTransform.position.z, destination.z, 0.0001f);

    engine.RemoveScene("FinalPlacementScene");
}

BOOST_AUTO_TEST_CASE(ProceduralScatteredFragmentsAndNegativeCoordsTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("ProceduralMapScene");
    BOOST_REQUIRE(scene);

    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(pathfindingSystem);
    pathfindingSystem->SetCellSize(2.0f);

    // Place fragments at negative coordinates and disjoint locations
    // Fragment A centered at (-3, 0, -3) -> grid cells [-2..-1] x [-2..-1] (2x2 cells, cellSize=2)
    Entity tileA = scene->CreateEntity("TileA");
    TransformComponent tA;
    tA.position = glm::vec3(-3.0f, 0.0f, -3.0f);
    scene->AddComponent<TransformComponent>(tileA, tA);
    GridFragment fA(2, 2);
    scene->AddComponent<GridFragment>(tileA, fA);

    // Fragment B centered at (1, 0, 1) -> grid cells [0..1] x [0..1] (2x2 cells)
    Entity tileB = scene->CreateEntity("TileB");
    TransformComponent tB;
    tB.position = glm::vec3(1.0f, 0.0f, 1.0f);
    scene->AddComponent<TransformComponent>(tileB, tB);
    GridFragment fB(2, 2);
    scene->AddComponent<GridFragment>(tileB, fB);

    // Bridge fragment connecting them centered at (-1, 0, -1) -> grid cells [-1..0] x [-1..0] (2x2 cells)
    Entity tileBridge = scene->CreateEntity("TileBridge");
    TransformComponent tBridge;
    tBridge.position = glm::vec3(-1.0f, 0.0f, -1.0f);
    scene->AddComponent<TransformComponent>(tileBridge, tBridge);
    GridFragment fBridge(2, 2);
    scene->AddComponent<GridFragment>(tileBridge, fBridge);

    pathfindingSystem->BuildNavGrid();

    // Verify bounds encompass [-2..1] x [-2..1] -> width 4, height 4
    BOOST_CHECK_EQUAL(pathfindingSystem->GetGridMin().x, -2);
    BOOST_CHECK_EQUAL(pathfindingSystem->GetGridMin().y, -2);
    BOOST_CHECK_EQUAL(pathfindingSystem->GetGridMax().x, 1);
    BOOST_CHECK_EQUAL(pathfindingSystem->GetGridMax().y, 1);
    BOOST_CHECK_EQUAL(pathfindingSystem->GetGridWidth(), 4);
    BOOST_CHECK_EQUAL(pathfindingSystem->GetGridHeight(), 4);

    // Pathfind from TileA center (-3.0, 0, -3.0) to TileB center (1.0, 0, 1.0)
    auto worldPath = pathfindingSystem->FindPath(glm::vec3(-3.0f, 0.0f, -3.0f), glm::vec3(1.0f, 0.0f, 1.0f), 1);
    BOOST_REQUIRE(!worldPath.empty());
    BOOST_CHECK_CLOSE(worldPath.front().x, -3.0f, 0.001f);
    BOOST_CHECK_CLOSE(worldPath.front().z, -3.0f, 0.001f);
    BOOST_CHECK_CLOSE(worldPath.back().x, 1.0f, 0.001f);
    BOOST_CHECK_CLOSE(worldPath.back().z, 1.0f, 0.001f);

    engine.RemoveScene("ProceduralMapScene");
}

BOOST_AUTO_TEST_CASE(PathfindingComponentsSerializationTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("SerializationScene");
    BOOST_REQUIRE(scene);

    Entity e = scene->CreateEntity("AgentWithFragment");
    TransformComponent t;
    t.position = glm::vec3(10.0f, 0.0f, 20.0f);
    scene->AddComponent<TransformComponent>(e, t);

    GridFragment frag(5, 7);
    frag.snapToGrid = true;
    scene->AddComponent<GridFragment>(e, frag);

    PathfindingAgent agent(2, 7.5f);
    agent.targetPosition = glm::vec3(30.0f, 0.0f, 40.0f);
    agent.hasTarget = true;
    scene->AddComponent<PathfindingAgent>(e, agent);

    rapidjson::Document doc;
    doc.SetObject();
    scene->SerializeToJson(doc);

    auto scene2 = engine.CreateScene("DeserializedScene");
    scene2->DeserializeFromJson(doc);

    auto entities = scene2->GetEntitiesWith<GridFragment, PathfindingAgent>();
    BOOST_REQUIRE_EQUAL(entities.size(), 1);

    Entity e2 = entities[0];
    auto& frag2 = scene2->GetComponent<GridFragment>(e2);
    BOOST_CHECK_EQUAL(frag2.width, 5);
    BOOST_CHECK_EQUAL(frag2.height, 7);
    BOOST_CHECK(frag2.snapToGrid);

    auto& agent2 = scene2->GetComponent<PathfindingAgent>(e2);
    BOOST_CHECK_EQUAL(agent2.agentSize, 2);
    BOOST_CHECK_CLOSE(agent2.speed, 7.5f, 0.001f);
    BOOST_CHECK_CLOSE(agent2.targetPosition.x, 30.0f, 0.001f);
    BOOST_CHECK_CLOSE(agent2.targetPosition.z, 40.0f, 0.001f);
    BOOST_CHECK(agent2.hasTarget);

    engine.RemoveScene("SerializationScene");
    engine.RemoveScene("DeserializedScene");
}

BOOST_AUTO_TEST_CASE(GridFragmentGizmoVisualizerTest) {
    // 1. Static concept compliance
    static_assert(HasGizmoVisualizer<GridFragment>, "GridFragment must have a valid gizmo visualizer specialization");
    BOOST_CHECK_EQUAL(ComponentGizmoVisualizer<GridFragment>::DisplayName, "Grid Fragments");
    BOOST_CHECK(ComponentGizmoVisualizer<GridFragment>::DefaultEnabled);

    // 2. Registry category toggle
    BOOST_CHECK(ComponentGizmoRegistry<>::IsGizmoEnabled<GridFragment>());
    ComponentGizmoRegistry<>::SetGizmoEnabled<GridFragment>(false);
    BOOST_CHECK(!ComponentGizmoRegistry<>::IsGizmoEnabled<GridFragment>());
    ComponentGizmoRegistry<>::SetGizmoEnabled<GridFragment>(true);
    BOOST_CHECK(ComponentGizmoRegistry<>::IsGizmoEnabled<GridFragment>());

    // 3. Coordinate calculation & screen projection
    GridFragment frag(4, 4);
    frag.showGizmo = true;
    BOOST_CHECK(frag.showGizmo);

    glm::mat4 worldTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    // Construct a standard orthographic/perspective projection & view for testing
    GizmoContext ctx;
    ctx.viewportPos = ImVec2(0.0f, 0.0f);
    ctx.viewportSize = ImVec2(800.0f, 600.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    ctx.viewProj = proj * view;

    ImVec2 screenPos;
    bool visible = ComponentGizmoVisualizer<GridFragment>::GetScreenPosition(frag, worldTransform, ctx, screenPos);
    BOOST_REQUIRE(visible);
    // Center of fragment is at (0, 0, 0), camera looks straight at (0, 0, 0) from (0, 10, 0)
    // In viewport 800x600, center is (400, 300)
    BOOST_CHECK_CLOSE(screenPos.x, 400.0f, 1.0f);
    BOOST_CHECK_CLOSE(screenPos.y, 300.0f, 1.0f);

    // 4. Hit test
    ctx.mousePos = ImVec2(402.0f, 301.0f);
    BOOST_CHECK(ComponentGizmoVisualizer<GridFragment>::HitTest(frag, worldTransform, ctx, 14.0f));

    ctx.mousePos = ImVec2(500.0f, 500.0f);
    BOOST_CHECK(!ComponentGizmoVisualizer<GridFragment>::HitTest(frag, worldTransform, ctx, 14.0f));
}

BOOST_AUTO_TEST_CASE(PathfindingSystemCellSizeReflectionTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("CellSizeReflectionScene");
    BOOST_REQUIRE(scene);

    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(pathfindingSystem);
    BOOST_CHECK_CLOSE(pathfindingSystem->cellSize, 1.0f, 0.001f);
    BOOST_CHECK_CLOSE(pathfindingSystem->GetCellSize(), 1.0f, 0.001f);

    pathfindingSystem->SetCellSize(2.5f);
    BOOST_CHECK_CLOSE(pathfindingSystem->cellSize, 2.5f, 0.001f);
    BOOST_CHECK_CLOSE(pathfindingSystem->GetCellSize(), 2.5f, 0.001f);

    engine.RemoveScene("CellSizeReflectionScene");
}

BOOST_AUTO_TEST_CASE(AgentsCannotPathfindThroughEachOtherTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("AgentObstacleScene");
    BOOST_REQUIRE(scene);

    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(pathfindingSystem);
    pathfindingSystem->SetCellSize(1.0f);

    // Create 5x5 floor (bounds [-2..2] x [-2..2])
    Entity floor = scene->CreateEntity("Floor");
    TransformComponent tFloor;
    tFloor.position = glm::vec3(0.0f, 0.0f, 0.0f);
    scene->AddComponent<TransformComponent>(floor, tFloor);
    GridFragment fFloor(5, 5);
    scene->AddComponent<GridFragment>(floor, fFloor);

    // Agent 1 is located at (0.5, 0, 0.5) (grid cell (0, 0))
    Entity agent1 = scene->CreateEntity("Agent1");
    TransformComponent t1;
    t1.position = glm::vec3(0.5f, 0.0f, 0.5f);
    scene->AddComponent<TransformComponent>(agent1, t1);
    PathfindingAgent a1(1, 5.0f);
    scene->AddComponent<PathfindingAgent>(agent1, a1);

    // Agent 2 is placed at start position (-1.5, 0, 0.5) (grid cell (-2, 0))
    Entity agent2 = scene->CreateEntity("Agent2");
    TransformComponent t2;
    t2.position = glm::vec3(-1.5f, 0.0f, 0.5f);
    scene->AddComponent<TransformComponent>(agent2, t2);
    PathfindingAgent a2(1, 5.0f);
    scene->AddComponent<PathfindingAgent>(agent2, a2);

    pathfindingSystem->BuildNavGrid();

    // Agent 2 requests path to (1.5, 0, 0.5) (grid cell (1, 0)), passing across (0, 0) where Agent 1 sits.
    // The path should route around Agent 1 (e.g. via y != 0) instead of passing through (0, 0).
    bool requested = pathfindingSystem->RequestPath(agent2, glm::vec3(1.5f, 0.0f, 0.5f));
    BOOST_REQUIRE(requested);

    auto& agent2Comp = scene->GetComponent<PathfindingAgent>(agent2);
    BOOST_CHECK(agent2Comp.isMoving);
    BOOST_REQUIRE(!agent2Comp.waypoints.empty());

    // Verify none of the waypoints pass through Agent 1's cell (0, 0)
    for (const auto& wp : agent2Comp.waypoints) {
        glm::ivec2 wpGrid = pathfindingSystem->WorldToGrid(wp);
        // Waypoint cannot be exactly in cell (0, 0)
        BOOST_CHECK(!(wpGrid.x == 0 && wpGrid.y == 0));
    }

    // Direct pathfinding to Agent 1's exact cell should fail because cell (0, 0) is blocked by Agent 1
    auto blockedPath = pathfindingSystem->FindPathGrid(glm::ivec2(-2, 0), glm::ivec2(0, 0), 1, agent2);
    BOOST_CHECK(blockedPath.empty());

    engine.RemoveScene("AgentObstacleScene");
}

BOOST_AUTO_TEST_CASE(AgentsCannotPathfindDiagonallyBetweenEachOtherTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("AgentDiagonalScene");
    BOOST_REQUIRE(scene);

    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(pathfindingSystem);
    pathfindingSystem->SetCellSize(1.0f);

    // Create 4x4 floor centered at (0, 0, 0) (bounds [-2..1] x [-2..1])
    Entity floor = scene->CreateEntity("Floor");
    TransformComponent tFloor;
    tFloor.position = glm::vec3(0.0f, 0.0f, 0.0f);
    scene->AddComponent<TransformComponent>(floor, tFloor);
    GridFragment fFloor(4, 4);
    scene->AddComponent<GridFragment>(floor, fFloor);

    // Agent 1 at (0.5, 0, -0.5) -> cell (0, -1)
    Entity agent1 = scene->CreateEntity("Agent1");
    TransformComponent t1;
    t1.position = glm::vec3(0.5f, 0.0f, -0.5f);
    scene->AddComponent<TransformComponent>(agent1, t1);
    PathfindingAgent a1(1, 5.0f);
    scene->AddComponent<PathfindingAgent>(agent1, a1);

    // Agent 2 at (-0.5, 0, 0.5) -> cell (-1, 0)
    Entity agent2 = scene->CreateEntity("Agent2");
    TransformComponent t2;
    t2.position = glm::vec3(-0.5f, 0.0f, 0.5f);
    scene->AddComponent<TransformComponent>(agent2, t2);
    PathfindingAgent a2(1, 5.0f);
    scene->AddComponent<PathfindingAgent>(agent2, a2);

    pathfindingSystem->BuildNavGrid();

    // Agent 3 tries to move from (-1, -1) to (0, 0).
    // Moving diagonally from (-1, -1) to (0, 0) requires passing between Agent 1 at (0, -1) and Agent 2 at (-1, 0).
    // Because both orthogonal cells are occupied by agents, diagonal movement between them must be blocked.
    Entity agent3 = scene->CreateEntity("Agent3");
    TransformComponent t3;
    t3.position = glm::vec3(-0.5f, 0.0f, -0.5f); // Cell (-1, -1)
    scene->AddComponent<TransformComponent>(agent3, t3);
    PathfindingAgent a3(1, 5.0f);
    scene->AddComponent<PathfindingAgent>(agent3, a3);

    auto path = pathfindingSystem->FindPathGrid(glm::ivec2(-1, -1), glm::ivec2(0, 0), 1, agent3);
    // If a path is found, it must NOT take the direct diagonal step (-1, -1) -> (0, 0)
    if (!path.empty()) {
        for (size_t i = 0; i + 1 < path.size(); ++i) {
            bool isDirectDiagonal = (path[i] == glm::ivec2(-1, -1) && path[i + 1] == glm::ivec2(0, 0));
            BOOST_CHECK(!isDirectDiagonal);
        }
    }

    engine.RemoveScene("AgentDiagonalScene");
}

BOOST_AUTO_TEST_CASE(MultiSizeAgentGridCoordinateTransformsTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TransformsTestScene");
    BOOST_REQUIRE(scene);

    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(pathfindingSystem);
    pathfindingSystem->SetCellSize(1.0f);

    // Size 1 (1x1): center of cell (0, 0) is (0.5, 0.5)
    glm::vec3 world1 = pathfindingSystem->GridToWorld(glm::ivec2(0, 0), 0.0f, 1);
    BOOST_CHECK_CLOSE(world1.x, 0.5f, 0.001f);
    BOOST_CHECK_CLOSE(world1.z, 0.5f, 0.001f);
    glm::ivec2 grid1 = pathfindingSystem->WorldToGrid(world1, 1);
    BOOST_CHECK_EQUAL(grid1.x, 0);
    BOOST_CHECK_EQUAL(grid1.y, 0);

    // Size 2 (2x2): chunk (0, 0) covers [0..2] x [0..2], center / cross-section is (1.0, 1.0)
    glm::vec3 world2 = pathfindingSystem->GridToWorld(glm::ivec2(0, 0), 0.0f, 2);
    BOOST_CHECK_CLOSE(world2.x, 1.0f, 0.001f);
    BOOST_CHECK_CLOSE(world2.z, 1.0f, 0.001f);
    glm::ivec2 grid2 = pathfindingSystem->WorldToGrid(world2, 2);
    BOOST_CHECK_EQUAL(grid2.x, 0);
    BOOST_CHECK_EQUAL(grid2.y, 0);

    // Size 2 with negative anchor (-3, -2): chunk covers [-3..-1] x [-2..0], center is (-2.0, -1.0)
    glm::vec3 world2Neg = pathfindingSystem->GridToWorld(glm::ivec2(-3, -2), 0.0f, 2);
    BOOST_CHECK_CLOSE(world2Neg.x, -2.0f, 0.001f);
    BOOST_CHECK_CLOSE(world2Neg.z, -1.0f, 0.001f);
    glm::ivec2 grid2Neg = pathfindingSystem->WorldToGrid(world2Neg, 2);
    BOOST_CHECK_EQUAL(grid2Neg.x, -3);
    BOOST_CHECK_EQUAL(grid2Neg.y, -2);

    // Size 3 (3x3): center of cell chunk (0, 0) is (1.5, 1.5)
    glm::vec3 world3 = pathfindingSystem->GridToWorld(glm::ivec2(0, 0), 0.0f, 3);
    BOOST_CHECK_CLOSE(world3.x, 1.5f, 0.001f);
    BOOST_CHECK_CLOSE(world3.z, 1.5f, 0.001f);
    glm::ivec2 grid3 = pathfindingSystem->WorldToGrid(world3, 3);
    BOOST_CHECK_EQUAL(grid3.x, 0);
    BOOST_CHECK_EQUAL(grid3.y, 0);

    // Size 2 with cellSize = 2.0f
    pathfindingSystem->SetCellSize(2.0f);
    glm::vec3 world2Cell2 = pathfindingSystem->GridToWorld(glm::ivec2(1, 2), 0.0f, 2);
    // (1 * 2 + 2.0, 2 * 2 + 2.0) = (4.0, 6.0)
    BOOST_CHECK_CLOSE(world2Cell2.x, 4.0f, 0.001f);
    BOOST_CHECK_CLOSE(world2Cell2.z, 6.0f, 0.001f);
    glm::ivec2 grid2Cell2 = pathfindingSystem->WorldToGrid(world2Cell2, 2);
    BOOST_CHECK_EQUAL(grid2Cell2.x, 1);
    BOOST_CHECK_EQUAL(grid2Cell2.y, 2);

    engine.RemoveScene("TransformsTestScene");
}

BOOST_AUTO_TEST_CASE(Size2AgentLandingAtCrossSectionMovementTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("Size2AgentMovementScene");
    BOOST_REQUIRE(scene);

    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(pathfindingSystem);
    pathfindingSystem->SetCellSize(1.0f);

    // Create 10x10 floor centered at (0, 0, 0)
    Entity ground = scene->CreateEntity("Ground");
    TransformComponent tGround;
    tGround.position = glm::vec3(0.0f, 0.0f, 0.0f);
    scene->AddComponent<TransformComponent>(ground, tGround);
    GridFragment fGround(10, 10);
    scene->AddComponent<GridFragment>(ground, fGround);

    // Create Agent with size 2 starting at (1.0, 0.0, 1.0) (anchored at grid (0, 0))
    Entity agentEntity = scene->CreateEntity("Size2Agent");
    TransformComponent tAgent;
    tAgent.position = glm::vec3(1.0f, 0.0f, 1.0f);
    scene->AddComponent<TransformComponent>(agentEntity, tAgent);

    PathfindingAgent agentComp(2, 5.0f); // Size 2
    scene->AddComponent<PathfindingAgent>(agentEntity, agentComp);

    pathfindingSystem->BuildNavGrid();

    // Destination target is cross section of 2x2 chunk anchored at grid (2, 2), which is (3.0, 0.0, 3.0)
    glm::vec3 destination = pathfindingSystem->GridToWorld(glm::ivec2(2, 2), 0.0f, 2);
    BOOST_CHECK_CLOSE(destination.x, 3.0f, 0.001f);
    BOOST_CHECK_CLOSE(destination.z, 3.0f, 0.001f);

    bool requested = pathfindingSystem->RequestPath(agentEntity, destination);
    BOOST_REQUIRE(requested);

    auto& agentRef = scene->GetComponent<PathfindingAgent>(agentEntity);
    BOOST_CHECK(agentRef.isMoving);
    BOOST_REQUIRE(!agentRef.waypoints.empty());

    // Waypoints for size 2 agent should all land on chunk cross sections (integer coordinates for cellSize 1.0)
    for (const auto& wp : agentRef.waypoints) {
        BOOST_CHECK_CLOSE(wp.x, std::round(wp.x), 0.001f);
        BOOST_CHECK_CLOSE(wp.z, std::round(wp.z), 0.001f);
    }

    // Step updates until reaching target
    for (int frame = 0; frame < 50; ++frame) {
        pathfindingSystem->Update(0.05f);
        if (scene->GetComponent<PathfindingAgent>(agentEntity).reachedTarget) {
            break;
        }
    }

    auto& finalAgent = scene->GetComponent<PathfindingAgent>(agentEntity);
    auto& finalTransform = scene->GetComponent<TransformComponent>(agentEntity);

    BOOST_CHECK(finalAgent.reachedTarget);
    BOOST_CHECK(!finalAgent.isMoving);
    // Agent with size 2 must be placed at the cross-section of the 2x2 chunk at (3.0, 0.0, 3.0)
    BOOST_CHECK_CLOSE(finalTransform.position.x, 3.0f, 0.001f);
    BOOST_CHECK_CLOSE(finalTransform.position.z, 3.0f, 0.001f);

    engine.RemoveScene("Size2AgentMovementScene");
}

BOOST_AUTO_TEST_SUITE_END()
