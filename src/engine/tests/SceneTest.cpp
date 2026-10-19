#include <boost/test/unit_test.hpp>
#include "../Engine.h"
#include "ecs/Scene.h"
#include "TestHelper.hpp"
#include "ecs/components/NameComponent.hpp"
#include "../ecs/components/TagComponent.hpp"
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

BOOST_AUTO_TEST_CASE(EntityHandleShorthandMethodsTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("EntityHandleTestScene");
    BOOST_REQUIRE(scene);

    // Create entity handle directly from Scene
    Entity parent = scene->CreateEntity("RootEntity");
    BOOST_REQUIRE(parent.IsValid());
    BOOST_REQUIRE(static_cast<bool>(parent));
    BOOST_CHECK_EQUAL(parent.GetName(), "RootEntity");
    BOOST_CHECK_EQUAL(parent.GetScene(), scene.get());

    // Rename via handle
    parent.SetName("CustomRoot");
    BOOST_CHECK_EQUAL(parent.GetName(), "CustomRoot");

    // Add component via handle
    parent.AddComponent<TagComponent>(TagComponent{"Hero"});
    BOOST_REQUIRE(parent.HasComponent<TagComponent>());
    BOOST_CHECK_EQUAL(parent.GetComponent<TagComponent>().tag, "Hero");

    // Active state of component
    BOOST_CHECK(parent.IsComponentActive<TagComponent>());
    parent.SetComponentActive<TagComponent>(false);
    BOOST_CHECK(!parent.IsComponentActive<TagComponent>());
    parent.SetComponentActive<TagComponent>(true);
    BOOST_CHECK(parent.IsComponentActive<TagComponent>());

    // Entity active state
    BOOST_CHECK(parent.IsActive());
    parent.SetActive(false);
    BOOST_CHECK(!parent.IsActive());
    parent.SetActive(true);
    BOOST_CHECK(parent.IsActive());

    // Create child entity and use hierarchy shorthands
    Entity child = scene->CreateEntity("ChildEntity");
    BOOST_REQUIRE(child.IsValid());
    BOOST_CHECK(!child.HasParent());

    child.SetParent(parent);
    BOOST_CHECK(child.HasParent());
    BOOST_CHECK_EQUAL(child.GetParent(), parent);

    auto children = parent.GetChildren();
    BOOST_REQUIRE_EQUAL(children.size(), 1);
    BOOST_CHECK_EQUAL(children[0], child);

    child.RemoveParent();
    BOOST_CHECK(!child.HasParent());
    BOOST_CHECK_EQUAL(parent.GetChildren().size(), 0);

    // Remove component via handle
    parent.RemoveComponent<TagComponent>();
    BOOST_CHECK(!parent.HasComponent<TagComponent>());

    // Destroy via handle
    Entity child2 = scene->CreateEntity("TempChild");
    BOOST_REQUIRE(child2.IsValid());
    child2.Destroy();

    engine.RemoveScene("EntityHandleTestScene");
}

