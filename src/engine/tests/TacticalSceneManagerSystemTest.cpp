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
#include "systems/tacticalSceneManagerSystem/TacticalSceneManagerSystem.hpp"
#include "systems/tacticalSceneManagerSystem/components/PawnComponent.hpp"
#include "systems/gizmoSystem/GizmoSystem.hpp"
#include "systems/hudSystem/components/HUDElementComponent.hpp"
#include "systems/hudSystem/components/HUDTextComponent.hpp"
#include "systems/hudSystem/HUDSystem.hpp"
#include "ecs/components/NameComponent.hpp"

using namespace engine;
using namespace engine::ecs;
using namespace engine::test;

BOOST_AUTO_TEST_SUITE(TacticalSceneManagerSystemTestSuite)

BOOST_AUTO_TEST_CASE(TacticalSceneManagerSystemRegistrationAndRetrievalTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalSceneManagerRegScene");
    BOOST_REQUIRE(scene != nullptr);

    auto managerSystem = scene->GetSystem<TacticalSceneManagerSystem>();
    BOOST_REQUIRE(managerSystem != nullptr);
    BOOST_CHECK(!managerSystem->HasSelectedEntity());
    BOOST_CHECK_EQUAL(managerSystem->GetSelectedEntity(), MAX_ENTITIES);
    BOOST_CHECK_EQUAL(managerSystem->GetSelectedEntityName(), "");
    BOOST_CHECK(!managerSystem->GetSelectedEntityHealth().has_value());

    engine.RemoveScene("TacticalSceneManagerRegScene");
}

BOOST_AUTO_TEST_CASE(TacticalSceneManagerSystemDirectSelectionAndLifecycleTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalSceneManagerLifecycleScene");
    BOOST_REQUIRE(scene != nullptr);

    auto managerSystem = scene->GetSystem<TacticalSceneManagerSystem>();
    BOOST_REQUIRE(managerSystem != nullptr);

    // Entity without PathfindingAgent
    Entity nonAgent = scene->CreateEntity("NonAgent");
    managerSystem->SetSelectedEntity(nonAgent);
    // Should be rejected by default because requirePathfindingAgent is true
    BOOST_CHECK(!managerSystem->HasSelectedEntity());
    BOOST_CHECK(!managerSystem->IsSelected(nonAgent));

    // Entity with PathfindingAgent
    Entity agentEntity = scene->CreateEntity("Agent");
    scene->AddComponent<PathfindingAgent>(agentEntity, PathfindingAgent{});
    managerSystem->SetSelectedEntity(agentEntity);
    BOOST_CHECK(managerSystem->HasSelectedEntity());
    BOOST_CHECK(managerSystem->IsSelected(agentEntity));
    BOOST_CHECK_EQUAL(managerSystem->GetSelectedEntity(), agentEntity);

    // Clear selection
    managerSystem->ClearSelection();
    BOOST_CHECK(!managerSystem->HasSelectedEntity());

    // Re-select and destroy entity
    managerSystem->SetSelectedEntity(agentEntity);
    BOOST_CHECK(managerSystem->HasSelectedEntity());
    scene->DestroyEntity(agentEntity);
    managerSystem->Update(0.016f);
    BOOST_CHECK(!managerSystem->HasSelectedEntity());

    engine.RemoveScene("TacticalSceneManagerLifecycleScene");
}

BOOST_AUTO_TEST_CASE(TacticalSceneManagerSystemAgentHealthAndNameTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("AgentHealthNameScene");
    BOOST_REQUIRE(scene != nullptr);

    auto managerSystem = scene->GetSystem<TacticalSceneManagerSystem>();
    BOOST_REQUIRE(managerSystem != nullptr);

    // Create tactical agent with explicit name and health via PawnComponent
    Entity soldier = scene->CreateEntity("DefaultSoldier");
    scene->AddComponent<PathfindingAgent>(soldier, PathfindingAgent{1, 6.0f, 5.0f});
    scene->AddComponent<PawnComponent>(soldier, PawnComponent{"Commander Shepherd", 80.0f, 100.0f});

    // Check PawnComponent data fields (pure data struct)
    auto& pawnComp = scene->GetComponent<PawnComponent>(soldier);
    BOOST_CHECK_EQUAL(pawnComp.name, "Commander Shepherd");
    BOOST_CHECK_CLOSE(pawnComp.currentHealth, 80.0f, 0.001f);
    BOOST_CHECK_CLOSE(pawnComp.maxHealth, 100.0f, 0.001f);

    pawnComp.currentHealth -= 10.0f;
    BOOST_CHECK_CLOSE(pawnComp.currentHealth, 70.0f, 0.001f);

    // Select the soldier
    managerSystem->SetSelectedEntity(soldier);
    BOOST_CHECK(managerSystem->HasSelectedEntity());

    // Test name and health queries when selected
    BOOST_CHECK_EQUAL(managerSystem->GetSelectedEntityName(), "Commander Shepherd");
    BOOST_REQUIRE(managerSystem->GetSelectedEntityHealth().has_value());
    BOOST_CHECK_CLOSE(managerSystem->GetSelectedEntityHealth().value(), 70.0f, 0.001f);
    BOOST_REQUIRE(managerSystem->GetSelectedEntityMaxHealth().has_value());
    BOOST_CHECK_CLOSE(managerSystem->GetSelectedEntityMaxHealth().value(), 100.0f, 0.001f);

    std::string outName;
    float outHealth = 0.0f, outMaxHealth = 0.0f;
    bool hasInfo = managerSystem->GetSelectedAgentInfo(outName, outHealth, outMaxHealth);
    BOOST_CHECK(hasInfo);
    BOOST_CHECK_EQUAL(outName, "Commander Shepherd");
    BOOST_CHECK_CLOSE(outHealth, 70.0f, 0.001f);
    BOOST_CHECK_CLOSE(outMaxHealth, 100.0f, 0.001f);

    auto pawnOpt = managerSystem->GetSelectedPawnComponent();
    BOOST_REQUIRE(pawnOpt.has_value());
    BOOST_CHECK_EQUAL(pawnOpt->name, "Commander Shepherd");
    BOOST_CHECK_CLOSE(pawnOpt->currentHealth, 70.0f, 0.001f);

    // Deselect and verify getters return empty/nullopt
    managerSystem->ClearSelection();
    BOOST_CHECK_EQUAL(managerSystem->GetSelectedEntityName(), "");
    BOOST_CHECK(!managerSystem->GetSelectedEntityHealth().has_value());
    BOOST_CHECK(!managerSystem->GetSelectedEntityMaxHealth().has_value());
    BOOST_CHECK(!managerSystem->GetSelectedPawnComponent().has_value());
    BOOST_CHECK(!managerSystem->GetSelectedAgentInfo(outName, outHealth, outMaxHealth));

    engine.RemoveScene("AgentHealthNameScene");
}

