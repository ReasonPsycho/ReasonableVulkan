#include <boost/test/unit_test.hpp>
#include "../Engine.h"
#include "ecs/Scene.h"
#include "TestHelper.hpp"
#include "ecs/components/NameComponent.hpp"
#include "../ecs/components/TagComponent.hpp"
#include "systems/transformSystem/componets/TransformComponent.hpp"
#include "systems/editorSystem/CommandHistory.hpp"
#include "systems/editorSystem/EditorCommands.hpp"

using namespace engine;
using namespace engine::ecs;
using namespace engine::editor;
using namespace engine::test;

BOOST_AUTO_TEST_SUITE(EditorCommandsTestSuite)

BOOST_AUTO_TEST_CASE(CommandHistoryStackOperationsTest) {
    CommandHistory history(10);
    BOOST_CHECK(!history.CanUndo());
    BOOST_CHECK(!history.CanRedo());
    BOOST_CHECK_EQUAL(history.GetCurrentIndex(), 0);

    class MockCommand : public ICommand {
    public:
        int& value;
        explicit MockCommand(int& v) : value(v) {}
        void Execute() override { value += 10; }
        void Undo() override { value -= 10; }
        std::string GetName() const override { return "MockCmd"; }
    };

    int val = 0;
    history.Execute(std::make_shared<MockCommand>(val));
    BOOST_CHECK_EQUAL(val, 10);
    BOOST_CHECK(history.CanUndo());
    BOOST_CHECK(!history.CanRedo());
    BOOST_CHECK_EQUAL(history.GetUndoName(), "MockCmd");

    history.Execute(std::make_shared<MockCommand>(val));
    BOOST_CHECK_EQUAL(val, 20);

    history.Undo();
    BOOST_CHECK_EQUAL(val, 10);
    BOOST_CHECK(history.CanRedo());
    BOOST_CHECK_EQUAL(history.GetRedoName(), "MockCmd");

    history.Redo();
    BOOST_CHECK_EQUAL(val, 20);

    history.JumpToIndex(0);
    BOOST_CHECK_EQUAL(val, 0);

    history.JumpToIndex(2);
    BOOST_CHECK_EQUAL(val, 20);

    history.Clear();
    BOOST_CHECK(!history.CanUndo());
    BOOST_CHECK(!history.CanRedo());
}

BOOST_AUTO_TEST_CASE(EntityAndComponentCommandsUndoRedoTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("CommandTestScene");
    BOOST_REQUIRE(scene != nullptr);

    CommandHistory history;

    // 1. Create Entity Command
    Entity e1 = scene->CreateEntity("BaseEntity");
    auto createCmd = std::make_shared<CreateEntityCommand>(scene.get(), e1, MAX_ENTITIES, "BaseEntity");
    history.Push(createCmd);

    // 2. Add Component Command
    auto addCompCmd = std::make_shared<AddComponentCommand>(scene.get(), e1, std::type_index(typeid(TagComponent)), "TagComponent");
    scene->AddComponent<TagComponent>(e1, TagComponent{"CmdTag"});
    history.Push(addCompCmd);
    BOOST_CHECK(scene->HasComponent<TagComponent>(e1));

    // 3. Set Entity Active Command
    auto setActiveCmd = std::make_shared<SetEntityActiveCommand>(scene.get(), std::vector<std::pair<Entity, bool>>{{e1, true}}, false);
    history.Execute(setActiveCmd);
    BOOST_CHECK(!scene->IsEntityActive(e1));

    history.Undo();
    BOOST_CHECK(scene->IsEntityActive(e1));

    history.Redo();
    BOOST_CHECK(!scene->IsEntityActive(e1));

    // 4. Transform Command
    glm::mat4 oldMat = glm::mat4(1.0f);
    glm::mat4 newMat = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 0.0f, 0.0f));
    auto transformCmd = std::make_shared<EntityTransformCommand>(scene.get(), e1, oldMat, newMat, oldMat, newMat);
    history.Execute(transformCmd);
    auto& tc = scene->GetComponent<TransformComponent>(e1);
    BOOST_CHECK_EQUAL(tc.position.x, 5.0f);

    history.Undo();
    BOOST_CHECK_EQUAL(tc.position.x, 0.0f);

    history.Redo();
    BOOST_CHECK_EQUAL(tc.position.x, 5.0f);

    // 5. Reparent Command
    Entity child = scene->CreateEntity("ChildEntity");
    auto reparentCmd = std::make_shared<ReparentEntityCommand>(scene.get(), child, MAX_ENTITIES, e1);
    history.Execute(reparentCmd);
    BOOST_CHECK(scene->HasParent(child));
    BOOST_CHECK_EQUAL(scene->GetParent(child), e1);

    history.Undo();
    BOOST_CHECK(!scene->HasParent(child));

    history.Redo();
    BOOST_CHECK(scene->HasParent(child));
    BOOST_CHECK_EQUAL(scene->GetParent(child), e1);

    engine.RemoveScene("CommandTestScene");
}

BOOST_AUTO_TEST_SUITE_END()
