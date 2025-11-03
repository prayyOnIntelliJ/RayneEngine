cmake_minimum_required(VERSION 3.16)
project(RayneEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- load SFML via FetchContent ---
include(FetchContent)

FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.x   # use the stable 2.6-Version
)

FetchContent_MakeAvailable(SFML)

add_executable(RayneEngine
        src/main.cpp
        src/Core/Application.cpp
        src/Core/SceneManager.cpp
)

# --- link SFML with project ---
target_link_libraries(RayneEngine
        PRIVATE
        sfml-system
        sfml-window
        sfml-graphics
        sfml-audio
)
