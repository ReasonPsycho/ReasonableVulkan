#define BOOST_TEST_MODULE Asset Manager Test Suite
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include "../src/AssetManager.hpp"

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
    am::AssetManager::getInstance().cleanupTestTempAssets();
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
