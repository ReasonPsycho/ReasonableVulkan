#define BOOST_TEST_MODULE PlatformTests
#include <boost/test/unit_test.hpp>
#include "../src/Platform.hpp"

struct PlatformTestFixture {
    // THIS doesn't work but prob is an error with vulkan itself so it stays here until i can rewrite it.
    PlatformTestFixture() {
        // Setup code if needed
    }

    ~PlatformTestFixture() {
        platform::Shutdown();
    }
};

BOOST_FIXTURE_TEST_SUITE(PlatformTests, PlatformTestFixture)

                                                            BOOST_AUTO_TEST_CASE(test_init) {
                                                                bool result = platform::Init("Test Window", 800, 600);
                                                                BOOST_CHECK(result);
                                                                BOOST_CHECK(platform::GetWindow() != nullptr);
                                                            }

                                                            BOOST_AUTO_TEST_CASE(test_window_info) {
                                                                // First initialize the platform
                                                                BOOST_REQUIRE(platform::Init("Test Window", 800, 600));

                                                                // Test getting window info
                                                                BOOST_CHECK_NO_THROW({
                                                                    platform::WindowInfo info = platform::GetWindowInfo(
                                                                    );
                                                                    BOOST_CHECK(info.hInstance != nullptr);
                                                                    BOOST_CHECK(info.wndProc != nullptr);
                                                                    BOOST_CHECK(info.hwnd != nullptr);
                                                                    });
                                                            }

                                                            BOOST_AUTO_TEST_CASE(test_delta_time) {
                                                                BOOST_REQUIRE(platform::Init("Test Window", 800, 600));

                                                                // Test initial delta time
        float initial_dt = platform::GetDeltaTime();
        BOOST_CHECK_GE(initial_dt, 0.0f);

        // Test delta time after some events
        bool running = true;
        platform::PollEvents(running);
        float dt = platform::GetDeltaTime();
        BOOST_CHECK_GE(dt, 0.0f);
    }

    BOOST_AUTO_TEST_CASE(test_poll_events) {
        BOOST_REQUIRE(platform::Init("Test Window", 800, 600));

        bool running = true;
        platform::PollEvents(running);
        // By default, running should still be true as we haven't triggered any quit events
        BOOST_CHECK(running);
    }

    BOOST_AUTO_TEST_CASE(test_shutdown) {
        BOOST_REQUIRE(platform::Init("Test Window", 800, 600));

        SDL_Window *window = platform::GetWindow();
        BOOST_REQUIRE(window != nullptr);

        platform::Shutdown();
        // After shutdown, the window pointer should be null
        BOOST_CHECK(platform::GetWindow() == nullptr);
    }

BOOST_AUTO_TEST_SUITE_END()
