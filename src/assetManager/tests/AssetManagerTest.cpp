#include <boost/test/unit_test.hpp>
#include "../src/AssetManager.hpp"
#include "../include/Asset.hpp"

struct MockData
{

};

// Mock Asset class for testing
class MockAsset : public am::Asset {
public:
    explicit MockAsset(am::ImportContext assetFactoryData)
        : Asset(assetFactoryData), mockHash(0) {}

    explicit MockAsset(const std::string& path, am::AssetFormat format)
        : Asset(path, format), mockHash(0) {}

    void setMockHash(size_t hash) { mockHash = hash; }

    size_t calculateContentHash() const override { return mockHash; }

    [[nodiscard]] am::AssetType getType() const override {
        return am::AssetType::Other;
    }

    std::any getAssetData() override { return &data; }

    MockData data;
private:
    size_t mockHash;
};

// Utility function to register mock factory (reusable)
void registerMockFactory(am::AssetManager& manager, size_t hash = 12345) {
    manager.registerFactory(am::AssetType::Other, [hash](am::ImportContext& data) {
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

        am::ImportContext context( "test_path.txt", am::AssetType::Texture);
        auto assetId = manager.registerAsset(context);
        BOOST_REQUIRE(assetId.has_value());

        auto result = manager.getAssetInfo(assetId.value());
        BOOST_REQUIRE(result.has_value());

        BOOST_TEST(result.value()->importPath == "test_path.txt");
        BOOST_TEST(result.value()->type == am::AssetType::Texture);
        BOOST_TEST(result.value()->id == assetId.value());
    }

    BOOST_AUTO_TEST_CASE(DuplicatePathReturnsSameAssetId) {
        auto& manager = am::AssetManager::getInstance();

        am::ImportContext data1( "duplicate.txt", am::AssetType::Texture);
        auto asset1 = manager.registerAsset(data1);

        am::ImportContext data2( "duplicate.txt", am::AssetType::Texture);
        auto asset2 = manager.registerAsset(data2);

        BOOST_TEST(asset1.value() == asset2.value());
    }

    BOOST_AUTO_TEST_CASE(GetAssetByUUIDReturnsCorrectInstance) {
        auto& manager = am::AssetManager::getInstance();

        am::ImportContext data("uuid_test.txt", am::AssetType::Texture);
        auto assetId = manager.registerAsset(data);

        auto asset = manager.getByUUID<am::Asset>(assetId.value());
        BOOST_REQUIRE(asset != nullptr);
        BOOST_TEST(asset->getType() == am::AssetType::Texture);
    }

    BOOST_AUTO_TEST_CASE(IdenticalContentHashMeansSameAsset) {
        auto& manager = am::AssetManager::getInstance();

        am::ImportContext data1( "a.txt", am::AssetType::Texture);
        auto asset1 = manager.registerAsset(data1);

        am::ImportContext data2( "b.txt", am::AssetType::Texture);
        auto asset2 = manager.registerAsset(data2);

        // Textures with same data (or same empty file) should have same hash
        BOOST_TEST(asset1.value() == asset2.value());
    }

    BOOST_AUTO_TEST_CASE(NonexistentAssetsReturnNull) {
        auto& manager = am::AssetManager::getInstance();
        auto randomUUID = boost::uuids::random_generator()();

        auto asset = manager.getByUUID<am::Asset>(randomUUID);
        BOOST_TEST(asset == nullptr);
    }

BOOST_AUTO_TEST_SUITE_END()