BOOST_AUTO_TEST_CASE(ActiveMultiViewIterationAndFilteringTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("ActiveViewTestScene");
    BOOST_REQUIRE(scene);

    Entity e1 = scene->CreateEntity();
    e1.AddComponent<TagComponent>(TagComponent{"Tag1"});
    e1.GetComponent<NameComponent>().name = "Name1";
    auto& t1 = scene->GetComponent<TransformComponent>(e1);
    t1.position = glm::vec3(1.0f, 1.0f, 1.0f);

    Entity e2 = scene->CreateEntity();
    e2.AddComponent<TagComponent>(TagComponent{"Tag2"});
    scene->RemoveComponent<NameComponent>(e2);
    auto& t2 = scene->GetComponent<TransformComponent>(e2);
    t2.position = glm::vec3(2.0f, 2.0f, 2.0f);

    Entity e3 = scene->CreateEntity();
    e3.AddComponent<TagComponent>(TagComponent{"Tag3"});
    scene->RemoveComponent<NameComponent>(e3);
    scene->RemoveComponent<TransformComponent>(e3);

    Entity e4 = scene->CreateEntity();
    e4.AddComponent<TagComponent>(TagComponent{"Tag4"});
    scene->RemoveComponent<NameComponent>(e4);
    auto& t4 = scene->GetComponent<TransformComponent>(e4);
    t4.position = glm::vec3(4.0f, 4.0f, 4.0f);

    // 1. Single-component active view on standard component (TagComponent)
    std::vector<Entity> tagEntities;
    for (auto [entity, tag] : scene->GetActiveView<TagComponent>()) {
        tagEntities.push_back(entity);
        tag.tag += "_Modified";
    }
    BOOST_CHECK_EQUAL(tagEntities.size(), 4);
    BOOST_CHECK_EQUAL(scene->GetComponent<TagComponent>(e1).tag, "Tag1_Modified");

    // 2. Multi-component active view (TagComponent + NameComponent)
    std::vector<Entity> tagAndNameEntities;
    for (auto [entity, tag, name] : scene->GetActiveView<TagComponent, NameComponent>()) {
        tagAndNameEntities.push_back(entity);
        BOOST_CHECK_EQUAL(name.name, "Name1");
    }
    BOOST_CHECK_EQUAL(tagAndNameEntities.size(), 1);
    BOOST_CHECK_EQUAL(tagAndNameEntities[0], e1);

    // 3. Multi-component active view (TagComponent + TransformComponent)
    std::vector<Entity> multiEntities;
    for (auto [entity, tag, transform] : scene->GetActiveView<TagComponent, TransformComponent>()) {
        multiEntities.push_back(entity);
    }
    BOOST_CHECK_EQUAL(multiEntities.size(), 3);

    // 4. Test deactivating entity e4
    scene->SetEntityActive(e4, false);
    multiEntities.clear();
    for (auto [entity, tag, transform] : scene->GetActiveView<TagComponent, TransformComponent>()) {
        multiEntities.push_back(entity);
    }
    BOOST_CHECK_EQUAL(multiEntities.size(), 2);
    BOOST_CHECK(std::find(multiEntities.begin(), multiEntities.end(), e4) == multiEntities.end());

    // 5. Test deactivating component on e2
    scene->SetComponentActive<TagComponent>(e2, false);
    multiEntities.clear();
    for (auto [entity, tag, transform] : scene->GetActiveView<TagComponent, TransformComponent>()) {
        multiEntities.push_back(entity);
    }
    BOOST_CHECK_EQUAL(multiEntities.size(), 1);
    BOOST_CHECK_EQUAL(multiEntities[0], e1);

    // 6. Test deactivating secondary component on e1
    scene->SetComponentActive<TransformComponent>(e1, false);
    multiEntities.clear();
    for (auto [entity, tag, transform] : scene->GetActiveView<TagComponent, TransformComponent>()) {
        multiEntities.push_back(entity);
    }
    BOOST_CHECK_EQUAL(multiEntities.size(), 0);

    // Re-activate components
    scene->SetComponentActive<TransformComponent>(e1, true);
    scene->SetComponentActive<TagComponent>(e2, true);
    scene->SetEntityActive(e4, true);

    // 7. Test early break in loop
    int count = 0;
    for (auto [entity, tag, transform] : scene->GetActiveView<TagComponent, TransformComponent>()) {
        ++count;
        if (count == 1) break;
    }
    BOOST_CHECK_EQUAL(count, 1);

    // 8. Test Integral as primary component
    int integralCount = 0;
    for (auto [entity, transform] : scene->GetActiveView<TransformComponent>()) {
        ++integralCount;
    }
    BOOST_CHECK(integralCount >= 3);

    engine.RemoveScene("ActiveViewTestScene");
}

