#include <boost/test/unit_test.hpp>
#include "../Engine.h"
#include "ecs/Scene.h"
#include "TestHelper.hpp"
#include "systems/hudSystem/HUDSystem.hpp"
#include "systems/hudSystem/components/HUDElementComponent.hpp"
#include "GraphicsEngine.hpp"

using namespace engine;
using namespace engine::ecs;
using namespace engine::test;

namespace {
    class MockHUDGraphicsEngine : public gfx::GraphicsEngine {
    public:
        std::vector<gfx::HUDDrawItem> lastSubmittedItems;

        void initialize(plt::PlatformInterface* platform, uint32_t width, uint32_t height) override {}
        glm::uvec2 getExtent() override { return glm::uvec2(800, 600); }
        void* getViewportTexturePointer() override { return nullptr; }
        void* getViewportTexturePointer(uint32_t cameraIndex) override { return nullptr; }

        void setCameraData(uint32_t cameraIndex, const glm::mat4& projection, const glm::mat4& view, const glm::vec3 cameraPos) override {}
        void setActiveCameraCount(uint32_t count) override {}

        void drawModel(uint32_t cameraIndex, gfx::MeshHandle meshHandle, gfx::MaterialHandle materialHandle, gfx::ShaderProgramHandle shaderHandle, const glm::mat4& transform) override {}
        void drawSkybox(uint32_t cameraIndex, gfx::MaterialHandle materialHandle, gfx::ShaderProgramHandle shaderHandle) override {}
        void drawSkybox(uint32_t cameraIndex, const glm::vec4& color) override {}
        void drawLight(gfx::PointLightData pointLightData, const glm::mat4& transform) override {}
        void drawLight(gfx::SpotLightData spotLightData, const glm::mat4& transform) override {}
        void drawLight(gfx::DirectionalLightData directionalLightData, const glm::mat4& transform) override {}

        gfx::ModelHandle loadModel(boost::uuids::uuid uuid) override { return gfx::ModelHandle::invalid(); }
        gfx::MeshHandle loadMesh(boost::uuids::uuid uuid) override { return gfx::MeshHandle::invalid(); }
        gfx::ShaderProgramHandle loadShader(boost::uuids::uuid uuid) override { return gfx::ShaderProgramHandle::invalid(); }
        gfx::TextureHandle loadTexture(boost::uuids::uuid uuid) override { return gfx::TextureHandle::invalid(); }
        gfx::MaterialHandle loadMaterial(boost::uuids::uuid uuid) override { return gfx::MaterialHandle::invalid(); }

        gfx::ModelHandle getModelHandle(const std::string& lookUpName) override { return gfx::ModelHandle::invalid(); }
        gfx::MeshHandle getMeshHandle(const std::string& lookUpName) override { return gfx::MeshHandle::invalid(); }
        gfx::ShaderProgramHandle getShaderHandle(const std::string& lookUpName) override { return gfx::ShaderProgramHandle::invalid(); }
        gfx::MaterialHandle getMaterialHandle(const std::string& lookUpName) override { return gfx::MaterialHandle::invalid(); }

        void beginFrame() override {}
        void renderFrame() override {}
        void endFrame() override {}

        void submitHUDItems(const std::vector<gfx::HUDDrawItem>& items) override {
            lastSubmittedItems = items;
        }

        void drawHUDItem(const gfx::HUDDrawItem& item) override {
            lastSubmittedItems.push_back(item);
        }
    };
}

BOOST_AUTO_TEST_SUITE(HUDSystemTestSuite)

BOOST_AUTO_TEST_CASE(HUDSystemRegistrationAndLifecycleTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("HUDLifecycleScene");
    BOOST_REQUIRE(scene != nullptr);

    auto hudSystem = scene->GetSystem<HUDSystem>();
    BOOST_REQUIRE(hudSystem != nullptr);

    Entity e = scene->CreateEntity("HUDEntity");
    HUDElementComponent hudComp;
    hudComp.size = glm::vec2(200.0f, 50.0f);
    hudComp.offset = glm::vec2(10.0f, 20.0f);
    hudComp.anchor = static_cast<int>(UIAnchor::TopLeft);
    hudComp.layerDepth = 1;
    scene->AddComponent<HUDElementComponent>(e, hudComp);

    BOOST_CHECK(scene->HasComponent<HUDElementComponent>(e));
    auto& retrievedComp = scene->GetComponent<HUDElementComponent>(e);
    BOOST_CHECK_EQUAL(retrievedComp.size.x, 200.0f);
    BOOST_CHECK_EQUAL(retrievedComp.size.y, 50.0f);

    engine.RemoveScene("HUDLifecycleScene");
}

