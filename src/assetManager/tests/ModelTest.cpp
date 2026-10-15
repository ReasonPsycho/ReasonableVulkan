#include <boost/test/unit_test.hpp>
#include "../src/assets/ModelAsset.h"
#include "../src/AssetManager.hpp"

BOOST_AUTO_TEST_SUITE(ModelTests)

// Helper to recursively collect all meshes from the node hierarchy
std::vector<std::shared_ptr<am::AssetInfo>> collectAllMeshes(const am::Node& node) {
    std::vector<std::shared_ptr<am::AssetInfo>> collected = node.meshes;
    for (const auto& child : node.mChildren) {
        auto childMeshes = collectAllMeshes(child);
        collected.insert(collected.end(), childMeshes.begin(), childMeshes.end());
    }
    return collected;
}

BOOST_AUTO_TEST_CASE(LoadBoxModelAndCheckProperties) {
    auto& manager = am::AssetManager::getInstance();

    am::ImportContext data( "res/.internal/tests/Box.fbx", am::AssetType::Model);
    auto info = manager.registerAsset(data);
    BOOST_REQUIRE(info.has_value());

    auto model = manager.getByUUID<am::ModelAsset>(info.value());
    BOOST_REQUIRE(model != nullptr);

    auto allMeshes = collectAllMeshes(model->data.rootNode);
    BOOST_REQUIRE(!allMeshes.empty());

    auto mesh = dynamic_cast<am::MeshAsset*>(allMeshes[0]->getAsset());
    BOOST_REQUIRE(mesh != nullptr);
    BOOST_TEST(mesh->getAssetDataAs<am::MeshData>()->material != nullptr);

    BOOST_TEST(model->getType() == am::AssetType::Model);
}

BOOST_AUTO_TEST_CASE(LoadPlaneAndSphereSharedMaterial) {
    auto& manager = am::AssetManager::getInstance();

    am::ImportContext planeData( "res/.internal/tests/Plane.fbx", am::AssetType::Model);
    auto planeInfo = manager.registerAsset(planeData);
    BOOST_REQUIRE(planeInfo.has_value());
    auto plane = manager.getByUUID<am::ModelAsset>(planeInfo.value());
    BOOST_REQUIRE(plane != nullptr);

    am::ImportContext sphereData( "res/.internal/tests/Sphere.fbx", am::AssetType::Model);
    auto sphereInfo = manager.registerAsset(sphereData);
    BOOST_REQUIRE(sphereInfo.has_value());
    auto sphere = manager.getByUUID<am::ModelAsset>(sphereInfo.value());
    BOOST_REQUIRE(sphere != nullptr);

    auto planeMeshes = collectAllMeshes(plane->data.rootNode);
    auto sphereMeshes = collectAllMeshes(sphere->data.rootNode);

    BOOST_REQUIRE(!planeMeshes.empty());
    BOOST_REQUIRE(!sphereMeshes.empty());

    auto planeMesh = planeMeshes[0]->getAsset()->getAssetDataAs<am::MeshData>();
    auto sphereMesh = sphereMeshes[0]->getAsset()->getAssetDataAs<am::MeshData>();

    BOOST_REQUIRE(planeMesh && sphereMesh);
    BOOST_REQUIRE(planeMesh->material && sphereMesh->material);
}

BOOST_AUTO_TEST_CASE(ModelSamePathSameHash) {
    auto& manager = am::AssetManager::getInstance();

    am::ImportContext data1( "res/.internal/tests/Box.fbx", am::AssetType::Model);
    auto model1 = manager.registerAsset(data1);

    am::ImportContext data2( "res/.internal/tests/Box.fbx", am::AssetType::Model);
    auto model2 = manager.registerAsset(data2);

    BOOST_REQUIRE(model1.has_value() && model2.has_value());
    BOOST_TEST(model1.value() == model2.value());
    auto info1 = manager.getAssetInfo(model1.value());
    auto info2 = manager.getAssetInfo(model2.value());
    BOOST_REQUIRE(info1.has_value() && info2.has_value());
    BOOST_TEST(info1.value()->contentHash == info2.value()->contentHash);
}

BOOST_AUTO_TEST_CASE(InvalidModelStillCreatesInfo) {
    auto& manager = am::AssetManager::getInstance();

    am::ImportContext data( "res/.internal/tests/temp/NonExistent.fbx", am::AssetType::Model);
    auto info = manager.registerAsset(data);
    BOOST_REQUIRE(info.has_value());

    auto model = manager.getByUUID<am::ModelAsset>(info.value());
    BOOST_REQUIRE(model != nullptr);

    auto allMeshes = collectAllMeshes(model->data.rootNode);
    BOOST_TEST(allMeshes.empty(), "Non-existent model should result in empty mesh list");
}

BOOST_AUTO_TEST_SUITE_END()
