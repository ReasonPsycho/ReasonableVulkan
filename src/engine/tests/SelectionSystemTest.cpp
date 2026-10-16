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
#include "systems/selectionSystem/SelectionSystem.hpp"
#include "systems/gizmoSystem/GizmoSystem.hpp"

using namespace engine;
using namespace engine::ecs;
using namespace engine::test;

BOOST_AUTO_TEST_SUITE(SelectionSystemTestSuite)

BOOST_AUTO_TEST_CASE(SelectionSystemRegistrationAndRetrievalTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("SelectionRegScene");
    BOOST_REQUIRE(scene != nullptr);

    auto selectionSystem = scene->GetSystem<SelectionSystem>();
    BOOST_REQUIRE(selectionSystem != nullptr);
    BOOST_CHECK(!selectionSystem->HasSelectedEntity());
    BOOST_CHECK_EQUAL(selectionSystem->GetSelectedEntity(), MAX_ENTITIES);

    engine.RemoveScene("SelectionRegScene");
}

BOOST_AUTO_TEST_CASE(SelectionSystemDirectSelectionAndLifecycleTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("SelectionLifecycleScene");
    BOOST_REQUIRE(scene != nullptr);

    auto selectionSystem = scene->GetSystem<SelectionSystem>();
    BOOST_REQUIRE(selectionSystem != nullptr);

    // Entity without PathfindingAgent
    Entity nonAgent = scene->CreateEntity("NonAgent");
    selectionSystem->SetSelectedEntity(nonAgent);
    // Should be rejected by default because requirePathfindingAgent is true
    BOOST_CHECK(!selectionSystem->HasSelectedEntity());
    BOOST_CHECK(!selectionSystem->IsSelected(nonAgent));

    // Entity with PathfindingAgent
    Entity agentEntity = scene->CreateEntity("Agent");
    scene->AddComponent<PathfindingAgent>(agentEntity, PathfindingAgent{});
    selectionSystem->SetSelectedEntity(agentEntity);
    BOOST_CHECK(selectionSystem->HasSelectedEntity());
    BOOST_CHECK(selectionSystem->IsSelected(agentEntity));
    BOOST_CHECK_EQUAL(selectionSystem->GetSelectedEntity(), agentEntity);

    // Clear selection
    selectionSystem->ClearSelection();
    BOOST_CHECK(!selectionSystem->HasSelectedEntity());

    // Re-select and destroy entity
    selectionSystem->SetSelectedEntity(agentEntity);
    BOOST_CHECK(selectionSystem->HasSelectedEntity());
    scene->DestroyEntity(agentEntity);
    selectionSystem->Update(0.016f);
    BOOST_CHECK(!selectionSystem->HasSelectedEntity());

    engine.RemoveScene("SelectionLifecycleScene");
}

BOOST_AUTO_TEST_CASE(SelectionSystemLeftClickSelectionRaycastTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("SelectionClickScene");
    BOOST_REQUIRE(scene != nullptr);

    auto selectionSystem = scene->GetSystem<SelectionSystem>();
    BOOST_REQUIRE(selectionSystem != nullptr);

    // Setup active camera positioned at (0, 0, 10) looking at (0, 0, 0)
    Entity camEntity = scene->CreateEntity("MainCamera");
    TransformComponent camTrans;
    camTrans.position = glm::vec3(0.0f, 0.0f, 10.0f);
    camTrans.globalMatrix = glm::translate(glm::mat4(1.0f), camTrans.position);
    scene->AddComponent<TransformComponent>(camEntity, camTrans);

    CameraComponent camComp;
    camComp.active = true;
    camComp.fov = 45.0f;
    camComp.nearPlane = 0.1f;
    camComp.farPlane = 100.0f;
    camComp.aspectRatio = 800.0f / 600.0f;
    camComp.view = glm::inverse(camTrans.globalMatrix);
    camComp.projection = glm::perspective(glm::radians(camComp.fov), camComp.aspectRatio, camComp.nearPlane, camComp.farPlane);
    scene->AddComponent<CameraComponent>(camEntity, camComp);

    // Setup Agent Entity located at (0, 0, 0)
    Entity agentEntity = scene->CreateEntity("AgentUnit");
    TransformComponent agentTrans;
    agentTrans.position = glm::vec3(0.0f, 0.0f, 0.0f);
    agentTrans.globalMatrix = glm::translate(glm::mat4(1.0f), agentTrans.position);
    scene->AddComponent<TransformComponent>(agentEntity, agentTrans);

    PathfindingAgent agentComp;
    agentComp.agentSize = 1;
    scene->AddComponent<PathfindingAgent>(agentEntity, agentComp);

    // Left click at center of screen (400, 300) on 800x600 window
    bool selected = selectionSystem->HandleLeftClick(400.0f, 300.0f, 800.0f, 600.0f);
    BOOST_CHECK(selected);
    BOOST_CHECK(selectionSystem->HasSelectedEntity());
    BOOST_CHECK_EQUAL(selectionSystem->GetSelectedEntity(), agentEntity);

    // Left click far off at top-left corner (10, 10) should miss and clear selection
    bool missed = selectionSystem->HandleLeftClick(10.0f, 10.0f, 800.0f, 600.0f);
    BOOST_CHECK(!missed);
    BOOST_CHECK(!selectionSystem->HasSelectedEntity());

    engine.RemoveScene("SelectionClickScene");
}

