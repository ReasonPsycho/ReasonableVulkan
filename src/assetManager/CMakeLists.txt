message(STATUS "Running asset manager cmake")
cmake_policy(SET CMP0167 NEW)

if(SPDLOG_DISABLE_LOGGING)
    add_compile_definitions(SPDLOG_DISABLE_LOGGING)
endif()

if(DEFINED SPDLOG_ACTIVE_LEVEL)
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=${SPDLOG_ACTIVE_LEVEL})
endif()


# Find Assimp package
find_package(assimp CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Stb REQUIRED)  # Add STB
find_package(spdlog CONFIG REQUIRED)

# Find Boost with required components
find_package(Boost REQUIRED COMPONENTS uuid hash2)

# Add option to enable/disable tests
option(AM_ENABLE_TESTS "Enable testing for asset manager module" OFF)

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
add_library(am STATIC ${BASE_SRC})

if (MSVC)
    add_compile_options(/utf-8)
endif ()

# Configure include directories
target_include_directories(am
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
        ${Stb_INCLUDE_DIR}  # Add STB include directory
)

add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
# Link Assimp and Boost UUID
target_link_libraries(am
        PUBLIC
        assimp::assimp
        Boost::uuid
        fmt::fmt
        spdlog::spdlog
)

# Enable tests in Debug mode by default or when explicitly enabled
if (AM_ENABLE_TESTS)
    message(STATUS "Configuring asset manager tests")

    # Find Boost with test components
    find_package(Boost REQUIRED COMPONENTS unit_test_framework)

    # Define test sources using GLOB_RECURSE
    file(GLOB_RECURSE TEST_SRC
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.hpp"
    )

    # Create test executable
    add_executable(am_test ${TEST_SRC})

    target_link_libraries(am_test
            PRIVATE
            am
            Boost::unit_test_framework
    )

    # Add test directory to include paths for test executable
    target_include_directories(am_test
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    # Enable CTest
    enable_testing()
    add_test(NAME am_test COMMAND am_test)
endif ()