# RayneEngine

<p align="center">
  <img width="100%" alt="RayneEngine Cover" src="https://github.com/user-attachments/assets/98536ef6-3e47-4e01-a0af-c2e264530ad6" />
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Language-C%2B%2B20-blue.svg" alt="C++20" />
  <img src="https://img.shields.io/badge/Framework-SFML_2.6-darkgreen.svg" alt="SFML 2.6" />
  <img src="https://img.shields.io/badge/Scripting-Lua_5.4_%2B_sol2-purple.svg" alt="Lua 5.4" />
  <img src="https://img.shields.io/badge/Serialization-nlohmann__json-orange.svg" alt="JSON" />
  <img src="https://img.shields.io/badge/Build-CMake_%E2%89%A5_3.16-brightgreen.svg" alt="CMake" />
  <img src="https://img.shields.io/badge/Platform-Windows_%7C_Linux-lightgrey.svg" alt="Platform" />
</p>

---

## Overview

**RayneEngine** is a modular 2D game engine built with modern C++20 and [SFML](https://www.sfml-dev.org/). Designed as an in-depth portfolio project during game engineering training, it focuses on exploring clean software design patterns, high performance, and core engine subsystems from first principles:

- Custom, cache-conscious **Entity Component System (ECS)**
- Native **Visual Level Editor** with live property inspection, grid snapping, and content browser
- Embedded **Lua 5.4 scripting environment** via `sol2` with lifecycle events
- **AABB Collision Detection** with multi-channel filtering
- Multi-channel **Audio Engine** and hardware input polling
- Complete **JSON Scene Serialization** and project auto-saving

---

## Architecture

The engine architecture is structured into decoupled modules under `src/Core/`:

```
RayneEngine
|-- Core/
|   |-- Application/     Main loop, DPI awareness, window management, engine versioning
|   |-- Audio/           AudioManager sound effect pool (32 channels) and music streaming
|   |-- ECS/             Registry, component pools, view iterators, entity handles
|   |-- Input/           InputManager keyboard & mouse tracking with frame edge detection
|   |-- Math/            MathR custom math routines, interpolation, trigonometric tables
|   |-- Primitives/      Geometric primitive factories (Rectangles, Circles, Polygons)
|   |-- Resources/       ResourceManager centralized caching (textures, fonts, sounds)
|   |-- Scenes/          SceneManager, EditorScene, GameScene, ContentBrowser, SceneSerializer
|   `-- Scripting/       LuaState bindings, ScriptComponent lifecycle, EventManager
`-- assets/              Scenes, scripts, audio files, fonts, and textures
```

<p align="center">
  <img width="100%" alt="RayneEngine Editor Interface" src="https://github.com/user-attachments/assets/939ad1b6-3de2-44d1-97d1-38adfb8ab955" />
</p>

---

## Subsystems and Features

### Visual Level Editor (`EditorScene`)

The built-in level editor provides a real-time environment for constructing and previewing 2D game scenes:

- **Entity Hierarchy:** Scrollable list displaying all active scene entities. Supports selection, inspection, and context menu actions (duplicate, delete).
- **Property Inspector:**
  - Live numerical manipulation of position (`x`, `y`) and size (`width`, `height`).
  - Color palette tinting (`R`, `G`, `B`) for primitives and sprites.
  - Script path assignment with automatic Lua binding.
  - Sprite asset assignment with aspect-correct scaling.
  - Collision channel configuration for physics filtering.
- **Content Browser:**
  - Integrated file browser with breadcrumb navigation and path history.
  - Asset category filters: All, Images, Scripts, Audio, and Scenes.
  - File management: Create scripts, scenes, or folders, rename assets, and delete files.
  - Drag-and-drop: Drag textures or scripts from the browser directly onto viewport entities.
  - Scene loading: Double-click or trigger scene loading requests directly from the browser.
- **Interactive Viewport:**
  - Free camera panning using Middle Mouse Button (invertible and sensitivity-configurable).
  - Smooth camera zooming with user-defined min/max limits.
  - 8-point interactive resize handles for live scaling of selected objects.
  - Grid rendering with configurable cell dimensions, color, and opacity.
  - Toggleable snap-to-grid alignment.
- **Auto-Save & Configuration:**
  - Configurable auto-save intervals with on-screen notification popups.
  - Persistent JSON-based editor settings dialog covering general, editor, camera, and debug parameters.
- **Debug Overlays:**
  - Real-time FPS monitoring with configurable framerate limits.
  - Collider outline rendering for rapid physics debugging.
  - Entity ID labels drawn in world space.

---

### Entity Component System (ECS)

The custom ECS emphasizes data locality, cache friendliness, and clean decoupling:

- **`Registry`:** Manages entity lifecycles (`CreateEntity`, `DestroyEntity`, `Clear`) and hosts type-safe component pools.
- **`Pool<T>`:** Contiguous memory pools storing component instances with sparse-to-dense mappings for fast iteration.
- **`View<Components...>`:** Multi-component query views enabling `Registry::ForEach<T1, T2>(...)` iteration patterns.
- **Available Components:**
  - `TransformComponent`: 2D position (`float x`, `float y`).
  - `VelocityComponent`: Movement delta (`float dx`, `float dy`).
  - `RenderComponent`: Visual representation (`sf::Color`, `sf::Vector2f size`, and `ShapeType`: Rectangle, Circle, Triangle, Pentagon, Hexagon).
  - `SpriteComponent`: Renderable SFML sprite with texture handle and dimensions.
  - `CameraComponent`: Marks an entity as the active camera focus (`bool active`).
  - `CollisionComponent`: Configures collision filtering via an integer `channel`.
  - `ScriptComponent`: Encapsulates a sol2 Lua state environment and lifecycle hooks.

---

### Physics & Event Pipeline

- **AABB Collision Detection:** Broad-phase and narrow-phase bounding box collision checks executed in `GameScene::CheckCollisions()`.
- **Channel Filtering:** Collisions only occur between entities sharing the same integer collision channel (`channel == 0` by default).
- **Edge Detection:** Tracks collision state between frame steps to dispatch events precisely on initial overlap.
- **`EventManager`:** Centralized observer mechanism triggering callbacks in both native C++ systems and active entity Lua scripts (`OnCollision`).

---

### Audio System (`AudioManager`)

- **Channel Pooling:** Dynamically managed pool supporting up to 32 simultaneous sound effect instances with automated cleanup upon completion.
- **Music Streaming:** Continuous background music playback with support for looping, pause, resume, individual volume control, and global master volume.
- **Asset Integration:** Communicates directly with the `ResourceManager` to ensure sound buffers are loaded once and reused across instances.

---

### Input Management (`InputManager`)

- **State Buffering:** Tracks continuous press states (`IsKeyDown`, `IsMouseDown`), single-frame press transitions (`IsKeyPressed`, `IsMousePressed`), and release events (`IsKeyReleased`, `IsMouseReleased`).
- **Cursor Tracking:** Real-time mouse coordinate queries (`MouseX`, `MouseY`) and scroll delta tracking.
- **Frame Lifecycle:** Automatic edge-state resets at the conclusion of each frame.

---

### Asset Management (`ResourceManager`)

- **Resource Cache:** Thread-safe singleton repository caching `sf::Texture`, `sf::Font`, and `sf::SoundBuffer` instances via `std::shared_ptr`.
- **Memory Optimization:** Manual cache flushing capabilities (`ClearTextures`, `ClearFonts`, `ClearSounds`, `ClearAll`).
- **Telemetry:** In-engine telemetry tracking loaded resource counts and memory utilization.

---

### Scene Management & Serialization

- **`SceneManager`:** Finite state machine managing transitions between scene states (`EditorScene` and `GameScene`).
- **`SceneSerializer`:** JSON serialization format preserving entity hierarchies, geometric types, colors, transforms, velocities, sprite textures, collision channels, and attached Lua scripts.

---

## Editor Hotkeys and Controls

| Shortcut / Input | Context | Action |
|---|---|---|
| `F5` | Editor | Run simulation in Play Mode (`GameScene`) |
| `Escape` | Game Mode | Return to Editor Mode |
| `Escape` | Editor | Cancel text input, close menus, or deselect active entity |
| `Ctrl + S` | Editor | Save current scene to JSON |
| `Ctrl + L` | Editor | Reload current scene from JSON |
| `Ctrl + D` | Editor | Duplicate selected entity with offset |
| `Ctrl + ,` | Editor | Open / Close Editor Settings modal |
| `G` | Editor | Toggle grid snapping on / off |
| `Delete` | Editor | Delete currently selected entity |
| `Middle Mouse Drag` | Editor | Pan camera viewport |
| `Mouse Scroll Wheel` | Editor | Zoom camera in / out |
| `Left Click (Entity)` | Editor | Select entity and drag to reposition |
| `Left Click (Handles)`| Editor | Resize entity along 8 anchor handles |
| `Left Click (Empty)`  | Editor | Place new primitive shape of selected type |

---

## Complete Lua Scripting API

RayneEngine embeds Lua 5.4 using `sol2`. Every script attached to an entity receives its own isolated environment with the entity handle available through the global `self` variable.

### Lifecycle Hooks

```lua
function OnCreate(self)
    -- Invoked once when the entity is instantiated in the scene
end

function OnUpdate(self, dt)
    -- Invoked every simulation frame; dt represents delta time in seconds
end

function OnCollision(self, other)
    -- Invoked upon collision with another entity ID on the same channel
end
```

---

### Entity & Component Manipulation

| Function | Signature | Description |
|---|---|---|
| `CreateEntity` | `() -> Entity` | Instantiates a new entity and returns its integer ID |
| `DestroyEntity` | `(e: Entity)` | Removes an entity and all its attached components |
| `AddTransform` | `(e: Entity, x: number, y: number)` | Attaches a `TransformComponent` |
| `GetTransform` | `(e: Entity) -> Transform` | Returns a mutable reference to `{ x, y }` |
| `SetPosition` | `(e: Entity, x: number, y: number)` | Sets spatial position directly |
| `HasTransform` | `(e: Entity) -> boolean` | Checks if entity has a transform |
| `AddVelocity` | `(e: Entity, dx: number, dy: number)` | Attaches a `VelocityComponent` |
| `GetVelocity` | `(e: Entity) -> Velocity` | Returns a mutable reference to `{ dx, dy }` |
| `SetVelocity` | `(e: Entity, dx: number, dy: number)` | Sets velocity vector directly |
| `HasVelocity` | `(e: Entity) -> boolean` | Checks if entity has velocity |
| `AddSprite` | `(e: Entity, path: string, w: number, h: number)` | Attaches a `SpriteComponent` |
| `SetSprite` | `(e: Entity, path: string)` | Updates or swaps the sprite texture |
| `SetSpriteSize` | `(e: Entity, w: number, h: number)` | Updates rendered dimensions of sprite |
| `HasSprite` | `(e: Entity) -> boolean` | Checks if entity has a sprite |
| `SetColor` | `(e: Entity, r: number, g: number, b: number, [a]: number)` | Sets color of `RenderComponent` (0-255) |
| `AddCamera` | `(e: Entity)` | Attaches camera tracking component |
| `RemoveCamera` | `(e: Entity)` | Removes camera component |
| `HasCamera` | `(e: Entity) -> boolean` | Checks if entity has camera tracking |
| `AddCollision` | `(e: Entity, [channel]: integer)` | Attaches a `CollisionComponent` (default channel 0) |
| `RemoveCollision` | `(e: Entity)` | Removes collision component |
| `HasCollision` | `(e: Entity) -> boolean` | Checks if entity has collision enabled |
| `SetCollisionChannel` | `(e: Entity, channel: integer)` | Sets collision filter channel |
| `GetCollisionChannel` | `(e: Entity) -> integer` | Reads collision filter channel |
| `LoadScene` | `(sceneName: string)` | Switches active scene to `assets/scenes/<sceneName>.json` |

---

### Math Library (`MathR`)

| Function | Signature | Description |
|---|---|---|
| `MathR.ClampI` | `(value: number, min: number, max: number) -> number` | Clamps an integer between bounds |
| `MathR.ClampF` | `(value: number, min: number, max: number) -> number` | Clamps a float between bounds |
| `MathR.ClampF01` | `(value: number) -> number` | Clamps a float into `[0.0, 1.0]` |
| `MathR.AbsI` | `(value: number) -> number` | Returns integer absolute value |
| `MathR.AbsF` | `(value: number) -> number` | Returns floating-point absolute value |
| `MathR.Ceil` | `(value: number) -> number` | Smallest integer greater than or equal to argument |
| `MathR.Floor` | `(value: number) -> number` | Largest integer less than or equal to argument |
| `MathR.Lerp` | `(start: number, endVal: number, factor: number) -> number` | Linearly interpolates between two values |
| `MathR.InverseLerp` | `(start: number, endVal: number, value: number) -> number` | Computes interpolation factor for value |
| `MathR.Sin` | `(x: number) -> number` | Sine computation via Taylor series approximation |
| `MathR.Cos` | `(x: number) -> number` | Cosine computation |

---

### Input Library (`Input`)

| Function | Signature | Description |
|---|---|---|
| `Input.IsKeyDown` | `(key: number \| string) -> boolean` | True while key is held down |
| `Input.IsKeyPressed` | `(key: number \| string) -> boolean` | True during the frame key was pressed |
| `Input.IsKeyReleased` | `(key: number \| string) -> boolean` | True during the frame key was released |
| `Input.IsMouseDown` | `(button: number \| string) -> boolean` | True while mouse button is held down |
| `Input.IsMousePressed` | `(button: number \| string) -> boolean` | True during the frame mouse button was pressed |
| `Input.IsMouseReleased` | `(button: number \| string) -> boolean` | True during the frame mouse button was released |
| `Input.MouseX` | `() -> number` | Mouse horizontal position in screen space |
| `Input.MouseY` | `() -> number` | Mouse vertical position in screen space |
| `Input.MouseScroll` | `() -> number` | Mouse wheel scroll delta for current frame |

**Key Enums (`Key`):** `A` through `Z`, `Space`, `Enter`, `Escape`, `LShift`, `RShift`, `LCtrl`, `RCtrl`, `Left`, `Right`, `Up`, `Down`, `Tab`, `Delete`.  
Key strings like `"w"`, `"s"`, `"Space"`, `"Left"` are also accepted.

**Mouse Enums (`Mouse`):** `Left`, `Right`, `Middle`.  
Strings `"Left"`, `"Right"`, `"Middle"` are also accepted.

---

### Audio Library (`Audio`)

| Function | Signature | Description |
|---|---|---|
| `Audio.PlaySound` | `(path: string, [volume=100]: number, [pitch=1.0]: number)` | Plays sound effect via channel pool |
| `Audio.StopAllSounds` | `()` | Stops all active sound channels |
| `Audio.PlayMusic` | `(path: string, [loop=true]: boolean, [volume=100]: number)` | Plays streaming background music |
| `Audio.StopMusic` | `()` | Stops background music stream |
| `Audio.PauseMusic` | `()` | Pauses background music stream |
| `Audio.ResumeMusic` | `()` | Resumes paused music stream |
| `Audio.SetMusicVolume` | `(volume: number)` | Adjusts music volume (0 - 100) |
| `Audio.SetMasterVolume` | `(volume: number)` | Adjusts global engine volume (0 - 100) |

---

### Resource Library (`Resource`)

| Function | Signature | Description |
|---|---|---|
| `Resource.PreloadTexture` | `(path: string) -> boolean` | Caches a texture in memory |
| `Resource.PreloadFont` | `(path: string) -> boolean` | Caches a font in memory |
| `Resource.PreloadSound` | `(path: string) -> boolean` | Caches a sound buffer in memory |
| `Resource.ClearTextures` | `()` | Flushes texture cache |
| `Resource.ClearFonts` | `()` | Flushes font cache |
| `Resource.ClearSounds` | `()` | Flushes sound cache |
| `Resource.ClearAll` | `()` | Flushes entire resource cache |
| `Resource.TextureCount` | `() -> number` | Number of cached textures |
| `Resource.FontCount` | `() -> number` | Number of cached fonts |
| `Resource.SoundCount` | `() -> number` | Number of cached sounds |
| `Resource.PrintStats` | `()` | Prints memory statistics to console |

---

### Complete Lua Script Example

```lua
-- Player controller demonstrating Input, Transform, Audio, and Collision
local speed = 250
local bounceAmplitude = 20
local timer = 0
local initialY = nil

function OnCreate(self)
    if not HasCollision(self) then
        AddCollision(self, 0)
    end
    print("Entity " .. tostring(self) .. " spawned successfully.")
end

function OnUpdate(self, dt)
    local t = GetTransform(self)
    if not t then return end

    if initialY == nil then
        initialY = t.y
    end

    local moveX = 0
    local moveY = 0

    -- Input polling via string names or Key enums
    if Input.IsKeyDown("w") or Input.IsKeyDown(Key.Up) then
        moveY = moveY - 1
    end
    if Input.IsKeyDown("s") or Input.IsKeyDown(Key.Down) then
        moveY = moveY + 1
    end
    if Input.IsKeyDown("a") or Input.IsKeyDown(Key.Left) then
        dx = dx - 1
    end
    if Input.IsKeyDown("d") or Input.IsKeyDown(Key.Right) then
        moveX = moveX + 1
    end

    -- Smooth movement calculation
    t.x = t.x + moveX * speed * dt
    t.y = t.y + moveY * speed * dt

    -- Trigonometric animation via MathR
    timer = timer + dt
    local bounceOffset = MathR.Sin(timer * 4.0) * bounceAmplitude

    -- Sound effect trigger on key press edge
    if Input.IsKeyPressed(Key.Space) then
        Audio.PlaySound("assets/sounds/test_sound.mp3", 80, 1.0)
    end
end

function OnCollision(self, other)
    print("Entity " .. tostring(self) .. " collided with Entity " .. tostring(other))
end
```

---

## Technologies

- **Language:** C++20
- **Graphics, Windowing & Audio:** [SFML 2.6+](https://www.sfml-dev.org/)
- **Scripting Engine:** [Lua 5.4](https://www.lua.org/) & [sol2](https://github.com/ThePhD/sol2)
- **JSON Parser & Serializer:** [nlohmann_json](https://github.com/nlohmann/json)
- **Build System:** CMake 3.16+ (automated dependency management via `FetchContent`)

---

## Building the Project

### Prerequisites

- C++20 compliant compiler:
  - MSVC (Visual Studio 2022 v17.0 or newer)
  - GCC 11+
  - Clang 13+
- CMake 3.16 or higher
- Git

All engine dependencies (SFML, Lua 5.4, sol2, nlohmann_json) are automatically downloaded, built, and linked by CMake during compilation.

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/prayyOnIntelliJ/RayneEngine.git
cd RayneEngine

# Generate build configuration
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Compile binary
cmake --build build --config Release
```

### Execution

Run the binary from the root project directory so that the relative `assets/` path resolves correctly:

```bash
# Windows
.\build\Release\RayneEngine.exe

# Linux / macOS
./build/RayneEngine
```

