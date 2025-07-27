#include <boost/test/unit_test.hpp>
#include "AssetManager.hpp"
#include "Asset.hpp"

// Mock Asset class for testing
class MockAsset : public am::Asset {
public:
    explicit MockAsset(am::AssetFactoryData assetFactoryData)
        : Asset(assetFactoryData), mockHash(0) {
    }

    void setMockHash(size_t hash) { mockHash = hash; }

    size_t calculateContentHash() const override {
        return mockHash;
    }

    [[nodiscard]] am::AssetType getType() const override {
        return am::AssetType::Other;
    }

private:
    size_t mockHash;
};

BOOST_AUTO_TEST_SUITE(AssetManagerTests)

    BOOST_AUTO_TEST_CASE(TestSingletonInstance) {
        auto &instance1 = am::AssetManager::getInstance();
        auto &instance2 = am::AssetManager::getInstance();

        BOOST_TEST(&instance1 == &instance2);
    }

    BOOST_AUTO_TEST_CASE(TestRegisterAndLookupAsset) {
        auto &manager = am::AssetManager::getInstance();
    // Register factory that creates assets with specific hash
    manager.registerFactory(am::AssetType::Other,
                            [](am::AssetFactoryData &data) -> std::unique_ptr<am::Asset> {
                                auto asset = std::make_unique<MockAsset>(data);
                                asset->setMockHash(12345); // Set same hash for all assets
                                return asset;
                            });

        // Create asset factory data
        am::AssetFactoryData factoryData(manager, "test_path.txt", am::AssetType::Other);

        // Register asset
        auto assetInfo = manager.registerAsset(&factoryData);
        BOOST_REQUIRE(assetInfo != nullptr);

        // Test lookupAssetInfoByPath
        auto uids = manager.getUUIDsByPath("test_path.txt");
        BOOST_REQUIRE(uids.size() == 1);
        auto lookedUpAsset = manager.lookupAssetInfo(uids[0]);
        BOOST_TEST(lookedUpAsset.value()->path == "test_path.txt");
        BOOST_TEST(lookedUpAsset.value()->type == am::AssetType::Other);
        BOOST_TEST(lookedUpAsset.value()->id == assetInfo->id);
    }

    BOOST_AUTO_TEST_CASE(TestDuplicatePathRegistration) {
        auto &manager = am::AssetManager::getInstance();

        // Register factory if not already registered
        manager.registerFactory(am::AssetType::Other,
                                [](am::AssetFactoryData &data) -> std::unique_ptr<am::Asset> {
                                    return std::make_unique<MockAsset>(data);
                                });

        // Register first asset
        am::AssetFactoryData firstData(manager, "duplicate_path.txt", am::AssetType::Other);
        auto firstAsset = manager.registerAsset(&firstData);

        // Try to register second asset with same path
        am::AssetFactoryData secondData(manager, "duplicate_path.txt", am::AssetType::Other);
        auto secondAsset = manager.registerAsset(&secondData);

        // Should return the same asset info
        BOOST_TEST(firstAsset->id == secondAsset->id);
        BOOST_TEST(firstAsset->path == secondAsset->path);
    }

    BOOST_AUTO_TEST_CASE(TestGetByUUID) {
        auto &manager = am::AssetManager::getInstance();

        // Register factory if not already registered
        manager.registerFactory(am::AssetType::Other,
                                [](am::AssetFactoryData &data) -> std::unique_ptr<am::Asset> {
                                    return std::make_unique<MockAsset>(data);
                                });

        // Register an asset
        am::AssetFactoryData factoryData(manager, "test_uuid.txt", am::AssetType::Other);
        auto assetInfo = manager.registerAsset(&factoryData);

        // Try to get asset by UUID
        auto asset = manager.getByUUID<am::Asset>(assetInfo->id);
        BOOST_REQUIRE(asset != nullptr);
    }

    BOOST_AUTO_TEST_CASE(TestContentHashDuplication) {
        auto &manager = am::AssetManager::getInstance();

        // Register factory that creates assets with specific hash
        manager.registerFactory(am::AssetType::Other,
                                [](am::AssetFactoryData &data) -> std::unique_ptr<am::Asset> {
                                    auto asset = std::make_unique<MockAsset>(data);
                                    asset->setMockHash(12345); // Set same hash for all assets
                                    return asset;
                                });

        // Register first asset
        am::AssetFactoryData firstData(manager, "path1.txt", am::AssetType::Other);
        auto firstAsset = manager.registerAsset(&firstData);

        // Register second asset with different path but same content hash
        am::AssetFactoryData secondData(manager, "path2.txt", am::AssetType::Other);
        auto secondAsset = manager.registerAsset(&secondData);

        // Should return the same asset info due to same content hash
        BOOST_TEST(firstAsset->id == secondAsset->id);
        BOOST_TEST(firstAsset->contentHash == secondAsset->contentHash);
    }

    BOOST_AUTO_TEST_CASE(TestNonExistentAsset) {
        auto &manager = am::AssetManager::getInstance();

        // Generate random UUID that shouldn't exist
        auto nonExistentUUID = boost::uuids::random_generator()();

        // Try to get non-existent asset
        auto asset = manager.getByUUID<am::Asset>(nonExistentUUID);
        BOOST_TEST(asset == nullptr);

        // Try to lookup non-existent path
        auto uids = manager.getUUIDsByPath("test_path1.txt");
        BOOST_TEST(uids.empty());
    }

BOOST_AUTO_TEST_SUITE_END()