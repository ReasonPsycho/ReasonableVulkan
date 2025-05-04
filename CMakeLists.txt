cmake_minimum_required(VERSION 3.20)
project(ReasonableVulkan)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)

add_subdirectory(src/platform)

# Add executable
add_executable(${PROJECT_NAME}
        src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 platform)