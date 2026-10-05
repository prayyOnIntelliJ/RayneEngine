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

add_executable(RayneEngine
        src/main.cpp
        src/Runtime/Application/Application.cpp
        src/Runtime/Application/SplashScreen.cpp
        src/Runtime/Scenes/SceneManager.cpp
        src/Runtime/Primitives/Primitive.h
        src/Runtime/Primitives/RectanglePrimitive.h
        src/Runtime/Primitives/CirclePrimitive.h
        src/Runtime/Primitives/TrianglePrimitive.h
        src/Runtime/Primitives/PrimitiveManager.h
        src/Runtime/Math/MathR.h
        src/Runtime/Math/Vector.h
        src/Runtime/Scripting/ScriptComponent.cpp
        src/Runtime/Scripting/ScriptComponent.h
        src/Runtime/Scripting/LuaState.cpp
        src/Runtime/Scripting/LuaState.h
        src/Runtime/Math/MathR.cpp
        src/Runtime/ECS/Entity.h
        src/Runtime/ECS/Pool.h
        src/Runtime/ECS/Registry.h
        src/Runtime/ECS/Components.h
        src/Runtime/ECS/View.h
        src/Editor/Core/EditorScene.cpp
        src/Editor/Core/EditorScene_Hierarchy.cpp
        src/Editor/Core/EditorScene_Inspector.cpp
        src/Editor/Core/EditorScene_Menu.cpp
        src/Editor/Core/EditorScene_SaveLoad.cpp
        src/Editor/Core/EditorScene_Settings.cpp
        src/Editor/Core/EditorScene_UI.cpp
        src/Editor/Core/FileDropManager.h
        src/Editor/Core/FileDropManager.cpp
        src/Editor/Scenes/UIEditorScene.cpp
        src/Editor/Scenes/ProjectHubScene.cpp
        src/Editor/Core/EditorScene.h
        src/Editor/Commands/EditorCommand.h
        src/Editor/Data/EditorObject.h
        src/Editor/Data/EditorSerializer.h
        src/Editor/Data/EditorSerializer.cpp
        src/Editor/Panels/HierarchyPanel.h
        src/Editor/Panels/HierarchyPanel.cpp
        src/Editor/Panels/InspectorPanel.h
        src/Editor/Panels/InspectorPanel.cpp
        src/Editor/Panels/MenuBarPanel.h
        src/Editor/Panels/MenuBarPanel.cpp
        src/Editor/Panels/ToolbarPanel.h
        src/Editor/Panels/ToolbarPanel.cpp
        src/Editor/Panels/SpotlightPalette.h
        src/Editor/Panels/SpotlightPalette.cpp
        src/Editor/Gizmos/GizmoManager.h
        src/Editor/Gizmos/GizmoManager.cpp
        src/Editor/Modals/EditorModals.h
        src/Editor/Modals/EditorModals.cpp
        src/Runtime/Scenes/GameScene.cpp
        src/Runtime/Scenes/GameScene.h
        src/Runtime/Scenes/CameraManager.cpp
        src/Runtime/Scenes/CameraManager.h
        src/Runtime/Input/InputManager.cpp
        src/Runtime/Input/InputManager.h
        src/Runtime/Scripting/EventManager.cpp
        src/Runtime/Scripting/EventManager.h
        src/Editor/Panels/ContentBrowser.cpp
        src/Editor/Panels/ContentBrowser.h
        src/Editor/Panels/ConsolePanel.cpp
        src/Editor/Panels/ConsolePanel.h
        src/Runtime/Resources/ResourceManager.cpp
        src/Runtime/Resources/ResourceManager.h
        src/Runtime/Audio/AudioManager.cpp
        src/Runtime/Audio/AudioManager.h
        src/Runtime/Scenes/SceneSerializer.cpp
        src/Runtime/Scenes/SceneSerializer.h
        src/Runtime/UI/UIManager.cpp
        src/Runtime/UI/UIManager.h
        src/Runtime/ECS/PhysicsSystem.cpp
        src/Runtime/ECS/PhysicsSystem.h
        src/Runtime/Graphics/SpriteBatch.h
        src/Runtime/Graphics/SpriteBatch.cpp
        src/Runtime/Profiler/Profiler.h
        src/Runtime/Profiler/Profiler.cpp
        src/Editor/Panels/ProfilerPanel.h
        src/Editor/Panels/ProfilerPanel.cpp
)

target_include_directories(RayneEngine PRIVATE
        src
        src/Runtime
        src/Editor
)

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

if(WIN32)
    target_link_libraries(RayneEngine PRIVATE comdlg32 shell32)
endif()

add_custom_command(TARGET RayneEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:RayneEngine>/assets
)

