# Specify the minimum CMake version required for this project
cmake_minimum_required(VERSION 3.20)

# Define the project name and create the project
project(ReasonableVulkan)

# Set C++20 as the C++ standard for all targets
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find and load SDL2 package with its configurations
message(STATUS "Searching for packages")
find_package(SDL2 CONFIG REQUIRED)

# Add subdirectories containing other CMake projects
message(STATUS "Adding subdirectories")
add_subdirectory(src/platform)
add_subdirectory(src/vks)

# Create the main executable target
message(STATUS "Creating main executable target: ${PROJECT_NAME}")
add_executable(${PROJECT_NAME}
        src/main.cpp
)

# Link required libraries to the main executable
message(STATUS "Linking libraries")
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 platform)