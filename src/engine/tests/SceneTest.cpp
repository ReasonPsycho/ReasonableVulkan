#define BOOST_TEST_MODULE EngineAndSceneTest
#include <boost/test/unit_test.hpp>
#include "../Engine.h"
#include "ecs/NameComponent.hpp"
#include "ecs/TagComponent.hpp"
#include "systems/renderingSystem/componets/RendererComponent.hpp"
#include "systems/renderingSystem/componets/MeshComponent.hpp"

using namespace engine;
using namespace engine::ecs;

// Sample component
struct Position {
    float x, y;
};

// Sample system
class MovementSystem : public System<MovementSystem, Position> {
public:
    explicit MovementSystem(Scene* scene) : System(scene) {}
    void Update(float deltaTime) override {
        for (auto entity : entities) {
            auto& pos = scene->GetComponent<Position>(entity);
            pos.x += deltaTime;
            pos.y += deltaTime;
        }
    }

    size_t EntityCount() const {
        return entities.size();
    }

    [[=NonSerialized{}]]
    std::vector<Entity> entities;

    [[=Range{0.0f, 100.0f, 0.5f}, =Tooltip{"Speed of movement"}]]
    float speed = 1.0f;

    [[=Tooltip{"Allow movement system updates"}]]
    bool enabled = true;

protected:
    void OnComponentAdded(ComponentID componentID, std::type_index type) override {
        entities.push_back(componentID);
    }
    void OnEntityRemoved(ComponentID componentID, std::type_index type) override {
        std::erase(entities, componentID);
    }
};

BOOST_AUTO_TEST_CASE(EngineAndSceneFunctionalTest) {
    Engine& engine = Engine::GetInstance();

    // Scene management
    std::shared_ptr<Scene> scene = engine.CreateScene("TestScene");
    BOOST_REQUIRE(scene);
    BOOST_REQUIRE(engine.GetScene("TestScene") == scene);
    engine.SetActiveScene("TestScene");
    BOOST_REQUIRE(engine.GetActiveScene() == scene);

    // Register component & system
    scene->RegisterComponent<Position>();
    auto movementSystem = scene->RegisterSystem<MovementSystem>();
    BOOST_REQUIRE(movementSystem);
    BOOST_REQUIRE_EQUAL(movementSystem->name, "MovementSystem");

    // Entity creation & component manipulation
    Entity e1 = scene->CreateEntity();
    Entity e2 = scene->CreateEntity();
    BOOST_REQUIRE(scene->HasComponent<Position>(e1) == false);

    scene->AddComponent<Position>(e1, {1.0f, 2.0f});
    scene->AddComponent<Position>(e2, {3.0f, 4.0f});
    BOOST_REQUIRE(scene->HasComponent<Position>(e1));
    BOOST_REQUIRE(scene->GetComponent<Position>(e2).x == 3.0f);
    BOOST_REQUIRE(scene->IsComponentActive<Position>(e2));
    scene->SetComponentActive<Position>(e2,false);
    BOOST_REQUIRE(!scene->IsComponentActive<Position>(e2));
    scene->SetComponentActive<Position>(e2,true);

    // Test system matching
    BOOST_REQUIRE(movementSystem->EntityCount() == 2);

    // Test system update logic
    scene->Update(1.0f);
    auto& pos1 = scene->GetComponent<Position>(e1);
    BOOST_REQUIRE(pos1.x == 2.0f);
    BOOST_REQUIRE(pos1.y == 3.0f);

    // Component removal
    scene->RemoveComponent<Position>(e2);
    BOOST_REQUIRE(!scene->HasComponent<Position>(e2));
    BOOST_REQUIRE(movementSystem->EntityCount() == 1); // system should react

    // Entity active toggle
    scene->SetEntityActive(e1, false);
    BOOST_REQUIRE(!scene->IsEntityActive(e1));
    scene->SetEntityActive(e1, true);
    BOOST_REQUIRE(scene->IsEntityActive(e1));

    // Scene graph tests
    Entity parent = scene->CreateEntity();
    Entity child1 = scene->CreateEntity();
    Entity child2 = scene->CreateEntity();
    scene->SetParent(child1, parent);
    scene->SetParent(child2, parent);

    BOOST_REQUIRE(scene->GetParent(child1) == parent);
    BOOST_REQUIRE(scene->GetChildren(parent).size() == 2);
    BOOST_REQUIRE(scene->HasParent(child1));
    scene->RemoveParent(child1);
    BOOST_REQUIRE(!scene->HasParent(child1));

    // Entity destruction
    scene->DestroyEntity(e1);
    BOOST_REQUIRE(!scene->HasComponent<Position>(e1));
    BOOST_REQUIRE(movementSystem->EntityCount() == 0);

    // Scene deletion
    engine.RemoveScene("TestScene");
    BOOST_REQUIRE(engine.GetScene("TestScene") == nullptr);
}

