#define BOOST_TEST_MODULE EngineAndSceneTest
#include <boost/test/unit_test.hpp>
#include "../Engine.h"

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

protected:
    void OnEntityAdded(Entity entity) override {}
    void OnEntityRemoved(Entity entity) override {}
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

    // Add Transforms
    scene->AddComponent<Transform>(parent, {});
    scene->AddComponent<Transform>(child1, {});
    scene->AddComponent<Transform>(child2, {});

    // Parent-child relationships
    scene->SetParent(child1, parent);
    scene->SetParent(child2, parent);

    // Set positions
    setLocalPosition(scene->GetComponent<Transform>(parent), {10.0f, 0.0f, 0.0f});
    setLocalPosition(scene->GetComponent<Transform>(child1), {0.0f, 5.0f, 0.0f});
    setLocalPosition(scene->GetComponent<Transform>(child2), {0.0f, 0.0f, 3.0f});

    // Verify all are dirty before update
    BOOST_REQUIRE(isDirty(scene->GetComponent<Transform>(parent)));
    BOOST_REQUIRE(isDirty(scene->GetComponent<Transform>(child1)));
    BOOST_REQUIRE(isDirty(scene->GetComponent<Transform>(child2)));

    // Update system
    scene->Update(0.0f);

    // Validate dirty flags cleared
    BOOST_REQUIRE(!isDirty(scene->GetComponent<Transform>(parent)));
    BOOST_REQUIRE(!isDirty(scene->GetComponent<Transform>(child1)));
    BOOST_REQUIRE(!isDirty(scene->GetComponent<Transform>(child2)));

    // Validate world transforms
    const glm::vec3 worldPosParent = getGlobalPosition(scene->GetComponent<Transform>(parent));
    const glm::vec3 worldPosChild1 = getGlobalPosition(scene->GetComponent<Transform>(child1));
    const glm::vec3 worldPosChild2 = getGlobalPosition(scene->GetComponent<Transform>(child2));

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
    setLocalPosition(scene->GetComponent<Transform>(child1), {0.0f, 10.0f, 0.0f});
    BOOST_REQUIRE(isDirty(scene->GetComponent<Transform>(child1)));

    scene->Update(0.0f);
    BOOST_REQUIRE(!isDirty(scene->GetComponent<Transform>(child1)));

    // Confirm new world position
    const glm::vec3 newWorldPosChild1 = getGlobalPosition(scene->GetComponent<Transform>(child1));
    BOOST_REQUIRE_CLOSE(newWorldPosChild1.x, 10.0f, 0.001f);
    BOOST_REQUIRE_CLOSE(newWorldPosChild1.y, 10.0f, 0.001f);
    BOOST_REQUIRE_CLOSE(newWorldPosChild1.z, 0.0f, 0.001f);

    engine.RemoveScene("TransformScene");
}