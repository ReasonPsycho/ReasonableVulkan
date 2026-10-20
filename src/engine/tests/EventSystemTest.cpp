#include <boost/test/unit_test.hpp>
#include "../Engine.h"
#include "ecs/Scene.h"
#include "events/EventDispatcher.hpp"
#include "events/Events.hpp"
#include "TestHelper.hpp"

using namespace engine;
using namespace engine::ecs;
using namespace engine::events;
using namespace engine::test;

struct CustomTestEvent {
    int value{0};
    std::string message;
};

struct AnotherTestEvent {
    float amount{0.0f};
};

BOOST_AUTO_TEST_SUITE(EventSystemTestSuite)

BOOST_AUTO_TEST_CASE(ImmediatePublishTest) {
    EventDispatcher dispatcher;
    int callCount = 0;
    int receivedValue = 0;
    std::string receivedMsg;

    ListenerID id = dispatcher.Subscribe<CustomTestEvent>([&](const CustomTestEvent& ev) {
        callCount++;
        receivedValue = ev.value;
        receivedMsg = ev.message;
    });

    BOOST_REQUIRE_NE(id, 0);
    BOOST_REQUIRE_EQUAL(dispatcher.GetListenerCount<CustomTestEvent>(), 1);
    BOOST_REQUIRE(dispatcher.HasListeners<CustomTestEvent>());

    dispatcher.Publish(CustomTestEvent{42, "hello"});

    BOOST_REQUIRE_EQUAL(callCount, 1);
    BOOST_REQUIRE_EQUAL(receivedValue, 42);
    BOOST_REQUIRE_EQUAL(receivedMsg, "hello");

    // Second publish
    dispatcher.Publish(CustomTestEvent{100, "world"});
    BOOST_REQUIRE_EQUAL(callCount, 2);
    BOOST_REQUIRE_EQUAL(receivedValue, 100);
    BOOST_REQUIRE_EQUAL(receivedMsg, "world");
}

BOOST_AUTO_TEST_CASE(UnsubscribeTest) {
    EventDispatcher dispatcher;
    int count1 = 0;
    int count2 = 0;

    ListenerID id1 = dispatcher.Subscribe<CustomTestEvent>([&](const CustomTestEvent&) {
        count1++;
    });

    ListenerID id2 = dispatcher.Subscribe<CustomTestEvent>([&](const CustomTestEvent&) {
        count2++;
    });

    BOOST_REQUIRE_EQUAL(dispatcher.GetListenerCount<CustomTestEvent>(), 2);

    dispatcher.Publish(CustomTestEvent{1, "first"});
    BOOST_REQUIRE_EQUAL(count1, 1);
    BOOST_REQUIRE_EQUAL(count2, 1);

    // Unsubscribe first listener
    dispatcher.Unsubscribe<CustomTestEvent>(id1);
    BOOST_REQUIRE_EQUAL(dispatcher.GetListenerCount<CustomTestEvent>(), 1);

    dispatcher.Publish(CustomTestEvent{2, "second"});
    BOOST_REQUIRE_EQUAL(count1, 1); // Should not increase
    BOOST_REQUIRE_EQUAL(count2, 2); // Should increase

    // Unsubscribe second listener
    dispatcher.Unsubscribe<CustomTestEvent>(id2);
    BOOST_REQUIRE_EQUAL(dispatcher.GetListenerCount<CustomTestEvent>(), 0);
    BOOST_REQUIRE(!dispatcher.HasListeners<CustomTestEvent>());

    dispatcher.Publish(CustomTestEvent{3, "third"});
    BOOST_REQUIRE_EQUAL(count1, 1);
    BOOST_REQUIRE_EQUAL(count2, 2);
}

BOOST_AUTO_TEST_CASE(ScopedEventListenerTest) {
    EventDispatcher dispatcher;
    int callCount = 0;

    {
        auto scopedListener = dispatcher.ScopedSubscribe<CustomTestEvent>([&](const CustomTestEvent&) {
            callCount++;
        });

        BOOST_REQUIRE(scopedListener.IsValid());
        BOOST_REQUIRE_EQUAL(dispatcher.GetListenerCount<CustomTestEvent>(), 1);

        dispatcher.Publish(CustomTestEvent{10, "test"});
        BOOST_REQUIRE_EQUAL(callCount, 1);

        // Move semantics
        auto movedListener = std::move(scopedListener);
        BOOST_REQUIRE(!scopedListener.IsValid());
        BOOST_REQUIRE(movedListener.IsValid());

        dispatcher.Publish(CustomTestEvent{20, "test"});
        BOOST_REQUIRE_EQUAL(callCount, 2);
    } // movedListener goes out of scope here

    BOOST_REQUIRE_EQUAL(dispatcher.GetListenerCount<CustomTestEvent>(), 0);
    dispatcher.Publish(CustomTestEvent{30, "test"});
    BOOST_REQUIRE_EQUAL(callCount, 2); // Should not increase after scope exit
}

BOOST_AUTO_TEST_CASE(QueuedEventsTest) {
    EventDispatcher dispatcher;
    std::vector<int> receivedValues;

    dispatcher.Subscribe<CustomTestEvent>([&](const CustomTestEvent& ev) {
        receivedValues.push_back(ev.value);
    });

    dispatcher.Enqueue(CustomTestEvent{1, "one"});
    dispatcher.Enqueue(CustomTestEvent{2, "two"});
    dispatcher.Enqueue(CustomTestEvent{3, "three"});

    BOOST_REQUIRE_EQUAL(dispatcher.GetQueueSize<CustomTestEvent>(), 3);
    BOOST_REQUIRE_EQUAL(receivedValues.size(), 0); // Not processed yet

    dispatcher.ProcessQueue<CustomTestEvent>();

    BOOST_REQUIRE_EQUAL(dispatcher.GetQueueSize<CustomTestEvent>(), 0);
    BOOST_REQUIRE_EQUAL(receivedValues.size(), 3);
    BOOST_REQUIRE_EQUAL(receivedValues[0], 1);
    BOOST_REQUIRE_EQUAL(receivedValues[1], 2);
    BOOST_REQUIRE_EQUAL(receivedValues[2], 3);
}

