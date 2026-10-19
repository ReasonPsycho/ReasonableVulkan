#include <boost/test/unit_test.hpp>
#include "../Engine.h"
#include "ecs/Scene.h"
#include "TestHelper.hpp"
#include "systems/hudSystem/HUDSystem.h"
#include "systems/hudSystem/components/RectTransformComponent.hpp"
#include "systems/hudSystem/components/UIImageComponent.hpp"
#include "systems/renderingSystem/componets/CameraComponent.hpp"
#include "systems/transformSystem/componets/TransformComponent.hpp"

using namespace engine;
using namespace engine::ecs;
using namespace engine::test;

namespace {
    class MockHUDGraphicsEngine : public gfx::GraphicsEngine {
    public:
        std::vector<std::pair<uint32_t, gfx::UIQuadData>> submittedQuads;
        glm::uvec2 extent{1920, 1080};

        void initialize(plt::PlatformInterface*, uint32_t, uint32_t) override {}
        glm::uvec2 getExtent() override { return extent; }
        void* getViewportTexturePointer() override { return nullptr; }
        void* getViewportTexturePointer(uint32_t) override { return nullptr; }
        void setCameraData(uint32_t, const glm::mat4&, const glm::mat4&, const glm::vec3) override {}
        void setActiveCameraCount(uint32_t) override {}
        void drawModel(uint32_t, gfx::MeshHandle, gfx::MaterialHandle, gfx::ShaderProgramHandle, const glm::mat4&) override {}
        void drawSkybox(uint32_t, gfx::MaterialHandle, gfx::ShaderProgramHandle) override {}
        void drawSkybox(uint32_t, const glm::vec4&) override {}
        void drawLight(gfx::PointLightData, const glm::mat4&) override {}
        void drawLight(gfx::SpotLightData, const glm::mat4&) override {}
        void drawLight(gfx::DirectionalLightData, const glm::mat4&) override {}

        void drawUIQuad(uint32_t cameraIndex, const gfx::UIQuadData& quadData) override {
            submittedQuads.push_back({cameraIndex, quadData});
        }

        gfx::ModelHandle loadModel(boost::uuids::uuid) override { return gfx::ModelHandle::invalid(); }
        gfx::MeshHandle loadMesh(boost::uuids::uuid) override { return gfx::MeshHandle::invalid(); }
        gfx::ShaderProgramHandle loadShader(boost::uuids::uuid) override { return gfx::ShaderProgramHandle::invalid(); }
        gfx::TextureHandle loadTexture(boost::uuids::uuid) override {
            return gfx::TextureHandle(42, 1);
        }
        gfx::MaterialHandle loadMaterial(boost::uuids::uuid) override { return gfx::MaterialHandle::invalid(); }
        gfx::ModelHandle getModelHandle(const std::string&) override { return gfx::ModelHandle::invalid(); }
        gfx::MeshHandle getMeshHandle(const std::string&) override { return gfx::MeshHandle::invalid(); }
        gfx::ShaderProgramHandle getShaderHandle(const std::string&) override { return gfx::ShaderProgramHandle::invalid(); }
        gfx::MaterialHandle getMaterialHandle(const std::string&) override { return gfx::MaterialHandle::invalid(); }
        void beginFrame() override {}
        void renderFrame() override {}
        void endFrame() override {}
    };
}

BOOST_AUTO_TEST_SUITE(HUDSystemTestSuite)

