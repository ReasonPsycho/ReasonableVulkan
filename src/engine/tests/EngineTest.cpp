#include <boost/test/unit_test.hpp>
#include "../Engine.h"
#include "ecs/Scene.h"
#include "TestHelper.hpp"
#include "../ecs/components/TagComponent.hpp"
#include "systems/editorSystem/EditorSystem.hpp"
#include "systems/renderingSystem/componets/RendererComponent.hpp"
#include "systems/renderingSystem/componets/MeshComponent.hpp"
#include "systems/scriptSystem/ScriptHotReloadManager.hpp"
#include <filesystem>
#include <fstream>

using namespace engine;
using namespace engine::ecs;
using namespace engine::test;

BOOST_AUTO_TEST_SUITE(EngineTestSuite)

BOOST_AUTO_TEST_CASE(MultipleActiveScenesTest) {
    Engine engine = CreateTestEngine();

    auto scene1 = engine.CreateScene("Scene1");
    auto scene2 = engine.CreateScene("Scene2");
    auto scene3 = engine.CreateScene("Scene3");

    BOOST_REQUIRE(scene1);
    BOOST_REQUIRE(scene2);
    BOOST_REQUIRE(scene3);

    // Initial state: inactive
    BOOST_REQUIRE(!scene1->IsActive());
    BOOST_REQUIRE(!scene2->IsActive());
    BOOST_REQUIRE(!scene3->IsActive());
    BOOST_REQUIRE_EQUAL(engine.GetActiveScenes().size(), 0);

    // Activate scene1 and scene2
    engine.SetActiveScene("Scene1");
    scene2->SetActive(true);

    BOOST_REQUIRE(scene1->IsActive());
    BOOST_REQUIRE(scene2->IsActive());
    BOOST_REQUIRE(!scene3->IsActive());

    auto activeScenes = engine.GetActiveScenes();
    BOOST_REQUIRE_EQUAL(activeScenes.size(), 2);

    // Entities with TransformComponent
    Entity e1 = scene1->CreateEntity();
    setLocalPosition(scene1->GetComponent<TransformComponent>(e1), {10.0f, 20.0f, 0.0f});

    Entity e2 = scene2->CreateEntity();
    setLocalPosition(scene2->GetComponent<TransformComponent>(e2), {100.0f, 200.0f, 0.0f});

    Entity e3 = scene3->CreateEntity();
    setLocalPosition(scene3->GetComponent<TransformComponent>(e3), {1.0f, 2.0f, 0.0f});

    BOOST_REQUIRE(isDirty(scene1->GetComponent<TransformComponent>(e1)));
    BOOST_REQUIRE(isDirty(scene2->GetComponent<TransformComponent>(e2)));
    BOOST_REQUIRE(isDirty(scene3->GetComponent<TransformComponent>(e3)));

    // Update active scenes via engine.Update
    engine.Update(0.0f);
    BOOST_REQUIRE(!isDirty(scene1->GetComponent<TransformComponent>(e1)));
    BOOST_REQUIRE(!isDirty(scene2->GetComponent<TransformComponent>(e2)));
    BOOST_REQUIRE(isDirty(scene3->GetComponent<TransformComponent>(e3))); // Inactive scene unchanged

    // Deactivate scene1
    engine.SetActiveScene("Scene1", false);
    BOOST_REQUIRE(!scene1->IsActive());
    BOOST_REQUIRE(scene2->IsActive());
    BOOST_REQUIRE_EQUAL(engine.GetActiveScenes().size(), 1);
    BOOST_REQUIRE_EQUAL(engine.GetActiveScenes()[0], scene2);

    // Deactivate scene2 via SetSceneActive
    engine.SetSceneActive("Scene2", false);
    BOOST_REQUIRE(!scene2->IsActive());
    BOOST_REQUIRE_EQUAL(engine.GetActiveScenes().size(), 0);
    BOOST_REQUIRE(engine.GetActiveScene() == nullptr);

    // Cleanup
    engine.RemoveScene("Scene1");
    engine.RemoveScene("Scene2");
    engine.RemoveScene("Scene3");
}

BOOST_AUTO_TEST_CASE(EngineEditorSystemAndMultiSceneTest) {
    Engine engine = CreateTestEngine();
    engine.Initialize();

    // EditorSystem should belong to engine
    auto editorSystem = engine.GetEditorSystem();
    BOOST_REQUIRE(editorSystem != nullptr);
    BOOST_REQUIRE_EQUAL(editorSystem->engine, &engine);

    // Create multiple scenes
    auto sceneA = engine.CreateScene("LevelA");
    auto sceneB = engine.CreateScene("LevelB");
    BOOST_REQUIRE(sceneA);
    BOOST_REQUIRE(sceneB);
    BOOST_REQUIRE_EQUAL(sceneA->GetName(), "LevelA");
    BOOST_REQUIRE_EQUAL(sceneB->GetName(), "LevelB");

    // EditorSystem should NOT be registered as a per-scene system in scenes
    auto systemsA = sceneA->GetSystems();
    bool hasEditorInSceneA = false;
    for (const auto& [typeIdx, sys] : systemsA) {
        if (sys->name == "EditorSystem") {
            hasEditorInSceneA = true;
        }
    }
    BOOST_REQUIRE(!hasEditorInSceneA);

    // Target scene in EditorSystem
    editorSystem->SetTargetScene(sceneA);
    BOOST_REQUIRE_EQUAL(editorSystem->GetTargetScene(), sceneA.get());

    Entity ea = sceneA->CreateEntity();
    editorSystem->SetSelectedEntity(ea);
    BOOST_REQUIRE_EQUAL(editorSystem->GetSelectedEntity(), ea);

    editorSystem->SetEntityName(ea, "Hero", sceneA.get());
    BOOST_REQUIRE_EQUAL(editorSystem->GetEntityRawName(ea, sceneA.get()), "Hero");

    // Switch target scene to sceneB
    editorSystem->SetTargetScene(sceneB);
    BOOST_REQUIRE_EQUAL(editorSystem->GetTargetScene(), sceneB.get());

    Entity eb = sceneB->CreateEntity();
    editorSystem->SetEntityName(eb, "Villain", sceneB.get());
    BOOST_REQUIRE_EQUAL(editorSystem->GetEntityRawName(eb, sceneB.get()), "Villain");

    // Cleanup
    engine.RemoveScene("LevelA");
    engine.RemoveScene("LevelB");
}

