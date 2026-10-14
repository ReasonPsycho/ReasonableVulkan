#include <boost/test/unit_test.hpp>
#include "../Engine.h"
#include "ecs/Scene.h"
#include "TestHelper.hpp"
#include "ecs/NameComponent.hpp"
#include "ecs/TagComponent.hpp"
#include "systems/transformSystem/componets/TransformComponent.hpp"
#include <algorithm>

using namespace engine;
using namespace engine::ecs;
using namespace engine::test;

BOOST_AUTO_TEST_SUITE(SceneTestSuite)

BOOST_AUTO_TEST_CASE(SceneLifecycleAndEntityTest) {
    Engine engine = CreateTestEngine();

    std::shared_ptr<Scene> scene = engine.CreateScene("SceneLifecycleTestScene");
    BOOST_REQUIRE(scene);
    BOOST_REQUIRE(engine.GetScene("SceneLifecycleTestScene") == scene);
    engine.SetActiveScene("SceneLifecycleTestScene");
    BOOST_REQUIRE(engine.GetActiveScene() == scene);

    // Entity creation & component manipulation
    Entity e1 = scene->CreateEntity();
    Entity e2 = scene->CreateEntity();
    BOOST_REQUIRE(!scene->HasComponent<TagComponent>(e1));

    scene->AddComponent<TagComponent>(e1, TagComponent{"EntityOne"});
    scene->AddComponent<TagComponent>(e2, TagComponent{"EntityTwo"});
    BOOST_REQUIRE(scene->HasComponent<TagComponent>(e1));
    BOOST_REQUIRE_EQUAL(scene->GetComponent<TagComponent>(e2).tag, "EntityTwo");
    BOOST_REQUIRE(scene->IsComponentActive<TagComponent>(e2));
    scene->SetComponentActive<TagComponent>(e2, false);
    BOOST_REQUIRE(!scene->IsComponentActive<TagComponent>(e2));
    scene->SetComponentActive<TagComponent>(e2, true);

    // Entity query
    auto matchingEntities = scene->GetEntitiesWith<TagComponent>();
    BOOST_REQUIRE_EQUAL(matchingEntities.size(), 2);

    // Component removal
    scene->RemoveComponent<TagComponent>(e2);
    BOOST_REQUIRE(!scene->HasComponent<TagComponent>(e2));

    // Entity active toggle
    scene->SetEntityActive(e1, false);
    BOOST_REQUIRE(!scene->IsEntityActive(e1));
    scene->SetEntityActive(e1, true);
    BOOST_REQUIRE(scene->IsEntityActive(e1));

    // Entity destruction
    scene->DestroyEntity(e1);
    BOOST_REQUIRE(!scene->HasComponent<TagComponent>(e1));

    engine.RemoveScene("SceneLifecycleTestScene");
    BOOST_REQUIRE(engine.GetScene("SceneLifecycleTestScene") == nullptr);
}

BOOST_AUTO_TEST_CASE(SceneHierarchyAndParentChildTest) {
    Engine engine = CreateTestEngine();
    auto testScene = engine.CreateScene("ParentChildTestScene");
    BOOST_REQUIRE(testScene);

    // Create root entity
    Entity parent = testScene->CreateEntity("ParentNode");
    BOOST_REQUIRE(parent != MAX_ENTITIES);
    BOOST_CHECK(!testScene->HasParent(parent));
    BOOST_CHECK_EQUAL(testScene->GetParent(parent), MAX_ENTITIES);

    // Create child directly using parent parameter
    Entity child1 = testScene->CreateEntity("ChildNode1", parent);
    BOOST_REQUIRE(child1 != MAX_ENTITIES);
    BOOST_CHECK(testScene->HasParent(child1));
    BOOST_CHECK_EQUAL(testScene->GetParent(child1), parent);

    TransformComponent childTransform;
    childTransform.position = glm::vec3(1.0f, 2.0f, 3.0f);
    Entity child2 = testScene->CreateEntity("ChildNode2", childTransform, parent);
    BOOST_REQUIRE(child2 != MAX_ENTITIES);
    BOOST_CHECK(testScene->HasParent(child2));
    BOOST_CHECK_EQUAL(testScene->GetParent(child2), parent);

    const auto& children = testScene->GetChildren(parent);
    BOOST_REQUIRE_EQUAL(children.size(), 2);
    BOOST_CHECK_EQUAL(children[0], child1);
    BOOST_CHECK_EQUAL(children[1], child2);

    // Hierarchy modification
    testScene->RemoveParent(child1);
    BOOST_REQUIRE(!testScene->HasParent(child1));
    BOOST_REQUIRE_EQUAL(testScene->GetChildren(parent).size(), 1);

    engine.RemoveScene("ParentChildTestScene");
}

