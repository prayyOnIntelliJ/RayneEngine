cmake_minimum_required(VERSION 3.16)
project(RayneEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- load SFML automatically ---
include(FetchContent)
FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.x  # version 2.6-stable
)
FetchContent_MakeAvailable(SFML)

# --- Engine-Executable ---
add_executable(RayneEngine
        src/main.cpp
        src/Core/Application/Application.cpp
        src/Core/Scenes/SceneManager.cpp
        src/Core/Scenes/Scene.cpp
        src/Core/ViewPort/EditorCamera2D.cpp
        src/Core/Primitives/Primitive.h
        src/Core/Primitives/RectanglePrimitive.h
        src/Core/Primitives/CirclePrimitive.h
        src/Core/Primitives/TrianglePrimitive.h
        src/Core/Primitives/PrimitiveManager.h
)

# --- link SFML module ---
target_link_libraries(RayneEngine
        PRIVATE
        sfml-system
        sfml-window
        sfml-graphics
        sfml-audio
)

# --- copy assets automatically ---
add_custom_command(TARGET RayneEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:RayneEngine>/assets
)

# --- set working directory ---
set_target_properties(RayneEngine PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)