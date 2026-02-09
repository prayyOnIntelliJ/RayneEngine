cmake_minimum_required(VERSION 3.16)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(RayneEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# --- 1. SFML laden ---
FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.x
)
FetchContent_MakeAvailable(SFML)

# --- 2. Sol2 laden ---
FetchContent_Declare(
        sol2
        GIT_REPOSITORY https://github.com/ThePhD/sol2.git
        GIT_TAG main  # Wir nutzen den aktuellen Stand, da v3.3.1 Probleme mit C++20 hat
)
FetchContent_MakeAvailable(sol2)

# --- 3. Lua manuell laden & konfigurieren ---
# Wir nutzen das offizielle Mirror-Repo von Lua.org
FetchContent_Declare(
        lua_source
        GIT_REPOSITORY https://github.com/lua/lua.git
        GIT_TAG v5.4.6
)
FetchContent_GetProperties(lua_source)
if(NOT lua_source_POPULATED)
    FetchContent_Populate(lua_source)

    # Hier erstellen wir manuell das Lua-Library-Target
    # Wir nehmen alle .c Dateien außer lua.c (CLI) und luac.c (Compiler)
    file(GLOB LUA_SOURCES "${lua_source_SOURCE_DIR}/*.c")
    list(REMOVE_ITEM LUA_SOURCES "${lua_source_SOURCE_DIR}/lua.c" "${lua_source_SOURCE_DIR}/luac.c")

    add_library(lua_lib STATIC ${LUA_SOURCES})

    # Wichtig für Linux/Mac: dl (dynamic link) Bibliothek linken
    if(UNIX)
        target_link_libraries(lua_lib PUBLIC dl)
    endif()

    # Include-Pfad setzen (damit sol2 <lua.h> findet)
    target_include_directories(lua_lib PUBLIC ${lua_source_SOURCE_DIR})

    # Compile-Optionen für C++ Kompatibilität
    target_compile_options(lua_lib PRIVATE -O2)
endif()

# --- Engine Executable ---
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
        src/Core/Scripting/LuaState.cpp
        src/Core/Scripting/LuaState.h
        src/Core/Math/MathR.cpp
)

# --- Link Libraries ---
target_link_libraries(RayneEngine
        PRIVATE
        sfml-system
        sfml-window
        sfml-graphics
        sfml-audio
        sol2::sol2    # Sol2 Header
        lua_lib       # Unsere manuell gebaute Lua Lib
)

# --- Assets kopieren ---
add_custom_command(TARGET RayneEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:RayneEngine>/assets
)

# --- Working Directory ---
set_target_properties(RayneEngine PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)