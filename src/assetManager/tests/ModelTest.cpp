#include <boost/test/unit_test.hpp>
#include "Model.h"
#include "AssetManager.hpp"

BOOST_AUTO_TEST_SUITE(ModelTests)

    BOOST_AUTO_TEST_CASE(LoadBoxModelAndCheckProperties) {
        auto& manager = am::AssetManager::getInstance();

        am::AssetFactoryData data(manager, "res/models/my/Box.fbx", am::AssetType::Model);
        auto info = manager.registerAsset(&data);
        BOOST_REQUIRE(info != nullptr);

        auto model = manager.getByUUID<am::Model>(info.value()->id);
        BOOST_REQUIRE(model != nullptr);
        BOOST_TEST(!model->meshes.empty());

        auto mesh = dynamic_cast<am::Mesh*>(model->meshes[0]->getAsset());
        BOOST_REQUIRE(mesh != nullptr);
        BOOST_TEST(mesh->material != nullptr);

        BOOST_TEST(model->getType() == am::AssetType::Model);
    }

    BOOST_AUTO_TEST_CASE(LoadPlaneAndSphereSharedMaterial) {
        auto& manager = am::AssetManager::getInstance();

        am::AssetFactoryData planeData(manager, "res/models/my/Plane.fbx", am::AssetType::Model);
        auto planeInfo = manager.registerAsset(&planeData);
        auto plane = manager.getByUUID<am::Model>(planeInfo.value()->id);
        BOOST_REQUIRE(plane != nullptr);
        BOOST_REQUIRE(!plane->meshes.empty());

        am::AssetFactoryData sphereData(manager, "res/models/my/Sphere.fbx", am::AssetType::Model);
        auto sphereInfo = manager.registerAsset(&sphereData);
        auto sphere = manager.getByUUID<am::Model>(sphereInfo.value()->id);
        BOOST_REQUIRE(sphere != nullptr);
        BOOST_REQUIRE(!sphere->meshes.empty());

        auto planeMesh = dynamic_cast<am::Mesh*>(plane->meshes[0]->getAsset());
        auto sphereMesh = dynamic_cast<am::Mesh*>(sphere->meshes[0]->getAsset());

        BOOST_REQUIRE(planeMesh && sphereMesh);
        BOOST_REQUIRE(planeMesh->material && sphereMesh->material);
        BOOST_TEST(planeMesh->material == sphereMesh->material, "Materials should be shared");
    }

    BOOST_AUTO_TEST_CASE(ModelSamePathSameHash) {
        auto& manager = am::AssetManager::getInstance();

        am::AssetFactoryData data1(manager, "res/models/my/Box.fbx", am::AssetType::Model);
        auto model1 = manager.registerAsset(&data1);

        am::AssetFactoryData data2(manager, "res/models/my/Box.fbx", am::AssetType::Model);
        auto model2 = manager.registerAsset(&data2);

        BOOST_TEST(model1.value()->id == model2.value()->id);
        BOOST_TEST(model1.value()->contentHash == model2.value()->contentHash);
    }

    BOOST_AUTO_TEST_CASE(InvalidModelStillCreatesInfo) {
        auto& manager = am::AssetManager::getInstance();

        am::AssetFactoryData data(manager, "res/models/my/NonExistent.fbx", am::AssetType::Model);
        auto info = manager.registerAsset(&data);
        BOOST_REQUIRE(info != nullptr);

        auto model = manager.getByUUID<am::Model>(info.value()->id);
        BOOST_REQUIRE(model != nullptr);

        BOOST_TEST(model->meshes.empty(), "Non-existent model should result in empty mesh list");
    }

BOOST_AUTO_TEST_SUITE_END()