BOOST_AUTO_TEST_CASE(HUDSystemAnchorCalculationsTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("HUDAnchorScene");
    BOOST_REQUIRE(scene != nullptr);

    auto hudSystem = scene->GetSystem<HUDSystem>();
    BOOST_REQUIRE(hudSystem != nullptr);

    glm::vec2 screenSize(800.0f, 600.0f);
    glm::vec2 elemSize(100.0f, 50.0f);
    glm::vec2 offset(10.0f, 5.0f);

    // TopLeft: (0, 0) + offset
    glm::vec2 posTL = hudSystem->CalculateAnchoredPosition(UIAnchor::TopLeft, offset, elemSize, screenSize);
    BOOST_CHECK_EQUAL(posTL.x, 10.0f);
    BOOST_CHECK_EQUAL(posTL.y, 5.0f);

    // TopCenter: (800/2 - 100/2, 0) + offset = (350, 0) + offset = (360, 5)
    glm::vec2 posTC = hudSystem->CalculateAnchoredPosition(UIAnchor::TopCenter, offset, elemSize, screenSize);
    BOOST_CHECK_EQUAL(posTC.x, 360.0f);
    BOOST_CHECK_EQUAL(posTC.y, 5.0f);

    // TopRight: (800 - 100, 0) + offset = (700, 0) + offset = (710, 5)
    glm::vec2 posTR = hudSystem->CalculateAnchoredPosition(UIAnchor::TopRight, offset, elemSize, screenSize);
    BOOST_CHECK_EQUAL(posTR.x, 710.0f);
    BOOST_CHECK_EQUAL(posTR.y, 5.0f);

    // Center: (350, 600/2 - 50/2) + offset = (350, 275) + offset = (360, 280)
    glm::vec2 posC = hudSystem->CalculateAnchoredPosition(UIAnchor::Center, offset, elemSize, screenSize);
    BOOST_CHECK_EQUAL(posC.x, 360.0f);
    BOOST_CHECK_EQUAL(posC.y, 280.0f);

    // BottomRight: (700, 600 - 50) + offset = (700, 550) + offset = (710, 555)
    glm::vec2 posBR = hudSystem->CalculateAnchoredPosition(UIAnchor::BottomRight, offset, elemSize, screenSize);
    BOOST_CHECK_EQUAL(posBR.x, 710.0f);
    BOOST_CHECK_EQUAL(posBR.y, 555.0f);

    engine.RemoveScene("HUDAnchorScene");
}

BOOST_AUTO_TEST_CASE(HUDSystemBatchingAndSortingTest) {
    MockHUDGraphicsEngine mockGfx;
    Engine engine(nullptr, &mockGfx, &am::AssetManager::getInstance());

    auto scene = engine.CreateScene("HUDBatchScene");
    BOOST_REQUIRE(scene != nullptr);

    auto hudSystem = scene->GetSystem<HUDSystem>();
    BOOST_REQUIRE(hudSystem != nullptr);

    // Entity 1: Layer depth 2 (should be drawn later)
    Entity e1 = scene->CreateEntity("ElemLayer2");
    HUDElementComponent comp1;
    comp1.layerDepth = 2;
    comp1.size = glm::vec2(50.0f, 50.0f);
    comp1.anchor = static_cast<int>(UIAnchor::TopLeft);
    scene->AddComponent<HUDElementComponent>(e1, comp1);

    // Entity 2: Layer depth 0 (should be drawn first)
    Entity e2 = scene->CreateEntity("ElemLayer0");
    HUDElementComponent comp2;
    comp2.layerDepth = 0;
    comp2.size = glm::vec2(800.0f, 600.0f);
    comp2.anchor = static_cast<int>(UIAnchor::TopLeft);
    scene->AddComponent<HUDElementComponent>(e2, comp2);

    // Entity 3: Hidden element (should not be submitted)
    Entity e3 = scene->CreateEntity("ElemHidden");
    HUDElementComponent comp3;
    comp3.layerDepth = 1;
    comp3.isVisible = false;
    scene->AddComponent<HUDElementComponent>(e3, comp3);

    hudSystem->Update(0.016f);

    BOOST_REQUIRE_EQUAL(mockGfx.lastSubmittedItems.size(), 2);
    // Verified sorted by layerDepth: 0 first, then 2
    BOOST_CHECK_EQUAL(mockGfx.lastSubmittedItems[0].layerDepth, 0);
    BOOST_CHECK_EQUAL(mockGfx.lastSubmittedItems[1].layerDepth, 2);

    engine.RemoveScene("HUDBatchScene");
}

BOOST_AUTO_TEST_SUITE_END()
