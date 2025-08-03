#define BOOST_TEST_MODULE Engine Test Suite
#include <boost/test/unit_test.hpp>
#include "../Engine.h"
using namespace engine::ecs;

// Example component
struct Position {
    float x, y;
};

// Example system
class MovementSystem : public System<MovementSystem, Position> {
public:
    explicit MovementSystem(Scene* scene) : System(scene) {}

    void Update(float deltaTime) override
    {

    };
    ~MovementSystem() override
    {

    };

protected:
    void OnEntityAdded(Entity entity) override {
    }

    void OnEntityRemoved(Entity entity) override {
    }

};

BOOST_AUTO_TEST_CASE(CreateEntityTest) {
    engine::Engine& engine = engine::Engine::GetInstance();
    std::shared_ptr<Scene> scene = engine.CreateScene("TestScene");
    BOOST_REQUIRE(engine.GetScene("TestScene"));
    BOOST_REQUIRE(scene);
    Entity testEntity = scene->CreateEntity();
    BOOST_REQUIRE(testEntity == 0);
    scene->RegisterComponent<Position>();
    scene->RegisterSystem<MovementSystem>(scene.get());
    scene->AddComponent<Position>(testEntity, {1.0f, 2.0f});
    BOOST_REQUIRE(scene->HasComponent<Position>(testEntity));
}
