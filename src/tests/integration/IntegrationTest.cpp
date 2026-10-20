#define BOOST_TEST_MODULE IntegrationTests
#include <boost/test/unit_test.hpp>

#include "engine/Engine.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/components/NameComponent.hpp"
#include "../../engine/ecs/components/TagComponent.hpp"
#include "engine/systems/transformSystem/TransformSystem.h"
#include "engine/systems/transformSystem/componets/TransformComponent.hpp"
#include "engine/systems/collisionSystem/CollisionSystem.hpp"
#include "engine/systems/pathfindingSystem/PathfindingSystem.hpp"
#include "engine/systems/gizmoSystem/GizmoSystem.hpp"
#include "engine/systems/renderingSystem/componets/MeshComponent.hpp"
#include "engine/systems/renderingSystem/componets/RendererComponent.hpp"
#include "engine/systems/renderingSystem/componets/CameraComponent.hpp"
#include "platform/src/Platform.hpp"
#include "assetManager/src/AssetManager.hpp"
#include "renderDocManager/RenderDocManager.hpp"

#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace {

inline void CleanupTestTempDir() {
    std::error_code ec;
    std::filesystem::path tempDirs[] = {
        "res/.internal/tests/temp",
        "../res/.internal/tests/temp",
        "../../res/.internal/tests/temp"
    };
    for (const auto& dir : tempDirs) {
        if (std::filesystem::exists(dir, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
                std::filesystem::remove_all(entry.path(), ec);
            }
        }
    }
    am::AssetManager::getInstance().cleanupTestTempAssets();
}

struct TestTempCleanupObserver : public boost::unit_test::test_observer {
    void test_unit_finish(const boost::unit_test::test_unit& /*tu*/, unsigned long /*elapsed*/) override {
        CleanupTestTempDir();
    }
    void test_finish() override {
        CleanupTestTempDir();
    }
};

struct TestTempCleanupFixture {
    TestTempCleanupObserver observer;
    TestTempCleanupFixture() {
        boost::unit_test::framework::register_observer(observer);
        CleanupTestTempDir();
    }
    ~TestTempCleanupFixture() {
        boost::unit_test::framework::deregister_observer(observer);
        CleanupTestTempDir();
    }
};

BOOST_TEST_GLOBAL_FIXTURE(TestTempCleanupFixture);

} // anonymous namespace

using namespace engine;
using namespace engine::ecs;

BOOST_AUTO_TEST_SUITE(IntegrationTestSuite)

BOOST_AUTO_TEST_CASE(FullEngineAssetAndSceneIntegrationTest) {
    auto& assetManager = am::AssetManager::getInstance();
    plt::Platform platform;
    BOOST_REQUIRE(platform.Init("Integration Test Window", 800, 600));

    auto& renderDoc = rd::RenderDocManager::getInstance();
    BOOST_CHECK(!renderDoc.isCapturing());
    if (renderDoc.isApiAvailable()) {
        renderDoc.requestCapture(1, false);
        BOOST_CHECK(renderDoc.isCaptureRequested());
    }

    Engine engine(&platform, nullptr, &assetManager);

    std::string sceneName = "E2E_TestScene";
    auto scene = engine.CreateScene(sceneName);
    BOOST_REQUIRE(scene != nullptr);
    engine.SetActiveScene(sceneName, true);
    BOOST_CHECK_EQUAL(engine.GetActiveScene(), scene);

    // Create scene hierarchy
    Entity car = scene->CreateEntity("CarRoot");
    scene->AddComponent<TagComponent>(car, TagComponent{"Vehicle"});
    auto& carTrans = scene->GetComponent<TransformComponent>(car);
    carTrans.position = glm::vec3(0.0f, 1.0f, 0.0f);

    Entity wheelFL = scene->CreateEntity("WheelFL", car);
    scene->AddComponent<TagComponent>(wheelFL, TagComponent{"Wheel"});
    auto& wheelTrans = scene->GetComponent<TransformComponent>(wheelFL);
    wheelTrans.position = glm::vec3(-1.0f, -0.5f, 1.5f);

    Entity cameraEntity = scene->CreateEntity("MainCamera");
    CameraComponent camera;
    camera.projection = glm::perspective(glm::radians(60.0f), 800.0f / 600.0f, 0.1f, 1000.0f);
    camera.view = glm::lookAt(glm::vec3(0, 5, 10), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    scene->AddComponent<CameraComponent>(cameraEntity, camera);

    // Run engine updates (Transform, Collision, Pathfinding, Gizmo)
    engine.Update(0.016f);

    // Verify parent-child transform calculation in integrated scene
    glm::mat4 wheelWorld = scene->GetComponent<TransformComponent>(wheelFL).globalMatrix;
    BOOST_CHECK_CLOSE(wheelWorld[3][0], -1.0f, 0.001f);
    BOOST_CHECK_CLOSE(wheelWorld[3][1], 0.5f, 0.001f);
    BOOST_CHECK_CLOSE(wheelWorld[3][2], 1.5f, 0.001f);

    // Save entity as Prefab
    rapidjson::Document prefabDoc;
    scene->SerializePrefabToJson(car, prefabDoc);
    BOOST_CHECK(prefabDoc.HasMember("entities"));

    // Instantiate saved Prefab in a new scene
    auto testScene2 = engine.CreateScene("PrefabConsumerScene");
    Entity instantiatedRoot = testScene2->InstantiatePrefab(prefabDoc);
    BOOST_CHECK(instantiatedRoot != MAX_ENTITIES);
    BOOST_CHECK(testScene2->HasComponent<TagComponent>(instantiatedRoot));
    BOOST_CHECK_EQUAL(testScene2->GetComponent<TagComponent>(instantiatedRoot).tag, "Vehicle");

    const auto& children = testScene2->GetChildren(instantiatedRoot);
    BOOST_CHECK_EQUAL(children.size(), 1);

    // Clean up
    engine.RemoveScene("PrefabConsumerScene");
    engine.RemoveScene(sceneName);
    platform.Shutdown();
}

BOOST_AUTO_TEST_SUITE_END()
