message(STATUS "Running engine cmake")
cmake_policy(SET CMP0167 NEW)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(SPDLOG_DISABLE_LOGGING)
    add_compile_definitions(SPDLOG_DISABLE_LOGGING)
endif()

if(DEFINED SPDLOG_ACTIVE_LEVEL)
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=${SPDLOG_ACTIVE_LEVEL})
endif()

find_package(Tracy  CONFIG REQUIRED)
find_package(glm  CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Define source files explicitly, excluding test directory
file(GLOB_RECURSE BASE_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.tpp"
)

# Remove test files from BASE_SRC
file(GLOB_RECURSE TEST_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*"
)

list(REMOVE_ITEM BASE_SRC ${TEST_FILES})

# Create static library
add_library(engine STATIC ${BASE_SRC})

# Configure include directories
target_include_directories(engine
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)


add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

# Platform specific linking
target_link_libraries(engine PUBLIC
        Tracy::TracyClient
        glm::glm
        spdlog::spdlog
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
            engine
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