BOOST_AUTO_TEST_CASE(ClearQueueTest) {
    EventDispatcher dispatcher;
    int callCount = 0;

    dispatcher.Subscribe<CustomTestEvent>([&](const CustomTestEvent&) {
        callCount++;
    });

    dispatcher.Enqueue(CustomTestEvent{1, "one"});
    dispatcher.Enqueue(CustomTestEvent{2, "two"});
    BOOST_REQUIRE_EQUAL(dispatcher.GetQueueSize<CustomTestEvent>(), 2);

    dispatcher.ClearQueue<CustomTestEvent>();
    BOOST_REQUIRE_EQUAL(dispatcher.GetQueueSize<CustomTestEvent>(), 0);

    dispatcher.ProcessAllQueues();
    BOOST_REQUIRE_EQUAL(callCount, 0);
}

BOOST_AUTO_TEST_CASE(MultipleEventTypesTest) {
    EventDispatcher dispatcher;
    int customCount = 0;
    int anotherCount = 0;

    dispatcher.Subscribe<CustomTestEvent>([&](const CustomTestEvent&) {
        customCount++;
    });

    dispatcher.Subscribe<AnotherTestEvent>([&](const AnotherTestEvent&) {
        anotherCount++;
    });

    dispatcher.Publish(CustomTestEvent{1, "test"});
    BOOST_REQUIRE_EQUAL(customCount, 1);
    BOOST_REQUIRE_EQUAL(anotherCount, 0);

    dispatcher.Publish(AnotherTestEvent{3.14f});
    BOOST_REQUIRE_EQUAL(customCount, 1);
    BOOST_REQUIRE_EQUAL(anotherCount, 1);
}

BOOST_AUTO_TEST_CASE(ReentrantEventsTest) {
    EventDispatcher dispatcher;
    std::vector<int> executionLog;

    dispatcher.Subscribe<CustomTestEvent>([&](const CustomTestEvent& ev) {
        executionLog.push_back(ev.value);
        if (ev.value == 1) {
            // Re-entrant publish of another event type
            dispatcher.Publish(AnotherTestEvent{99.0f});
        }
    });

    dispatcher.Subscribe<AnotherTestEvent>([&](const AnotherTestEvent& ev) {
        executionLog.push_back(static_cast<int>(ev.amount));
    });

    dispatcher.Publish(CustomTestEvent{1, "trigger"});

    BOOST_REQUIRE_EQUAL(executionLog.size(), 2);
    BOOST_REQUIRE_EQUAL(executionLog[0], 1);
    BOOST_REQUIRE_EQUAL(executionLog[1], 99);
}

BOOST_AUTO_TEST_CASE(SceneEventIntegrationTest) {
    Engine engine = CreateTestEngine();
    auto scene = engine.CreateScene("EventTestScene");
    BOOST_REQUIRE(scene);

    int immediateHits = 0;
    int queuedHits = 0;
    glm::vec2 lastMousePos{0.0f, 0.0f};

    scene->GetEventDispatcher().Subscribe<HUDClickEvent>([&](const HUDClickEvent& ev) {
        immediateHits++;
        lastMousePos = ev.mousePosition;
    });

    scene->GetEventDispatcher().Subscribe<EntityCollisionEvent>([&](const EntityCollisionEvent&) {
        queuedHits++;
    });

    // Immediate publish via Scene helper
    scene->PublishEvent(HUDClickEvent{
        .entity = 5,
        .mousePosition = {150.0f, 200.0f},
        .mouseButton = 1
    });

    BOOST_REQUIRE_EQUAL(immediateHits, 1);
    BOOST_REQUIRE_CLOSE(lastMousePos.x, 150.0f, 0.001f);
    BOOST_REQUIRE_CLOSE(lastMousePos.y, 200.0f, 0.001f);

    // Enqueue event
    scene->EnqueueEvent(EntityCollisionEvent{
        .entityA = 1,
        .entityB = 2,
        .hitPoint = {0.0f, 0.0f, 0.0f},
        .normal = {0.0f, 1.0f, 0.0f}
    });

    BOOST_REQUIRE_EQUAL(queuedHits, 0);

    // Scene Update processes queued events
    scene->Update(0.016f);

    BOOST_REQUIRE_EQUAL(queuedHits, 1);

    engine.RemoveScene("EventTestScene");
}

BOOST_AUTO_TEST_CASE(EngineEventIntegrationTest) {
    Engine engine = CreateTestEngine();
    int resizeEventsReceived = 0;
    int width = 0;
    int height = 0;

    engine.GetEventDispatcher().Subscribe<WindowResizeEvent>([&](const WindowResizeEvent& ev) {
        resizeEventsReceived++;
        width = ev.width;
        height = ev.height;
    });

    engine.PublishEvent(WindowResizeEvent{1920, 1080});

    BOOST_REQUIRE_EQUAL(resizeEventsReceived, 1);
    BOOST_REQUIRE_EQUAL(width, 1920);
    BOOST_REQUIRE_EQUAL(height, 1080);
}

BOOST_AUTO_TEST_SUITE_END()
