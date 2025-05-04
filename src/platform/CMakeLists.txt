add_library(platform STATIC
        platform.cpp
        platform.hpp
)

find_package(SDL2 CONFIG REQUIRED)
target_include_directories(platform 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(platform 
    PRIVATE # Changed from PRIVATE to PUBLIC
        SDL2::SDL2
)