message(STATUS "Running gfx cmake")
cmake_policy(SET CMP0167 NEW)

# Add option to enable/disable tests
option(GFX_ENABLE_TESTS "Enable testing for GFX module" OFF)

# Define source files explicitly, excluding test directory
file(GLOB_RECURSE BASE_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)
# Remove test files from BASE_SRC
file(GLOB_RECURSE TEST_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*"
)
list(REMOVE_ITEM BASE_SRC ${TEST_FILES})

# Create platform static library
add_library(gfx STATIC ${BASE_SRC})

# Configure include directories
target_include_directories(gfx
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Enable tests in Debug mode by default or when explicitly enabled
if (GFX_ENABLE_TESTS)
    message(STATUS "Configuring GFX tests")

    # Find Boost with test components
    find_package(Boost REQUIRED COMPONENTS unit_test_framework)

    # Define test sources using GLOB_RECURSE
    file(GLOB_RECURSE TEST_SRC
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.hpp"
    )

    # Create test executable
    add_executable(gfx_tests ${TEST_SRC})
    target_link_libraries(gfx_tests
            PRIVATE
            gfx
            Boost::unit_test_framework
    )

    # Add test directory to include paths for test executable
    target_include_directories(gfx_tests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    # Enable CTest
    enable_testing()
    add_test(NAME gfx_tests COMMAND gfx_tests)
endif ()