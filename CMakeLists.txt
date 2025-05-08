# Specify the minimum CMake version required for this project
cmake_minimum_required(VERSION 3.24)

# Define the project name and create the project
project(ReasonableVulkan)

if (MSVC) # Caouse windows error
    add_compile_definitions(_AMD64_)
endif ()

# Set C++20 as the C++ standard for all targets
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Adding debug and build definitions 
if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG_BUILD)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    add_definitions(-DRELEASE_BUILD)
endif ()


# Find and load SDL2 package with its configurations
message(STATUS "Searching for packages")
find_package(SDL2 CONFIG REQUIRED)

# Add subdirectories containing other CMake projects
message(STATUS "Adding subdirectories")
add_subdirectory(src/platform)
add_subdirectory(src/vks)
add_subdirectory(src/gfx)

# Create the main executable target
message(STATUS "Creating main executable target: ${PROJECT_NAME}")

if (MSVC)
    # Add executable with specific flags for MSVC
    add_executable(${PROJECT_NAME} src/main.cpp)
    target_link_options(${PROJECT_NAME} PRIVATE "/SUBSYSTEM:CONSOLE" "/ENTRY:mainCRTStartup")
else ()
    add_executable(${PROJECT_NAME} src/main.cpp)
endif ()


# Link required libraries to the main executable
message(STATUS "Linking libraries")
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 platform vks)

# ---- Post build linkage ----
# Add and link to resource files in build folder
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/res
        ${CMAKE_CURRENT_BINARY_DIR}/res)

# Add and link to tracy files in build folder
#add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E create_symlink
#        ${CMAKE_SOURCE_DIR}/tracey
#        ${CMAKE_CURRENT_BINARY_DIR}/tracey)