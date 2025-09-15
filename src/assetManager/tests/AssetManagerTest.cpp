#include <boost/test/unit_test.hpp>
#include "../src/AssetManager.hpp"
#include "../include/Asset.hpp"

struct MockData
{

};

// Mock Asset class for testing
class MockAsset : public am::Asset {
public:
    explicit MockAsset(am::AssetFactoryData assetFactoryData)
        : Asset(assetFactoryData), mockHash(0) {}

    void setMockHash(size_t hash) { mockHash = hash; }

    size_t calculateContentHash() const override { return mockHash; }

    [[nodiscard]] am::AssetType getType() const override {
        return am::AssetType::Other;
    }

    void* getAssetData() override { return &data; }

    MockData data;
private:
    size_t mockHash;
};

// Utility function to register mock factory (reusable)
void registerMockFactory(am::AssetManager& manager, size_t hash = 12345) {
    manager.registerFactory(am::AssetType::Other, [hash](am::AssetFactoryData& data) {
        auto asset = std::make_unique<MockAsset>(data);
        asset->setMockHash(hash);
        return asset;
    });
}

BOOST_AUTO_TEST_SUITE(AssetManagerTests)

    BOOST_AUTO_TEST_CASE(SingletonInstanceIsSame) {
        auto& instance1 = am::AssetManager::getInstance();
        auto& instance2 = am::AssetManager::getInstance();

        BOOST_TEST(&instance1 == &instance2, "AssetManager singleton instances must be equal");
    }

    BOOST_AUTO_TEST_CASE(RegisterAndLookupAssetWorks) {
        auto& manager = am::AssetManager::getInstance();
        registerMockFactory(manager);

        am::AssetFactoryData data( "test_path.txt", am::AssetType::Other);
        auto assetInfo = manager.registerAsset(&data);
        BOOST_REQUIRE(assetInfo != nullptr);

        auto uids = manager.getUUIDsByPath("test_path.txt");
        BOOST_REQUIRE_EQUAL(uids.size(), 1);

        auto result = manager.getAssetInfo(uids[0]);
        BOOST_REQUIRE(result.has_value());

        BOOST_TEST(result.value()->path == "test_path.txt");
        BOOST_TEST(result.value()->type == am::AssetType::Other);
        BOOST_TEST(result.value()->id == assetInfo.value()->id);
    }

    BOOST_AUTO_TEST_CASE(DuplicatePathReturnsSameAssetInfo) {
        auto& manager = am::AssetManager::getInstance();
        registerMockFactory(manager);

        am::AssetFactoryData data1( "duplicate.txt", am::AssetType::Other);
        auto asset1 = manager.registerAsset(&data1);

        am::AssetFactoryData data2( "duplicate.txt", am::AssetType::Other);
        auto asset2 = manager.registerAsset(&data2);

        BOOST_TEST(asset1.value()->id == asset2.value()->id);
        BOOST_TEST(asset1.value()->path == asset2.value()->path);
    }

    BOOST_AUTO_TEST_CASE(GetAssetByUUIDReturnsCorrectInstance) {
        auto& manager = am::AssetManager::getInstance();
        registerMockFactory(manager);

        am::AssetFactoryData data("uuid_test.txt", am::AssetType::Other);
        auto assetInfo = manager.registerAsset(&data);

        auto asset = manager.getByUUID<am::Asset>(assetInfo.value()->id);
        BOOST_REQUIRE(asset != nullptr);
        BOOST_TEST(asset->getType() == am::AssetType::Other);
    }

    BOOST_AUTO_TEST_CASE(IdenticalContentHashMeansSameAsset) {
        auto& manager = am::AssetManager::getInstance();
        registerMockFactory(manager, 98765);  // all assets get the same hash

        am::AssetFactoryData data1( "a.txt", am::AssetType::Other);
        auto asset1 = manager.registerAsset(&data1);

        am::AssetFactoryData data2( "b.txt", am::AssetType::Other);
        auto asset2 = manager.registerAsset(&data2);

        BOOST_TEST(asset1.value()->id == asset2.value()->id);
        BOOST_TEST(asset1.value()->contentHash == asset2.value()->contentHash);
    }

    BOOST_AUTO_TEST_CASE(NonexistentAssetsReturnNull) {
        auto& manager = am::AssetManager::getInstance();
        auto randomUUID = boost::uuids::random_generator()();

        auto asset = manager.getByUUID<am::Asset>(randomUUID);
        BOOST_TEST(asset == nullptr);

        auto uids = manager.getUUIDsByPath("no_such_path.txt");
        BOOST_TEST(uids.empty());
    }

BOOST_AUTO_TEST_SUITE_END()