BOOST_AUTO_TEST_CASE(TransformSystemMatrixAndDirtyPropagationTest) {
    Engine& engine = Engine::GetInstance();
    auto scene = engine.CreateScene("TransformScene");
    engine.SetActiveScene("TransformScene");


    // Register Transform component and system
    auto transformSystem = scene->GetSystem<TransformSystem>();;
    BOOST_REQUIRE(transformSystem);

    // Create entities
    Entity parent = scene->CreateEntity();
    Entity child1 = scene->CreateEntity();
    Entity child2 = scene->CreateEntity();

    // Parent-child relationships
    scene->SetParent(child1, parent);
    scene->SetParent(child2, parent);

    // Set positions
    setLocalPosition(scene->GetComponent<TransformComponent>(parent), {10.0f, 0.0f, 0.0f});
    setLocalPosition(scene->GetComponent<TransformComponent>(child1), {0.0f, 5.0f, 0.0f});
    setLocalPosition(scene->GetComponent<TransformComponent>(child2), {0.0f, 0.0f, 3.0f});

    // Verify all are dirty before update
    BOOST_REQUIRE(isDirty(scene->GetComponent<TransformComponent>(parent)));
    BOOST_REQUIRE(isDirty(scene->GetComponent<TransformComponent>(child1)));
    BOOST_REQUIRE(isDirty(scene->GetComponent<TransformComponent>(child2)));

    // Update system
    scene->Update(0.0f);

    // Validate dirty flags cleared
    BOOST_REQUIRE(!isDirty(scene->GetComponent<TransformComponent>(parent)));
    BOOST_REQUIRE(!isDirty(scene->GetComponent<TransformComponent>(child1)));
    BOOST_REQUIRE(!isDirty(scene->GetComponent<TransformComponent>(child2)));

    // Validate world transforms
    const glm::vec3 worldPosParent = getGlobalPosition(scene->GetComponent<TransformComponent>(parent));
    const glm::vec3 worldPosChild1 = getGlobalPosition(scene->GetComponent<TransformComponent>(child1));
    const glm::vec3 worldPosChild2 = getGlobalPosition(scene->GetComponent<TransformComponent>(child2));

    BOOST_REQUIRE_CLOSE(worldPosParent.x, 10.0f, 0.001f);
    BOOST_REQUIRE_CLOSE(worldPosParent.y, 0.0f, 0.001f);
    BOOST_REQUIRE_CLOSE(worldPosParent.z, 0.0f, 0.001f);

    BOOST_REQUIRE_CLOSE(worldPosChild1.x, 10.0f, 0.001f);  // 10 + 0
    BOOST_REQUIRE_CLOSE(worldPosChild1.y, 5.0f, 0.001f);   // 0 + 5
    BOOST_REQUIRE_CLOSE(worldPosChild1.z, 0.0f, 0.001f);

    BOOST_REQUIRE_CLOSE(worldPosChild2.x, 10.0f, 0.001f);
    BOOST_REQUIRE_CLOSE(worldPosChild2.y, 0.0f, 0.001f);
    BOOST_REQUIRE_CLOSE(worldPosChild2.z, 3.0f, 0.001f);

    // Now mark only child1 dirty
    setLocalPosition(scene->GetComponent<TransformComponent>(child1), {0.0f, 10.0f, 0.0f});
    BOOST_REQUIRE(isDirty(scene->GetComponent<TransformComponent>(child1)));

    scene->Update(0.0f);
    BOOST_REQUIRE(!isDirty(scene->GetComponent<TransformComponent>(child1)));

    // Confirm new world position
    const glm::vec3 newWorldPosChild1 = getGlobalPosition(scene->GetComponent<TransformComponent>(child1));
    BOOST_REQUIRE_CLOSE(newWorldPosChild1.x, 10.0f, 0.001f);
    BOOST_REQUIRE_CLOSE(newWorldPosChild1.y, 10.0f, 0.001f);
    BOOST_REQUIRE_CLOSE(newWorldPosChild1.z, 0.0f, 0.001f);

    engine.RemoveScene("TransformScene");
}

