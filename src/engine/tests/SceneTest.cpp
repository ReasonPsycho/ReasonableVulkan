#define BOOST_TEST_MODULE EngineAndSceneTest
#include <boost/test/unit_test.hpp>
#include "../Engine.h"
#include "ecs/NameComponent.hpp"
#include "ecs/TagComponent.hpp"

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
}