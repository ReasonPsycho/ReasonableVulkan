#include <boost/test/unit_test.hpp>
#include "../src/AssetManager.hpp"
#include "../src/assets/configAsset/ConfigAsset.h"
#include "../src/assets/engineAssets/SceneAsset.h"
#include "../src/assets/engineAssets/PrefabAsset.h"
#include "../src/assets/materialAsset/MaterialAsset.hpp"
#include "../src/assets/textureAsset/TextureAsset.h"
#include "../src/assets/shaderAsset/ShaderAsset.h"
#include "../src/assets/shaderAsset/ShaderIncluder.hpp"
#include "../src/assets/shaderProgram/ShaderProgramAsset.h"
#include "../src/assets/meshAsset/MeshAsset.h"
#include <filesystem>
#include <fstream>

using namespace am;

BOOST_AUTO_TEST_SUITE(AssetsTestSuite)

BOOST_AUTO_TEST_CASE(ConfigAssetJsonAndBinaryTest) {
    std::filesystem::create_directories("res/.internal/tests/temp");
    auto id = boost::uuids::random_generator()();
    ConfigAsset config(id, "dummy_config.json");
    BOOST_CHECK(config.getType() == AssetType::Config);

    config.configData.SetObject();
    auto& alloc = config.configData.GetAllocator();
    config.configData.AddMember("testKey", rapidjson::Value("testValue", alloc), alloc);
    config.configData.AddMember("number", 42, alloc);

    BOOST_CHECK(config.calculateContentHash() != 0);
    BOOST_CHECK(config.getAssetData().has_value());

    rapidjson::Document doc;
    doc.SetObject();
    config.SaveAssetToJson(doc);
    BOOST_CHECK(doc.HasMember("testKey"));
    BOOST_CHECK_EQUAL(doc["number"].GetInt(), 42);

    std::string binPath = "res/.internal/tests/temp/temp_config_test.b_config";
    config.SaveAssetToBin(binPath);
    BOOST_CHECK(std::filesystem::exists(binPath));

    ConfigAsset loadedFromBin(binPath, AssetFormat::Binary);
    BOOST_CHECK(loadedFromBin.configData.IsObject());
    BOOST_CHECK(loadedFromBin.configData.HasMember("testKey"));
    BOOST_CHECK_EQUAL(loadedFromBin.configData["number"].GetInt(), 42);

    std::error_code ec;
    std::filesystem::remove(binPath, ec);
}

BOOST_AUTO_TEST_CASE(SceneAndPrefabAssetTest) {
    std::filesystem::create_directories("res/.internal/tests/temp");
    auto id = boost::uuids::random_generator()();
    SceneAsset scene(id, "dummy_scene.scene");
    BOOST_CHECK(scene.getType() == AssetType::Scene);

    scene.sceneData.SetObject();
    auto& alloc = scene.sceneData.GetAllocator();
    scene.sceneData.AddMember("sceneName", "MainScene", alloc);

    BOOST_CHECK(scene.getAssetData().has_value());
    size_t hash = scene.calculateContentHash();
    BOOST_CHECK(hash != 0);

    rapidjson::Document jsonDoc;
    jsonDoc.SetObject();
    scene.SaveAssetToJson(jsonDoc);
    BOOST_CHECK(jsonDoc.HasMember("sceneName"));

    std::string binPath = "res/.internal/tests/temp/temp_scene_test.b_scene";
    scene.SaveAssetToBin(binPath);
    BOOST_CHECK(std::filesystem::exists(binPath));

    SceneAsset loadedScene(binPath, AssetFormat::Binary);
    BOOST_CHECK(loadedScene.sceneData.IsObject());
    BOOST_CHECK(loadedScene.sceneData.HasMember("sceneName"));

    std::error_code ec;
    std::filesystem::remove(binPath, ec);

    PrefabAsset prefab(id, "dummy_prefab.prefab");
    BOOST_CHECK(prefab.getType() == AssetType::Prefab);
    prefab.prefabData.SetObject();
    auto& pAlloc = prefab.prefabData.GetAllocator();
    prefab.prefabData.AddMember("prefabName", "VehiclePrefab", pAlloc);

    std::string pBinPath = "res/.internal/tests/temp/temp_prefab_test.b_prefab";
    prefab.SaveAssetToBin(pBinPath);
    BOOST_CHECK(std::filesystem::exists(pBinPath));

    PrefabAsset loadedPrefab(pBinPath, AssetFormat::Binary);
    BOOST_CHECK(loadedPrefab.prefabData.HasMember("prefabName"));
    std::filesystem::remove(pBinPath, ec);
}

BOOST_AUTO_TEST_CASE(ShaderAssetCompilationAndDefinesTest) {
    std::filesystem::create_directories("res/.internal/tests/temp");
    std::string vertShaderPath = "res/.internal/tests/temp/temp_test_shader.vert";
    {
        std::ofstream out(vertShaderPath);
        out << "#version 450\n"
            << "#ifdef USE_EXTRA_COLOR\n"
            << "layout(location = 0) out vec4 outColor;\n"
            << "#endif\n"
            << "void main() {\n"
            << "    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n"
            << "}\n";
    }

    auto id = boost::uuids::random_generator()();
    ImportContext ctx(vertShaderPath, AssetType::Shader);

    ShaderAsset shader(id, ctx);
    BOOST_CHECK(shader.getType() == AssetType::Shader);
    BOOST_CHECK(shader.getStage() == ShaderStage::Vertex);
    BOOST_CHECK(!shader.getBytecode().empty());
    BOOST_CHECK(shader.calculateContentHash() != 0);

    bool recompiled = shader.setDefineAndRecompile("USE_EXTRA_COLOR", "1");
    BOOST_CHECK(recompiled);
    BOOST_CHECK_EQUAL(shader.getDefines().at("USE_EXTRA_COLOR"), "1");

    std::string binPath = "res/.internal/tests/temp/temp_test_shader.b_shader";
    shader.SaveAssetToBin(binPath);
    BOOST_CHECK(std::filesystem::exists(binPath));

    ShaderAsset loadedShader(binPath, AssetFormat::Binary);
    BOOST_CHECK(loadedShader.getStage() == ShaderStage::Vertex);
    BOOST_CHECK(!loadedShader.getBytecode().empty());

    std::error_code ec;
    std::filesystem::remove(vertShaderPath, ec);
    std::filesystem::remove(binPath, ec);
}

