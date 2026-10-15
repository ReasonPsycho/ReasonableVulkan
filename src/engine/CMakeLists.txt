message(STATUS "Running engine cmake")
cmake_policy(SET CMP0167 NEW)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(ENABLE_IMGUI)
    add_compile_definitions(ENABLE_IMGUI)
endif()


if(EDITOR_ENABLED)
    add_compile_definitions(EDITOR_ENABLED)
endif()

if(SPDLOG_DISABLE_LOGGING)
    add_compile_definitions(SPDLOG_DISABLE_LOGGING)
endif()

if(DEFINED SPDLOG_ACTIVE_LEVEL)
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=${SPDLOG_ACTIVE_LEVEL})
endif()

find_package(Tracy  CONFIG REQUIRED)
find_package(glm  CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)

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

# Auto-discover script files in res/scripts
file(GLOB_RECURSE RES_SCRIPT_FILES
        CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/res/scripts/*.cpp"
        "${CMAKE_SOURCE_DIR}/res/scripts/*.hpp"
        "${CMAKE_SOURCE_DIR}/res/scripts/*.h"
)

file(GLOB_RECURSE RES_SCRIPT_HEADERS
        CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/res/scripts/*.hpp"
        "${CMAKE_SOURCE_DIR}/res/scripts/*.h"
)

set(SCRIPT_INCLUDES "")
set(SCRIPT_EXPORT_ENTRIES "")
foreach(HEADER_FILE ${RES_SCRIPT_HEADERS})
    file(TO_CMAKE_PATH "${HEADER_FILE}" NORMALIZED_HEADER)
    get_filename_component(SCRIPT_CLASS_NAME "${HEADER_FILE}" NAME_WLE)
    string(APPEND SCRIPT_INCLUDES "#include \"${NORMALIZED_HEADER}\"\n")
    string(APPEND SCRIPT_EXPORT_ENTRIES "    {\n")
    string(APPEND SCRIPT_EXPORT_ENTRIES "        \"${SCRIPT_CLASS_NAME}\",\n")
    string(APPEND SCRIPT_EXPORT_ENTRIES "        &engine::ecs::ScriptRegistry::CreateScriptInstance<engine::ecs::${SCRIPT_CLASS_NAME}>,\n")
    string(APPEND SCRIPT_EXPORT_ENTRIES "        &engine::ecs::ScriptRegistry::SerializeScriptInstance<engine::ecs::${SCRIPT_CLASS_NAME}>,\n")
    string(APPEND SCRIPT_EXPORT_ENTRIES "        &engine::ecs::ScriptRegistry::DeserializeScriptInstance<engine::ecs::${SCRIPT_CLASS_NAME}>\n")
    string(APPEND SCRIPT_EXPORT_ENTRIES "    },\n")
endforeach()

set(GENERATED_SCRIPT_REGISTRY "${CMAKE_CURRENT_BINARY_DIR}/GeneratedScriptRegistry.cpp")
file(GENERATE OUTPUT "${GENERATED_SCRIPT_REGISTRY}" CONTENT "
#include \"systems/scriptSystem/ScriptRegistry.hpp\"
${SCRIPT_INCLUDES}

static const engine::ecs::ScriptExportEntry s_dynamicScriptEntries[] = {
${SCRIPT_EXPORT_ENTRIES}
};

extern \"C\" SCRIPT_API size_t GetDynamicScriptCount() {
    return sizeof(s_dynamicScriptEntries) / sizeof(s_dynamicScriptEntries[0]);
}

extern \"C\" SCRIPT_API const engine::ecs::ScriptExportEntry* GetDynamicScriptEntries() {
    return s_dynamicScriptEntries;
}
")

# Interface target is defined in root CMakeLists.txt to allow clean dependency ordering
if (NOT TARGET engine_interface)
    add_library(engine_interface INTERFACE)
    target_include_directories(engine_interface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(engine_interface INTERFACE platform_interface)
endif()

# Create gameplay_scripts shared library
add_library(gameplay_scripts SHARED ${RES_SCRIPT_FILES} ${GENERATED_SCRIPT_REGISTRY})
target_compile_definitions(gameplay_scripts PRIVATE GAMEPLAY_SCRIPTS_EXPORTS GLM_ENABLE_EXPERIMENTAL IMGUI_DEFINE_MATH_OPERATORS)
if(ENABLE_IMGUI)
    target_compile_definitions(gameplay_scripts PRIVATE ENABLE_IMGUI)
endif()
if(EDITOR_ENABLED)
    target_compile_definitions(gameplay_scripts PRIVATE EDITOR_ENABLED)
endif()
if(SPDLOG_DISABLE_LOGGING)
    target_compile_definitions(gameplay_scripts PRIVATE SPDLOG_DISABLE_LOGGING)
endif()
if(DEFINED SPDLOG_ACTIVE_LEVEL)
    target_compile_definitions(gameplay_scripts PRIVATE SPDLOG_ACTIVE_LEVEL=${SPDLOG_ACTIVE_LEVEL})
endif()

target_include_directories(gameplay_scripts
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/res
        ${CMAKE_SOURCE_DIR}/res/scripts
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Create static library
add_library(engine STATIC ${BASE_SRC})

target_link_libraries(gameplay_scripts PUBLIC
        engine
        am
        vks
        platform
        renderDocManager
        engine_interface
        glm::glm
        RapidJSON
        spdlog::spdlog
        imgui::imgui
)

# Configure include directories
target_include_directories(engine
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/res
        ${CMAKE_SOURCE_DIR}/res/scripts
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)


add_definitions(-DGLM_ENABLE_EXPERIMENTAL -DIMGUI_DEFINE_MATH_OPERATORS)


# Platform specific linking
target_link_libraries(engine PUBLIC
        engine_interface
        vks_interface
        am_interface
        platform_interface
        PRIVATE
        Tracy::TracyClient
        glm::glm
        RapidJSON
        spdlog::spdlog
        imgui::imgui
        imguizmo::imguizmo
        renderDocManager
)

# Enable tests in Debug mode by default or when explicitly enabled
if (ENGINE_ENABLE_TESTS)
    # Define test sources using GLOB_RECURSE
    file(GLOB_RECURSE TEST_SRC
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.hpp"
    )

    if (TEST_SRC)
        message(STATUS "Configuring engine tests")
        add_compile_definitions(ENGINE_ENABLE_TESTS)

        # Find Boost with test components
        find_package(Boost REQUIRED COMPONENTS unit_test_framework)

        # Create test executable
        add_executable(engine_test ${TEST_SRC})

        target_link_libraries(engine_test
                PRIVATE
                engine
                am
                vks
                platform
                renderDocManager
                Boost::unit_test_framework
                spdlog::spdlog
                glm::glm
                RapidJSON
                imgui::imgui
                imguizmo::imguizmo
        )

        # Add test directory to include paths for test executable
        target_include_directories(engine_test
                PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/tests
                ${CMAKE_CURRENT_SOURCE_DIR}
                ${CMAKE_CURRENT_SOURCE_DIR}/src
        )

        # Enable CTest
        enable_testing()
        add_test(NAME engine_test COMMAND engine_test WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endif ()
endif ()