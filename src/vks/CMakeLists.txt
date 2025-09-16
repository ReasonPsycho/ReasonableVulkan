message(STATUS "Running vks cmake")
# Collect all source and header files in current directory recursively
file(GLOB_RECURSE BASE_SRC "*.cpp" "*.hpp" "*.h" "*.tpp")
file(GLOB_RECURSE BASE_HEADERS "*.hpp" "*.h" "*.tpp")

# Find required packages
message(STATUS "Searching for required packages for platform library")
find_package(ktx CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# Create static library
add_library(vks STATIC ${BASE_SRC})

# Platform specific linking
target_link_libraries(vks
        PRIVATE
        KTX::ktx
        Vulkan::Vulkan
        SDL2::SDL2
        imgui::imgui
        glm::glm
        am
)

# tinygltf is header-only, so we just need to include its directory
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
target_include_directories(vks PRIVATE ${TINYGLTF_INCLUDE_DIRS})