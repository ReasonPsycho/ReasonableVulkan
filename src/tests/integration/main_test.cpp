#define BOOST_TEST_MODULE IntegrationTests
#define SDL_MAIN_HANDLED
#include <boost/test/unit_test.hpp>

#include <cstdlib>
#include <iostream>
#include <SDL2/SDL.h>
#include "platform.hpp"
#include "vks/vulkanRenderer.h"

struct IntegrationTestFixture {
    VulkanExampleBase *vulkanExample;

    IntegrationTestFixture() {
        vulkanExample = new VulkanExampleBase();
    }

    ~IntegrationTestFixture() {
        if (vulkanExample) {
            platform::Shutdown();
            delete vulkanExample;
        }
    }
};

BOOST_FIXTURE_TEST_SUITE(IntegrationTests, IntegrationTestFixture)

    BOOST_AUTO_TEST_CASE(three_frame_test) {
        // Initialize platform
        BOOST_REQUIRE(platform::Init("Integration Test", 1280, 720));
        BOOST_REQUIRE(vulkanExample->initVulkan());

        platform::WindowInfo window_info = platform::GetWindowInfo();
        vulkanExample->setupWindow(window_info.hInstance, window_info.wndProc, window_info.hwnd);
        vulkanExample->prepare();

        // Run for exactly 3 frames
        bool running = true;
        int frameCount = 0;
        while (running && frameCount < 3) {
            platform::PollEvents(running);
            float deltaTime = platform::GetDeltaTime();
            vulkanExample->render();
            frameCount++;
        }

        // Verify we completed 3 frames
        BOOST_CHECK_EQUAL(frameCount, 3);
    }

BOOST_AUTO_TEST_SUITE_END()
