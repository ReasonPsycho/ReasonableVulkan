message(STATUS "Running platform cmake")

# Add option to enable/disable tests
option(PLATFORM_ENABLE_TESTS "Enable testing for Platform module" OFF)

# Define source files explicitly
file(GLOB_RECURSE BASE_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

# Remove test files from BASE_SRC if they exist
file(GLOB_RECURSE TEST_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*"
)
list(REMOVE_ITEM BASE_SRC ${TEST_FILES})

# Find required dependencies
message(STATUS "Searching for platform library dependencies")
find_package(SDL3 CONFIG REQUIRED)

# Create platform static library
add_library(platform STATIC ${BASE_SRC})

# Configure include directories
target_include_directories(platform 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
message(STATUS "Configuring platform library dependencies")
target_link_libraries(platform 
    PRIVATE
        SDL3::SDL3
)

# Configure tests
if (PLATFORM_ENABLE_TESTS)
    message(STATUS "Configuring Platform tests")

    # Find Boost with test components
    find_package(Boost REQUIRED COMPONENTS unit_test_framework)

    # Define test sources
    file(GLOB_RECURSE TEST_SRC
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.hpp"
    )

    # Create test executable
    add_executable(platform_tests ${TEST_SRC})
    target_link_libraries(platform_tests
            PRIVATE
            platform
            Boost::unit_test_framework
    )

    # Add test directory to include paths for test executable
    target_include_directories(platform_tests
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    # Enable CTest
    enable_testing()
    add_test(NAME platform_tests COMMAND platform_tests)
endif ()