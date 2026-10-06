message(STATUS "Running renderDocManager cmake")
cmake_policy(SET CMP0167 NEW)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(renderDocManager STATIC RenderDocManager.cpp RenderDocManager.hpp renderdoc_app.h)


if(SPDLOG_DISABLE_LOGGING)
    add_compile_definitions(SPDLOG_DISABLE_LOGGING)
endif()


if(DEFINED SPDLOG_ACTIVE_LEVEL)
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=${SPDLOG_ACTIVE_LEVEL})
endif()

# Find required packages
message(STATUS "Searching for required packages for renderDocManager library")
find_package(spdlog CONFIG REQUIRED)

target_include_directories(renderDocManager 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(renderDocManager PRIVATE spdlog::spdlog)
