#include <boost/test/unit_test.hpp>
#include "../src/AssetManager.hpp"
#include "../src/assets/meshAsset/MeshAsset.h"
#include <filesystem>
#include <fstream>
#include <chrono>

using namespace am;

BOOST_AUTO_TEST_SUITE(AssetManagerAdvancedTestSuite)

BOOST_AUTO_TEST_CASE(AssetManagerHelperFunctionsTest) {
    BOOST_CHECK_EQUAL(GetExtensionFromAssetType(AssetType::Model), ".model");
    BOOST_CHECK_EQUAL(GetExtensionFromAssetType(AssetType::Texture), ".texture");
    BOOST_CHECK_EQUAL(GetExtensionFromAssetType(AssetType::Shader), ".shader");
    BOOST_CHECK_EQUAL(GetExtensionFromAssetType(AssetType::Material), ".material");
    BOOST_CHECK_EQUAL(GetExtensionFromAssetType(AssetType::Scene), ".scene");
    BOOST_CHECK_EQUAL(GetExtensionFromAssetType(AssetType::Prefab), ".prefab");

    BOOST_CHECK(GetAssetTypeFromExtension(".fbx") == AssetType::Model);
    BOOST_CHECK(GetAssetTypeFromExtension(".png") == AssetType::Texture);
    BOOST_CHECK(GetAssetTypeFromExtension(".vert") == AssetType::Shader);
    BOOST_CHECK(GetAssetTypeFromExtension(".frag") == AssetType::Shader);
    BOOST_CHECK(GetAssetTypeFromExtension(".spv") == AssetType::Shader);
    BOOST_CHECK(GetAssetTypeFromExtension(".spdv") == AssetType::Shader);
    BOOST_CHECK(GetAssetTypeFromExtension(".material") == AssetType::Material);
    BOOST_CHECK(GetAssetTypeFromExtension(".scene") == AssetType::Scene);
    BOOST_CHECK(GetAssetTypeFromExtension(".prefab") == AssetType::Prefab);

    BOOST_CHECK(GetEditorSavesToBin(AssetType::Texture));
    BOOST_CHECK(GetEditorSavesToBin(AssetType::Shader));
    BOOST_CHECK(!GetEditorSavesToBin(AssetType::Material));
    BOOST_CHECK(!GetEditorSavesToBin(AssetType::Scene));

    BOOST_CHECK_EQUAL(GetShaderSufix(ShaderStage::Vertex), "vs");
    BOOST_CHECK_EQUAL(GetShaderSufix(ShaderStage::Fragment), "fs");
    BOOST_CHECK_EQUAL(GetShaderSufix(ShaderStage::Compute), "cs");

    std::string binPath = GetBinPath("test_file.png", "");
    BOOST_CHECK(binPath.find(".b_png") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(AssetCreationAndLifecycleTest) {
    auto& manager = AssetManager::getInstance();
    std::filesystem::create_directories("res/.internal/tests/temp");
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();

    std::string matName = "LifecycleMat_" + std::to_string(now);
    std::string matPath = "res/.internal/tests/temp/" + matName + ".material";

    auto matUuidOpt = manager.createAsset(AssetType::Material, matPath, matName);
    BOOST_REQUIRE(matUuidOpt.has_value());
    auto matUuid = matUuidOpt.value();

    auto info = manager.getAssetInfo(matUuid);
    BOOST_REQUIRE(info.has_value());
    BOOST_CHECK_EQUAL(info.value()->lookUpName, matName);

    auto uuidByLookup = manager.getAssetUuid(matName);
    BOOST_REQUIRE(uuidByLookup.has_value());
    BOOST_CHECK(uuidByLookup.value() == matUuid);

    auto uuidByPath = manager.getAssetUuidByPath(matPath);
    BOOST_REQUIRE(uuidByPath.has_value());
    BOOST_CHECK(uuidByPath.value() == matUuid);

    manager.saveAsset(matUuid);

    auto texOpt = manager.registerAsset("res/textures/Missing_Texture.png");
    BOOST_REQUIRE(texOpt.has_value());
    bool reimported = manager.reimportAsset(texOpt.value());
    BOOST_CHECK(reimported);

    bool reimportedByPath = manager.reimportAsset(std::filesystem::path("res/textures/Missing_Texture.png"));
    BOOST_CHECK(reimportedByPath);

    std::error_code ec;
    std::filesystem::remove(matPath, ec);
    std::filesystem::remove(matPath + ".meta", ec);
}

BOOST_AUTO_TEST_CASE(AssetRegistryQueriesAndMetadataCacheTest) {
    auto& manager = AssetManager::getInstance();
    std::filesystem::create_directories("res/.internal/tests/temp");

    auto names = manager.getRegisteredAssetsNames();
    BOOST_CHECK(!names.empty());

    auto textureNames = manager.getRegisteredAssetsNames(AssetType::Texture);
    BOOST_CHECK(!textureNames.empty());

    auto uuids = manager.getRegisteredAssetsUuids();
    BOOST_CHECK(!uuids.empty());

    auto textureUuids = manager.getRegisteredAssetsUuids(AssetType::Texture);
    BOOST_CHECK(!textureUuids.empty());

    auto texData = manager.getAssetData(textureUuids[0]);
    BOOST_CHECK(texData.has_value());

    std::string metaCacheFile = "res/.internal/tests/temp/temp_meta_cache.json";
    bool savedCache = manager.saveMetaCache(metaCacheFile);
    BOOST_CHECK(savedCache);
    BOOST_CHECK(std::filesystem::exists(metaCacheFile));

    bool loadedCache = manager.loadMetaCache(metaCacheFile);
    BOOST_CHECK(loadedCache);

    std::string registryFile = "res/.internal/tests/temp/temp_registry_meta.json";
    bool savedReg = manager.saveRegistryMetadataToFile(registryFile);
    BOOST_CHECK(savedReg);
    BOOST_CHECK(std::filesystem::exists(registryFile));

    bool loadedReg = manager.loadRegistryMetadataFromFile(registryFile);
    BOOST_CHECK(loadedReg);

    std::error_code ec;
    std::filesystem::remove(metaCacheFile, ec);
    std::filesystem::remove(registryFile, ec);
}

BOOST_AUTO_TEST_CASE(MeshAssetBinarySerializationTest) {
    auto id = boost::uuids::random_generator()();
    MeshAsset mesh(id, "dummy_mesh.mesh");
    BOOST_CHECK(mesh.getType() == AssetType::Mesh);

    auto mData = mesh.getAssetDataAs<MeshData>();
    BOOST_REQUIRE(mData != nullptr);

    VertexAsset v1{};
    v1.Position = glm::vec3(0.0f, 0.0f, 0.0f);
    v1.Color = glm::vec4(1.0f);
    VertexAsset v2{};
    v2.Position = glm::vec3(1.0f, 1.0f, 1.0f);
    v2.Color = glm::vec4(0.5f);

    mData->vertices = {v1, v2};
    mData->indices = {0, 1};
    mData->boundingBoxMin = glm::vec3(0.0f);
    mData->boundingBoxMax = glm::vec3(1.0f);

    size_t hash = mesh.calculateContentHash();
    BOOST_CHECK(hash != 0);

    std::filesystem::create_directories("res/.internal/tests/temp");
    std::string binPath = "res/.internal/tests/temp/temp_mesh_test.b_mesh";
    mesh.SaveAssetToBin(binPath);
    BOOST_CHECK(std::filesystem::exists(binPath));

    MeshAsset loadedMesh(binPath, AssetFormat::Binary);
    auto lData = loadedMesh.getAssetDataAs<MeshData>();
    BOOST_REQUIRE(lData != nullptr);
    BOOST_CHECK_EQUAL(lData->vertices.size(), 2);
    BOOST_CHECK_EQUAL(lData->indices.size(), 2);
    BOOST_CHECK_CLOSE(lData->boundingBoxMax.x, 1.0f, 0.001f);

    std::error_code ec;
    std::filesystem::remove(binPath, ec);
}

BOOST_AUTO_TEST_CASE(CheckAndRecompileOutdatedShadersTest) {
    auto& manager = AssetManager::getInstance();

    // Register an existing shader (e.g. mesh.vert)
    std::string shaderPath = "res/shaders/glsl/mesh.vert";
    if (std::filesystem::exists(shaderPath)) {
        auto uuidOpt = manager.registerAsset(shaderPath);
        BOOST_REQUIRE(uuidOpt.has_value());

        auto infoOpt = manager.getAssetInfo(uuidOpt.value());
        BOOST_REQUIRE(infoOpt.has_value());
        BOOST_CHECK(std::filesystem::exists(infoOpt.value()->path));

        // Calling checkAndRecompileOutdatedShaders should process without error
        BOOST_CHECK_NO_THROW(manager.checkAndRecompileOutdatedShaders());
    }
}

BOOST_AUTO_TEST_CASE(ThumbnailExclusionFromAutoImportTest) {
    // 1. Test isExcludedFromImport helper with various path formats
    BOOST_CHECK(AssetManager::isExcludedFromImport("res/.cache/thumbnails/test.png"));
    BOOST_CHECK(AssetManager::isExcludedFromImport("res\\.cache\\thumbnails\\test.png"));
    BOOST_CHECK(AssetManager::isExcludedFromImport(".cache/thumbnails/test.png"));
    BOOST_CHECK(AssetManager::isExcludedFromImport(".cache\\thumbnails\\test.png"));
    BOOST_CHECK(AssetManager::isExcludedFromImport("C:/project/res/.cache/thumbnails/uuid.png"));
    BOOST_CHECK(AssetManager::isExcludedFromImport("C:\\project\\res\\.cache\\thumbnails\\uuid.png"));

    BOOST_CHECK(!AssetManager::isExcludedFromImport("res/textures/Missing_Texture.png"));
    BOOST_CHECK(!AssetManager::isExcludedFromImport("res/models/cube.fbx"));
    BOOST_CHECK(!AssetManager::isExcludedFromImport("res/.cache/config/fileBrowser.config"));
    BOOST_CHECK(!AssetManager::isExcludedFromImport(""));

    // 2. Test that attempting to register or reimport excluded paths fails gracefully
    auto& manager = AssetManager::getInstance();
    auto regOpt = manager.registerAsset("res/.cache/thumbnails/test_thumb.png");
    BOOST_CHECK(!regOpt.has_value());

    auto regOptWithLookup = manager.registerAsset("res/.cache/thumbnails/test_thumb.png", "test_thumb.png");
    BOOST_CHECK(!regOptWithLookup.has_value());

    ImportContext ctx("res/.cache/thumbnails/test_thumb.png", AssetType::Texture);
    auto regCtxOpt = manager.registerAsset(ctx);
    BOOST_CHECK(!regCtxOpt.has_value());

    bool reimported = manager.reimportAsset(std::filesystem::path("res/.cache/thumbnails/test_thumb.png"));
    BOOST_CHECK(!reimported);
}

BOOST_AUTO_TEST_SUITE_END()
