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
    explicit MovementSystem(Scene* scene)
        : System<MovementSystem, Position>(scene)
    {
    }

    void OnAddEntity(Entity entity) override {
    }

    void OnRemoveEntity(Entity entity) override {
    }

public:
    void Update(float deltaTime) override;
};

BOOST_AUTO_TEST_CASE(CreateEntityTest) {
    engine::Engine& engine = engine::Engine::GetInstance();
    engine.CreateScene("TestScene");
    BOOST_CHECK(engine.GetScene("TestScene"));
}