BOOST_AUTO_TEST_CASE(StaticReflectionAndSerializationTest) {
    // 1. Test TransformComponent reflection and serialization
    TransformComponent transform;
    transform.position = {1.5f, 2.5f, 3.5f};
    transform.scale = {2.0f, 2.0f, 2.0f};
    transform.localMatrix = glm::mat4(5.0f);
    transform.isDirty = false;

    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    SerializeTypeToJson(transform, doc, allocator);

    // Verify transient fields are omitted
    BOOST_REQUIRE(doc.HasMember("position"));
    BOOST_REQUIRE(doc.HasMember("rotation"));
    BOOST_REQUIRE(doc.HasMember("scale"));
    BOOST_REQUIRE(!doc.HasMember("localMatrix"));
    BOOST_REQUIRE(!doc.HasMember("globalMatrix"));
    BOOST_REQUIRE(!doc.HasMember("isDirty"));

    // Deserialize into a new component
    TransformComponent deserializedTransform;
    DeserializeTypeFromJson(deserializedTransform, doc);

    BOOST_REQUIRE_CLOSE(deserializedTransform.position.x, 1.5f, 0.001f);
    BOOST_REQUIRE_CLOSE(deserializedTransform.position.y, 2.5f, 0.001f);
    BOOST_REQUIRE_CLOSE(deserializedTransform.position.z, 3.5f, 0.001f);
    BOOST_REQUIRE_CLOSE(deserializedTransform.scale.x, 2.0f, 0.001f);
    BOOST_REQUIRE(deserializedTransform.isDirty == true); // PostDeserialize sets isDirty

    // 2. Test CameraComponent reflection and serialization
    CameraComponent camera;
    camera.fov = 60.0f;
    camera.nearPlane = 0.5f;
    camera.farPlane = 500.0f;
    camera.active = true;

    rapidjson::Document camDoc;
    camDoc.SetObject();
    SerializeTypeToJson(camera, camDoc, camDoc.GetAllocator());

    BOOST_REQUIRE(camDoc.HasMember("fov"));
    BOOST_REQUIRE(camDoc.HasMember("nearPlane"));
    BOOST_REQUIRE(camDoc.HasMember("farPlane"));
    BOOST_REQUIRE(camDoc.HasMember("active"));
    BOOST_REQUIRE(!camDoc.HasMember("projection"));
    BOOST_REQUIRE(!camDoc.HasMember("view"));

    CameraComponent deserializedCam;
    DeserializeTypeFromJson(deserializedCam, camDoc);
    BOOST_REQUIRE_CLOSE(deserializedCam.fov, 60.0f, 0.001f);
    BOOST_REQUIRE_CLOSE(deserializedCam.nearPlane, 0.5f, 0.001f);
    BOOST_REQUIRE_CLOSE(deserializedCam.farPlane, 500.0f, 0.001f);
    BOOST_REQUIRE(deserializedCam.active == true);

    // 3. Test RendererComponent reflection and serialization
    RendererComponent renderer;
    boost::uuids::uuid testModelUuid = boost::uuids::string_generator()("01234567-89ab-cdef-0123-456789abcdef");
    renderer.modelUuid = testModelUuid;

    rapidjson::Document rendDoc;
    rendDoc.SetObject();
    SerializeTypeToJson(renderer, rendDoc, rendDoc.GetAllocator());

    BOOST_REQUIRE(rendDoc.HasMember("modelUuid"));
    BOOST_REQUIRE(rendDoc.HasMember("shaderUuid"));

    RendererComponent deserializedRend;
    DeserializeTypeFromJson(deserializedRend, rendDoc);
    BOOST_REQUIRE(deserializedRend.modelUuid == testModelUuid);

    // 4. Test LightComponent reflection and serialization with variant data
    LightComponent light;
    light.setType(LightComponent::Type::Point);
    light.color = {0.8f, 0.2f, 0.1f};
    light.intensity = 4.5f;
    std::get<PointLightData>(light.data).radius = 25.0f;
    std::get<PointLightData>(light.data).falloff = 2.0f;

    rapidjson::Document lightDoc;
    lightDoc.SetObject();
    SerializeTypeToJson(light, lightDoc, lightDoc.GetAllocator());

    BOOST_REQUIRE(lightDoc.HasMember("type"));
    BOOST_REQUIRE(lightDoc.HasMember("color"));
    BOOST_REQUIRE(lightDoc.HasMember("intensity"));
    BOOST_REQUIRE(lightDoc.HasMember("data"));
    BOOST_REQUIRE(lightDoc["data"].HasMember("radius"));
    BOOST_REQUIRE_CLOSE(lightDoc["data"]["radius"].GetFloat(), 25.0f, 0.001f);

    LightComponent deserializedLight;
    DeserializeTypeFromJson(deserializedLight, lightDoc);
    BOOST_REQUIRE(deserializedLight.getType() == LightComponent::Type::Point);
    BOOST_REQUIRE_CLOSE(deserializedLight.color.x, 0.8f, 0.001f);
    BOOST_REQUIRE_CLOSE(deserializedLight.intensity, 4.5f, 0.001f);
    BOOST_REQUIRE(std::holds_alternative<PointLightData>(deserializedLight.data));
    BOOST_REQUIRE_CLOSE(std::get<PointLightData>(deserializedLight.data).radius, 25.0f, 0.001f);
    // 5. Test Alternative Attributes (NonSerialized, RuntimeOnly, SkipSerialize)
    struct CustomAttrTestComponent {
        [[=NonSerialized{}]]
        float a{10.0f};

        [[=RuntimeOnly{}]]
        int b{20};

        [[=SkipSerialize{}]]
        bool c{true};

        float d{30.0f};
    };

    CustomAttrTestComponent customComp;
    customComp.a = 111.0f;
    customComp.b = 222;
    customComp.c = false;
    customComp.d = 333.0f;

    rapidjson::Document customDoc;
    customDoc.SetObject();
    SerializeTypeToJson(customComp, customDoc, customDoc.GetAllocator());

    BOOST_REQUIRE(!customDoc.HasMember("a"));
    BOOST_REQUIRE(!customDoc.HasMember("b"));
    BOOST_REQUIRE(!customDoc.HasMember("c"));
    BOOST_REQUIRE(customDoc.HasMember("d"));
    BOOST_REQUIRE_CLOSE(customDoc["d"].GetFloat(), 333.0f, 0.001f);

    // 6. Test System Reflection and Serialization
    movementSystem->speed = 5.5f;
    movementSystem->enabled = false;

    rapidjson::Document sysDoc;
    sysDoc.SetObject();
    movementSystem->SerializeToJson(sysDoc, sysDoc.GetAllocator());

    BOOST_REQUIRE(sysDoc.HasMember("name"));
    BOOST_REQUIRE_EQUAL(sysDoc["name"].GetString(), "MovementSystem");
    BOOST_REQUIRE(sysDoc.HasMember("extraData"));
    BOOST_REQUIRE(sysDoc["extraData"].HasMember("speed"));
    BOOST_REQUIRE_CLOSE(sysDoc["extraData"]["speed"].GetFloat(), 5.5f, 0.001f);
    BOOST_REQUIRE(sysDoc["extraData"].HasMember("enabled"));
    BOOST_REQUIRE_EQUAL(sysDoc["extraData"]["enabled"].GetBool(), false);
    BOOST_REQUIRE(!sysDoc["extraData"].HasMember("entities")); // NonSerialized

    // Test Deserialization back
    MovementSystem newMovementSystem(scene.get());
    newMovementSystem.speed = 1.0f;
    newMovementSystem.enabled = true;
    newMovementSystem.DeserializeFromJson(sysDoc);

    BOOST_REQUIRE_CLOSE(newMovementSystem.speed, 5.5f, 0.001f);
    BOOST_REQUIRE_EQUAL(newMovementSystem.enabled, false);

    // 7. Test Scene clean names serialization and component array GetName
    BOOST_REQUIRE_EQUAL(scene->GetComponentArray<RendererComponent>()->GetName(), "RendererComponent");
    BOOST_REQUIRE_EQUAL(scene->GetComponentArray<CameraComponent>()->GetName(), "CameraComponent");
    BOOST_REQUIRE_EQUAL(scene->GetIntegralComponentArray<TransformComponent>()->GetName(), "TransformComponent");

    rapidjson::Document fullSceneDoc;
    fullSceneDoc.SetObject();
    scene->SerializeToJson(fullSceneDoc);

    BOOST_REQUIRE(fullSceneDoc.HasMember("components"));
    BOOST_REQUIRE(fullSceneDoc["components"].HasMember("RendererComponent"));
    BOOST_REQUIRE(fullSceneDoc["components"].HasMember("CameraComponent"));
    BOOST_REQUIRE(fullSceneDoc["components"].HasMember("TransformComponent"));
    BOOST_REQUIRE(!fullSceneDoc["components"].HasMember("N6engine3ecs17RendererComponentE"));

    BOOST_REQUIRE(fullSceneDoc.HasMember("systems"));
    BOOST_REQUIRE(fullSceneDoc["systems"].HasMember("RenderSystem"));
    BOOST_REQUIRE(!fullSceneDoc["systems"].HasMember("N6engine3ecs12RenderSystemE"));

    // 8. Test NameComponent and TagComponent
    Entity namedEntity = scene->CreateEntity("PlayerEntity");
    BOOST_REQUIRE(scene->HasComponent<NameComponent>(namedEntity));
    BOOST_REQUIRE_EQUAL(scene->GetComponent<NameComponent>(namedEntity).name, "PlayerEntity");
    BOOST_REQUIRE_EQUAL(scene->GetEntityName(namedEntity), "PlayerEntity");

    scene->SetEntityName(namedEntity, "RenamedPlayer");
    BOOST_REQUIRE_EQUAL(scene->GetEntityName(namedEntity), "RenamedPlayer");
    BOOST_REQUIRE_EQUAL(scene->GetComponent<NameComponent>(namedEntity).name, "RenamedPlayer");

    scene->AddComponent<TagComponent>(namedEntity, TagComponent{"Player"});
    BOOST_REQUIRE(scene->HasComponent<TagComponent>(namedEntity));
    BOOST_REQUIRE_EQUAL(scene->GetComponent<TagComponent>(namedEntity).tag, "Player");

    // Serialization of NameComponent
    NameComponent nameComp{"MainCamera"};
    rapidjson::Document nameDoc;
    nameDoc.SetObject();
    SerializeTypeToJson(nameComp, nameDoc, nameDoc.GetAllocator());
    BOOST_REQUIRE(nameDoc.HasMember("name"));
    BOOST_REQUIRE_EQUAL(nameDoc["name"].GetString(), "MainCamera");

    NameComponent deserializedName;
    DeserializeTypeFromJson(deserializedName, nameDoc);
    BOOST_REQUIRE_EQUAL(deserializedName.name, "MainCamera");

    // Serialization of TagComponent
    TagComponent tagComp{"Enemy"};
    rapidjson::Document tagDoc;
    tagDoc.SetObject();
    SerializeTypeToJson(tagComp, tagDoc, tagDoc.GetAllocator());
    BOOST_REQUIRE(tagDoc.HasMember("tag"));
    BOOST_REQUIRE_EQUAL(tagDoc["tag"].GetString(), "Enemy");

    TagComponent deserializedTag;
    DeserializeTypeFromJson(deserializedTag, tagDoc);
    BOOST_REQUIRE_EQUAL(deserializedTag.tag, "Enemy");

    // ComponentArray GetName
    BOOST_REQUIRE_EQUAL(scene->GetIntegralComponentArray<NameComponent>()->GetName(), "NameComponent");
    BOOST_REQUIRE_EQUAL(scene->GetComponentArray<TagComponent>()->GetName(), "TagComponent");

    // 9. Test HidenInInspector attribute
    struct TestInspectorComponent : public Component {
        [[=HidenInInspector{}]]
        int hiddenField = 42;

        [[=Range{0.0f, 10.0f, 0.1f}]]
        float visibleField = 3.14f;
    };

    static_assert(has_annotation<HidenInInspector>(std::meta::nonstatic_data_members_of(^^TestInspectorComponent, std::meta::access_context::current())[0]));
    static_assert(is_hiddenInInspector<std::meta::nonstatic_data_members_of(^^TestInspectorComponent, std::meta::access_context::current())[0]>());
    static_assert(!is_hiddenInInspector<std::meta::nonstatic_data_members_of(^^TestInspectorComponent, std::meta::access_context::current())[1]>());

    TestInspectorComponent inspComp{100, 2.5f};
    rapidjson::Document inspDoc;
    inspDoc.SetObject();
    SerializeTypeToJson(inspComp, inspDoc, inspDoc.GetAllocator());
    BOOST_REQUIRE(inspDoc.HasMember("hiddenField"));
    BOOST_REQUIRE_EQUAL(inspDoc["hiddenField"].GetInt(), 100);
    BOOST_REQUIRE(inspDoc.HasMember("visibleField"));
}