BOOST_AUTO_TEST_CASE(TacticalSceneManagerSystemLeftClickSelectionRaycastTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalClickScene");
    BOOST_REQUIRE(scene != nullptr);

    auto managerSystem = scene->GetSystem<TacticalSceneManagerSystem>();
    BOOST_REQUIRE(managerSystem != nullptr);

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

    // Setup Agent Entity located at (0, 0, 0) with Name and Health
    Entity agentEntity = scene->CreateEntity("AgentUnit");
    TransformComponent agentTrans;
    agentTrans.position = glm::vec3(0.0f, 0.0f, 0.0f);
    agentTrans.globalMatrix = glm::translate(glm::mat4(1.0f), agentTrans.position);
    scene->AddComponent<TransformComponent>(agentEntity, agentTrans);

    PathfindingAgent agentComp;
    agentComp.agentSize = 1;
    scene->AddComponent<PathfindingAgent>(agentEntity, agentComp);
    scene->AddComponent<PawnComponent>(agentEntity, PawnComponent{"AgentUnit", 100.0f, 100.0f});

    // Left click at center of screen (400, 300) on 800x600 window
    bool selected = managerSystem->HandleLeftClick(400.0f, 300.0f, 800.0f, 600.0f);
    BOOST_CHECK(selected);
    BOOST_CHECK(managerSystem->HasSelectedEntity());
    BOOST_CHECK_EQUAL(managerSystem->GetSelectedEntity(), agentEntity);
    BOOST_CHECK_EQUAL(managerSystem->GetSelectedEntityName(), "AgentUnit");
    BOOST_CHECK_CLOSE(managerSystem->GetSelectedEntityHealth().value(), 100.0f, 0.001f);

    // Left click far off at top-left corner (10, 10) should miss and clear selection
    bool missed = managerSystem->HandleLeftClick(10.0f, 10.0f, 800.0f, 600.0f);
    BOOST_CHECK(!missed);
    BOOST_CHECK(!managerSystem->HasSelectedEntity());

    engine.RemoveScene("TacticalClickScene");
}

