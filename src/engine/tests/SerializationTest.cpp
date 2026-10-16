#include <boost/test/unit_test.hpp>
#include "../Engine.h"
#include "ecs/Scene.h"
#include "TestHelper.hpp"
#include "ecs/components/NameComponent.hpp"
#include "../ecs/components/TagComponent.hpp"
#include "systems/transformSystem/TransformSystem.h"
#include "systems/transformSystem/componets/TransformComponent.hpp"
#include "systems/renderingSystem/componets/CameraComponent.hpp"
#include "systems/renderingSystem/componets/RendererComponent.hpp"
#include "systems/renderingSystem/componets/MeshComponent.hpp"
#include "systems/renderingSystem/componets/LightComponent.hpp"
#include <rapidjson/document.h>

using namespace engine;
using namespace engine::ecs;
using namespace engine::test;

BOOST_AUTO_TEST_SUITE(SerializationTestSuite)

BOOST_AUTO_TEST_CASE(StaticReflectionAndSerializationTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("ReflectionTestScene");

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
    boost::uuids::uuid testShaderUuid = boost::uuids::string_generator()("01234567-89ab-cdef-0123-456789abcdef");
    boost::uuids::uuid testMatUuid = boost::uuids::string_generator()("fedcba98-7654-3210-fedc-ba9876543210");
    renderer.shaderUuid = testShaderUuid;
    renderer.materialUuid = testMatUuid;

    rapidjson::Document rendDoc;
    rendDoc.SetObject();
    SerializeTypeToJson(renderer, rendDoc, rendDoc.GetAllocator());

    BOOST_REQUIRE(rendDoc.HasMember("shaderUuid"));
    BOOST_REQUIRE(rendDoc.HasMember("materialUuid"));

    RendererComponent deserializedRend;
    DeserializeTypeFromJson(deserializedRend, rendDoc);
    BOOST_REQUIRE(deserializedRend.shaderUuid == testShaderUuid);
    BOOST_REQUIRE(deserializedRend.materialUuid == testMatUuid);

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

    // 5. Test NonSerialized attributes
    struct CustomAttrTestComponent : public Component {
        [[=NonSerialized{}]]
        float a{10.0f};

        [[=NonSerialized{}]]
        int b{20};

        [[=NonSerialized{}]]
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
    auto transformSys = scene->GetSystem<TransformSystem>();
    BOOST_REQUIRE(transformSys);

    rapidjson::Document sysDoc;
    sysDoc.SetObject();
    transformSys->SerializeToJson(sysDoc, sysDoc.GetAllocator());

    BOOST_REQUIRE(sysDoc.HasMember("name"));
    BOOST_REQUIRE_EQUAL(sysDoc["name"].GetString(), "TransformSystem");

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

    TestInspectorComponent inspComp;
    inspComp.hiddenField = 100;
    inspComp.visibleField = 2.5f;
    rapidjson::Document inspDoc;
    inspDoc.SetObject();
    SerializeTypeToJson(inspComp, inspDoc, inspDoc.GetAllocator());
    BOOST_REQUIRE(inspDoc.HasMember("hiddenField"));
    BOOST_REQUIRE_EQUAL(inspDoc["hiddenField"].GetInt(), 100);
    BOOST_REQUIRE(inspDoc.HasMember("visibleField"));

    engine.RemoveScene("ReflectionTestScene");
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

BOOST_AUTO_TEST_SUITE_END()