BOOST_AUTO_TEST_CASE(InvalidEntityBoundsTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("BoundsTestScene");
    BOOST_REQUIRE(scene);

    Entity invalidEntity;
    BOOST_CHECK_EQUAL(invalidEntity.GetID(), MAX_ENTITIES);
    BOOST_CHECK(!invalidEntity.IsValid());

    // Checking HasComponent on MAX_ENTITIES / invalid entities should safely return false without assertion failure
    BOOST_CHECK(!scene->HasComponent<TransformComponent>(MAX_ENTITIES));
    BOOST_CHECK(!scene->HasComponent<NameComponent>(MAX_ENTITIES));
    BOOST_CHECK(!scene->HasComponent<TagComponent>(MAX_ENTITIES));
    BOOST_CHECK(!scene->HasComponent<TransformComponent>(invalidEntity));
    BOOST_CHECK(!scene->HasComponent<NameComponent>(invalidEntity));
    BOOST_CHECK(!scene->HasComponent<TagComponent>(invalidEntity));

    // Active state checks
    BOOST_CHECK(!scene->IsEntityActive(MAX_ENTITIES));
    BOOST_CHECK(!scene->IsEntityActive(invalidEntity));
    BOOST_CHECK(!scene->IsComponentActive<TransformComponent>(MAX_ENTITIES));
    BOOST_CHECK(!scene->IsComponentActive<NameComponent>(MAX_ENTITIES));
    BOOST_CHECK(!scene->IsComponentActive<TagComponent>(MAX_ENTITIES));

    // Name checks
    BOOST_CHECK_EQUAL(scene->GetEntityName(MAX_ENTITIES), "Entity");
    BOOST_CHECK_EQUAL(scene->GetEntityName(invalidEntity), "Entity");

    // Safe operations on invalid entities should not crash
    scene->SetEntityActive(MAX_ENTITIES, true);
    scene->SetComponentActive<TransformComponent>(MAX_ENTITIES, true);
    scene->SetComponentActive<TagComponent>(MAX_ENTITIES, true);
    scene->SetEntityName(MAX_ENTITIES, "TestName");

    // Verify valid entity still works as expected
    Entity validEntity = scene->CreateEntity("ValidEntity");
    BOOST_CHECK(scene->HasComponent<TransformComponent>(validEntity));
    BOOST_CHECK(scene->HasComponent<NameComponent>(validEntity));
    BOOST_CHECK_EQUAL(scene->GetEntityName(validEntity), "ValidEntity");

    engine.RemoveScene("BoundsTestScene");
}

struct TestCustomStartSystem : public System<TestCustomStartSystem> {
    bool onStartCalled = false;
    explicit TestCustomStartSystem(Scene* scene) : System(scene) {}
    void Update(float /*deltaTime*/) override {}
    void OnStart() override {
        onStartCalled = true;
    }
};

BOOST_AUTO_TEST_CASE(SystemOnStartTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("OnStartTestScene");
    BOOST_REQUIRE(scene);

    auto customSystem = scene->RegisterSystem<TestCustomStartSystem>();
    BOOST_REQUIRE(customSystem);
    BOOST_CHECK(!customSystem->onStartCalled);

    scene->OnStart();
    BOOST_CHECK(customSystem->onStartCalled);

    engine.RemoveScene("OnStartTestScene");
}

BOOST_AUTO_TEST_CASE(EngineStartPlayModeInvokesOnStartTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("PlayModeTestScene");
    BOOST_REQUIRE(scene);
    engine.SetActiveScene("PlayModeTestScene", true);

    BOOST_CHECK(!engine.IsPlaying());
    engine.StartPlayMode();
    BOOST_CHECK(engine.IsPlaying());

    engine.StopPlayMode();
    BOOST_CHECK(!engine.IsPlaying());

    engine.RemoveScene("PlayModeTestScene");
}

BOOST_AUTO_TEST_CASE(ComponentRemovalSafetyAndCompactionTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("RemovalSafetyScene");
    BOOST_REQUIRE(scene);

    Entity e1 = scene->CreateEntity("Entity1");
    Entity e2 = scene->CreateEntity("Entity2");
    Entity e3 = scene->CreateEntity("Entity3");

    // Attempting to remove non-existent component should safely no-op
    scene->RemoveComponent<TagComponent>(e1);
    BOOST_CHECK(!scene->HasComponent<TagComponent>(e1));

    // Add TagComponent to all three entities
    scene->AddComponent<TagComponent>(e1, TagComponent{"Tag1"});
    scene->AddComponent<TagComponent>(e2, TagComponent{"Tag2"});
    scene->AddComponent<TagComponent>(e3, TagComponent{"Tag3"});

    scene->SetComponentActive<TagComponent>(e2, false);
    scene->SetComponentActive<TagComponent>(e3, true);

    // Remove middle entity's component (causes compaction: e3 moves to index of e2)
    scene->RemoveComponent<TagComponent>(e2);
    BOOST_CHECK(!scene->HasComponent<TagComponent>(e2));
    BOOST_CHECK(scene->HasComponent<TagComponent>(e1));
    BOOST_CHECK(scene->HasComponent<TagComponent>(e3));
    BOOST_CHECK_EQUAL(scene->GetComponent<TagComponent>(e1).tag, "Tag1");
    BOOST_CHECK_EQUAL(scene->GetComponent<TagComponent>(e3).tag, "Tag3");
    BOOST_CHECK(scene->IsComponentActive<TagComponent>(e3));

    // Repeated removal should safely no-op
    scene->RemoveComponent<TagComponent>(e2);

    engine.RemoveScene("RemovalSafetyScene");
}

BOOST_AUTO_TEST_SUITE_END()