BOOST_AUTO_TEST_CASE(ScenePrefabSerializationAndInstantiationTest) {
    Engine engine = CreateTestEngine();
    auto sourceScene = engine.CreateScene("PrefabSourceScene");
    BOOST_REQUIRE(sourceScene);

    Entity rootObj = sourceScene->CreateEntity("CarRoot");
    sourceScene->AddComponent<TagComponent>(rootObj, TagComponent{"Vehicle"});

    Entity wheel1 = sourceScene->CreateEntity("WheelFL", rootObj);
    sourceScene->AddComponent<TagComponent>(wheel1, TagComponent{"Wheel"});

    Entity wheel2 = sourceScene->CreateEntity("WheelFR", rootObj);
    sourceScene->AddComponent<TagComponent>(wheel2, TagComponent{"Wheel"});

    // Serialize object to prefab JSON document
    rapidjson::Document prefabDoc;
    sourceScene->SerializeObjectToJson(rootObj, prefabDoc);

    // Verify document structure: should have entities, components, sceneGraph, but NO systems
    BOOST_CHECK(prefabDoc.HasMember("entities"));
    BOOST_CHECK(prefabDoc.HasMember("components"));
    BOOST_CHECK(prefabDoc.HasMember("sceneGraph"));
    BOOST_CHECK(!prefabDoc.HasMember("systems"));

    // Check components in prefab doc
    BOOST_CHECK(prefabDoc["components"].HasMember("NameComponent"));
    BOOST_CHECK(prefabDoc["components"].HasMember("TagComponent"));
    BOOST_CHECK(prefabDoc["components"].HasMember("TransformComponent"));

    // Instantiate prefab into a new scene
    auto targetScene = engine.CreateScene("PrefabTargetScene");
    BOOST_REQUIRE(targetScene);

    Entity existingEntity = targetScene->CreateEntity("WorldAnchor");

    // Instantiate prefab as child of existingEntity
    Entity instantiatedRoot = targetScene->InstantiatePrefab(prefabDoc, existingEntity);
    BOOST_REQUIRE(instantiatedRoot != MAX_ENTITIES);
    BOOST_CHECK_EQUAL(targetScene->GetEntityName(instantiatedRoot), "CarRoot");
    BOOST_CHECK(targetScene->HasComponent<TagComponent>(instantiatedRoot));
    BOOST_CHECK_EQUAL(targetScene->GetComponent<TagComponent>(instantiatedRoot).tag, "Vehicle");
    BOOST_CHECK_EQUAL(targetScene->GetParent(instantiatedRoot), existingEntity);

    // Verify children were created and parented correctly
    const auto& children = targetScene->GetChildren(instantiatedRoot);
    BOOST_REQUIRE_EQUAL(children.size(), 2);

    std::vector<std::string> childNames;
    for (Entity child : children) {
        childNames.push_back(targetScene->GetEntityName(child));
        BOOST_CHECK(targetScene->HasComponent<TagComponent>(child));
        BOOST_CHECK_EQUAL(targetScene->GetComponent<TagComponent>(child).tag, "Wheel");
    }
    BOOST_CHECK(std::find(childNames.begin(), childNames.end(), "WheelFL") != childNames.end());
    BOOST_CHECK(std::find(childNames.begin(), childNames.end(), "WheelFR") != childNames.end());

    // Also test standalone instantiation (as root entity)
    Entity standaloneRoot = targetScene->InstantiatePrefab(prefabDoc);
    BOOST_REQUIRE(standaloneRoot != MAX_ENTITIES);
    BOOST_CHECK_EQUAL(targetScene->GetEntityName(standaloneRoot), "CarRoot");
    BOOST_CHECK(!targetScene->HasParent(standaloneRoot));

    engine.RemoveScene("PrefabSourceScene");
    engine.RemoveScene("PrefabTargetScene");
}

BOOST_AUTO_TEST_SUITE_END()