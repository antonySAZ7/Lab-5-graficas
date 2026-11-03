cmake_minimum_required(VERSION 3.20)

project(Lab5ProceduralPlanets LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.32.10
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.2
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL2 glm)

add_executable(lab5
    src/main.cpp
    src/framebuffer.cpp
    src/mesh.cpp
    src/shaders.cpp
    src/triangle.cpp
)

target_include_directories(lab5 PRIVATE include)
target_link_libraries(lab5 PRIVATE SDL2::SDL2-static glm::glm)

if(TARGET SDL2::SDL2main)
    target_link_libraries(lab5 PRIVATE SDL2::SDL2main)
endif()

if(MSVC)
    target_compile_options(lab5 PRIVATE /W4 /permissive-)
else()
    target_compile_options(lab5 PRIVATE -Wall -Wextra -Wpedantic)
endif()