BOOST_AUTO_TEST_CASE(SelectionSystemRightClickMoveToGridPlacementTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("SelectionMoveScene");
    BOOST_REQUIRE(scene != nullptr);

    auto selectionSystem = scene->GetSystem<SelectionSystem>();
    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(selectionSystem != nullptr);
    BOOST_REQUIRE(pathfindingSystem != nullptr);

    pathfindingSystem->SetCellSize(1.0f);

    // Create 7x7 Grid Fragment centered at (0, 0, 0) (covers [-3..3] x [-3..3])
    Entity floorEntity = scene->CreateEntity("Floor");
    TransformComponent floorTrans;
    floorTrans.position = glm::vec3(0.0f, 0.0f, 0.0f);
    floorTrans.globalMatrix = glm::translate(glm::mat4(1.0f), floorTrans.position);
    scene->AddComponent<TransformComponent>(floorEntity, floorTrans);
    GridFragment floorFrag(7, 7);
    scene->AddComponent<GridFragment>(floorEntity, floorFrag);

    pathfindingSystem->BuildNavGrid();

    // Setup Agent Entity at start pos (-2, 0, -2)
    Entity agentEntity = scene->CreateEntity("AgentUnit");
    TransformComponent agentTrans;
    agentTrans.position = glm::vec3(-2.0f, 0.0f, -2.0f);
    agentTrans.globalMatrix = glm::translate(glm::mat4(1.0f), agentTrans.position);
    scene->AddComponent<TransformComponent>(agentEntity, agentTrans);

    PathfindingAgent agentComp;
    agentComp.agentSize = 1;
    agentComp.speed = 5.0f;
    scene->AddComponent<PathfindingAgent>(agentEntity, agentComp);

    // Select the agent
    selectionSystem->SetSelectedEntity(agentEntity);
    BOOST_CHECK(selectionSystem->HasSelectedEntity());

    // Right click directly via world pos
    bool movedDirect = selectionSystem->MoveSelectedToWorldPos(glm::vec3(2.0f, 0.0f, 2.0f));
    BOOST_CHECK(movedDirect);

    auto& agent = scene->GetComponent<PathfindingAgent>(agentEntity);
    BOOST_CHECK(agent.isMoving);
    BOOST_CHECK(!agent.waypoints.empty());
    BOOST_CHECK_CLOSE(agent.targetPosition.x, 2.5f, 0.001f);
    BOOST_CHECK_CLOSE(agent.targetPosition.z, 2.5f, 0.001f);

    // Setup active camera looking straight down from top
    Entity camEntity = scene->CreateEntity("TopCamera");
    TransformComponent camTrans;
    camTrans.position = glm::vec3(0.0f, 10.0f, 0.0f);
    // Looking down along -Y
    camTrans.globalMatrix = glm::translate(glm::mat4(1.0f), camTrans.position) *
                            glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    scene->AddComponent<TransformComponent>(camEntity, camTrans);

    CameraComponent camComp;
    camComp.active = true;
    camComp.fov = 60.0f;
    camComp.nearPlane = 0.1f;
    camComp.farPlane = 100.0f;
    camComp.aspectRatio = 800.0f / 600.0f;
    camComp.view = glm::inverse(camTrans.globalMatrix);
    camComp.projection = glm::perspective(glm::radians(camComp.fov), camComp.aspectRatio, camComp.nearPlane, camComp.farPlane);
    scene->AddComponent<CameraComponent>(camEntity, camComp);

    // Right click at screen center (400, 300) should project to ground center (0, 0, 0)
    bool movedClick = selectionSystem->HandleRightClick(400.0f, 300.0f, 800.0f, 600.0f);
    BOOST_CHECK(movedClick);

    auto& updatedAgent = scene->GetComponent<PathfindingAgent>(agentEntity);
    BOOST_CHECK(updatedAgent.isMoving);
    BOOST_CHECK_CLOSE(updatedAgent.targetPosition.x, 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(updatedAgent.targetPosition.z, 0.5f, 0.01f);

    engine.RemoveScene("SelectionMoveScene");
}

BOOST_AUTO_TEST_CASE(SelectionSystemUpdateAndGizmoTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("SelectionGizmoScene");
    BOOST_REQUIRE(scene != nullptr);

    auto selectionSystem = scene->GetSystem<SelectionSystem>();
    auto gizmoSystem = scene->GetSystem<GizmoSystem>();
    BOOST_REQUIRE(selectionSystem != nullptr);
    BOOST_REQUIRE(gizmoSystem != nullptr);

    Entity agentEntity = scene->CreateEntity("AgentUnit");
    TransformComponent agentTrans;
    agentTrans.position = glm::vec3(3.0f, 0.0f, 4.0f);
    scene->AddComponent<TransformComponent>(agentEntity, agentTrans);

    PathfindingAgent agentComp;
    agentComp.agentSize = 1;
    scene->AddComponent<PathfindingAgent>(agentEntity, agentComp);

    selectionSystem->SetSelectedEntity(agentEntity);
    selectionSystem->showSelectionGizmo = true;

    size_t prevCommands = gizmoSystem->gizmoRenderCommandQueue.size();
    selectionSystem->Update(0.016f);
    // A cube gizmo command should have been queued
    BOOST_CHECK(gizmoSystem->gizmoRenderCommandQueue.size() >= prevCommands + 1);

    engine.RemoveScene("SelectionGizmoScene");
}

BOOST_AUTO_TEST_CASE(SelectionSystemSize2AgentRightClickPlacementTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("SelectionSize2MoveScene");
    BOOST_REQUIRE(scene != nullptr);

    auto selectionSystem = scene->GetSystem<SelectionSystem>();
    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(selectionSystem != nullptr);
    BOOST_REQUIRE(pathfindingSystem != nullptr);

    pathfindingSystem->SetCellSize(1.0f);

    // Create 8x8 Grid Fragment centered at (0, 0, 0)
    Entity floorEntity = scene->CreateEntity("Floor");
    TransformComponent floorTrans;
    floorTrans.position = glm::vec3(0.0f, 0.0f, 0.0f);
    floorTrans.globalMatrix = glm::translate(glm::mat4(1.0f), floorTrans.position);
    scene->AddComponent<TransformComponent>(floorEntity, floorTrans);
    GridFragment floorFrag(8, 8);
    scene->AddComponent<GridFragment>(floorEntity, floorFrag);

    pathfindingSystem->BuildNavGrid();

    // Setup Size 2 Agent Entity at start pos (-2.0, 0, -2.0)
    Entity agentEntity = scene->CreateEntity("Size2AgentUnit");
    TransformComponent agentTrans;
    agentTrans.position = glm::vec3(-2.0f, 0.0f, -2.0f);
    agentTrans.globalMatrix = glm::translate(glm::mat4(1.0f), agentTrans.position);
    scene->AddComponent<TransformComponent>(agentEntity, agentTrans);

    PathfindingAgent agentComp;
    agentComp.agentSize = 2;
    agentComp.speed = 5.0f;
    scene->AddComponent<PathfindingAgent>(agentEntity, agentComp);

    // Select the agent
    selectionSystem->SetSelectedEntity(agentEntity);
    BOOST_CHECK(selectionSystem->HasSelectedEntity());

    // Right click via world pos targeting (2.1f, 0.0f, 1.9f)
    bool movedDirect = selectionSystem->MoveSelectedToWorldPos(glm::vec3(2.1f, 0.0f, 1.9f));
    BOOST_CHECK(movedDirect);

    auto& agent = scene->GetComponent<PathfindingAgent>(agentEntity);
    BOOST_CHECK(agent.isMoving);
    BOOST_REQUIRE(!agent.waypoints.empty());
    // For size 2 agent, destination should snap to chunk cross section (integer coordinates 2.0, 2.0)
    BOOST_CHECK_CLOSE(agent.targetPosition.x, 2.0f, 0.001f);
    BOOST_CHECK_CLOSE(agent.targetPosition.z, 2.0f, 0.001f);

    engine.RemoveScene("SelectionSize2MoveScene");
}

BOOST_AUTO_TEST_SUITE_END()
