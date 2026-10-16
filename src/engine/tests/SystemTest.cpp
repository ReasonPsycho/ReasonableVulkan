#include <boost/test/unit_test.hpp>
#include "../Engine.h"
#include "ecs/Scene.h"
#include "TestHelper.hpp"
#include "systems/transformSystem/TransformSystem.h"
#include "systems/transformSystem/componets/TransformComponent.hpp"

using namespace engine;
using namespace engine::ecs;
using namespace engine::test;

BOOST_AUTO_TEST_SUITE(SystemTestSuite)

BOOST_AUTO_TEST_CASE(SystemRegistrationAndUpdateTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("SystemTestScene");
    BOOST_REQUIRE(scene);

    auto transformSystem = scene->GetSystem<TransformSystem>();
    BOOST_REQUIRE(transformSystem);
    BOOST_REQUIRE_EQUAL(transformSystem->name, "TransformSystem");

    Entity e1 = scene->CreateEntity();
    Entity e2 = scene->CreateEntity();
    BOOST_REQUIRE(scene->HasComponent<TransformComponent>(e1));
    BOOST_REQUIRE(scene->HasComponent<TransformComponent>(e2));

    setLocalPosition(scene->GetComponent<TransformComponent>(e1), {10.0f, 20.0f, 0.0f});
    setLocalPosition(scene->GetComponent<TransformComponent>(e2), {30.0f, 40.0f, 0.0f});

    BOOST_REQUIRE(isDirty(scene->GetComponent<TransformComponent>(e1)));
    BOOST_REQUIRE(isDirty(scene->GetComponent<TransformComponent>(e2)));

    scene->Update(0.0f);

    BOOST_REQUIRE(!isDirty(scene->GetComponent<TransformComponent>(e1)));
    BOOST_REQUIRE(!isDirty(scene->GetComponent<TransformComponent>(e2)));

    const glm::vec3 pos1 = getGlobalPosition(scene->GetComponent<TransformComponent>(e1));
    const glm::vec3 pos2 = getGlobalPosition(scene->GetComponent<TransformComponent>(e2));
    BOOST_REQUIRE_CLOSE(pos1.x, 10.0f, 0.001f);
    BOOST_REQUIRE_CLOSE(pos1.y, 20.0f, 0.001f);
    BOOST_REQUIRE_CLOSE(pos2.x, 30.0f, 0.001f);
    BOOST_REQUIRE_CLOSE(pos2.y, 40.0f, 0.001f);

    engine.RemoveScene("SystemTestScene");
}

BOOST_AUTO_TEST_CASE(TransformSystemMatrixAndDirtyPropagationTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TransformScene");
    engine.SetActiveScene("TransformScene");

    // Register Transform component and system
    auto transformSystem = scene->GetSystem<TransformSystem>();
    BOOST_REQUIRE(transformSystem);

    // Create entities
    Entity parent = scene->CreateEntity();
    Entity child1 = scene->CreateEntity();
    Entity child2 = scene->CreateEntity();

    // Parent-child relationships
    scene->SetParent(child1, parent);
    scene->SetParent(child2, parent);

    // Set positions
    setLocalPosition(scene->GetComponent<TransformComponent>(parent), {10.0f, 0.0f, 0.0f});
    setLocalPosition(scene->GetComponent<TransformComponent>(child1), {0.0f, 5.0f, 0.0f});
    setLocalPosition(scene->GetComponent<TransformComponent>(child2), {0.0f, 0.0f, 3.0f});

    // Verify all are dirty before update
    BOOST_REQUIRE(isDirty(scene->GetComponent<TransformComponent>(parent)));
    BOOST_REQUIRE(isDirty(scene->GetComponent<TransformComponent>(child1)));
    BOOST_REQUIRE(isDirty(scene->GetComponent<TransformComponent>(child2)));

    // Update system
    scene->Update(0.0f);

    // Validate dirty flags cleared
    BOOST_REQUIRE(!isDirty(scene->GetComponent<TransformComponent>(parent)));
    BOOST_REQUIRE(!isDirty(scene->GetComponent<TransformComponent>(child1)));
    BOOST_REQUIRE(!isDirty(scene->GetComponent<TransformComponent>(child2)));

    // Validate world transforms
    const glm::vec3 worldPosParent = getGlobalPosition(scene->GetComponent<TransformComponent>(parent));
    const glm::vec3 worldPosChild1 = getGlobalPosition(scene->GetComponent<TransformComponent>(child1));
    const glm::vec3 worldPosChild2 = getGlobalPosition(scene->GetComponent<TransformComponent>(child2));

    BOOST_REQUIRE_CLOSE(worldPosParent.x, 10.0f, 0.001f);
    BOOST_REQUIRE_CLOSE(worldPosParent.y, 0.0f, 0.001f);
    BOOST_REQUIRE_CLOSE(worldPosParent.z, 0.0f, 0.001f);

    BOOST_REQUIRE_CLOSE(worldPosChild1.x, 10.0f, 0.001f);  // 10 + 0
    BOOST_REQUIRE_CLOSE(worldPosChild1.y, 5.0f, 0.001f);   // 0 + 5
    BOOST_REQUIRE_CLOSE(worldPosChild1.z, 0.0f, 0.001f);

    BOOST_REQUIRE_CLOSE(worldPosChild2.x, 10.0f, 0.001f);
    BOOST_REQUIRE_CLOSE(worldPosChild2.y, 0.0f, 0.001f);
    BOOST_REQUIRE_CLOSE(worldPosChild2.z, 3.0f, 0.001f);

    // Now mark only child1 dirty
    setLocalPosition(scene->GetComponent<TransformComponent>(child1), {0.0f, 10.0f, 0.0f});
    BOOST_REQUIRE(isDirty(scene->GetComponent<TransformComponent>(child1)));

    scene->Update(0.0f);
    BOOST_REQUIRE(!isDirty(scene->GetComponent<TransformComponent>(child1)));

    // Confirm new world position
    const glm::vec3 newWorldPosChild1 = getGlobalPosition(scene->GetComponent<TransformComponent>(child1));
    BOOST_REQUIRE_CLOSE(newWorldPosChild1.x, 10.0f, 0.001f);
    BOOST_REQUIRE_CLOSE(newWorldPosChild1.y, 10.0f, 0.001f);
    BOOST_REQUIRE_CLOSE(newWorldPosChild1.z, 0.0f, 0.001f);

    engine.RemoveScene("TransformScene");
}

BOOST_AUTO_TEST_SUITE_END()