BOOST_AUTO_TEST_CASE(GridOverlayShaderCompilationTest) {
    if (std::filesystem::exists("res/shaders/glsl/entry/grid_overlay.vert")) {
        auto id = boost::uuids::random_generator()();
        ImportContext ctx("res/shaders/glsl/entry/grid_overlay.vert", AssetType::Shader);
        ShaderAsset shader(id, ctx);
        BOOST_CHECK(shader.getType() == AssetType::Shader);
        BOOST_CHECK(shader.getStage() == ShaderStage::Vertex);
        BOOST_CHECK(!shader.getBytecode().empty());
    }
}

BOOST_AUTO_TEST_CASE(ShaderProgramAssetTest) {
    std::filesystem::create_directories("res/.internal/tests/temp");
    std::string vertPath = "res/.internal/tests/temp/temp_prog_test.vert";
    std::string fragPath = "res/.internal/tests/temp/temp_prog_test.frag";
    {
        std::ofstream outV(vertPath);
        outV << "#version 450\nvoid main() { gl_Position = vec4(0.0); }\n";
        std::ofstream outF(fragPath);
        outF << "#version 450\nlayout(location = 0) out vec4 color;\nvoid main() { color = vec4(1.0); }\n";
    }

    auto& manager = AssetManager::getInstance();
    auto vId = manager.registerAsset(ImportContext(vertPath, AssetType::Shader));
    auto fId = manager.registerAsset(ImportContext(fragPath, AssetType::Shader));

    if (vId && fId) {
        auto progId = boost::uuids::random_generator()();
        ShaderProgramAsset prog(progId, "dummy_prog.shaderProgram");
        BOOST_CHECK(prog.getType() == AssetType::ShaderProgram);

        auto pData = prog.getAssetDataAs<ShaderProgramData>();
        BOOST_REQUIRE(pData != nullptr);
        pData->vertexShader = manager.getAssetInfo(vId.value()).value_or(nullptr);
        pData->fragmentShader = manager.getAssetInfo(fId.value()).value_or(nullptr);

        rapidjson::Document doc;
        doc.SetObject();
        prog.SaveAssetToJson(doc);
        BOOST_CHECK(doc.HasMember("uuid"));
    }

    std::error_code ec;
    std::filesystem::remove(vertPath, ec);
    std::filesystem::remove(fragPath, ec);
}

BOOST_AUTO_TEST_CASE(TextureAssetLoadingAndBinaryCacheTest) {
    auto id = boost::uuids::random_generator()();
    ImportContext ctx("res/textures/Missing_Texture.png", AssetType::Texture);
    TextureAsset tex(id, ctx);

    BOOST_CHECK(tex.getType() == AssetType::Texture);
    auto data = tex.getAssetDataAs<TextureData>();
    BOOST_REQUIRE(data != nullptr);
    BOOST_CHECK(data->width > 0);
    BOOST_CHECK(data->height > 0);
    BOOST_CHECK(data->channels > 0);
    BOOST_CHECK(!data->pixels.empty());
    BOOST_CHECK(tex.calculateContentHash() != 0);

    rapidjson::Document jsonDoc;
    jsonDoc.SetObject();
    tex.SaveAssetToJson(jsonDoc);
    BOOST_CHECK(jsonDoc.HasMember("uuid"));

    std::filesystem::create_directories("res/.internal/tests/temp");
    std::string binPath = "res/.internal/tests/temp/temp_texture_cache.b_texture";
    tex.SaveAssetToBin(binPath);
    BOOST_CHECK(std::filesystem::exists(binPath));

    TextureAsset loadedTex(binPath, AssetFormat::Binary);
    auto lData = loadedTex.getAssetDataAs<TextureData>();
    BOOST_REQUIRE(lData != nullptr);
    BOOST_CHECK_EQUAL(lData->width, data->width);
    BOOST_CHECK_EQUAL(lData->height, data->height);
    BOOST_CHECK_EQUAL(lData->pixels.size(), data->pixels.size());

    std::error_code ec;
    std::filesystem::remove(binPath, ec);
}

BOOST_AUTO_TEST_CASE(MaterialAssetPropertiesAndSerializationTest) {
    auto id = boost::uuids::random_generator()();
    MaterialAsset mat(id, "dummy_mat.material");
    BOOST_CHECK(mat.getType() == AssetType::Material);

    auto mData = mat.getAssetDataAs<MaterialData>();
    BOOST_REQUIRE(mData != nullptr);
    mData->metallicFactor = 0.8f;
    mData->roughnessFactor = 0.4f;
    mData->alphaCutoff = 0.5f;
    mData->isOpaque = true;

    rapidjson::Document doc;
    doc.SetObject();
    mat.SaveAssetToJson(doc);
    BOOST_CHECK(doc.HasMember("uuid"));

    size_t hash = mat.calculateContentHash();
    BOOST_CHECK(hash != 0);
}

BOOST_AUTO_TEST_SUITE_END()
