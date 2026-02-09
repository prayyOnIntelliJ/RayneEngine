cmake_minimum_required(VERSION 3.16)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(RayneEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# --- load SFML automatically ---
FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.x
)
FetchContent_MakeAvailable(SFML)

# --- load Lua automatically ---
FetchContent_Declare(
        lua
        GIT_REPOSITORY https://github.com/walterschell/Lua.git
        GIT_TAG v5.4.6
)

FetchContent_MakeAvailable(lua)


# --- load sol3 automatically ---
FetchContent_Declare(
        sol3
        GIT_REPOSITORY https://github.com/ThePhD/sol2.git
        GIT_TAG v3.3.1
)
FetchContent_MakeAvailable(sol3)

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
        src/Core/Math/MathR.h
        src/Core/Math/Vector.h
        src/Core/Scripting/ScriptComponent.cpp
        src/Core/Scripting/ScriptComponent.h
)

# --- link libraries ---
target_link_libraries(RayneEngine
        PRIVATE
        sfml-system
        sfml-window
        sfml-graphics
        sfml-audio
        lua
        sol2::sol2
)

# --- sol3 includes ---
target_include_directories(RayneEngine
        PRIVATE
        ${sol3_SOURCE_DIR}/single
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
