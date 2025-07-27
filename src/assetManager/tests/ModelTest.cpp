#include <boost/test/unit_test.hpp>
#include "Model.h"
#include "AssetManager.hpp"

BOOST_AUTO_TEST_SUITE(ModelTests)

    BOOST_AUTO_TEST_CASE(TestLoadBox) {
        auto &manager = am::AssetManager::getInstance();

        // Create asset factory data for Box model
        am::AssetFactoryData factoryData(manager, "res/models/my/Box.fbx",
                                         am::AssetType::Model);

        // Register and load the model
        auto modelInfo = manager.registerAsset(&factoryData);
        BOOST_REQUIRE(modelInfo != nullptr);

        // Get the actual model
        auto model = manager.getByUUID<am::Model>(modelInfo.get()->id);
        BOOST_REQUIRE(model != nullptr);

        // Box should have meshes
        BOOST_TEST(!model->meshes.empty());

        // Mesh should have material
        BOOST_TEST(dynamic_cast<am::Mesh*>(model->meshes[0]->getAsset())->material != nullptr);
    
        // Test that the model type is correct
        BOOST_TEST(model->getType() == am::AssetType::Model);
    }

    BOOST_AUTO_TEST_CASE(TestLoadPlaneAndSphere) {
        auto &manager = am::AssetManager::getInstance();

        // Load Plane
        am::AssetFactoryData planeData(manager, "res/models/my/Plane.fbx",
                                       am::AssetType::Model);
        auto planeInfo = manager.registerAsset(&planeData);
        auto planeModel = manager.getByUUID<am::Model>(planeInfo->id);
        BOOST_REQUIRE(planeModel != nullptr);

        // Load Sphere
        am::AssetFactoryData sphereData(
            manager, "res/models/my/Sphere.fbx", am::AssetType::Model);
        auto sphereInfo = manager.registerAsset(&sphereData);
        auto sphereModel = manager.getByUUID<am::Model>(sphereInfo->id);
        BOOST_REQUIRE(sphereModel != nullptr);

        // Since they share the same texture, their texture catalogues should have the same content hash
        BOOST_TEST(!planeModel->meshes.empty());
        BOOST_TEST(!sphereModel->meshes.empty());

        BOOST_TEST(dynamic_cast<am::Mesh*>(planeModel->meshes[0]->getAsset())->material != nullptr);
        BOOST_TEST(dynamic_cast<am::Mesh*>(sphereModel->meshes[0]->getAsset())->material != nullptr);


        BOOST_TEST(dynamic_cast<am::Mesh*>(sphereModel->meshes[0]->getAsset())->material == dynamic_cast<am::Mesh*>(planeModel->meshes[0]->getAsset())->material);
    }

    BOOST_AUTO_TEST_CASE(TestModelContentHash) {
        auto &manager = am::AssetManager::getInstance();

        // Load the same model twice
        am::AssetFactoryData firstData(manager, "res/models/my/Box.fbx",
                                       am::AssetType::Model);
        auto firstModelInfo = manager.registerAsset(&firstData);

        am::AssetFactoryData secondData(manager, "res/models/my/Box.fbx", am::AssetType::Model);
        auto secondModelInfo = manager.registerAsset(&secondData);

        // Should return the same asset info due to same content hash
        BOOST_TEST(firstModelInfo->id == secondModelInfo->id);
        BOOST_TEST(firstModelInfo->contentHash == secondModelInfo->contentHash);
    }

    BOOST_AUTO_TEST_CASE(TestInvalidModel) {
        auto &manager = am::AssetManager::getInstance();

        // Try to load non-existent model
        am::AssetFactoryData invalidData(
            manager, "res/models/my/NonExistent.fbx",
            am::AssetType::Model);
        auto modelInfo = manager.registerAsset(&invalidData);

        // The asset info should be created but the model should have no meshes
        BOOST_REQUIRE(modelInfo != nullptr);
        auto model = manager.getByUUID<am::Model>(modelInfo->id);
        BOOST_REQUIRE(model != nullptr);
        BOOST_TEST(model->meshes.empty());
    }


BOOST_AUTO_TEST_SUITE_END()
