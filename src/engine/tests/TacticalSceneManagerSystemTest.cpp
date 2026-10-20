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

BOOST_AUTO_TEST_SUITE_END()
