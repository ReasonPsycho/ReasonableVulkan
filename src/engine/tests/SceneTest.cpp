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
    auto movementSystem = scene->RegisterSystem<MovementSystem>(scene.get());
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