BOOST_AUTO_TEST_CASE(TacticalSceneManagerSystemRightClickMoveToGridPlacementTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalMoveScene");
    BOOST_REQUIRE(scene != nullptr);

    auto managerSystem = scene->GetSystem<TacticalSceneManagerSystem>();
    auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
    BOOST_REQUIRE(managerSystem != nullptr);
    BOOST_REQUIRE(pathfindingSystem != nullptr);

    pathfindingSystem->SetCellSize(1.0f);

    // Setup active camera positioned at (0, 10, 0) looking down at (0, 0, 0)
    Entity camEntity = scene->CreateEntity("TopDownCamera");
    TransformComponent camTrans;
    camTrans.position = glm::vec3(0.0f, 10.0f, 0.0f);
    camTrans.globalMatrix = glm::lookAt(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    camTrans.globalMatrix = glm::inverse(camTrans.globalMatrix); // World matrix
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

    // Setup 10x10 Grid fragment at (0, 0, 0)
    Entity gridEntity = scene->CreateEntity("WalkableFloor");
    TransformComponent gridTrans;
    gridTrans.position = glm::vec3(0.0f, 0.0f, 0.0f);
    scene->AddComponent<TransformComponent>(gridEntity, gridTrans);
    GridFragment frag(10, 10);
    scene->AddComponent<GridFragment>(gridEntity, frag);
    pathfindingSystem->BuildNavGrid();

    // Setup Agent Entity
    Entity agentEntity = scene->CreateEntity("Scout");
    TransformComponent agentTrans;
    agentTrans.position = glm::vec3(-2.0f, 0.0f, -2.0f);
    scene->AddComponent<TransformComponent>(agentEntity, agentTrans);
    PathfindingAgent agentComp(1, 10.0f);
    scene->AddComponent<PathfindingAgent>(agentEntity, agentComp);
    scene->AddComponent<PawnComponent>(agentEntity, PawnComponent{"Scout", 50.0f, 50.0f});

    // Select the agent
    managerSystem->SetSelectedEntity(agentEntity);
    BOOST_CHECK(managerSystem->HasSelectedEntity());

    // Right click directly via world pos
    bool movedDirect = managerSystem->MoveSelectedToWorldPos(glm::vec3(2.0f, 0.0f, 2.0f));
    BOOST_CHECK(movedDirect);

    auto& agentAfter = scene->GetComponent<PathfindingAgent>(agentEntity);
    BOOST_CHECK(agentAfter.hasTarget);

    // Right click at screen center (400, 300) should project to ground center (0, 0, 0)
    bool movedClick = managerSystem->HandleRightClick(400.0f, 300.0f, 800.0f, 600.0f);
    BOOST_CHECK(movedClick);

    engine.RemoveScene("TacticalMoveScene");
}

BOOST_AUTO_TEST_CASE(TacticalSceneManagerSystemUpdateAndGizmoTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalGizmoScene");
    BOOST_REQUIRE(scene != nullptr);

    auto managerSystem = scene->GetSystem<TacticalSceneManagerSystem>();
    auto gizmoSystem = scene->GetSystem<GizmoSystem>();
    BOOST_REQUIRE(managerSystem != nullptr);
    BOOST_REQUIRE(gizmoSystem != nullptr);

    Entity agentEntity = scene->CreateEntity("GizmoAgent");
    TransformComponent agentTrans;
    agentTrans.position = glm::vec3(1.0f, 0.0f, 1.0f);
    scene->AddComponent<TransformComponent>(agentEntity, agentTrans);
    PathfindingAgent agentComp(1);
    scene->AddComponent<PathfindingAgent>(agentEntity, agentComp);
    scene->AddComponent<PawnComponent>(agentEntity, PawnComponent{"GizmoAgent", 75.0f, 100.0f});

    managerSystem->SetSelectedEntity(agentEntity);
    managerSystem->showSelectionGizmo = true;
    managerSystem->showHealthGizmo = true;

    size_t prevCommands = gizmoSystem->gizmoRenderCommandQueue.size();
    managerSystem->Update(0.016f);
    // Selection box and health bar gizmo cubes should have been queued
    BOOST_CHECK(gizmoSystem->gizmoRenderCommandQueue.size() >= prevCommands + 2);

    // Disable gizmos
    managerSystem->showSelectionGizmo = false;
    managerSystem->showHealthGizmo = false;
    gizmoSystem->gizmoRenderCommandQueue.clear();
    managerSystem->Update(0.016f);
    BOOST_CHECK_EQUAL(gizmoSystem->gizmoRenderCommandQueue.size(), 0);

    engine.RemoveScene("TacticalGizmoScene");
}

BOOST_AUTO_TEST_CASE(TacticalSceneManagerSystemHoverTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalHoverScene");
    BOOST_REQUIRE(scene != nullptr);

    auto managerSystem = scene->GetSystem<TacticalSceneManagerSystem>();
    BOOST_REQUIRE(managerSystem != nullptr);

    // When nothing is selected, hover returns false
    bool hoveredNoSel = managerSystem->HandleMouseMove(400.0f, 300.0f, 800.0f, 600.0f);
    BOOST_CHECK(!hoveredNoSel);

    Entity agentEntity = scene->CreateEntity("HoverAgent");
    scene->AddComponent<PathfindingAgent>(agentEntity, PathfindingAgent{});
    managerSystem->SetSelectedEntity(agentEntity);

    // Setup active camera positioned at (0, 10, 0) looking down at (0, 0, 0)
    Entity camEntity = scene->CreateEntity("HoverCamera");
    TransformComponent camTrans;
    camTrans.position = glm::vec3(0.0f, 10.0f, 0.0f);
    camTrans.globalMatrix = glm::lookAt(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    camTrans.globalMatrix = glm::inverse(camTrans.globalMatrix);
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

    bool hovered = managerSystem->HandleMouseMove(400.0f, 300.0f, 800.0f, 600.0f);
    BOOST_CHECK(hovered);

    engine.RemoveScene("TacticalHoverScene");
}

BOOST_AUTO_TEST_CASE(TacticalSceneManagerSystemPawnHUDTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalHUDScene");
    BOOST_REQUIRE(scene != nullptr);

    auto managerSystem = scene->GetSystem<TacticalSceneManagerSystem>();
    BOOST_REQUIRE(managerSystem != nullptr);

    // 1. Create HUD entities for selected pawn
    managerSystem->CreateSelectedPawnHUDEntities(glm::vec2(200.0f, 20.0f), glm::vec2(-100.0f, -60.0f), UIAnchor::BottomCenter);
    BOOST_REQUIRE(managerSystem->hudHealthBackgroundEntity != MAX_ENTITIES);
    BOOST_REQUIRE(managerSystem->hudHealthFillEntity != MAX_ENTITIES);
    BOOST_REQUIRE(managerSystem->hudNameTextEntity != MAX_ENTITIES);

    BOOST_CHECK(scene->HasComponent<HUDElementComponent>(managerSystem->hudHealthBackgroundEntity));
    BOOST_CHECK(scene->HasComponent<HUDElementComponent>(managerSystem->hudHealthFillEntity));
    BOOST_CHECK(scene->HasComponent<HUDTextComponent>(managerSystem->hudNameTextEntity));

    auto& bgComp = scene->GetComponent<HUDElementComponent>(managerSystem->hudHealthBackgroundEntity);
    auto& fillComp = scene->GetComponent<HUDElementComponent>(managerSystem->hudHealthFillEntity);
    auto& textComp = scene->GetComponent<HUDTextComponent>(managerSystem->hudNameTextEntity);

    // Initial state: not visible when no pawn is selected
    BOOST_CHECK(!bgComp.isVisible);
    BOOST_CHECK(!fillComp.isVisible);
    BOOST_CHECK(!textComp.isVisible);

    // 2. Create pawn entity and select it
    Entity soldier = scene->CreateEntity("TacticalSoldier");
    scene->AddComponent<PathfindingAgent>(soldier, PathfindingAgent{1});
    scene->AddComponent<PawnComponent>(soldier, PawnComponent{"Soldier 76", 100.0f, 100.0f});

    managerSystem->SetSelectedEntity(soldier);
    BOOST_CHECK(managerSystem->HasSelectedEntity());

    // When selected with 100% health, HUD elements and name text should become visible
    BOOST_CHECK(bgComp.isVisible);
    BOOST_CHECK(fillComp.isVisible);
    BOOST_CHECK(textComp.isVisible);
    BOOST_CHECK_EQUAL(textComp.text, "Soldier 76");
    BOOST_CHECK_CLOSE(fillComp.size.x, 200.0f, 0.001f);
    BOOST_CHECK_CLOSE(fillComp.color.g, 1.0f, 0.001f); // Green
    BOOST_CHECK_CLOSE(fillComp.color.r, 0.0f, 0.001f);

    // 3. Modify health to 50% and update HUD
    auto& pawn = scene->GetComponent<PawnComponent>(soldier);
    pawn.currentHealth = 50.0f;
    managerSystem->Update(0.016f);

    BOOST_CHECK(bgComp.isVisible);
    BOOST_CHECK(fillComp.isVisible);
    BOOST_CHECK(textComp.isVisible);
    BOOST_CHECK_CLOSE(fillComp.size.x, 100.0f, 0.001f); // 50% of 200.0f = 100.0f
    BOOST_CHECK_CLOSE(fillComp.color.r, 0.5f, 0.001f);
    BOOST_CHECK_CLOSE(fillComp.color.g, 0.5f, 0.001f);

    // 4. Modify pawn name and update HUD
    pawn.name = "Commander 76";
    managerSystem->UpdatePawnHUD();
    BOOST_CHECK_EQUAL(textComp.text, "Commander 76");

    // 5. Modify health to 0%
    pawn.currentHealth = 0.0f;
    managerSystem->UpdatePawnHUD();
    BOOST_CHECK_CLOSE(fillComp.size.x, 0.0f, 0.001f);
    BOOST_CHECK_CLOSE(fillComp.color.r, 1.0f, 0.001f); // Red
    BOOST_CHECK_CLOSE(fillComp.color.g, 0.0f, 0.001f);

    // 6. Clear selection -> HUD elements should become invisible
    managerSystem->ClearSelection();
    BOOST_CHECK(!bgComp.isVisible);
    BOOST_CHECK(!fillComp.isVisible);
    BOOST_CHECK(!textComp.isVisible);

    // 7. Reselect and disable showPawnHUD
    managerSystem->SetSelectedEntity(soldier);
    BOOST_CHECK(bgComp.isVisible);
    BOOST_CHECK(fillComp.isVisible);
    BOOST_CHECK(textComp.isVisible);

    managerSystem->showPawnHUD = false;
    managerSystem->UpdatePawnHUD();
    BOOST_CHECK(!bgComp.isVisible);
    BOOST_CHECK(!fillComp.isVisible);
    BOOST_CHECK(!textComp.isVisible);

    // 8. Test DestroySelectedPawnHUDEntities
    managerSystem->DestroySelectedPawnHUDEntities();
    BOOST_CHECK_EQUAL(managerSystem->hudHealthBackgroundEntity, MAX_ENTITIES);
    BOOST_CHECK_EQUAL(managerSystem->hudHealthFillEntity, MAX_ENTITIES);
    BOOST_CHECK_EQUAL(managerSystem->hudNameTextEntity, MAX_ENTITIES);

    engine.RemoveScene("TacticalHUDScene");
}

BOOST_AUTO_TEST_CASE(TacticalCameraMovementPanningAndSpeedTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalCameraPanScene");
    BOOST_REQUIRE(scene != nullptr);

    auto manager = scene->GetSystem<TacticalSceneManagerSystem>();
    BOOST_REQUIRE(manager != nullptr);

    manager->enableSmoothing = false;
    manager->SetCameraYaw(0.0f, true);
    manager->SetCameraTarget(glm::vec3(0.0f, 0.0f, 0.0f), true);
    manager->panSpeed = 10.0f;
    manager->fastPanMultiplier = 2.0f;

    // Yaw 0: Forward is (0, 0, -1), Right is (1, 0, 0)
    // 1. Pan forward (0, 1) for 1 second -> delta = (0, 0, -10)
    manager->PanCamera(glm::vec2(0.0f, 1.0f), 1.0f, false);
    glm::vec3 target = manager->GetCameraTarget();
    BOOST_CHECK_CLOSE(target.x, 0.0f, 0.01f);
    BOOST_CHECK_CLOSE(target.y, 0.0f, 0.01f);
    BOOST_CHECK_CLOSE(target.z, -10.0f, 0.01f);

    // 2. Pan right (1, 0) for 1 second with fast multiplier (2x) -> delta = (20, 0, 0)
    manager->PanCamera(glm::vec2(1.0f, 0.0f), 1.0f, true);
    target = manager->GetCameraTarget();
    BOOST_CHECK_CLOSE(target.x, 20.0f, 0.01f);
    BOOST_CHECK_CLOSE(target.y, 0.0f, 0.01f);
    BOOST_CHECK_CLOSE(target.z, -10.0f, 0.01f);

    // 3. Pan by pixels (e.g. mouse drag)
    manager->SetCameraDistance(10.0f, true);
    glm::vec3 preDrag = manager->GetCameraTarget();
    manager->PanCameraByPixels(100.0f, 50.0f);
    glm::vec3 postDrag = manager->GetCameraTarget();
    BOOST_CHECK(postDrag.x < preDrag.x); // Dragging right moves target left
    BOOST_CHECK(postDrag.z < preDrag.z); // Dragging down moves target forward (-z)

    engine.RemoveScene("TacticalCameraPanScene");
}

BOOST_AUTO_TEST_CASE(TacticalCameraRotationAndZoomTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalCameraRotZoomScene");
    BOOST_REQUIRE(scene != nullptr);

    auto manager = scene->GetSystem<TacticalSceneManagerSystem>();
    BOOST_REQUIRE(manager != nullptr);

    manager->enableSmoothing = false;
    manager->SetCameraYaw(0.0f, true);
    manager->SetCameraDistance(15.0f, true);
    manager->minCameraDistance = 5.0f;
    manager->maxCameraDistance = 30.0f;
    manager->minPitch = 20.0f;
    manager->maxPitch = 70.0f;

    // 1. Rotation snapping (Q and E step rotations)
    manager->RotateCameraRight(45.0f);
    BOOST_CHECK_CLOSE(manager->GetCameraYaw(), 45.0f, 0.01f);

    manager->RotateCameraLeft(90.0f);
    BOOST_CHECK_CLOSE(manager->GetCameraYaw(), -45.0f, 0.01f);

    // 2. Zoom controls & clamping
    manager->ZoomIn(5.0f); // 15 - 5 = 10
    BOOST_CHECK_CLOSE(manager->GetCameraDistance(), 10.0f, 0.01f);

    manager->ZoomIn(100.0f); // Should clamp to minCameraDistance (5.0f)
    BOOST_CHECK_CLOSE(manager->GetCameraDistance(), 5.0f, 0.01f);

    manager->ZoomOut(100.0f); // Should clamp to maxCameraDistance (30.0f)
    BOOST_CHECK_CLOSE(manager->GetCameraDistance(), 30.0f, 0.01f);

    // 3. Pitch clamping
    manager->SetCameraPitch(10.0f); // Clamped to minPitch (20.0f)
    BOOST_CHECK_CLOSE(manager->GetCameraPitch(), 20.0f, 0.01f);

    manager->SetCameraPitch(85.0f); // Clamped to maxPitch (70.0f)
    BOOST_CHECK_CLOSE(manager->GetCameraPitch(), 70.0f, 0.01f);

    engine.RemoveScene("TacticalCameraRotZoomScene");
}

BOOST_AUTO_TEST_CASE(TacticalCameraFocusAndActiveCameraSyncTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalCameraFocusScene");
    BOOST_REQUIRE(scene != nullptr);

    auto manager = scene->GetSystem<TacticalSceneManagerSystem>();
    BOOST_REQUIRE(manager != nullptr);

    // Create an active camera in the scene
    Entity camEntity = scene->CreateEntity("TacticalGameCamera");
    CameraComponent camComp;
    camComp.active = true;
    camComp.fov = 60.0f;
    TransformComponent camTrans;
    scene->AddComponent<CameraComponent>(camEntity, camComp);
    scene->AddComponent<TransformComponent>(camEntity, camTrans);

    // Create a pawn
    Entity soldier = scene->CreateEntity("SoldierPawn");
    TransformComponent soldierTrans;
    soldierTrans.position = glm::vec3(12.0f, 0.0f, -8.0f);
    computeLocalMatrix(soldierTrans);
    scene->AddComponent<TransformComponent>(soldier, soldierTrans);
    scene->AddComponent<PathfindingAgent>(soldier, PathfindingAgent{});
    scene->AddComponent<PawnComponent>(soldier, PawnComponent{"Soldier", 100.0f, 100.0f});

    manager->enableSmoothing = false;
    manager->SetSelectedEntity(soldier);
    BOOST_REQUIRE(manager->HasSelectedEntity());

    // Focus on selected pawn
    manager->FocusOnSelected(true);
    BOOST_CHECK_CLOSE(manager->GetCameraTarget().x, 12.0f, 0.01f);
    BOOST_CHECK_CLOSE(manager->GetCameraTarget().y, 0.0f, 0.01f);
    BOOST_CHECK_CLOSE(manager->GetCameraTarget().z, -8.0f, 0.01f);

    // Set camera distance and pitch/yaw
    manager->SetCameraDistance(10.0f, true);
    manager->SetCameraPitch(45.0f);
    manager->SetCameraYaw(0.0f, true);

    // Run system update -> should sync transform to active camera entity
    manager->Update(0.016f);

    const auto& updatedTrans = scene->GetComponent<TransformComponent>(camEntity);
    // At yaw 0, pitch 45 deg, dist 10:
    // pos = target + (0, 10*sin(45), 10*cos(45)) = (12, 7.071, -8 + 7.071) = (12, 7.071, -0.929)
    float expectedY = 10.0f * std::sin(glm::radians(45.0f));
    float expectedZ = -8.0f + 10.0f * std::cos(glm::radians(45.0f));

    BOOST_CHECK_CLOSE(updatedTrans.position.x, 12.0f, 0.01f);
    BOOST_CHECK_CLOSE(updatedTrans.position.y, expectedY, 0.01f);
    BOOST_CHECK_CLOSE(updatedTrans.position.z, expectedZ, 0.01f);

    // Test smoothing update
    manager->enableSmoothing = true;
    manager->smoothingSpeed = 10.0f;
    manager->SetCameraTarget(glm::vec3(0.0f), false); // Sets targetCameraTarget without snapping
    BOOST_CHECK_CLOSE(manager->GetCameraTarget().x, 12.0f, 0.01f); // Initially still at 12

    manager->Update(0.02f); // Advance small time step -> should interpolate partway towards (0, 0, 0)
    BOOST_CHECK(manager->GetCameraTarget().x < 12.0f);
    BOOST_CHECK(manager->GetCameraTarget().x > 0.0f);

    engine.RemoveScene("TacticalCameraFocusScene");
}

BOOST_AUTO_TEST_CASE(TacticalSceneManagerSystemShootActionAndUnitTargetingTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalShootTargetingScene");
    BOOST_REQUIRE(scene != nullptr);

    auto manager = scene->GetSystem<TacticalSceneManagerSystem>();
    BOOST_REQUIRE(manager != nullptr);

    // 1. Create a Player unit
    Entity playerUnit = scene->CreateEntity("PlayerRanger");
    TransformComponent playerTrans;
    playerTrans.position = glm::vec3(0.0f, 0.0f, 0.0f);
    scene->AddComponent<TransformComponent>(playerUnit, playerTrans);
    scene->AddComponent<PathfindingAgent>(playerUnit, PathfindingAgent{1});
    PawnComponent playerPawn{"Ranger", 100.0f, 100.0f, PawnAlliance::Player};
    playerPawn.attackDamage = 35.0f;
    scene->AddComponent<PawnComponent>(playerUnit, playerPawn);

    // 2. Create an Enemy unit
    Entity enemyUnit = scene->CreateEntity("AlienSectoid");
    TransformComponent enemyTrans;
    enemyTrans.position = glm::vec3(5.0f, 0.0f, 5.0f);
    scene->AddComponent<TransformComponent>(enemyUnit, enemyTrans);
    scene->AddComponent<PathfindingAgent>(enemyUnit, PathfindingAgent{1});
    PawnComponent enemyPawn{"Sectoid", 50.0f, 50.0f, PawnAlliance::Enemy};
    scene->AddComponent<PawnComponent>(enemyUnit, enemyPawn);

    // 3. Create a Neutral unit
    Entity neutralUnit = scene->CreateEntity("Civilian");
    TransformComponent neutralTrans;
    neutralTrans.position = glm::vec3(-3.0f, 0.0f, 2.0f);
    scene->AddComponent<TransformComponent>(neutralUnit, neutralTrans);
    scene->AddComponent<PathfindingAgent>(neutralUnit, PathfindingAgent{1});
    PawnComponent neutralPawn{"Civilian", 20.0f, 20.0f, PawnAlliance::Neutral};
    scene->AddComponent<PawnComponent>(neutralUnit, neutralPawn);

    // Verify all units retrieval
    auto allUnits = manager->GetAllTacticalUnits();
    BOOST_CHECK_EQUAL(allUnits.size(), 3);

    // Select Player unit
    manager->SetSelectedEntity(playerUnit);
    BOOST_REQUIRE(manager->HasSelectedEntity());
    BOOST_CHECK(!manager->IsShootModeActive());

    // Targetable units from player's perspective (excludes self)
    auto targetable = manager->GetTargetableUnits();
    BOOST_CHECK_EQUAL(targetable.size(), 2);

    auto targetableEnemies = manager->GetTargetableUnits(PawnAlliance::Enemy);
    BOOST_CHECK_EQUAL(targetableEnemies.size(), 1);
    BOOST_CHECK_EQUAL(targetableEnemies[0], enemyUnit);

    // Toggle Shoot Mode -> should activate and auto-select enemy unit
    manager->ToggleShootMode();
    BOOST_CHECK(manager->IsShootModeActive());
    BOOST_CHECK_EQUAL(manager->GetTargetUnit(), enemyUnit);

    // Cycle targets
    manager->CycleNextTarget();
    BOOST_CHECK(manager->GetTargetUnit() != MAX_ENTITIES);
    BOOST_CHECK(manager->GetTargetUnit() == neutralUnit || manager->GetTargetUnit() == enemyUnit);

    manager->SelectTargetUnit(enemyUnit);
    BOOST_CHECK_EQUAL(manager->GetTargetUnit(), enemyUnit);

    // Execute Shoot on selected enemy target
    bool shotResult = manager->ExecuteShoot();
    BOOST_CHECK(shotResult);

    const auto& updatedEnemy = scene->GetComponent<PawnComponent>(enemyUnit);
    // 50 max HP - 35 attack damage = 15 HP remaining
    BOOST_CHECK_CLOSE(updatedEnemy.currentHealth, 15.0f, 0.01f);
    BOOST_CHECK(manager->lastShotFeedbackTimer > 0.0f);

    // Shoot again to eliminate / clamp at 0 HP
    manager->ExecuteShoot(enemyUnit);
    const auto& deadEnemy = scene->GetComponent<PawnComponent>(enemyUnit);
    BOOST_CHECK_CLOSE(deadEnemy.currentHealth, 0.0f, 0.01f);

    // Exit Shoot Mode
    manager->SetShootMode(false);
    BOOST_CHECK(!manager->IsShootModeActive());

    engine.RemoveScene("TacticalShootTargetingScene");
}

BOOST_AUTO_TEST_CASE(TacticalSceneManagerSystemHUDSystemIntegrationTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalHUDSystemIntegrationScene");
    BOOST_REQUIRE(scene != nullptr);

    auto manager = scene->GetSystem<TacticalSceneManagerSystem>();
    BOOST_REQUIRE(manager != nullptr);

    // 1. Create HUD entities using the HUD system
    manager->CreateSelectedPawnHUDEntities(glm::vec2(240.0f, 20.0f), glm::vec2(-260.0f, -40.0f), UIAnchor::BottomRight);

    BOOST_REQUIRE(manager->hudHealthBackgroundEntity != MAX_ENTITIES);
    BOOST_REQUIRE(manager->hudHealthFillEntity != MAX_ENTITIES);
    BOOST_REQUIRE(manager->hudNameTextEntity != MAX_ENTITIES);
    BOOST_REQUIRE(manager->hudStatsTextEntity != MAX_ENTITIES);
    BOOST_REQUIRE(manager->hudActionButtonEntity != MAX_ENTITIES);
    BOOST_REQUIRE(manager->hudActionTextEntity != MAX_ENTITIES);
    BOOST_REQUIRE(manager->hudTargetHeaderEntity != MAX_ENTITIES);
    BOOST_REQUIRE(manager->hudFeedbackTextEntity != MAX_ENTITIES);
    BOOST_CHECK(!manager->hudTargetCards.empty());

    // Initially all HUD components should be hidden (no pawn selected)
    auto& actBg = scene->GetComponent<HUDElementComponent>(manager->hudActionButtonEntity);
    auto& actText = scene->GetComponent<HUDTextComponent>(manager->hudActionTextEntity);
    auto& targetHeader = scene->GetComponent<HUDTextComponent>(manager->hudTargetHeaderEntity);
    auto& feedbackText = scene->GetComponent<HUDTextComponent>(manager->hudFeedbackTextEntity);

    BOOST_CHECK(!actBg.isVisible);
    BOOST_CHECK(!actText.isVisible);
    BOOST_CHECK(!targetHeader.isVisible);
    BOOST_CHECK(!feedbackText.isVisible);

    // 2. Create units
    Entity ranger = scene->CreateEntity("RangerUnit");
    scene->AddComponent<PathfindingAgent>(ranger, PathfindingAgent{1});
    PawnComponent rangerPawn{"Ranger", 100.0f, 100.0f, PawnAlliance::Player};
    rangerPawn.attackDamage = 30.0f;
    scene->AddComponent<PawnComponent>(ranger, rangerPawn);

    Entity alien = scene->CreateEntity("AlienUnit");
    scene->AddComponent<PathfindingAgent>(alien, PathfindingAgent{1});
    PawnComponent alienPawn{"Alien", 60.0f, 60.0f, PawnAlliance::Enemy};
    scene->AddComponent<PawnComponent>(alien, alienPawn);

    // Select Ranger -> right-side HUD and middle shoot button become visible
    manager->SetSelectedEntity(ranger);
    BOOST_CHECK(manager->HasSelectedEntity());

    BOOST_CHECK(actBg.isVisible);
    BOOST_CHECK(actText.isVisible);
    BOOST_CHECK_EQUAL(actText.text, "SHOOT (1)");
    BOOST_CHECK(!targetHeader.isVisible);

    auto& statsComp = scene->GetComponent<HUDTextComponent>(manager->hudStatsTextEntity);
    BOOST_CHECK(statsComp.isVisible);

    // 3. Activate Shoot Mode -> action button changes to FIRE, target header and target cards become visible
    manager->ToggleShootMode();
    BOOST_CHECK(manager->IsShootModeActive());
    BOOST_CHECK_EQUAL(actText.text, "FIRE (Space)");
    BOOST_CHECK(targetHeader.isVisible);

    // Check target card 0 for Alien enemy
    BOOST_CHECK(manager->hudTargetCards[0].entityRef == alien);
    auto& card0Bg = scene->GetComponent<HUDElementComponent>(manager->hudTargetCards[0].bgEntity);
    auto& card0Text = scene->GetComponent<HUDTextComponent>(manager->hudTargetCards[0].textEntity);
    BOOST_CHECK(card0Bg.isVisible);
    BOOST_CHECK(card0Text.isVisible);

    // 4. Execute Shoot -> feedback banner becomes visible
    bool shotOk = manager->ExecuteShoot();
    BOOST_CHECK(shotOk);
    BOOST_CHECK(feedbackText.isVisible);
    BOOST_CHECK(!feedbackText.text.empty());

    // 5. Clear selection -> all HUD elements hidden
    manager->ClearSelection();
    BOOST_CHECK(!actBg.isVisible);
    BOOST_CHECK(!actText.isVisible);
    BOOST_CHECK(!targetHeader.isVisible);

    engine.RemoveScene("TacticalHUDSystemIntegrationScene");
}

BOOST_AUTO_TEST_CASE(TacticalSceneManagerSystemHUDClickEventTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TacticalHUDClickEventScene");
    BOOST_REQUIRE(scene != nullptr);

    auto manager = scene->GetSystem<TacticalSceneManagerSystem>();
    BOOST_REQUIRE(manager != nullptr);

    auto hudSystem = scene->GetSystem<HUDSystem>();
    BOOST_REQUIRE(hudSystem != nullptr);

    manager->CreateSelectedPawnHUDEntities(glm::vec2(240.0f, 20.0f), glm::vec2(-260.0f, -40.0f), UIAnchor::BottomRight);

    // Create units
    Entity ranger = scene->CreateEntity("RangerUnit");
    scene->AddComponent<PathfindingAgent>(ranger, PathfindingAgent{1});
    PawnComponent rangerPawn{"Ranger", 100.0f, 100.0f, PawnAlliance::Player};
    rangerPawn.attackDamage = 40.0f;
    scene->AddComponent<PawnComponent>(ranger, rangerPawn);

    Entity alien1 = scene->CreateEntity("AlienUnit1");
    scene->AddComponent<PathfindingAgent>(alien1, PathfindingAgent{1});
    PawnComponent alienPawn1{"Alien1", 80.0f, 80.0f, PawnAlliance::Enemy};
    scene->AddComponent<PawnComponent>(alien1, alienPawn1);

    Entity alien2 = scene->CreateEntity("AlienUnit2");
    scene->AddComponent<PathfindingAgent>(alien2, PathfindingAgent{1});
    PawnComponent alienPawn2{"Alien2", 60.0f, 60.0f, PawnAlliance::Enemy};
    scene->AddComponent<PawnComponent>(alien2, alienPawn2);

    // Select Ranger
    manager->SetSelectedEntity(ranger);
    BOOST_REQUIRE(manager->HasSelectedEntity());
    BOOST_CHECK(!manager->IsShootModeActive());

    // 1. Simulate clicking the Shoot Action Button via EventDispatcher
    scene->PublishEvent(events::HUDClickEvent{
        .entity = manager->hudActionButtonEntity,
        .mousePosition = glm::vec2(400.0f, 570.0f),
        .mouseButton = 0
    });

    // Shoot mode should now be toggled ON
    BOOST_CHECK(manager->IsShootModeActive());
    BOOST_CHECK_EQUAL(manager->GetTargetUnit(), alien1);

    // 2. Click target card 1 via HUDSystem::HandleClick at calculated screen position
    glm::vec2 windowSize(800.0f, 600.0f);
    auto& card1 = manager->hudTargetCards[1];
    BOOST_REQUIRE(card1.entityRef == alien2);
    auto& card1Bg = scene->GetComponent<HUDElementComponent>(card1.bgEntity);
    glm::vec2 card1Pos = hudSystem->CalculateAnchoredPosition(
        static_cast<UIAnchor>(card1Bg.anchor), card1Bg.offset, card1Bg.size, windowSize);

    bool handled = manager->HandleLeftClick(card1Pos.x + 10.0f, card1Pos.y + 10.0f, windowSize.x, windowSize.y);
    BOOST_CHECK(handled);
    BOOST_CHECK_EQUAL(manager->GetTargetUnit(), alien2);

    // 3. Click Shoot Action Button via HandleLeftClick to Execute Shot on alien2
    auto& actBg = scene->GetComponent<HUDElementComponent>(manager->hudActionButtonEntity);
    glm::vec2 actPos = hudSystem->CalculateAnchoredPosition(
        static_cast<UIAnchor>(actBg.anchor), actBg.offset, actBg.size, windowSize);

    handled = manager->HandleLeftClick(actPos.x + 20.0f, actPos.y + 10.0f, windowSize.x, windowSize.y);
    BOOST_CHECK(handled);

    // Alien2 took 40 damage: 80 - 40 = 40 HP
    auto& ap2 = scene->GetComponent<PawnComponent>(alien2);
    BOOST_CHECK_CLOSE(ap2.currentHealth, 20.0f, 0.001f); // 60 - 40 = 20 HP

    engine.RemoveScene("TacticalHUDClickEventScene");
}

BOOST_AUTO_TEST_SUITE_END()
