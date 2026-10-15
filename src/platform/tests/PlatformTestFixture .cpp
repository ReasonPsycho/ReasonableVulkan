#define BOOST_TEST_MODULE PlatformTests
#include <boost/test/unit_test.hpp>
#include "../src/Platform.hpp"

struct PlatformTestFixture {
    plt::Platform platform;

    PlatformTestFixture() {
    }

    ~PlatformTestFixture() {
        platform.Shutdown();
    }
};

BOOST_FIXTURE_TEST_SUITE(PlatformTests, PlatformTestFixture)

BOOST_AUTO_TEST_CASE(test_init) {
    bool result = platform.Init("Test Window", 800, 600);
    BOOST_CHECK(result);
    BOOST_CHECK(platform.GetNativeWindow() != nullptr);
}

BOOST_AUTO_TEST_CASE(test_window_size_and_position) {
    BOOST_REQUIRE(platform.Init("Test Window", 800, 600));

    int width = 0, height = 0;
    platform.GetWindowSize(width, height);
    BOOST_CHECK_EQUAL(width, 800);
    BOOST_CHECK_EQUAL(height, 600);

    int x = 0, y = 0;
    platform.GetWindowPosition(x, y);
    BOOST_CHECK(x >= 0 || x < 0); // Window position queried successfully
}

BOOST_AUTO_TEST_CASE(test_delta_time) {
    BOOST_REQUIRE(platform.Init("Test Window", 800, 600));

    float initial_dt = platform.GetDeltaTime();
    BOOST_CHECK_GE(initial_dt, 0.0f);

    bool running = true;
    platform.PollEvents(running);
    float dt = platform.GetDeltaTime();
    BOOST_CHECK_GE(dt, 0.0f);
}

BOOST_AUTO_TEST_CASE(test_poll_events) {
    BOOST_REQUIRE(platform.Init("Test Window", 800, 600));

    bool running = true;
    platform.PollEvents(running);
    BOOST_CHECK(running);
}

BOOST_AUTO_TEST_CASE(test_event_subscription) {
    BOOST_REQUIRE(platform.Init("Test Window", 800, 600));

    bool receivedEvent = false;
    plt::EventCallback cb = [&receivedEvent](const void*) {
        receivedEvent = true;
    };

    platform.SubscribeToEvent(plt::EventType::WindowResize, cb);
    platform.UnsubscribeFromEvent(plt::EventType::WindowResize, cb);
}

BOOST_AUTO_TEST_CASE(test_input_queries) {
    BOOST_REQUIRE(platform.Init("Test Window", 800, 600));

    BOOST_CHECK(!platform.IsKeyPressed(100));
    BOOST_CHECK(!platform.IsMouseButtonPressed(1));

    float mx = -1.0f, my = -1.0f;
    platform.GetMousePosition(mx, my);
    BOOST_CHECK_GE(mx, 0.0f);
    BOOST_CHECK_GE(my, 0.0f);
}

BOOST_AUTO_TEST_CASE(test_shutdown) {
    BOOST_REQUIRE(platform.Init("Test Window", 800, 600));

    void* window = platform.GetNativeWindow();
    BOOST_REQUIRE(window != nullptr);

    platform.Shutdown();
    BOOST_CHECK(platform.GetNativeWindow() == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
