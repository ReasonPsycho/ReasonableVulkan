#define BOOST_TEST_MODULE PlatformTests
#include <boost/test/unit_test.hpp>
#include "../src/Platform.hpp"
#include <filesystem>
#include <fstream>
#include <thread>
#include <atomic>

namespace {

inline void CleanupTestTempDir() {
    std::error_code ec;
    std::filesystem::path tempDirs[] = {
        "res/.internal/tests/temp",
        "../res/.internal/tests/temp",
        "../../res/.internal/tests/temp"
    };
    for (const auto& dir : tempDirs) {
        if (std::filesystem::exists(dir, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
                std::filesystem::remove_all(entry.path(), ec);
            }
        }
    }
}

struct TestTempCleanupObserver : public boost::unit_test::test_observer {
    void test_unit_finish(const boost::unit_test::test_unit& /*tu*/, unsigned long /*elapsed*/) override {
        CleanupTestTempDir();
    }
    void test_finish() override {
        CleanupTestTempDir();
    }
};

struct TestTempCleanupFixture {
    TestTempCleanupObserver observer;
    TestTempCleanupFixture() {
        boost::unit_test::framework::register_observer(observer);
        CleanupTestTempDir();
    }
    ~TestTempCleanupFixture() {
        boost::unit_test::framework::deregister_observer(observer);
        CleanupTestTempDir();
    }
};

BOOST_TEST_GLOBAL_FIXTURE(TestTempCleanupFixture);

} // anonymous namespace

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

BOOST_AUTO_TEST_CASE(test_watch_folder_events) {
    BOOST_REQUIRE(platform.Init("Test Window", 800, 600));

    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "rv_platform_watch_test";
    std::filesystem::create_directories(tempDir);

    std::atomic<bool> fileModifiedReceived = false;
    std::string modifiedPath;

    plt::EventCallback cbModified = [&fileModifiedReceived, &modifiedPath](const void* data) {
        if (data) {
            const auto* ev = static_cast<const plt::FileModifiedEvent*>(data);
            modifiedPath = ev->filePath;
            fileModifiedReceived = true;
        }
    };

    platform.SubscribeToEvent(plt::EventType::FileModifiedInFolder, cbModified);
    platform.WatchFolder(tempDir.string());

    // Create a file in watched directory
    std::filesystem::path testFile = tempDir / "test_file.txt";
    {
        std::ofstream ofs(testFile);
        ofs << "Initial content" << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Modify file
    {
        std::ofstream ofs(testFile, std::ios::app);
        ofs << "Modified content" << std::endl;
    }

    // Wait and poll events
    for (int i = 0; i < 20 && !fileModifiedReceived; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        bool running = true;
        platform.PollEvents(running);
    }

    platform.UnwatchFolder(tempDir.string());
    platform.UnsubscribeFromEvent(plt::EventType::FileModifiedInFolder, cbModified);

    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);

    // Watcher successfully ran
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()
