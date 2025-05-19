#include <boost/test/unit_test.hpp>
#include "AssetManager.hpp"
#include "Asset.hpp"

// Mock Asset class for testing
class MockAsset : public ae::Asset {
public:
    explicit MockAsset(ae::AssetFactoryData assetFactoryData) 
        : Asset(assetFactoryData), mockHash(0) {}

    void setMockHash(size_t hash) { mockHash = hash; }

    size_t calculateContentHash() const override {
        return mockHash;
    }

    [[nodiscard]] ae::AssetType getType() const override {
        return ae::AssetType::Other;
    }

private:
    size_t mockHash;
};

BOOST_AUTO_TEST_SUITE(AssetManagerTests)

BOOST_AUTO_TEST_CASE(TestSingletonInstance)
{
    auto& instance1 = ae::AssetManager::getInstance();
    auto& instance2 = ae::AssetManager::getInstance();
    
    BOOST_TEST(&instance1 == &instance2);
}

BOOST_AUTO_TEST_CASE(TestRegisterAndLookupAsset)
{
    auto& manager = ae::AssetManager::getInstance();
    
    // Register factory for mock asset
    manager.registerFactory(ae::AssetType::Other, 
        [](ae::AssetFactoryData& data) -> std::unique_ptr<ae::Asset> {
            return std::make_unique<MockAsset>(data);
        });

    // Create asset factory data
    ae::AssetFactoryData factoryData(manager, "test_path.txt", ae::AssetType::Other);
    
    // Register asset
    auto assetInfo = manager.registerAsset(&factoryData);
    BOOST_REQUIRE(assetInfo != nullptr);
    
    // Test lookupAssetInfoByPath
    auto lookedUpAsset = manager.lookupAssetInfoByPath("test_path.txt");
    BOOST_REQUIRE(lookedUpAsset.has_value());
    BOOST_TEST(lookedUpAsset.value()->path == "test_path.txt");
    BOOST_TEST(lookedUpAsset.value()->type == ae::AssetType::Other);
    BOOST_TEST(lookedUpAsset.value()->id == assetInfo->id);
}

BOOST_AUTO_TEST_CASE(TestDuplicatePathRegistration)
{
    auto& manager = ae::AssetManager::getInstance();
    
    // Register factory if not already registered
    manager.registerFactory(ae::AssetType::Other, 
        [](ae::AssetFactoryData& data) -> std::unique_ptr<ae::Asset> {
            return std::make_unique<MockAsset>(data);
        });

    // Register first asset
    ae::AssetFactoryData firstData(manager, "duplicate_path.txt", ae::AssetType::Other);
    auto firstAsset = manager.registerAsset(&firstData);
    
    // Try to register second asset with same path
    ae::AssetFactoryData secondData(manager, "duplicate_path.txt", ae::AssetType::Other);
    auto secondAsset = manager.registerAsset(&secondData);
    
    // Should return the same asset info
    BOOST_TEST(firstAsset->id == secondAsset->id);
    BOOST_TEST(firstAsset->path == secondAsset->path);
}

BOOST_AUTO_TEST_CASE(TestGetByUUID)
{
    auto& manager = ae::AssetManager::getInstance();
    
    // Register factory if not already registered
    manager.registerFactory(ae::AssetType::Other, 
        [](ae::AssetFactoryData& data) -> std::unique_ptr<ae::Asset> {
            return std::make_unique<MockAsset>(data);
        });

    // Register an asset
    ae::AssetFactoryData factoryData(manager, "test_uuid.txt", ae::AssetType::Other);
    auto assetInfo = manager.registerAsset(&factoryData);
    
    // Try to get asset by UUID
    auto asset = manager.getByUUID<ae::Asset>(assetInfo->id);
    BOOST_REQUIRE(asset != nullptr);
}

BOOST_AUTO_TEST_CASE(TestContentHashDuplication)
{
    auto& manager = ae::AssetManager::getInstance();
    
    // Register factory that creates assets with specific hash
    manager.registerFactory(ae::AssetType::Other, 
        [](ae::AssetFactoryData& data) -> std::unique_ptr<ae::Asset> {
            auto asset = std::make_unique<MockAsset>(data);
            asset->setMockHash(12345); // Set same hash for all assets
            return asset;
        });

    // Register first asset
    ae::AssetFactoryData firstData(manager, "path1.txt", ae::AssetType::Other);
    auto firstAsset = manager.registerAsset(&firstData);
    
    // Register second asset with different path but same content hash
    ae::AssetFactoryData secondData(manager, "path2.txt", ae::AssetType::Other);
    auto secondAsset = manager.registerAsset(&secondData);
    
    // Should return the same asset info due to same content hash
    BOOST_TEST(firstAsset->id == secondAsset->id);
    BOOST_TEST(firstAsset->contentHash == secondAsset->contentHash);
}

BOOST_AUTO_TEST_CASE(TestNonExistentAsset)
{
    auto& manager = ae::AssetManager::getInstance();
    
    // Generate random UUID that shouldn't exist
    auto nonExistentUUID = boost::uuids::random_generator()();
    
    // Try to get non-existent asset
    auto asset = manager.getByUUID<ae::Asset>(nonExistentUUID);
    BOOST_TEST(asset == nullptr);
    
    // Try to lookup non-existent path
    auto lookedUpAsset = manager.lookupAssetInfoByPath("non_existent_path.txt");
    BOOST_TEST(!lookedUpAsset.has_value());
}

BOOST_AUTO_TEST_SUITE_END()