add_custom_command(TARGET RayneEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/engine_content
        $<TARGET_FILE_DIR:RayneEngine>/engine_content
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

add_custom_command(TARGET RayneEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:RayneEngine>/templates
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/templates/RayneGame.exe
        $<TARGET_FILE_DIR:RayneEngine>/templates/RayneGame.exe
)

set_target_properties(RayneEngine PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

target_compile_definitions(RayneEngine PRIVATE "ASSET_PATH=\"assets\"" "ENGINE_ASSET_PATH=\"engine_content\"")

add_executable(RayneGame
        src/main.cpp
        src/Runtime/Application/Application.cpp
        src/Runtime/Application/SplashScreen.cpp
        src/Runtime/Scenes/SceneManager.cpp
        src/Runtime/Primitives/Primitive.h
        src/Runtime/Primitives/RectanglePrimitive.h
        src/Runtime/Primitives/CirclePrimitive.h
        src/Runtime/Primitives/TrianglePrimitive.h
        src/Runtime/Primitives/PrimitiveManager.h
        src/Runtime/Math/MathR.h
        src/Runtime/Math/Vector.h
        src/Runtime/Scripting/ScriptComponent.cpp
        src/Runtime/Scripting/ScriptComponent.h
        src/Runtime/Scripting/LuaState.cpp
        src/Runtime/Scripting/LuaState.h
        src/Runtime/Math/MathR.cpp
        src/Runtime/ECS/Entity.h
        src/Runtime/ECS/Pool.h
        src/Runtime/ECS/Registry.h
        src/Runtime/ECS/Components.h
        src/Runtime/ECS/View.h
        src/Editor/Core/EditorScene.cpp
        src/Editor/Core/EditorScene_Hierarchy.cpp
        src/Editor/Core/EditorScene_Inspector.cpp
        src/Editor/Core/EditorScene_Menu.cpp
        src/Editor/Core/EditorScene_SaveLoad.cpp
        src/Editor/Core/EditorScene_Settings.cpp
        src/Editor/Core/EditorScene_UI.cpp
        src/Editor/Core/FileDropManager.h
        src/Editor/Core/FileDropManager.cpp
        src/Editor/Scenes/UIEditorScene.cpp
        src/Editor/Scenes/ProjectHubScene.cpp
        src/Editor/Core/EditorScene.h
        src/Editor/Commands/EditorCommand.h
        src/Editor/Data/EditorObject.h
        src/Editor/Data/EditorSerializer.h
        src/Editor/Data/EditorSerializer.cpp
        src/Editor/Panels/HierarchyPanel.h
        src/Editor/Panels/HierarchyPanel.cpp
        src/Editor/Panels/InspectorPanel.h
        src/Editor/Panels/InspectorPanel.cpp
        src/Editor/Panels/MenuBarPanel.h
        src/Editor/Panels/MenuBarPanel.cpp
        src/Editor/Panels/ToolbarPanel.h
        src/Editor/Panels/ToolbarPanel.cpp
        src/Editor/Panels/SpotlightPalette.h
        src/Editor/Panels/SpotlightPalette.cpp
        src/Editor/Gizmos/GizmoManager.h
        src/Editor/Gizmos/GizmoManager.cpp
        src/Editor/Modals/EditorModals.h
        src/Editor/Modals/EditorModals.cpp
        src/Runtime/Scenes/GameScene.cpp
        src/Runtime/Scenes/GameScene.h
        src/Runtime/Scenes/CameraManager.cpp
        src/Runtime/Scenes/CameraManager.h
        src/Runtime/Input/InputManager.cpp
        src/Runtime/Input/InputManager.h
        src/Runtime/Scripting/EventManager.cpp
        src/Runtime/Scripting/EventManager.h
        src/Editor/Panels/ContentBrowser.cpp
        src/Editor/Panels/ContentBrowser.h
        src/Editor/Panels/ConsolePanel.cpp
        src/Editor/Panels/ConsolePanel.h
        src/Runtime/Resources/ResourceManager.cpp
        src/Runtime/Resources/ResourceManager.h
        src/Runtime/Audio/AudioManager.cpp
        src/Runtime/Audio/AudioManager.h
        src/Runtime/Scenes/SceneSerializer.cpp
        src/Runtime/Scenes/SceneSerializer.h
        src/Runtime/UI/UIManager.cpp
        src/Runtime/UI/UIManager.h
        src/Runtime/ECS/PhysicsSystem.cpp
        src/Runtime/ECS/PhysicsSystem.h
        src/Runtime/Graphics/SpriteBatch.h
        src/Runtime/Graphics/SpriteBatch.cpp
        src/Runtime/Profiler/Profiler.h
        src/Runtime/Profiler/Profiler.cpp
        src/Editor/Panels/ProfilerPanel.h
        src/Editor/Panels/ProfilerPanel.cpp
)

target_include_directories(RayneGame PRIVATE
        src
        src/Runtime
        src/Editor
)

target_link_libraries(RayneGame PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
        nlohmann_json::nlohmann_json
        sol2::sol2
        lua_lib
)

set_target_properties(RayneGame PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:RayneGame>"
)

target_compile_definitions(RayneGame PRIVATE RAYNE_STANDALONE "ASSET_PATH=\"assets\"" "ENGINE_ASSET_PATH=\"engine_content\"")

if(MSVC)
    target_compile_options(RayneEngine PRIVATE /bigobj)
    target_compile_options(RayneGame PRIVATE /bigobj)
endif()

add_dependencies(RayneEngine RayneGame)

add_custom_command(TARGET RayneGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/templates
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:RayneGame>
        ${CMAKE_SOURCE_DIR}/templates/RayneGame.exe
)

if(WIN32)
    set(OPENAL_PACKAGE_ITEM "openal32.dll")
else()
    set(OPENAL_PACKAGE_ITEM "")
endif()

add_custom_target(PackageEngine
        COMMAND ${CMAKE_COMMAND} -E echo "Creating RayneEngine-Editor.zip..."
        COMMAND ${CMAKE_COMMAND} -E tar cf "${CMAKE_SOURCE_DIR}/RayneEngine-Editor.zip" --format=zip
                "$<TARGET_FILE_NAME:RayneEngine>"
                "assets"
                "engine_content"
                "templates"
                "${OPENAL_PACKAGE_ITEM}"
                "$<TARGET_FILE_NAME:sfml-system>"
                "$<TARGET_FILE_NAME:sfml-window>"
                "$<TARGET_FILE_NAME:sfml-graphics>"
                "$<TARGET_FILE_NAME:sfml-audio>"
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:RayneEngine>"
        DEPENDS RayneEngine
        COMMENT "Packaging complete RayneEngine Editor into RayneEngine-Editor.zip..."
)