BOOST_AUTO_TEST_CASE(HUDSystemRegistrationAndLifecycleTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("HUDLifecycleScene");
    BOOST_REQUIRE(scene != nullptr);

    auto hudSystem = scene->GetSystem<HUDSystem>();
    BOOST_REQUIRE(hudSystem != nullptr);

    Entity uiEntity = scene->CreateEntity("HealthBar");
    RectTransformComponent rect(glm::vec2(10.0f, 20.0f), glm::vec2(200.0f, 30.0f));
    rect.zIndex = 5;
    scene->AddComponent<RectTransformComponent>(uiEntity, rect);

    UIImageComponent img(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    scene->AddComponent<UIImageComponent>(uiEntity, img);

    BOOST_CHECK(scene->HasComponent<RectTransformComponent>(uiEntity));
    BOOST_CHECK(scene->HasComponent<UIImageComponent>(uiEntity));

    auto& fetchedRect = scene->GetComponent<RectTransformComponent>(uiEntity);
    BOOST_CHECK_EQUAL(fetchedRect.position.x, 10.0f);
    BOOST_CHECK_EQUAL(fetchedRect.position.y, 20.0f);
    BOOST_CHECK_EQUAL(fetchedRect.size.x, 200.0f);
    BOOST_CHECK_EQUAL(fetchedRect.size.y, 30.0f);
    BOOST_CHECK_EQUAL(fetchedRect.zIndex, 5);

    auto& fetchedImg = scene->GetComponent<UIImageComponent>(uiEntity);
    BOOST_CHECK_EQUAL(fetchedImg.color.r, 1.0f);
    BOOST_CHECK_EQUAL(fetchedImg.color.g, 0.0f);
    BOOST_CHECK_EQUAL(fetchedImg.color.b, 0.0f);
    BOOST_CHECK(fetchedImg.visible);

    engine.RemoveScene("HUDLifecycleScene");
}

BOOST_AUTO_TEST_CASE(HUDSystemCoordinateCalculationsTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("HUDCoordsScene");
    BOOST_REQUIRE(scene != nullptr);

    auto hudSystem = scene->GetSystem<HUDSystem>();
    BOOST_REQUIRE(hudSystem != nullptr);

    float screenWidth = 1920.0f;
    float screenHeight = 1080.0f;

    // 1. Top-Left Point Anchor (0,0) with Pivot (0,0)
    Entity topLeftEntity = scene->CreateEntity("TopLeft");
    RectTransformComponent r1;
    r1.anchorMin = glm::vec2(0.0f, 0.0f);
    r1.anchorMax = glm::vec2(0.0f, 0.0f);
    r1.pivot = glm::vec2(0.0f, 0.0f);
    r1.position = glm::vec2(25.0f, 50.0f);
    r1.size = glm::vec2(100.0f, 40.0f);
    scene->AddComponent<RectTransformComponent>(topLeftEntity, r1);

    auto [pos1, size1] = hudSystem->ComputeScreenRect(topLeftEntity, screenWidth, screenHeight);
    BOOST_CHECK_CLOSE(pos1.x, 25.0f, 0.01f);
    BOOST_CHECK_CLOSE(pos1.y, 50.0f, 0.01f);
    BOOST_CHECK_CLOSE(size1.x, 100.0f, 0.01f);
    BOOST_CHECK_CLOSE(size1.y, 40.0f, 0.01f);

    // 2. Center Point Anchor (0.5, 0.5) with Center Pivot (0.5, 0.5)
    Entity centerEntity = scene->CreateEntity("Crosshair");
    RectTransformComponent r2;
    r2.anchorMin = glm::vec2(0.5f, 0.5f);
    r2.anchorMax = glm::vec2(0.5f, 0.5f);
    r2.pivot = glm::vec2(0.5f, 0.5f);
    r2.position = glm::vec2(0.0f, 0.0f);
    r2.size = glm::vec2(64.0f, 64.0f);
    scene->AddComponent<RectTransformComponent>(centerEntity, r2);

    auto [pos2, size2] = hudSystem->ComputeScreenRect(centerEntity, screenWidth, screenHeight);
    // Center is (960, 540), top-left should be (960 - 32, 540 - 32) = (928, 508)
    BOOST_CHECK_CLOSE(pos2.x, 928.0f, 0.01f);
    BOOST_CHECK_CLOSE(pos2.y, 508.0f, 0.01f);
    BOOST_CHECK_CLOSE(size2.x, 64.0f, 0.01f);
    BOOST_CHECK_CLOSE(size2.y, 64.0f, 0.01f);

    // 3. Full-Screen Stretch Anchor (0..1, 0..1) with Pivot (0.5, 0.5)
    Entity fullScreenEntity = scene->CreateEntity("Background");
    RectTransformComponent r3;
    r3.anchorMin = glm::vec2(0.0f, 0.0f);
    r3.anchorMax = glm::vec2(1.0f, 1.0f);
    r3.pivot = glm::vec2(0.5f, 0.5f);
    r3.position = glm::vec2(0.0f, 0.0f);
    r3.size = glm::vec2(0.0f, 0.0f);
    scene->AddComponent<RectTransformComponent>(fullScreenEntity, r3);

    auto [pos3, size3] = hudSystem->ComputeScreenRect(fullScreenEntity, screenWidth, screenHeight);
    BOOST_CHECK_CLOSE(pos3.x, 0.0f, 0.01f);
    BOOST_CHECK_CLOSE(pos3.y, 0.0f, 0.01f);
    BOOST_CHECK_CLOSE(size3.x, 1920.0f, 0.01f);
    BOOST_CHECK_CLOSE(size3.y, 1080.0f, 0.01f);

    // 4. Hierarchical Child Element inside Parent Panel
    Entity parentPanel = scene->CreateEntity("Panel");
    RectTransformComponent rParent;
    rParent.anchorMin = glm::vec2(0.0f, 0.0f);
    rParent.anchorMax = glm::vec2(0.0f, 0.0f);
    rParent.pivot = glm::vec2(0.0f, 0.0f);
    rParent.position = glm::vec2(100.0f, 100.0f);
    rParent.size = glm::vec2(400.0f, 300.0f);
    scene->AddComponent<RectTransformComponent>(parentPanel, rParent);

    Entity childButton = scene->CreateEntity("Button");
    scene->SetParent(childButton, parentPanel);
    RectTransformComponent rChild;
    rChild.anchorMin = glm::vec2(0.5f, 0.5f);
    rChild.anchorMax = glm::vec2(0.5f, 0.5f);
    rChild.pivot = glm::vec2(0.5f, 0.5f);
    rChild.position = glm::vec2(0.0f, 0.0f); // Center of parent
    rChild.size = glm::vec2(120.0f, 40.0f);
    scene->AddComponent<RectTransformComponent>(childButton, rChild);

    auto [childPos, childSize] = hudSystem->ComputeScreenRect(childButton, screenWidth, screenHeight);
    // Parent top-left (100, 100), size (400, 300) -> parent center is (300, 250).
    // Child size (120, 40) centered at (300, 250) -> child top-left is (240, 230).
    BOOST_CHECK_CLOSE(childPos.x, 240.0f, 0.01f);
    BOOST_CHECK_CLOSE(childPos.y, 230.0f, 0.01f);
    BOOST_CHECK_CLOSE(childSize.x, 120.0f, 0.01f);
    BOOST_CHECK_CLOSE(childSize.y, 40.0f, 0.01f);

    engine.RemoveScene("HUDCoordsScene");
}

BOOST_AUTO_TEST_CASE(HUDSystemRenderSubmissionAndSortingTest) {
    MockHUDGraphicsEngine mockGfx;
    Engine engine(nullptr, &mockGfx, &am::AssetManager::getInstance());
    auto scene = engine.CreateScene("HUDRenderScene");
    BOOST_REQUIRE(scene != nullptr);

    // Entity 1: Background Layer 0
    Entity bg = scene->CreateEntity("BG");
    RectTransformComponent rBg(glm::vec2(0.0f, 0.0f), glm::vec2(1920.0f, 1080.0f));
    rBg.zIndex = 0;
    scene->AddComponent<RectTransformComponent>(bg, rBg);
    UIImageComponent imgBg(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
    scene->AddComponent<UIImageComponent>(bg, imgBg);

    // Entity 2: Foreground Layer 10
    Entity fg = scene->CreateEntity("FG");
    RectTransformComponent rFg(glm::vec2(100.0f, 100.0f), glm::vec2(50.0f, 50.0f));
    rFg.zIndex = 10;
    scene->AddComponent<RectTransformComponent>(fg, rFg);
    UIImageComponent imgFg(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
    scene->AddComponent<UIImageComponent>(fg, imgFg);

    // Entity 3: Hidden element
    Entity hidden = scene->CreateEntity("Hidden");
    RectTransformComponent rHidden(glm::vec2(50.0f, 50.0f), glm::vec2(10.0f, 10.0f));
    scene->AddComponent<RectTransformComponent>(hidden, rHidden);
    UIImageComponent imgHidden(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    imgHidden.visible = false;
    scene->AddComponent<UIImageComponent>(hidden, imgHidden);

    // Entity 4: Middle Layer 5 with texture UUID
    boost::uuids::uuid testTexId = boost::uuids::string_generator()("{12345678-1234-1234-1234-123456789abc}");
    Entity mid = scene->CreateEntity("Icon");
    RectTransformComponent rMid(glm::vec2(200.0f, 200.0f), glm::vec2(80.0f, 80.0f));
    rMid.zIndex = 5;
    scene->AddComponent<RectTransformComponent>(mid, rMid);
    UIImageComponent imgMid(testTexId, glm::vec4(1.0f, 1.0f, 1.0f, 0.8f));
    scene->AddComponent<UIImageComponent>(mid, imgMid);

    // Trigger Scene Render
    scene->Render(0.016f);

    BOOST_REQUIRE_EQUAL(mockGfx.submittedQuads.size(), 3);

    // Verify layer order: Layer 0 -> Layer 5 -> Layer 10
    BOOST_CHECK_EQUAL(mockGfx.submittedQuads[0].second.layer, 0);
    BOOST_CHECK_EQUAL(mockGfx.submittedQuads[1].second.layer, 5);
    BOOST_CHECK_EQUAL(mockGfx.submittedQuads[2].second.layer, 10);

    // Verify quad data properties
    BOOST_CHECK_EQUAL(mockGfx.submittedQuads[0].second.color.r, 0.1f);
    BOOST_CHECK_EQUAL(mockGfx.submittedQuads[1].second.color.a, 0.8f);
    BOOST_CHECK(mockGfx.submittedQuads[1].second.texture.isValid());
    BOOST_CHECK_EQUAL(mockGfx.submittedQuads[2].second.color.r, 1.0f);
    BOOST_CHECK_EQUAL(mockGfx.submittedQuads[2].second.color.g, 1.0f);
    BOOST_CHECK_EQUAL(mockGfx.submittedQuads[2].second.color.b, 0.0f);

    engine.RemoveScene("HUDRenderScene");
}

BOOST_AUTO_TEST_SUITE_END()
