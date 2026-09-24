# RayneEngine

<p align="center">
  <img width="100%" alt="RayneEngine Cover" src="https://github.com/user-attachments/assets/98536ef6-3e47-4e01-a0af-c2e264530ad6" />
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Version-0.1.0-blue.svg" alt="Version 0.1.0" />
  <img src="https://img.shields.io/badge/Language-C%2B%2B20-blue.svg" alt="C++20" />
  <img src="https://img.shields.io/badge/Framework-SFML_2.6-darkgreen.svg" alt="SFML 2.6" />
  <img src="https://img.shields.io/badge/Scripting-Lua_5.4_%2B_sol2-purple.svg" alt="Lua 5.4" />
  <img src="https://img.shields.io/badge/Serialization-nlohmann__json-orange.svg" alt="JSON" />
  <img src="https://img.shields.io/badge/Build-CMake_%E2%89%A5_3.16-brightgreen.svg" alt="CMake" />
  <img src="https://img.shields.io/badge/Platform-Windows_%7C_Linux-lightgrey.svg" alt="Platform" />
</p>

> **Disclaimer**: This project, or portions of its code and assets, were created with the assistance of Artificial Intelligence (AI).

---

## Overview

**RayneEngine** is a modular 2D game engine built with modern C++20 and [SFML](https://www.sfml-dev.org/). Designed as an in-depth portfolio project during game engineering training, it focuses on exploring clean software design patterns, high performance, and core engine subsystems from first principles:

- Custom, cache-conscious **Entity Component System (ECS)** supporting up to 5 000 entities
- **Project Hub** first-run setup modal for easily configuring project settings and standalone target specs
- Native **Visual Level Editor** with live property inspection, grid snapping, and undo/redo
- Dedicated **UI Editor** for designing game HUDs visually on a fixed 1920×1080 canvas
- **In-Editor Console Panel** with live stdout/stderr capture, scrollable log, and command input
- Embedded **Lua 5.4 scripting environment** via `sol2` with lifecycle events and hot-reload
- **Solid & Static AABB Collision Detection** with multi-channel filtering and physical push-apart resolution
- Multi-channel **Audio Engine** and hardware input polling
- Complete **JSON Scene & UI Serialization** and project auto-saving
- **UI Manager** for runtime Text, Panel, and Button elements controlled from Lua
- **Standalone Game Export** creating an optimized `RayneGame` executable using your project settings

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
|   |-- Math/            MathR custom math routines, interpolation, trigonometric tables; Vector3
|   |-- Primitives/      Geometric primitive factories (Rectangles, Circles, Polygons)
|   |-- Resources/       ResourceManager centralized caching (textures, fonts, sounds)
|   |-- Scenes/          SceneManager, EditorScene, UIEditorScene, GameScene,
|   |                    ContentBrowser, ConsolePanel, SceneSerializer
|   |-- Scripting/       LuaState bindings, ScriptComponent lifecycle, EventManager,
|   |                    TimerManager, TweenManager, api_stub.lua
|   `-- UI/              UIManager (Text, Panel, Button elements, JSON persistence)
`-- assets/              Scenes, scripts, audio files, fonts, textures, and UI layouts
```

<p align="center">
  <img width="100%" alt="RayneEngine Editor Interface" src="https://github.com/user-attachments/assets/939ad1b6-3de2-44d1-97d1-38adfb8ab955" />
</p>

---

## Subsystems and Features

### Project Hub (First-Run Setup)

When launching the engine for the first time (i.e. no `project_settings.json` is found), the **Project Hub** will appear as a sleek modal dialog over a dimmed editor background. Similar to modern engine hubs, it allows you to configure your new project before jumping into development:
- **Project Details:** Define the project name and author.
- **Resolution:** Set the target window width and height for your standalone game export.
- **Engine Settings:** Toggle VSync and specify a target framerate (FPS).

These settings are serialized to `project_settings.json` which governs the standalone game's runtime behavior without interfering with the editor's fixed resolution.

---

### Visual Level Editor (`EditorScene`)

The built-in level editor provides a real-time environment for constructing and previewing 2D game scenes:

- **Entity Hierarchy:** Scrollable list displaying all active scene entities. Supports multi-selection, tag inspection, and context menu actions (rename, duplicate, delete).
- **Property Inspector:**
  - Live numerical manipulation of position (`x`, `y`), size (`width`, `height`), rotation (degrees), and scale (`scaleX`, `scaleY`).
  - Entity Tag and Name assignment for easy query from Lua scripts.
  - Color palette tinting (`R`, `G`, `B`) for primitives and sprites.
  - Script path assignment with automatic Lua binding.
  - Sprite asset assignment with aspect-correct scaling.
  - Collision channel and type configuration (`Static` / `Solid`) for physics filtering.
  - UI element text editing directly from the inspector (`UIText` field).
- **Content Browser:**
  - Integrated file browser with breadcrumb navigation and path history.
  - Asset category filters: All, Images, Scripts, Audio, Scenes.
  - Live search bar with instant filtering across all entries.
  - File management: Create scripts, scenes, or folders; rename, duplicate, or delete assets; copy asset path to clipboard; reveal file in OS file explorer.
  - Drag-and-drop: Drag textures or scripts from the browser directly onto viewport entities.
  - Scene loading: Double-click or trigger scene loading requests directly from the browser.
- **In-Editor Console Panel:**
  - Switchable bottom panel (Content Browser ↔ Console via tab bar).
  - Captures all `std::cout` and `std::cerr` output in real time via stream redirectors installed at engine boot.
  - Color-coded log lines: normal output in light grey, errors in red.
  - Scrollable log area with draggable scrollbar and mouse-wheel support; auto-scrolls to the newest message.
  - Command input field with blinking cursor, Ctrl+V paste support, and Enter-to-submit.
  - Built-in `clear` command to reset the log. Buffer capped at 2 000 messages.
- **Interactive Viewport & Selection:**
  - Free camera panning using Middle Mouse Button (invertible and sensitivity-configurable).
  - Smooth camera zooming with user-defined min/max limits.
  - **Multi-Selection & Box-Select:** Drag in empty space to create a selection box (marquee select); hold `Shift` or `Ctrl` to toggle entities in the selection group.
  - **Group Manipulation:** Move or delete multiple selected entities simultaneously.
  - **Undo / Redo (Command Pattern):** Full history stack supporting `Ctrl + Z` and `Ctrl + Y` for entity creation, deletion, dragging, and resizing — including atomic `MacroCommand` grouping for multi-object operations.
  - 8-point interactive resize handles for live scaling of selected objects.
  - Grid rendering with configurable cell dimensions, color, and opacity.
  - Toggleable snap-to-grid alignment.
- **Scene Serialization:**
  - Completely relative asset paths: Sprite textures and script files are saved relative to `assets/` for cross-platform and team portability.
- **Auto-Save & Configuration:**
  - Configurable auto-save intervals with on-screen notification popups.
  - Persistent JSON-based editor settings dialog covering general, editor, camera, and debug parameters (including pan inversion, scroll sensitivity, log level, and title bar auto-save indicator).
- **Debug Overlays:**
  - Real-time FPS monitoring with configurable framerate limits.
  - Collider outline rendering for rapid physics debugging.
  - Entity ID labels drawn in world space.

---

### UI Editor (`UIEditorScene`)

A dedicated scene for visually designing the game's HUD and UI layouts:

- **Fixed 1920×1080 canvas** — matches the runtime UI view for pixel-perfect WYSIWYG design.
- **Element Palette** — create Text, Panel, and Button UI elements with a single click.
- **Canvas Interaction** — select, drag, and resize elements using 8-point handles; middle-mouse pan across the canvas.
- **Element Hierarchy** — scrollable panel listing all UI elements by auto-generated ID (`text_1`, `panel_1`, `btn_1`, …).
- **Property Inspector** — live editing of 40+ fields including position, size, z-index, opacity, color (RGBA), text content, font size, letter/line spacing, text alignment (`Left` / `Center` / `Right`), text style bitmask (Bold, Italic, Underline, StrikeThrough), text outline, text offset, button normal/hover/pressed/disabled colors, and panel border/outline.
- **Persistence** — UI layouts are saved to and loaded from `assets/ui.json` via the `UIManager`. The engine reloads this file on startup automatically.
- **Menu Bar** — Save, Load, New UI, and Back to Editor actions.

---

### UI Manager (`UIManager`)

The runtime UI system renders interactive elements on top of the game world in a fixed 1920×1080 screen-space view:

- **Element types:** `Text`, `Panel`, `Button`
- **Z-ordering:** Elements are sorted by `zIndex` for correct layering (ascending for render, descending for hit testing).
- **Button interactivity:** Tracks hover, pressed, and released states per frame; supports disabled state with a distinct color.
- **JSON persistence:** `UIManager::Save()` / `UIManager::Load()` serialise and restore all element properties.
- **Lua integration:** All properties are controllable from Lua scripts at runtime (see [UI Library](#ui-library-ui) below).

---

### Entity Component System (ECS)

The custom ECS emphasizes data locality, cache friendliness, and clean decoupling. The registry supports up to **5 000 entities** (`MAX_ENTITIES = 5000`):

- **`Registry`:** Manages entity lifecycles (`CreateEntity`, `DestroyEntity`, `Clear`) and hosts type-safe component pools. Entity IDs start at `1`; `NULL_ENTITY = 0`.
- **`Pool<T>`:** Contiguous memory pools storing component instances with sparse-to-dense mappings for fast iteration (swap-and-pop removal).
- **`View<Components...>`:** Multi-component query views enabling `Registry::ForEach<T1, T2>(...)` iteration patterns via range-based for loop support.
- **Available Components:**
  - `TagComponent`: Name and classification tag for entities (`std::string tag`).
  - `TransformComponent`: 2D spatial position (`float x`, `float y`), rotation angle in degrees (`float rotation`), and scaling (`float scaleX`, `float scaleY`).
  - `VelocityComponent`: Movement delta (`float dx`, `float dy`).
  - `RenderComponent`: Visual representation (`sf::Color`, `sf::Vector2f size`, and `ShapeType`: Rectangle, Circle, Triangle, Pentagon, Hexagon).
  - `SpriteComponent`: Renderable SFML sprite with texture handle and dimensions; auto-loads via `ResourceManager` and computes scale on construction.
  - `CameraComponent`: Marks an entity as the active camera focus (`bool active`).
  - `CollisionComponent`: Configures collision filtering via an integer `channel` and a `CollisionType` (`Static` or `Solid`).
  - `ScriptComponent`: Encapsulates a sol2 Lua environment, filesystem modification timestamp tracking for live hot-reloading, and lifecycle hooks.

---

### Physics & Event Pipeline

- **AABB Collision Detection:** Broad-phase and narrow-phase bounding box collision checks executed in `GameScene::CheckCollisions()`.
- **Channel Filtering:** Collisions only occur between entities sharing the same integer collision channel (`channel == 0` by default).
- **Collision Types:**
  - `CollisionType::Static` *(default)* — passthrough detection; fires `OnCollision` event only.
  - `CollisionType::Solid` — physical push-apart resolution using minimum overlap axis; only moves entities that have a `VelocityComponent`.
- **Edge Detection:** Tracks collision state between frame steps to dispatch events precisely on the initial overlap frame only.
- **`EventManager`:** Centralized observer mechanism triggering callbacks in both native C++ systems and active entity Lua scripts (`OnCollision`).

---

### Audio System (`AudioManager`)

- **Channel Pooling:** Dynamically managed pool supporting up to 32 simultaneous sound effect instances with automated cleanup upon completion.
- **Music Streaming:** Continuous background music playback with support for looping, pause, resume, individual volume control, and global master volume (rescales all active sounds proportionally).
- **Asset Integration:** Communicates directly with the `ResourceManager` to ensure sound buffers are loaded once and reused across instances.

---

### Input Management (`InputManager`)

- **State Buffering:** Tracks continuous press states (`IsKeyDown`, `IsMouseDown`), single-frame press transitions (`IsKeyPressed`, `IsMousePressed`), and release events (`IsKeyReleased`, `IsMouseReleased`).
- **Cursor Tracking:** Real-time mouse coordinate queries (`MouseX`, `MouseY`) and scroll delta tracking.
- **Frame Lifecycle:** Automatic edge-state resets at the conclusion of each frame.

---

### Asset Management (`ResourceManager`)

- **Resource Cache:** Singleton repository caching `sf::Texture`, `sf::Font`, and `sf::SoundBuffer` instances via `std::shared_ptr`. Textures are loaded with `setSmooth(false)` by default.
- **Memory Optimization:** Manual cache flushing capabilities (`ClearTextures`, `ClearFonts`, `ClearSounds`, `ClearAll`).
- **Telemetry:** In-engine telemetry tracking loaded resource counts and memory utilization.

---

### Scene Management & Serialization

- **`SceneManager`:** Finite state machine managing transitions between scene states: `"editor"` (`EditorScene`), `"ui_editor"` (`UIEditorScene`), and `"game"` (`GameScene`). Calls `OnExit()` / `OnEnter()` on transitions; throws `std::runtime_error` for unknown scene names.
- **`SceneSerializer`:** JSON serialization format preserving entity hierarchies, geometric types, colors, transforms, velocities, sprite textures, collision channels, collision types, and attached Lua scripts.

---

### Engine Versioning

The `Rayne` namespace in `EngineVersion.h` provides compile-time constants and helpers:

- `Rayne::VERSION_MAJOR / MINOR / PATCH` — current version (`0.1.0`)
- `Rayne::VersionString()` — returns `"0.1.0"`
- `Rayne::PlatformString()` — returns `"Windows"` / `"macOS"` / `"Linux"` / `"Unknown"` based on compile-time macros
- `Rayne::DEFAULT_PROJECT_NAME` — default window title prefix used in Play Mode

---

## Editor Hotkeys and Controls

| Shortcut / Input | Context | Action |
|---|---|---|
| `F5` | Editor | Run simulation in Play Mode (`GameScene`) |
| `F6` | Game Mode | Hot-reload all modified Lua scripts dynamically |
| `Escape` | Game Mode | Return to Editor Mode |
| `Escape` | Editor | Cancel text input, close menus, or deselect active entity |
| `Ctrl + Z` | Editor | Undo last action (create, delete, move, resize) |
| `Ctrl + Y` | Editor | Redo last undone action |
| `Ctrl + S` | Editor | Save current scene to JSON (using relative asset paths) |
| `Ctrl + L` | Editor | Reload current scene from JSON |
| `Ctrl + D` | Editor | Duplicate selected entity with offset |
| `Ctrl + ,` | Editor | Open / Close Editor Settings modal |
| `G` | Editor | Toggle grid snapping on / off |
| `Delete` | Editor | Delete all currently selected entities |
| `Middle Mouse Drag` | Editor | Pan camera viewport |
| `Mouse Scroll Wheel` | Editor | Zoom camera in / out |
| `Left Click (Entity)` | Editor | Select entity and drag to reposition |
| `Shift / Ctrl + Click`| Editor | Add or toggle entity in multi-selection group |
| `Left Click + Drag (Empty)` | Editor | Box-select (marquee) multiple entities in viewport |
| `Left Click (Handles)`| Editor | Resize entity along 8 anchor handles |
| `Left Click (Empty)`  | Editor | Place new primitive shape of selected type (on click) |

---

## Complete Lua Scripting API

RayneEngine embeds Lua 5.4 using `sol2`. Every script attached to an entity receives its own isolated environment with the entity handle available through the global `self` variable. Scripts are automatically monitored and hot-reloaded at runtime upon file modification.

An `api_stub.lua` file (`---@meta`) ships with the engine, providing full LSP type annotations for IDE autocomplete in editors like VS Code with the Lua Language Server extension.

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
| `AddTag` | `(e: Entity, tag: string)` | Attaches a new `TagComponent` |
| `SetTag` | `(e: Entity, tag: string)` | Attaches or updates an existing `TagComponent` |
| `GetTag` | `(e: Entity) -> string` | Retrieves entity tag / name string |
| `HasTag` | `(e: Entity) -> boolean` | Checks if entity has a `TagComponent` |
| `FindEntityWithTag` | `(tag: string) -> Entity` | Searches and returns first entity with matching tag, or `0` |
| `AddTransform` | `(e: Entity, x: number, y: number)` | Attaches a `TransformComponent` |
| `GetTransform` | `(e: Entity) -> Transform` | Returns a mutable reference to `{ x, y, rotation, scaleX, scaleY }` |
| `SetPosition` | `(e: Entity, x: number, y: number)` | Sets spatial position directly |
| `SetRotation` | `(e: Entity, angle: number)` | Sets spatial rotation angle in degrees |
| `SetScale` | `(e: Entity, sx: number, sy: number)` | Sets spatial scale factors |
| `HasTransform` | `(e: Entity) -> boolean` | Checks if entity has a transform |
| `AddVelocity` | `(e: Entity, dx: number, dy: number)` | Attaches a `VelocityComponent` |
| `GetVelocity` | `(e: Entity) -> Velocity` | Returns a mutable reference to `{ dx, dy }` |
| `SetVelocity` | `(e: Entity, dx: number, dy: number)` | Sets velocity vector directly |
| `HasVelocity` | `(e: Entity) -> boolean` | Checks if entity has velocity |
| `AddSprite` | `(e: Entity, path: string, w: number, h: number)` | Attaches a `SpriteComponent` |
| `SetSprite` | `(e: Entity, path: string)` | Updates or swaps the sprite texture |
| `SetSpriteSize` | `(e: Entity, w: number, h: number)` | Updates rendered dimensions of sprite |
| `HasSprite` | `(e: Entity) -> boolean` | Checks if entity has a sprite |
| `SetColor` | `(e: Entity, r: number, g: number, b: number, [a]: number)` | Sets color of `RenderComponent` (0–255) |
| `AddCamera` | `(e: Entity)` | Attaches camera tracking component |
| `RemoveCamera` | `(e: Entity)` | Removes camera component |
| `HasCamera` | `(e: Entity) -> boolean` | Checks if entity has camera tracking |
| `AddCollision` | `(e: Entity, [channel]: integer)` | Attaches a `CollisionComponent` (default channel `0`, type `Static`) |
| `RemoveCollision` | `(e: Entity)` | Removes collision component |
| `HasCollision` | `(e: Entity) -> boolean` | Checks if entity has collision enabled |
| `SetCollisionChannel` | `(e: Entity, channel: integer)` | Sets collision filter channel |
| `GetCollisionChannel` | `(e: Entity) -> integer` | Reads collision filter channel |
| `SetCollisionType` | `(e: Entity, type: string)` | Sets collision type: `"static"` or `"solid"` |
| `GetCollisionType` | `(e: Entity) -> string` | Returns current collision type as string |
| `LoadScene` | `(sceneName: string)` | Switches active scene to `assets/scenes/<sceneName>.json` |

> **Tip:** `GetTransform(e)` returns a mutable table — you can read and write `t.x`, `t.y`, `t.rotation`, `t.scaleX`, `t.scaleY` directly on the returned reference.

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
| `MathR.Lerp` | `(start: number, endVal: number, factor: number) -> number` | Linearly interpolates between two values (factor clamped to `[0, 1]`) |
| `MathR.InverseLerp` | `(start: number, endVal: number, value: number) -> number` | Computes interpolation factor for value |
| `MathR.Sin` | `(x: number) -> number` | Sine via 10-term Taylor series approximation |
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

**Key strings** (case-insensitive): single letters `"a"`–`"z"`, digits `"0"`–`"9"`, `"space"`, `"enter"` / `"return"`, `"escape"` / `"esc"`, `"shift"` / `"lshift"`, `"rshift"`, `"ctrl"` / `"lctrl"`, `"rctrl"`, `"alt"` / `"lalt"`, `"ralt"`, `"left"`, `"right"`, `"up"`, `"down"`, `"tab"`, `"delete"` / `"del"`, `"backspace"`.

**Mouse Enums (`Mouse`):** `Left`, `Right`, `Middle`.  
Strings `"left"` / `"0"`, `"right"` / `"1"`, `"middle"` / `"2"` are also accepted.

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
| `Audio.SetMusicVolume` | `(volume: number)` | Adjusts music volume (0–100) |
| `Audio.SetMasterVolume` | `(volume: number)` | Adjusts global engine volume (0–100), rescales all active sounds |

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

### Timer Library (`Timer`)

The `Timer` utility allows scheduling one-shot Lua callbacks without manual delta-time accumulators:

| Function | Signature | Description |
|---|---|---|
| `Timer.After` | `(delay: number, callback: function)` | Executes the callback function once after `delay` seconds |

```lua
-- Example: Spawn an effect after 1.5 seconds
Timer.After(1.5, function()
    print("One-shot timer fired!")
end)
```

---

### Tween Library (`Tween`)

The `Tween` utility provides smooth position interpolation for entities with easing curves:

| Function | Signature | Description |
|---|---|---|
| `Tween.Position` | `(entity: Entity, targetX: number, targetY: number, duration: number, [easing]: string)` | Interpolates entity position over `duration` seconds |

**Supported Easing Modes:**
- `"linear"` *(default)* — constant velocity interpolation
- `"EaseInQuad"` — quadratic acceleration (starts slowly)
- `"EaseOutQuad"` — quadratic deceleration (ends gently)
- `"EaseInOutQuad"` — smooth ease-in followed by ease-out

```lua
-- Example: Move an entity smoothly to (500, 300) over 1.2 seconds
Tween.Position(self, 500, 300, 1.2, "EaseOutQuad")
```

---

### UI Library (`UI`)

All UI elements designed in the **UI Editor** can be queried and manipulated from Lua scripts at runtime.

| Function | Signature | Description |
|---|---|---|
| `UI_SetText` | `(id: string, text: string)` | Sets the display text of a Text or Button element |
| `UI_GetText` | `(id: string) -> string` | Gets the current text content |
| `UI_SetPosition` | `(id: string, x: number, y: number)` | Moves the element on the screen |
| `UI_SetSize` | `(id: string, w: number, h: number)` | Resizes the element |
| `UI_SetColor` | `(id: string, r, g, b, a: number)` | Sets the background fill color (0–255) |
| `UI_SetZIndex` | `(id: string, z: integer)` | Controls render/hit-test order |
| `UI_GetZIndex` | `(id: string) -> integer` | Reads current z-index |
| `UI_IsButtonClicked` | `(id: string) -> boolean` | True during the frame the button was clicked |
| `UI_IsButtonHovered` | `(id: string) -> boolean` | True while cursor is over the button |
| `UI_SetVisible` | `(id: string, visible: boolean)` | Shows or hides the element |
| `UI_GetVisible` | `(id: string) -> boolean` | Returns current visibility |
| `UI_SetOpacity` | `(id: string, opacity: number)` | Sets opacity (0–255) |
| `UI_SetTextStyle` | `(id: string, style: integer)` | SFML style bitmask: 0=Regular, 1=Bold, 2=Italic, 4=Underline, 8=StrikeThrough |
| `UI_SetTextAlign` | `(id: string, align: integer)` | Text alignment: 0=Left, 1=Center, 2=Right |
| `UI_SetUpperCase` | `(id: string, upper: boolean)` | Converts text to uppercase |
| `UI_SetFontSize` | `(id: string, size: integer)` | Sets character size in pixels |
| `UI_SetLetterSpacing` | `(id: string, spacing: number)` | Sets letter spacing factor |
| `UI_SetLineSpacing` | `(id: string, spacing: number)` | Sets line spacing factor |
| `UI_SetTextOutline` | `(id: string, r, g, b, a, thickness: number)` | Sets text outline color and thickness |
| `UI_SetTextOffset` | `(id: string, ox: number, oy: number)` | Offsets text within the element |
| `UI_SetTextColor` | `(id: string, r, g, b, a: number)` | Sets the text color independently |
| `UI_SetOutline` | `(id: string, r, g, b, a, thickness: number)` | Sets panel/button border color and thickness |
| `UI_SetDisabled` | `(id: string, disabled: boolean)` | Disables button interaction and applies disabled color |

```lua
-- Example: Show score label and react to button click
function OnUpdate(self, dt)
    UI_SetText("score_label", "Score: " .. tostring(score))

    if UI_IsButtonClicked("btn_start") then
        LoadScene("level1")
    end
end
```

---

### Complete Lua Script Example

```lua
-- Player controller demonstrating Input, Transform, Audio, Collision, and UI
local speed = 250
local bounceAmplitude = 20
local timer = 0

function OnCreate(self)
    if not HasCollision(self) then
        AddCollision(self, 0)
        SetCollisionType(self, "solid")
    end
    print("Entity " .. tostring(self) .. " spawned successfully.")
end

function OnUpdate(self, dt)
    local t = GetTransform(self)
    if not t then return end

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
        moveX = moveX - 1
    end
    if Input.IsKeyDown("d") or Input.IsKeyDown(Key.Right) then
        moveX = moveX + 1
    end

    -- Smooth movement
    t.x = t.x + moveX * speed * dt
    t.y = t.y + moveY * speed * dt

    -- Trigonometric animation via MathR
    timer = timer + dt
    local bounceOffset = MathR.Sin(timer * 4.0) * bounceAmplitude

    -- Update HUD
    UI_SetText("pos_label", string.format("X: %.0f  Y: %.0f", t.x, t.y))

    -- Sound effect on key press edge
    if Input.IsKeyPressed(Key.Space) then
        Audio.PlaySound("assets/sounds/jump.wav", 80, 1.0)
        Tween.Position(self, t.x, t.y - 100, 0.3, "EaseOutQuad")
    end
end

function OnCollision(self, other)
    local tag = HasTag(other) and GetTag(other) or "unknown"
    print("Collided with: " .. tag)
end
```

---

## Technologies

- **Language:** C++20
- **Graphics, Windowing & Audio:** [SFML 2.6+](https://www.sfml-dev.org/)
- **Scripting Engine:** [Lua 5.4](https://www.lua.org/) & [sol2](https://github.com/ThePhD/sol2)
- **JSON Parser & Serializer:** [nlohmann_json 3.11.3](https://github.com/nlohmann/json)
- **Build System:** CMake 3.16+ (automated dependency management via `FetchContent`)
- **Platform:** Windows (primary), Linux

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

### Standalone Export (`RayneGame`)

The project includes a standalone target called `RayneGame` that compiles without the editor tools and overhead. It uses the `project_settings.json` configured via the **Project Hub** to boot directly into your game, making it ready for distribution.

### Execution

Run the binary from the root project directory so that the relative `assets/` path resolves correctly:

```bash
# Windows
.\build\Release\RayneEngine.exe
.\build\Release\RayneGame.exe  # To test the standalone game

# Linux
./build/RayneEngine
./build/RayneGame
```
