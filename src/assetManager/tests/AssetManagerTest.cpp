#include <boost/test/unit_test.hpp>
#include "../src/AssetManager.hpp"
#include "../include/Asset.hpp"
#include "../src/assets/engineAssets/PrefabAsset.h"

struct MockData
{

};

// Mock Asset class for testing
class MockAsset : public am::Asset {
public:
    explicit MockAsset(const boost::uuids::uuid& id, std::string path)
        : Asset(id, path), mockHash(0) {}

    explicit MockAsset(const boost::uuids::uuid& id, const am::ImportContext& assetFactoryData)
        : Asset(id, assetFactoryData), mockHash(0) {}

    explicit MockAsset(const std::string& path, am::AssetFormat format)
        : Asset(path, format), mockHash(0) {}

    void setMockHash(size_t hash) { mockHash = hash; }

    size_t calculateContentHash() const override { return mockHash; }

    [[nodiscard]] am::AssetType getType() const override {
        return am::AssetType::Other;
    }

    void SaveAssetToJson(rapidjson::Document& document) override {}
    void SaveAssetMetadata(rapidjson::Document& document) override {}
    void LoadAssetMetadata(rapidjson::Document& document) override {}

    std::any getAssetData() override { return &data; }

    MockData data;
private:
    size_t mockHash;
};

// Utility function to register mock factory (reusable)
void registerMockFactory(am::AssetManager& manager, size_t hash = 12345) {
    manager.RegisterAssetType<MockAsset>();
}

static void ensureFileExists(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        std::ofstream out(path);
        out << "dummy test content\n";
    }
}

BOOST_AUTO_TEST_SUITE(AssetManagerTests)

    BOOST_AUTO_TEST_CASE(SingletonInstanceIsSame) {
        auto& instance1 = am::AssetManager::getInstance();
        auto& instance2 = am::AssetManager::getInstance();

        BOOST_TEST(&instance1 == &instance2, "AssetManager singleton instances must be equal");
    }

    BOOST_AUTO_TEST_CASE(RegisterAndLookupAssetWorks) {
        auto& manager = am::AssetManager::getInstance();

        am::ImportContext context("res/textures/Missing_Texture.png", am::AssetType::Texture);
        auto assetId = manager.registerAsset(context);
        BOOST_REQUIRE(assetId.has_value());

        auto result = manager.getAssetInfo(assetId.value());
        BOOST_REQUIRE(result.has_value());

        BOOST_TEST(result.value()->type == am::AssetType::Texture);
        BOOST_TEST(result.value()->id == assetId.value());
    }

    BOOST_AUTO_TEST_CASE(DuplicatePathReturnsSameAssetId) {
        auto& manager = am::AssetManager::getInstance();

        am::ImportContext data1("res/textures/Missing_Texture.png", am::AssetType::Texture);
        auto asset1 = manager.registerAsset(data1);

        am::ImportContext data2("res/textures/Missing_Texture.png", am::AssetType::Texture);
        auto asset2 = manager.registerAsset(data2);

        BOOST_REQUIRE(asset1.has_value() && asset2.has_value());
        BOOST_TEST(asset1.value() == asset2.value());
    }

    BOOST_AUTO_TEST_CASE(GetAssetByUUIDReturnsCorrectInstance) {
        auto& manager = am::AssetManager::getInstance();

        am::ImportContext data("res/textures/Missing_Texture.png", am::AssetType::Texture);
        auto assetId = manager.registerAsset(data);
        BOOST_REQUIRE(assetId.has_value());

        auto asset = manager.getByUUID<am::Asset>(assetId.value());
        BOOST_REQUIRE(asset != nullptr);
        BOOST_TEST(asset->getType() == am::AssetType::Texture);
    }

    BOOST_AUTO_TEST_CASE(RealTextureRegistrationWorks) {
        auto& manager = am::AssetManager::getInstance();

        am::ImportContext data1("res/textures/Missing_Texture.png", am::AssetType::Texture);
        auto asset1 = manager.registerAsset(data1);
        BOOST_REQUIRE(asset1.has_value());

        am::ImportContext data2("res/textures/Missing_Texture.png", am::AssetType::Texture);
        auto asset2 = manager.registerAsset(data2);
        BOOST_REQUIRE(asset2.has_value());

        BOOST_TEST(asset1.value() == asset2.value());
    }

    BOOST_AUTO_TEST_CASE(NonexistentAssetsReturnNull) {
        auto& manager = am::AssetManager::getInstance();
        auto randomUUID = boost::uuids::random_generator()();

        auto asset = manager.getByUUID<am::Asset>(randomUUID);
        BOOST_TEST(asset == nullptr);
    }

    BOOST_AUTO_TEST_CASE(PrefabAssetRegistrationAndTypeCheck) {
        auto& manager = am::AssetManager::getInstance();
        std::filesystem::create_directories("res/.internal/tests/temp");
        auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        std::string name = "TestPrefab_" + std::to_string(now);
        std::string path = "res/.internal/tests/temp/" + name + ".prefab";
        auto prefabUuidOpt = manager.createAsset(am::AssetType::Prefab, path, name);
        BOOST_REQUIRE(prefabUuidOpt.has_value());

        auto assetInfo = manager.getAssetInfo(prefabUuidOpt.value());
        BOOST_REQUIRE(assetInfo.has_value());
        BOOST_CHECK(assetInfo.value()->type == am::AssetType::Prefab);

        auto prefabAsset = manager.getByUUID<am::PrefabAsset>(prefabUuidOpt.value());
        BOOST_REQUIRE(prefabAsset != nullptr);
        BOOST_CHECK(prefabAsset->getType() == am::AssetType::Prefab);
    }

BOOST_AUTO_TEST_SUITE_END()
