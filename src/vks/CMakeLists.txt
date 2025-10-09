
message(STATUS "Running vks cmake")
cmake_policy(SET CMP0167 NEW)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(ENABLE_IMGUI)
    add_compile_definitions(ENABLE_IMGUI)
endif()

if(PLATFORM_SDL3)
    add_compile_definitions(PLATFORM_SDL3)
endif()

if(SPDLOG_DISABLE_LOGGING)
    add_compile_definitions(SPDLOG_DISABLE_LOGGING)
endif()


if(DEFINED SPDLOG_ACTIVE_LEVEL)
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=${SPDLOG_ACTIVE_LEVEL})
endif()

# Find required packages
message(STATUS "Searching for required packages for platform library")
find_package(ktx CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Define source files explicitly, excluding test directory
file(GLOB_RECURSE BASE_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.tpp"
)

# Remove test files from BASE_SRC
file(GLOB_RECURSE TEST_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*"
)
list(REMOVE_ITEM BASE_SRC ${TEST_FILES})

# Create static library
add_library(vks STATIC ${BASE_SRC})

# Configure include directories
target_include_directories(vks
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

# Platform specific linking
target_link_libraries(vks
        PUBLIC
        am
        KTX::ktx
        SDL3::SDL3
        imgui::imgui
        glm::glm
        Vulkan::Vulkan
        fmt::fmt
        spdlog::spdlog
)

# tinygltf is header-only, so we just need to include its directory
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
target_include_directories(vks PRIVATE ${TINYGLTF_INCLUDE_DIRS})

# Enable tests in Debug mode by default or when explicitly enabled
if (VKS_ENABLE_TESTS)
    message(STATUS "Configuring vks tests")
    add_compile_definitions(VKS_ENABLE_TESTS)

    # Find Boost with test components
    find_package(Boost REQUIRED COMPONENTS unit_test_framework)

    # Define test sources using GLOB_RECURSE
    file(GLOB_RECURSE TEST_SRC
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.hpp"
    )

    # Create test executable
    add_executable(vks_test ${TEST_SRC})

    target_link_libraries(vks_test
            PRIVATE
            vks
            Boost::unit_test_framework
    )

    # Add test directory to include paths for test executable
    target_include_directories(vks_test
            PRIVATE
            tests
    )

    # Enable CTest
    enable_testing()
    add_test(NAME vks_test COMMAND vks_test WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()