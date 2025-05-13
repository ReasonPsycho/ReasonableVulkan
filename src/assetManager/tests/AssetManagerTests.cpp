// test_model_and_asset_manager.cpp
#define BOOST_TEST_MODULE ModelAndAssetManagerTests
#include <set>
#include <boost/test/unit_test.hpp>
#include "Model.h"
#include "AssetManager.hpp"

namespace ae {
namespace test {
    struct TestFixture {
        TestFixture() {
            assetManager = &AssetManager::getInstance();
        }

        AssetManager *assetManager;
        const std::string boxPath = "../../res/models/my/Box.fbx";
        const std::string spherePath = "../../res/models/my/Sphere.fbx";
        const std::string planePath = "../../res/models/my/Plane.fbx";
    };

    BOOST_FIXTURE_TEST_SUITE(ModelAndAssetManagerTests, TestFixture)

        BOOST_AUTO_TEST_CASE(AssetManager_RegisterAndRetrieveAsset) {
            // Test registering a model asset
            BaseFactoryContext context{
                *assetManager,
                boxPath,
                AssetType::Model
            };

            auto uuid = assetManager->registerAsset(context);
            BOOST_CHECK(uuid != boost::uuids::nil_uuid());

            // Test retrieving the asset
            auto model = assetManager->getByUUID<Model>(uuid);
            BOOST_CHECK(model != nullptr);
            BOOST_CHECK(!model->meshes.empty());
        }

        BOOST_AUTO_TEST_CASE(AssetManager_LookupAssetByPath) {
            // Register an asset first
            BaseFactoryContext context{
                *assetManager,
                spherePath,
                AssetType::Model
            };

            auto uuid = assetManager->registerAsset(context);

            // Test looking up asset info by path
            auto assetInfo = assetManager->lookupAssetInfoByPath(spherePath);
            BOOST_CHECK(assetInfo.has_value());
            BOOST_CHECK_EQUAL(assetInfo->path, spherePath);
            BOOST_CHECK_EQUAL(assetInfo->type, AssetType::Model);
            BOOST_CHECK_EQUAL(assetInfo->id, uuid);
        }

        BOOST_AUTO_TEST_CASE(Model_LoadAndVerifyStructure) {
            BaseFactoryContext context{
                *assetManager,
                planePath,
                AssetType::Model
            };

            auto uuid = assetManager->registerAsset(context);
            auto model = assetManager->getByUUID<Model>(uuid);

            BOOST_CHECK(model != nullptr);
            BOOST_CHECK(!model->meshes.empty());

            // Verify bone info map initialization
            BOOST_CHECK_EQUAL(model->GetBoneCount(), 0);
            BOOST_CHECK(model->GetBoneInfoMap().empty());
        }

        /*BOOST_AUTO_TEST_CASE(Model_BoneWeightOperations) {
            BaseFactoryContext context{
                *assetManager,
                boxPath,
                AssetType::Model
            };
            
            auto uuid = assetManager->registerAsset(context);
            auto model = assetManager->getByUUID<Model>(uuid);
            
            // Create a test vertex
            Vertex vertex;
            model->SetVertexBoneDataToDefault(vertex);
            
            // Test setting bone weights
            model->SetVertexBoneData(vertex, 0, 0.5f);
            model->SetVertexBoneData(vertex, 1, 0.5f);
            
            // Verify bone weights
            BOOST_CHECK_EQUAL(vertex.weights[0], 0.5f);
            BOOST_CHECK_EQUAL(vertex.weights[1], 0.5f);
            BOOST_CHECK_EQUAL(vertex.weights[2], 0.0f);
            BOOST_CHECK_EQUAL(vertex.weights[3], 0.0f);
        }*/

        BOOST_AUTO_TEST_CASE(AssetManager_MultipleAssetTypes) {
            // Register multiple assets
            std::vector<std::string> paths = {boxPath, spherePath, planePath};
            std::vector<boost::uuids::uuid> uuids;

            for (const auto &path: paths) {
                BaseFactoryContext context{
                    *assetManager,
                    path,
                    AssetType::Model
                };
                uuids.push_back(assetManager->registerAsset(context));
            }

            // Verify all assets are loaded and unique
            set<boost::uuids::uuid> uniqueIds(uuids.begin(), uuids.end());
            BOOST_CHECK_EQUAL(uniqueIds.size(), paths.size());

            // Verify each asset can be retrieved
    for (const auto& uuid : uuids) {
        auto model = assetManager->getByUUID<Model>(uuid);
        BOOST_CHECK(model != nullptr);
    }
}

BOOST_AUTO_TEST_CASE(AssetManager_NonexistentAsset) {
    // Try to retrieve a non-existent asset
    auto nullModel = assetManager->getByUUID<Model>(boost::uuids::nil_uuid());
    BOOST_CHECK(nullModel == nullptr);
    
    // Try to look up non-existent asset info
    auto nonexistentInfo = assetManager->lookupAssetInfoByPath("/nonexistent/path.fbx");
    BOOST_CHECK(!nonexistentInfo.has_value());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace test
} // namespace ae