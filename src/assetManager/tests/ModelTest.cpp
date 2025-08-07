#include <boost/test/unit_test.hpp>
#include "../src/assets/Model.h"
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

    am::AssetFactoryData data( "res/models/my/Box.fbx", am::AssetType::Model);
    auto info = manager.registerAsset(&data);
    BOOST_REQUIRE(info != nullptr);

    auto model = manager.getByUUID<am::Model>(info.value()->id);
    BOOST_REQUIRE(model != nullptr);

    auto allMeshes = collectAllMeshes(model->getAssetDataAs<am::ModelData>()->rootNode);
    BOOST_REQUIRE(!allMeshes.empty());

    auto mesh = dynamic_cast<am::Mesh*>(allMeshes[0]->getAsset());
    BOOST_REQUIRE(mesh != nullptr);
    BOOST_TEST(mesh->getAssetDataAs<am::MeshData>()->material != nullptr);

    BOOST_TEST(model->getType() == am::AssetType::Model);
}

BOOST_AUTO_TEST_CASE(LoadPlaneAndSphereSharedMaterial) {
    auto& manager = am::AssetManager::getInstance();

    am::AssetFactoryData planeData( "res/models/my/Plane.fbx", am::AssetType::Model);
    auto planeInfo = manager.registerAsset(&planeData);
    auto plane = manager.getByUUID<am::Model>(planeInfo.value()->id);
    BOOST_REQUIRE(plane != nullptr);

    am::AssetFactoryData sphereData( "res/models/my/Sphere.fbx", am::AssetType::Model);
    auto sphereInfo = manager.registerAsset(&sphereData);
    auto sphere = manager.getByUUID<am::Model>(sphereInfo.value()->id);
    BOOST_REQUIRE(sphere != nullptr);

    auto planeMeshes = collectAllMeshes(plane->getAssetDataAs<am::ModelData>()->rootNode);
    auto sphereMeshes = collectAllMeshes(sphere->getAssetDataAs<am::ModelData>()->rootNode);

    BOOST_REQUIRE(!planeMeshes.empty());
    BOOST_REQUIRE(!sphereMeshes.empty());

    auto planeMesh = planeMeshes[0]->getAsset()->getAssetDataAs<am::MeshData>();
    auto sphereMesh =  planeMeshes[0]->getAsset()->getAssetDataAs<am::MeshData>();

    BOOST_REQUIRE(planeMesh && sphereMesh);
    BOOST_REQUIRE(planeMesh->material && sphereMesh->material);
    BOOST_TEST(planeMesh->material == sphereMesh->material, "Materials should be shared");
}

BOOST_AUTO_TEST_CASE(ModelSamePathSameHash) {
    auto& manager = am::AssetManager::getInstance();

    am::AssetFactoryData data1( "res/models/my/Box.fbx", am::AssetType::Model);
    auto model1 = manager.registerAsset(&data1);

    am::AssetFactoryData data2( "res/models/my/Box.fbx", am::AssetType::Model);
    auto model2 = manager.registerAsset(&data2);

    BOOST_TEST(model1.value()->id == model2.value()->id);
    BOOST_TEST(model1.value()->contentHash == model2.value()->contentHash);
}

BOOST_AUTO_TEST_CASE(InvalidModelStillCreatesInfo) {
    auto& manager = am::AssetManager::getInstance();

    am::AssetFactoryData data( "res/models/my/NonExistent.fbx", am::AssetType::Model);
    auto info = manager.registerAsset(&data);
    BOOST_REQUIRE(info != nullptr);

    auto model = manager.getByUUID<am::Model>(info.value()->id);
    BOOST_REQUIRE(model != nullptr);

    auto allMeshes = collectAllMeshes(model->getAssetDataAs<am::ModelData>()->rootNode);
    BOOST_TEST(allMeshes.empty(), "Non-existent model should result in empty mesh list");
}

BOOST_AUTO_TEST_SUITE_END()
