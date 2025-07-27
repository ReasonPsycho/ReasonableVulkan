message(STATUS "Running engine cmake")
# Collect all source and header files in current directory recursively
file(GLOB_RECURSE BASE_SRC "*.cpp" "*.hpp" "*.h")
file(GLOB_RECURSE BASE_HEADERS "*.hpp" "*.h")

# Find required packages
message(STATUS "Searching for required packages for platform library")


# Create static library
add_library(engine STATIC ${BASE_SRC})

# Platform specific linking
target_link_libraries(engine
        PRIVATE
)


# Enable tests in Debug mode by default or when explicitly enabled
if (ENGINE_ENABLE_TESTS)
    message(STATUS "Configuring asset manager tests")
    add_compile_definitions(ENGINE_ENABLE_TESTS)

    # Find Boost with test components
    find_package(Boost REQUIRED COMPONENTS unit_test_framework)

    # Define test sources using GLOB_RECURSE
    file(GLOB_RECURSE TEST_SRC
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.hpp"
    )

    # Create test executable
    add_executable(engine_test ${TEST_SRC})

    target_link_libraries(engine_test
            PRIVATE
#            am
            Boost::unit_test_framework
    )

    # Add test directory to include paths for test executable
    target_include_directories(engine_test
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    # Enable CTest
    enable_testing()
    add_test(NAME engine_test COMMAND engine_test WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif ()