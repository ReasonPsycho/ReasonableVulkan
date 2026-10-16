#include <boost/test/unit_test.hpp>
#include "../Engine.h"
#include "ecs/Scene.h"
#include "TestHelper.hpp"
#include "systems/transformSystem/TransformSystem.h"
#include "systems/transformSystem/componets/TransformComponent.hpp"
#include "systems/collisionSystem/CollisionSystem.hpp"
#include "systems/gizmoSystem/GizmoSystem.hpp"
#include "systems/renderingSystem/componets/CameraComponent.hpp"
#include "systems/renderingSystem/componets/MeshComponent.hpp"
#include "systems/renderingSystem/componets/LightComponent.hpp"
#include "systems/renderingSystem/componets/RendererComponent.hpp"

using namespace engine;
using namespace engine::ecs;
using namespace engine::test;

BOOST_AUTO_TEST_SUITE(SystemsAdvancedTestSuite)

BOOST_AUTO_TEST_CASE(TransformSystemHierarchyMatrixTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("TransformSystemScene");
    BOOST_REQUIRE(scene != nullptr);

    TransformSystem transformSys(scene.get());

    Entity parent = scene->CreateEntity("Parent");
    auto& pTrans = scene->GetComponent<TransformComponent>(parent);
    pTrans.position = glm::vec3(10.0f, 0.0f, 0.0f);

    Entity child = scene->CreateEntity("Child", parent);
    auto& cTrans = scene->GetComponent<TransformComponent>(child);
    cTrans.position = glm::vec3(0.0f, 5.0f, 0.0f);

    transformSys.Update(0.016f);

    glm::mat4 pWorld = pTrans.globalMatrix;
    glm::mat4 cWorld = cTrans.globalMatrix;

    BOOST_CHECK_CLOSE(pWorld[3][0], 10.0f, 0.001f);
    BOOST_CHECK_CLOSE(cWorld[3][0], 10.0f, 0.001f);
    BOOST_CHECK_CLOSE(cWorld[3][1], 5.0f, 0.001f);

    engine.RemoveScene("TransformSystemScene");
}

BOOST_AUTO_TEST_CASE(CollisionSystemRayIntersectionTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("CollisionScene");
    BOOST_REQUIRE(scene != nullptr);

    CollisionSystem colSys(scene.get());

    Ray ray{glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, -1.0f)};
    glm::vec3 boxMin(-1.0f, -1.0f, -1.0f);
    glm::vec3 boxMax(1.0f, 1.0f, 1.0f);
    glm::mat4 worldMat(1.0f);

    float distance = 0.0f;
    bool hit = colSys.RayIntersectsAABB(ray, boxMin, boxMax, worldMat, distance);
    BOOST_CHECK(hit);
    BOOST_CHECK_CLOSE(distance, 9.0f, 0.001f);

    Ray missRay{glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 0.0f, -1.0f)};
    bool miss = colSys.RayIntersectsAABB(missRay, boxMin, boxMax, worldMat, distance);
    BOOST_CHECK(!miss);

    CameraComponent camera;
    camera.projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    camera.view = glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    Ray screenRay = colSys.ScreenToWorldRay(camera, 400.0f, 300.0f, 800.0f, 600.0f);
    BOOST_CHECK_CLOSE(screenRay.direction.z, -1.0f, 0.05f);

    engine.RemoveScene("CollisionScene");
}

BOOST_AUTO_TEST_CASE(GizmoSystemQueueAndLifetimesTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("GizmoScene");
    BOOST_REQUIRE(scene != nullptr);

    GizmoSystem gizmoSys(scene.get());
    BOOST_CHECK(gizmoSys.gizmoRenderCommandQueue.empty());

    gizmoSys.DrawRay(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 0.5f);
    BOOST_CHECK_EQUAL(gizmoSys.gizmoRenderCommandQueue.size(), 1);
    BOOST_CHECK_EQUAL(gizmoSys.gizmoRenderCommandQueue[0].type, GizmoType::RAY);

    gizmoSys.DrawCube(glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.2f);
    BOOST_CHECK_EQUAL(gizmoSys.gizmoRenderCommandQueue.size(), 2);
    BOOST_CHECK_EQUAL(gizmoSys.gizmoRenderCommandQueue[1].type, GizmoType::CUBE);

    gizmoSys.Update(0.0f);
    gizmoSys.Update(0.3f);
    // Cube command (0.2s lifetime) should have expired, Ray command (0.5s lifetime) remains
    BOOST_CHECK_EQUAL(gizmoSys.gizmoRenderCommandQueue.size(), 1);

    gizmoSys.Update(0.3f);
    // Ray command also expires
    BOOST_CHECK(gizmoSys.gizmoRenderCommandQueue.empty());

    engine.RemoveScene("GizmoScene");
}

BOOST_AUTO_TEST_SUITE_END()