BOOST_AUTO_TEST_CASE(MultipleActiveScenesTest) {
    Engine& engine = Engine::GetInstance();

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

    // Register position component & movement system on scenes
    scene1->RegisterComponent<Position>();
    auto moveSys1 = scene1->RegisterSystem<MovementSystem>();
    Entity e1 = scene1->CreateEntity();
    scene1->AddComponent<Position>(e1, {10.0f, 20.0f});

    scene2->RegisterComponent<Position>();
    auto moveSys2 = scene2->RegisterSystem<MovementSystem>();
    Entity e2 = scene2->CreateEntity();
    scene2->AddComponent<Position>(e2, {100.0f, 200.0f});

    scene3->RegisterComponent<Position>();
    auto moveSys3 = scene3->RegisterSystem<MovementSystem>();
    Entity e3 = scene3->CreateEntity();
    scene3->AddComponent<Position>(e3, {1.0f, 2.0f});

    // Update active scenes via engine.Update
    engine.Update(1.0f);
    BOOST_REQUIRE_EQUAL(scene1->GetComponent<Position>(e1).x, 11.0f);
    BOOST_REQUIRE_EQUAL(scene2->GetComponent<Position>(e2).x, 101.0f);
    BOOST_REQUIRE_EQUAL(scene3->GetComponent<Position>(e3).x, 1.0f); // Inactive scene unchanged

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
    Engine& engine = Engine::GetInstance();
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

BOOST_AUTO_TEST_CASE(ReflectionLookupNameAnnotationTest) {
    struct TestComponent : public Component {
        [[=LookupName{}]]
        boost::uuids::uuid anyAssetUuid{boost::uuids::nil_uuid()};

        [[=LookupName{am::AssetType::Material}]]
        boost::uuids::uuid matUuid{boost::uuids::nil_uuid()};

        [[=UuidToLookupName{am::AssetType::ShaderProgram}]]
        boost::uuids::uuid shaderUuid{boost::uuids::nil_uuid()};

        [[=AssetLookup{am::AssetType::Mesh}]]
        boost::uuids::uuid meshUuid{boost::uuids::nil_uuid()};

        boost::uuids::uuid plainUuid{boost::uuids::nil_uuid()};
    };

    static constexpr auto members = get_members_array<TestComponent>();
    static_assert(members.size() == 5);

    static_assert(is_uuid_to_lookup_name<members[0]>());
    static_assert(!get_lookup_name_asset_type<members[0]>().has_value());

    static_assert(is_uuid_to_lookup_name<members[1]>());
    static_assert(get_lookup_name_asset_type<members[1]>().has_value());
    static_assert(get_lookup_name_asset_type<members[1]>().value() == am::AssetType::Material);

    static_assert(is_uuid_to_lookup_name<members[2]>());
    static_assert(get_lookup_name_asset_type<members[2]>().has_value());
    static_assert(get_lookup_name_asset_type<members[2]>().value() == am::AssetType::ShaderProgram);

    static_assert(is_uuid_to_lookup_name<members[3]>());
    static_assert(get_lookup_name_asset_type<members[3]>().has_value());
    static_assert(get_lookup_name_asset_type<members[3]>().value() == am::AssetType::Mesh);

    static_assert(!is_uuid_to_lookup_name<members[4]>());

    // Also test RendererComponent reflection
    static constexpr auto rendererMembers = get_members_array<RendererComponent>();
    static_assert(is_uuid_to_lookup_name<rendererMembers[0]>());
    static_assert(get_lookup_name_asset_type<rendererMembers[0]>().value() == am::AssetType::ShaderProgram);
    static_assert(is_uuid_to_lookup_name<rendererMembers[2]>());
    static_assert(get_lookup_name_asset_type<rendererMembers[2]>().value() == am::AssetType::Material);

    // Test MeshComponent reflection
    static constexpr auto meshMembers = get_members_array<MeshComponent>();
    static_assert(is_uuid_to_lookup_name<meshMembers[0]>());
    static_assert(get_lookup_name_asset_type<meshMembers[0]>().value() == am::AssetType::Mesh);

    // Verify isDirty initialization and reflection deserialization fallback
    RendererComponent renderer;
    BOOST_CHECK(renderer.isDirty);
    renderer.isDirty = false;
    rapidjson::Document doc;
    doc.SetObject();
    DeserializeTypeFromJson(renderer, doc);
    BOOST_CHECK(renderer.isDirty);

    MeshComponent mesh;
    BOOST_CHECK(mesh.isDirty);
    mesh.isDirty = false;
    DeserializeTypeFromJson(mesh, doc);
    BOOST_CHECK(mesh.isDirty);

    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(ScenePrefabSerializationAndInstantiationTest) {
    Engine& engine = Engine::GetInstance();
    auto sourceScene = engine.CreateScene("PrefabSourceScene");
    BOOST_REQUIRE(sourceScene);

    Entity rootObj = sourceScene->CreateEntity("CarRoot");
    sourceScene->AddComponent<TagComponent>(rootObj, TagComponent{"Vehicle"});

    Entity wheel1 = sourceScene->CreateEntity("WheelFL", rootObj);
    sourceScene->AddComponent<TagComponent>(wheel1, TagComponent{"Wheel"});

    Entity wheel2 = sourceScene->CreateEntity("WheelFR", rootObj);
    sourceScene->AddComponent<TagComponent>(wheel2, TagComponent{"Wheel"});

    // Serialize object to prefab JSON document
    rapidjson::Document prefabDoc;
    sourceScene->SerializeObjectToJson(rootObj, prefabDoc);

    // Verify document structure: should have entities, components, sceneGraph, but NO systems
    BOOST_CHECK(prefabDoc.HasMember("entities"));
    BOOST_CHECK(prefabDoc.HasMember("components"));
    BOOST_CHECK(prefabDoc.HasMember("sceneGraph"));
    BOOST_CHECK(!prefabDoc.HasMember("systems"));

    // Check components in prefab doc
    BOOST_CHECK(prefabDoc["components"].HasMember("NameComponent"));
    BOOST_CHECK(prefabDoc["components"].HasMember("TagComponent"));
    BOOST_CHECK(prefabDoc["components"].HasMember("TransformComponent"));

    // Instantiate prefab into a new scene
    auto targetScene = engine.CreateScene("PrefabTargetScene");
    BOOST_REQUIRE(targetScene);

    Entity existingEntity = targetScene->CreateEntity("WorldAnchor");

    // Instantiate prefab as child of existingEntity
    Entity instantiatedRoot = targetScene->InstantiatePrefab(prefabDoc, existingEntity);
    BOOST_REQUIRE(instantiatedRoot != MAX_ENTITIES);
    BOOST_CHECK_EQUAL(targetScene->GetEntityName(instantiatedRoot), "CarRoot");
    BOOST_CHECK(targetScene->HasComponent<TagComponent>(instantiatedRoot));
    BOOST_CHECK_EQUAL(targetScene->GetComponent<TagComponent>(instantiatedRoot).tag, "Vehicle");
    BOOST_CHECK_EQUAL(targetScene->GetParent(instantiatedRoot), existingEntity);

    // Verify children were created and parented correctly
    const auto& children = targetScene->GetChildren(instantiatedRoot);
    BOOST_REQUIRE_EQUAL(children.size(), 2);

    std::vector<std::string> childNames;
    for (Entity child : children) {
        childNames.push_back(targetScene->GetEntityName(child));
        BOOST_CHECK(targetScene->HasComponent<TagComponent>(child));
        BOOST_CHECK_EQUAL(targetScene->GetComponent<TagComponent>(child).tag, "Wheel");
    }
    BOOST_CHECK(std::find(childNames.begin(), childNames.end(), "WheelFL") != childNames.end());
    BOOST_CHECK(std::find(childNames.begin(), childNames.end(), "WheelFR") != childNames.end());

    // Also test standalone instantiation (as root entity)
    Entity standaloneRoot = targetScene->InstantiatePrefab(prefabDoc);
    BOOST_REQUIRE(standaloneRoot != MAX_ENTITIES);
    BOOST_CHECK_EQUAL(targetScene->GetEntityName(standaloneRoot), "CarRoot");
    BOOST_CHECK(!targetScene->HasParent(standaloneRoot));

    engine.RemoveScene("PrefabSourceScene");
    engine.RemoveScene("PrefabTargetScene");
}

BOOST_AUTO_TEST_CASE(EngineSaveEntityAsPrefabTest) {
    Engine& engine = Engine::GetInstance();
    auto sourceScene = engine.CreateScene("SavePrefabSourceScene");
    BOOST_REQUIRE(sourceScene);

    Entity house = sourceScene->CreateEntity("House");
    sourceScene->AddComponent<TagComponent>(house, TagComponent{"Building"});

    Entity door = sourceScene->CreateEntity("Door", house);
    sourceScene->AddComponent<TagComponent>(door, TagComponent{"Interactable"});

    // Save entity as prefab
    bool saved = engine.SaveEntityAsPrefab(house, "res/prefabs", sourceScene.get());
    BOOST_CHECK(saved);

    std::filesystem::path expectedPath = "res/prefabs/House.prefab";
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
    Engine& engine = Engine::GetInstance();
    auto sourceScene = engine.CreateScene("PrefabLoadSourceScene");
    BOOST_REQUIRE(sourceScene);

    Entity tower = sourceScene->CreateEntity("Tower");
    sourceScene->AddComponent<TagComponent>(tower, TagComponent{"Structure"});

    Entity roof = sourceScene->CreateEntity("Roof", tower);
    sourceScene->AddComponent<TagComponent>(roof, TagComponent{"RoofTop"});

    bool saved = engine.SaveEntityAsPrefab(tower, "res/prefabs", sourceScene.get());
    BOOST_REQUIRE(saved);

    std::filesystem::path prefabPath = "res/prefabs/Tower.prefab";
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

BOOST_AUTO_TEST_CASE(EngineInstantiateModelTest) {
    Engine& engine = Engine::GetInstance();
    auto am = engine.assetManagerInterface;
    BOOST_REQUIRE(am != nullptr);

    std::filesystem::path modelPath = "res/models/box/Box.model";
    auto modelUuidOpt = am->getAssetUuidByPath(modelPath);
    if (!modelUuidOpt) modelUuidOpt = am->registerAsset(modelPath.string());
    BOOST_REQUIRE(modelUuidOpt.has_value());

    auto testScene = engine.CreateScene("ModelInstantiateTestScene");
    BOOST_REQUIRE(testScene);

    Entity rootEnt = testScene->InstantiateModel(modelUuidOpt.value());
    BOOST_REQUIRE(rootEnt != MAX_ENTITIES);
    BOOST_CHECK_EQUAL(testScene->GetEntityName(rootEnt), "Box");

    // Check if either rootEnt or its child has MeshComponent and RendererComponent
    Entity meshEntity = rootEnt;
    if (!testScene->HasComponent<MeshComponent>(rootEnt)) {
        const auto& children = testScene->GetChildren(rootEnt);
        BOOST_REQUIRE(!children.empty());
        meshEntity = children[0];
    }

    BOOST_REQUIRE(testScene->HasComponent<MeshComponent>(meshEntity));
    BOOST_REQUIRE(testScene->HasComponent<RendererComponent>(meshEntity));

    auto& meshComp = testScene->GetComponent<MeshComponent>(meshEntity);
    auto& rendComp = testScene->GetComponent<RendererComponent>(meshEntity);

    BOOST_CHECK(!meshComp.meshUuid.is_nil());
    BOOST_CHECK(!rendComp.shaderUuid.is_nil());

    auto pbrOpt = am->getAssetUuid("pbrShader");
    if (pbrOpt) {
        BOOST_CHECK_EQUAL(rendComp.shaderUuid, pbrOpt.value());
    }

    engine.RemoveScene("ModelInstantiateTestScene");
}