BOOST_AUTO_TEST_CASE(EngineSaveEntityAsPrefabTest) {
    Engine engine = CreateTestEngine();
    engine.Initialize();
    auto sourceScene = engine.CreateScene("SavePrefabSourceScene");
    BOOST_REQUIRE(sourceScene);

    Entity house = sourceScene->CreateEntity("House");
    sourceScene->AddComponent<TagComponent>(house, TagComponent{"Building"});

    Entity door = sourceScene->CreateEntity("Door", house);
    sourceScene->AddComponent<TagComponent>(door, TagComponent{"Interactable"});

    // Save entity as prefab via EditorSystem
    auto editorSystem = engine.GetEditorSystem();
    BOOST_REQUIRE(editorSystem != nullptr);
    std::filesystem::create_directories("res/.internal/tests/temp");
    bool saved = editorSystem->SaveEntityAsPrefab(house, "res/.internal/tests/temp", sourceScene.get());
    BOOST_CHECK(saved);

    std::filesystem::path expectedPath = "res/.internal/tests/temp/House.prefab";
    std::error_code ec;
    BOOST_CHECK(std::filesystem::exists(expectedPath, ec));

    auto am = engine.assetManagerInterface;
    if (am) {
        auto uuidOpt = am->getAssetUuidByPath(expectedPath);
        BOOST_CHECK(uuidOpt.has_value());
    }

    engine.RemoveScene("SavePrefabSourceScene");
}

BOOST_AUTO_TEST_CASE(EnginePrefabLoadAndInstantiateTest) {
    Engine engine = CreateTestEngine();
    engine.Initialize();
    auto sourceScene = engine.CreateScene("PrefabLoadSourceScene");
    BOOST_REQUIRE(sourceScene);

    Entity tower = sourceScene->CreateEntity("Tower");
    sourceScene->AddComponent<TagComponent>(tower, TagComponent{"Structure"});

    Entity roof = sourceScene->CreateEntity("Roof", tower);
    sourceScene->AddComponent<TagComponent>(roof, TagComponent{"RoofTop"});

    auto editorSystem = engine.GetEditorSystem();
    BOOST_REQUIRE(editorSystem != nullptr);
    std::filesystem::create_directories("res/.internal/tests/temp");
    bool saved = editorSystem->SaveEntityAsPrefab(tower, "res/.internal/tests/temp", sourceScene.get());
    BOOST_REQUIRE(saved);

    std::filesystem::path prefabPath = "res/.internal/tests/temp/Tower.prefab";
    auto am = engine.assetManagerInterface;
    BOOST_REQUIRE(am != nullptr);
    auto uuidOpt = am->getAssetUuidByPath(prefabPath);
    if (!uuidOpt) uuidOpt = am->registerAsset(prefabPath.string());
    BOOST_REQUIRE(uuidOpt.has_value());

    auto targetScene = engine.CreateScene("PrefabLoadTargetScene");
    BOOST_REQUIRE(targetScene);

    Entity instantiated = targetScene->InstantiatePrefab(uuidOpt.value());
    BOOST_REQUIRE(instantiated != MAX_ENTITIES);
    BOOST_CHECK_EQUAL(targetScene->GetEntityName(instantiated), "Tower");
    BOOST_CHECK(targetScene->HasComponent<TagComponent>(instantiated));
    BOOST_CHECK_EQUAL(targetScene->GetComponent<TagComponent>(instantiated).tag, "Structure");

    const auto& children = targetScene->GetChildren(instantiated);
    BOOST_REQUIRE_EQUAL(children.size(), 1);
    BOOST_CHECK_EQUAL(targetScene->GetEntityName(children[0]), "Roof");

    engine.RemoveScene("PrefabLoadSourceScene");
    engine.RemoveScene("PrefabLoadTargetScene");
}

BOOST_AUTO_TEST_CASE(EngineInitializationShaderAndScriptCheckTest) {
    Engine engine = CreateTestEngine();
    BOOST_CHECK_NO_THROW(engine.Initialize());
}

BOOST_AUTO_TEST_CASE(ScriptHotReloadManagerStalenessTest) {
    auto& manager = ecs::ScriptHotReloadManager::Instance();
    std::filesystem::path tempScriptsDir = std::filesystem::temp_directory_path() / "rv_temp_scripts_test";
    std::filesystem::create_directories(tempScriptsDir);

    // Non-existent DLL should be detected as outdated if script files exist
    std::ofstream dummyScript(tempScriptsDir / "TestScript.hpp");
    dummyScript << "// test script" << std::endl;
    dummyScript.close();

    bool outdated = manager.AreScriptsOutdated(tempScriptsDir);
    BOOST_CHECK(outdated);

    std::error_code ec;
    std::filesystem::remove_all(tempScriptsDir, ec);
}

BOOST_AUTO_TEST_SUITE_END()
