message(STATUS "Running platform cmake")
# Define source files explicitly
file(GLOB_RECURSE BASE_SRC 
    "platform.cpp"
    "platform.hpp"
)

# Find required dependencies
message(STATUS "Searching for platform library dependencies")
find_package(SDL2 CONFIG REQUIRED)

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
        SDL2::SDL2
)