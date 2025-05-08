#define BOOST_TEST_MODULE GraphicsEngine_Tests
#include <boost/test/unit_test.hpp>
#include "GraphicsEngine.hpp"

namespace {
    // Mock implementation for testing
    class MockGraphicsEngine : public gfx::GraphicsEngine {
    public:
        // Track function calls for verification
        bool beginFrameCalled = false;
        bool renderCalled = false;
        bool endFrameCalled = false;

        // Implementation of virtual functions
        void beginFrame() override {
            beginFrameCalled = true;
        }

        void render() override {
            renderCalled = true;
        }

        void endFrame() override {
            endFrameCalled = true;
        }

        // Helper to reset state
        void reset() {
            beginFrameCalled = false;
            renderCalled = false;
            endFrameCalled = false;
        }
    };
}

BOOST_AUTO_TEST_SUITE(GraphicsEngineTests)

BOOST_AUTO_TEST_CASE(BasicFunctionality) {
    MockGraphicsEngine engine;

    // Test initial state
    BOOST_CHECK_EQUAL(engine.beginFrameCalled, false);
    BOOST_CHECK_EQUAL(engine.renderCalled, false);
        BOOST_CHECK_EQUAL(engine.endFrameCalled, false);

        // Test frame sequence
        engine.beginFrame();
        BOOST_CHECK_EQUAL(engine.beginFrameCalled, true);

        engine.render();
        BOOST_CHECK_EQUAL(engine.renderCalled, true);

        engine.endFrame();
        BOOST_CHECK_EQUAL(engine.endFrameCalled, true);
    }

    BOOST_AUTO_TEST_CASE(SequenceReset) {
        MockGraphicsEngine engine;

        // Complete frame sequence
        engine.beginFrame();
        engine.render();
        engine.endFrame();

        // Reset state
        engine.reset();

        // Verify reset worked
        BOOST_CHECK_EQUAL(engine.beginFrameCalled, false);
        BOOST_CHECK_EQUAL(engine.renderCalled, false);
        BOOST_CHECK_EQUAL(engine.endFrameCalled, false);
    }

    BOOST_AUTO_TEST_CASE(MultipleFrames) {
        MockGraphicsEngine engine;

        // Test multiple frame sequences
        for (int i = 0; i < 3; ++i) {
            engine.reset();

            engine.beginFrame();
            BOOST_CHECK_EQUAL(engine.beginFrameCalled, true);

            engine.render();
            BOOST_CHECK_EQUAL(engine.renderCalled, true);

            engine.endFrame();
            BOOST_CHECK_EQUAL(engine.endFrameCalled, true);
        }
    }

BOOST_AUTO_TEST_SUITE_END()
