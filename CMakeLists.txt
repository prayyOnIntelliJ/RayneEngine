cmake_minimum_required(VERSION 3.16)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(RayneEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.x
)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
        sol2
        GIT_REPOSITORY https://github.com/ThePhD/sol2.git
        GIT_TAG main
)
FetchContent_MakeAvailable(sol2)

FetchContent_Declare(
        lua_source
        GIT_REPOSITORY https://github.com/lua/lua.git
        GIT_TAG v5.4.6
)
FetchContent_GetProperties(lua_source)
if(NOT lua_source_POPULATED)
    FetchContent_Populate(lua_source)

    file(GLOB LUA_SOURCES "${lua_source_SOURCE_DIR}/*.c")
    list(REMOVE_ITEM LUA_SOURCES
            "${lua_source_SOURCE_DIR}/lua.c"
            "${lua_source_SOURCE_DIR}/luac.c"
            "${lua_source_SOURCE_DIR}/onelua.c"
    )

    add_library(lua_lib STATIC ${LUA_SOURCES}
            )

    if(UNIX)
        target_link_libraries(lua_lib PUBLIC dl)
    endif()

    target_include_directories(lua_lib PUBLIC ${lua_source_SOURCE_DIR})

    target_compile_options(lua_lib PRIVATE $<$<CONFIG:Release>:/O2>)
endif()

# --- Engine Executable ---
add_executable(RayneEngine
        src/main.cpp
        src/Core/Application/Application.cpp
        src/Core/Scenes/SceneManager.cpp
        src/Core/Primitives/Primitive.h
        src/Core/Primitives/RectanglePrimitive.h
        src/Core/Primitives/CirclePrimitive.h
        src/Core/Primitives/TrianglePrimitive.h
        src/Core/Primitives/PrimitiveManager.h
        src/Core/Math/MathR.h
        src/Core/Math/Vector.h
        src/Core/Scripting/ScriptComponent.cpp
        src/Core/Scripting/ScriptComponent.h
        src/Core/Scripting/LuaState.cpp
        src/Core/Scripting/LuaState.h
        src/Core/Math/MathR.cpp
        src/Core/ECS/Entity.h
        src/Core/ECS/Pool.h
        src/Core/ECS/Registry.h
        src/Core/ECS/Components.h
        src/Core/ECS/View.h
        src/Core/Scenes/EditorScene.cpp
        src/Core/Scenes/EditorScene.h
        src/Core/Scenes/GameScene.cpp
        src/Core/Scenes/GameScene.h
        src/Core/Input/InputManager.cpp
        src/Core/Input/InputManager.h
        src/Core/Scripting/EventManager.cpp
        src/Core/Scripting/EventManager.h
        src/Core/Scenes/ContentBrowser.cpp
        src/Core/Scenes/ContentBrowser.h
        src/Core/Resources/ResourceManager.cpp
        src/Core/Resources/ResourceManager.h
        src/Core/Audio/AudioManager.cpp
        src/Core/Audio/AudioManager.h
)

# --- Link Libraries ---
target_link_libraries(RayneEngine
        PRIVATE
        sfml-system
        sfml-window
        sfml-graphics
        sfml-audio
        sol2::sol2
        lua_lib
        nlohmann_json::nlohmann_json
)

# --- Copy assets ---
add_custom_command(TARGET RayneEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:RayneEngine>/assets
)

add_custom_command(TARGET RayneEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:sfml-system>
        $<TARGET_FILE:sfml-window>
        $<TARGET_FILE:sfml-graphics>
        $<TARGET_FILE:sfml-audio>
        $<TARGET_FILE_DIR:RayneEngine>
)

if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SFML_OPENAL_DLL "${sfml_SOURCE_DIR}/extlibs/bin/x64/openal32.dll")
    else()
        set(SFML_OPENAL_DLL "${sfml_SOURCE_DIR}/extlibs/bin/x86/openal32.dll")
    endif()

    add_custom_command(TARGET RayneEngine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SFML_OPENAL_DLL}"
            $<TARGET_FILE_DIR:RayneEngine>
    )
endif()

# --- Working Directory ---
set_target_properties(RayneEngine PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

target_compile_definitions(RayneEngine PRIVATE ASSET_PATH="${CMAKE_SOURCE_DIR}/assets/")