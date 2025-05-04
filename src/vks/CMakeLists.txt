message(STATUS "Running vks cmake")
# Collect all source and header files in current directory recursively
file(GLOB_RECURSE BASE_SRC "*.cpp" "*.hpp" "*.h")
file(GLOB_RECURSE BASE_HEADERS "*.hpp" "*.h")

# Find required packages
message(STATUS "Searching for required packages for platform library")
find_package(SDL2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# Create static library
add_library(vks STATIC ${BASE_SRC})

# Platform specific linking
if(WIN32)
    target_link_libraries(vks
        PRIVATE
        SDL2::SDL2
        imgui::imgui
        glm::glm
    )
else()
    target_link_libraries(vks
        PRIVATE
        SDL2::SDL2
        imgui::imgui
        glm::glm
        ${CMAKE_THREAD_LIBS_INIT}
    )
endif()