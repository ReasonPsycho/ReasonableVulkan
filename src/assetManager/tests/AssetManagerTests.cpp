// MockAssetTest.cpp
#define BOOST_TEST_MODULE MockAssetTests
#include <boost/test/unit_test.hpp>
#include "AssetManager.hpp"
#include "MockAsset.hpp"

namespace ae {
    // Factory function for MockAsset
    std::shared_ptr<Asset> createMockAsset(BaseFactoryContext context) {
        return std::make_shared<MockAsset>(context);
    }

    BOOST_AUTO_TEST_SUITE(MockAssetTests)

    BOOST_AUTO_TEST_CASE(test_asset_registration) {
        AssetManager &manager = AssetManager::getInstance();
        manager.registerFactory(AssetType::Model, createMockAsset);

        BaseFactoryContext context{
            manager,
            "test/path/mock.asset",
            AssetType::Model
        };

        // Register the asset
        auto id = manager.registerAsset(context);
        BOOST_TEST(id != boost::uuids::nil_uuid());

        // Try to register the same path again - should return the same ID
        auto id2 = manager.registerAsset(context);
        BOOST_TEST(id == id2);

        // Lookup the asset info
        auto assetInfo = manager.lookupAssetInfoByPath("test/path/mock.asset");
        BOOST_TEST(assetInfo.has_value());
        BOOST_TEST(assetInfo->id == id);
        BOOST_TEST(assetInfo->path == "test/path/mock.asset");
        BOOST_TEST(assetInfo->type == AssetType::Model);
    }

    BOOST_AUTO_TEST_CASE(test_content_hash_deduplication) {
        AssetManager &manager = AssetManager::getInstance();
        manager.registerFactory(AssetType::Model, createMockAsset);

        BaseFactoryContext context1{
            manager,
            "test/path/mock1.asset",
            AssetType::Model
        };

        BaseFactoryContext context2{
            manager,
            "test/path/mock2.asset",
            AssetType::Model
        };

        // Register first asset
        auto id1 = manager.registerAsset(context1);

        // Register second asset with same content (should return same ID due to content hash)
        auto id2 = manager.registerAsset(context2);
        BOOST_TEST(id1 == id2);

        // Verify that both paths point to the same asset
        auto info1 = manager.lookupAssetInfoByPath("test/path/mock1.asset");
        auto info2 = manager.lookupAssetInfoByPath("test/path/mock2.asset");
        BOOST_TEST(info1.has_value());
        BOOST_TEST(info2.has_value());
        BOOST_TEST(info1->contentHash == info2->contentHash);
    }

    BOOST_AUTO_TEST_CASE(test_asset_type_validation) {
        AssetManager &manager = AssetManager::getInstance();

        BaseFactoryContext context{
            manager,
            "test/path/mock.asset",
            AssetType::Texture // Wrong asset type
        };

        // Try to register asset with wrong type (no factory registered)
        BOOST_CHECK_THROW(manager.registerAsset(context), std::runtime_error);
    }

    BOOST_AUTO_TEST_CASE(test_asset_retrieval) {
        AssetManager &manager = AssetManager::getInstance();
        manager.registerFactory(AssetType::Model, createMockAsset);

        BaseFactoryContext context{
            manager,
            "test/path/mock.asset",
            AssetType::Model
        };

        auto id = manager.registerAsset(context);

        // Test getByUUID
        auto asset = manager.getByUUID<MockAsset>(id);
        BOOST_TEST((asset != nullptr));
        BOOST_TEST(asset->getType() == AssetType::Model);
    }

BOOST_AUTO_TEST_SUITE_END()

}  // namespace ae