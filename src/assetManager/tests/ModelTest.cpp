#include <boost/test/unit_test.hpp>
#include "Model.h"
#include "AssetManager.hpp"

BOOST_AUTO_TEST_SUITE(ModelTests)

    BOOST_AUTO_TEST_CASE(TestLoadBox) {
        auto &manager = ae::AssetManager::getInstance();

        // Create asset factory data for Box model
        ae::AssetFactoryData factoryData(manager, "C:/Users/redkc/CLionProjects/ReasonableVulkan/res/models/my/Box.fbx",
                                         ae::AssetType::Model);

        // Register and load the model
        auto modelInfo = manager.registerAsset(&factoryData);
        BOOST_REQUIRE(modelInfo != nullptr);

        // Get the actual model
        auto model = manager.getByUUID<ae::Model>(modelInfo->id);
        BOOST_REQUIRE(model != nullptr);

        // Box should have meshes
        BOOST_TEST(!model->meshes.empty());

        // Test that the model type is correct
        BOOST_TEST(model->getType() == ae::AssetType::Model);
    }

    BOOST_AUTO_TEST_CASE(TestLoadPlaneAndSphere) {
        auto &manager = ae::AssetManager::getInstance();

        // Load Plane
        ae::AssetFactoryData planeData(manager, "C:/Users/redkc/CLionProjects/ReasonableVulkan/res/models/my/Plane.fbx",
                                       ae::AssetType::Model);
        auto planeInfo = manager.registerAsset(&planeData);
        auto planeModel = manager.getByUUID<ae::Model>(planeInfo->id);
        BOOST_REQUIRE(planeModel != nullptr);

        // Load Sphere
        ae::AssetFactoryData sphereData(
            manager, "C:/Users/redkc/CLionProjects/ReasonableVulkan/res/models/my/Sphere.fbx", ae::AssetType::Model);
        auto sphereInfo = manager.registerAsset(&sphereData);
        auto sphereModel = manager.getByUUID<ae::Model>(sphereInfo->id);
        BOOST_REQUIRE(sphereModel != nullptr);

        // Since they share the same texture, their texture catalogues should have the same content hash
        BOOST_TEST(!planeModel->textureCatalogue.empty());
        BOOST_TEST(!sphereModel->textureCatalogue.empty());

        // Compare texture content hashes
        if (!planeModel->textureCatalogue.empty() && !sphereModel->textureCatalogue.empty()) {
            BOOST_TEST(planeModel->textureCatalogue[0]->contentHash ==
                sphereModel->textureCatalogue[0]->contentHash);
        }
    }

    BOOST_AUTO_TEST_CASE(TestModelContentHash) {
        auto &manager = ae::AssetManager::getInstance();

        // Load the same model twice
        ae::AssetFactoryData firstData(manager, "C:/Users/redkc/CLionProjects/ReasonableVulkan/res/models/my/Box.fbx",
                                       ae::AssetType::Model);
        auto firstModelInfo = manager.registerAsset(&firstData);

        ae::AssetFactoryData secondData(manager, "res/models/my/Box.fbx", ae::AssetType::Model);
        auto secondModelInfo = manager.registerAsset(&secondData);

        // Should return the same asset info due to same content hash
        BOOST_TEST(firstModelInfo->id == secondModelInfo->id);
        BOOST_TEST(firstModelInfo->contentHash == secondModelInfo->contentHash);
    }

    BOOST_AUTO_TEST_CASE(TestInvalidModel) {
        auto &manager = ae::AssetManager::getInstance();

        // Try to load non-existent model
        ae::AssetFactoryData invalidData(
            manager, "C:/Users/redkc/CLionProjects/ReasonableVulkan/res/models/my/NonExistent.fbx",
            ae::AssetType::Model);
        auto modelInfo = manager.registerAsset(&invalidData);

        // The asset info should be created but the model should have no meshes
        BOOST_REQUIRE(modelInfo != nullptr);
        auto model = manager.getByUUID<ae::Model>(modelInfo->id);
        BOOST_REQUIRE(model != nullptr);
        BOOST_TEST(model->meshes.empty());
    }


BOOST_AUTO_TEST_SUITE_END()
