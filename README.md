# RayneEngine

<p align="center">
  <img width="1024" height="1024" alt="rayne_icon" src="https://github.com/user-attachments/assets/528b554f-836c-4fa4-918d-ef9f790e1cac" />
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
- **2D Rigidbody Physics System** with impulse-based collision response, friction, triggers, and raycasting
- **Template (Prefab) System** for saving reusable entity blueprints and instantiating them dynamically
- **Parent-Child Entity Hierarchy** with local and world transform propagation and drag-and-drop tree editing
- **Project Hub** first-run setup modal for easily configuring project settings and standalone target specs
- Native **Visual Level Editor** with live property inspection, Inspector Lock, smooth scrolling, grid snapping, and undo/redo
- **Script Export Variables** supporting `Image`, `Vec2`, `Color`, `Entity`, and `Template` with live thumbnail previews and color picker
- Dedicated **UI Editor** for designing game HUDs visually on a fixed 1920×1080 canvas
- **In-Editor Console Panel** with live stdout/stderr capture, scrollable log, and command input
- Embedded **Lua 5.4 scripting environment** via `sol2` with lifecycle events and hot-reload
- Multi-channel **Audio Engine** and hardware input polling
- Complete **JSON Scene & UI Serialization** and project auto-saving
- **UI Manager** for runtime Text, Panel, and Button elements controlled from Lua
- **Standalone Game Export** creating an optimized `RayneGame` executable using your project settings

---

## Recent Changelog

### Added
- **Add Object Spotlight Palette & Interactive Placement System (`Ctrl + Space`):**
  - Replaced the simple Add dropdown with an intelligent, keyboard-navigable **Spotlight Palette modal**:
    - **Global Shortcut:** Press `Ctrl + Space` or click `+ Add` in the top toolbar to open the palette instantly.
    - **Live Real-time Search:** Filter across object names, descriptions, and categories dynamically with instant keystroke matching.
    - **Category Filters:** Quick category chips (`All`, `Primitives`, `Gameplay`, `Physics`, `Media & FX`) with keyboard navigation (`Left`, `Right`, `Tab`, `Up`, `Down`, `Enter`, `Esc`).
    - **Interactive Placement Mode:** Selecting any preset puts the editor into placement mode with a cursor-following ghost preview showing exact dimensions, shape, colors, and contextual labels (`[TRIGGER]`, `World Text`, `SPAWN`, camera frustum). Left-click to place anywhere on the canvas with grid snapping.
  - **15 Built-in Object Presets Across 4 Categories:**
    - **Primitives:** `Rectangle`, `Circle`, `Triangle`, `Pentagon`, `Hexagon`.
    - **Gameplay:** `Empty Entity` (transform node for parenting & hierarchy pivots), `Spawn Point` (actor spawn point with animated beacon gizmo), `Trigger Zone` (sensor collider with `isTrigger=true`), `Camera` (in-game camera with live viewfinder frustum).
    - **Physics:** `Physics Box` (dynamic Box2D entity with collider and Rigidbody2D), `Physics Ball` (dynamic bouncy ball with Rigidbody2D), `Static Platform` (solid static ground/barrier).
    - **Media & FX:** `Sprite` (entity ready for texture drag-and-drop), `World Text` (formatted 2D world text with custom font/size/color), `Audio Source` (spatial/ambient sound emitter), `Particle Emitter` (live real-time 2D particle simulation).
  - **New Core ECS Components:**
    - `TextComponent`: Render formatted text in 2D world space with customizable string, character size, color, alignment, and outlines.
    - `AudioSourceComponent`: Play sound clips with volume (0-100), pitch, looping, play-on-start, and spatial attenuation settings. Drag-and-drop `.wav` / `.ogg` files directly from the Content Browser onto objects or the canvas.
    - `ParticleEmitterComponent`: Full real-time 2D particle simulation engine running both in play mode and live in the editor. Configurable emission rate, particle lifetime, speed, variance, cone spread angle, start/end size interpolation, start/end RGBA color interpolation, and gravity vectors.
  - **Inspector UI & Serialization:**
    - Dedicated Inspector controls for `TextComponent`, `AudioSourceComponent`, and `ParticleEmitterComponent` including interactive color pickers via native dialog, numeric steppers, toggles, and test playback button.
    - Full JSON serialization and deserialization in scene files and template prefabs.
  - **Lua Scripting Bindings:**
    - `AddText`, `SetText`, `GetText`, `SetTextSize`, `SetTextColor`, `HasText`.
    - `AddAudioSource`, `PlayAudio`, `SetAudioClip`, `SetAudioVolume`, `SetAudioPitch`, `SetAudioLoop`, `HasAudioSource`.
    - `AddParticleEmitter`, `SetParticleEmitting`, `IsParticleEmitting`, `SetParticleRate`, `SetParticleSpeed`, `HasParticleEmitter`.

- **In-Game Camera System & Scripting Library (`Camera`):**
  - Added full-featured `CameraManager` handling dynamic viewport transforms, zoom, rotation, screen shake, multi-entity following, dynamic bounding framing, and boundary constraints during play mode.
  - Added `Camera` entity to the editor **Add** dropdown menu (`ObjectType::Camera`):
    - Placed as a dedicated camera entity with a custom camera gizmo icon and recording LED.
    - Freely movable, resizable, and rotatable in the editor and scriptable at runtime.
    - Full serialization to/from JSON scene files and undo/redo history.
  - **Live Camera Viewport Frustum / Viewfinder Visualizer:**
    - Live viewport representation in `EditorScene` showing the exact screen frame (`1920×1080` base resolution scaled by zoom, rotated by camera angle, and offset).
    - Center crosshair and informative overlay badge displaying current zoom and dimensions.
  - **Multi-Camera & Multi-Target Follow Resolution System:**
    - Devised and implemented conflict-free multi-target tracking when two or more objects have camera follow active (`CameraMultiFollowMode`):
      - `CameraMultiFollowMode.Priority` (0): The camera target with the highest priority value takes control; lower-priority cameras are ignored.
      - `CameraMultiFollowMode.Average` (1): Midpoint follow that smoothly averages the world positions of all active follow targets.
      - `CameraMultiFollowMode.AutoFrame` (2): Intelligent bounding-box framing that automatically tracks the centroid of all targets and dynamically zooms the camera in/out to keep all targets comfortably on-screen with configurable world padding (`autoFramePadding`) and zoom clamps (`minZoom`, `maxZoom`).
    - Multi-camera visualizer in `EditorScene`: draws connecting dashed link lines between all active camera targets and renders the shared focus / auto-frame centroid indicator in real time.
    - Upgraded `CameraComponent` with `priority`, `multiFollowMode`, `minZoom`, `maxZoom`, and `autoFramePadding`.
    - Added Inspector UI controls for `CameraComponent`: active toggle, multi-target mode selector cycle button, priority, smooth speed, offsets, zoom, padding, and zoom limits.
  - Added global `Camera` Lua library table for complete runtime control:
    - **Position & Free Movement:** `Camera.SetPosition(x, y)`, `Camera.GetPosition()`, `Camera.GetX()`, `Camera.GetY()`, `Camera.Move(dx, dy)`.
    - **Zoom:** `Camera.SetZoom(zoom)`, `Camera.GetZoom()`, `Camera.Zoom(factor)`.
    - **Rotation:** `Camera.SetRotation(deg)`, `Camera.GetRotation()`, `Camera.Rotate(deltaDeg)`.
    - **View Size & Reset:** `Camera.SetSize(w, h)`, `Camera.GetSize()`, `Camera.Reset()`.
    - **Target Following:** `Camera.Follow(entity, [smoothSpeed], [offsetX], [offsetY])`, `Camera.StopFollow()`, `Camera.ResumeFollow()`, `Camera.IsFollowing()`, `Camera.GetFollowTarget()`, `Camera.SetFollowSpeed(speed)`, `Camera.GetFollowSpeed()`, `Camera.SetFollowOffset(ox, oy)`, `Camera.GetFollowOffset()`.
    - **Multi-Target Tracking:** `Camera.AddFollowTarget(entity)`, `Camera.RemoveFollowTarget(entity)`, `Camera.ClearFollowTargets()`, `Camera.FollowGroup(targets, [mode], [padding])`, `Camera.GetFollowTargets()`, `Camera.GetFollowTargetCount()`, `Camera.SetMultiFollowMode(mode)`, `Camera.GetMultiFollowMode()`, `Camera.SetAutoFramePadding(padding)`, `Camera.GetAutoFramePadding()`, `Camera.SetAutoFrameZoomLimits(minZoom, maxZoom)`, `Camera.GetAutoFrameZoomLimits()`, `Camera.SetPrimary(entity)`, `Camera.GetPrimary()`.
    - **Boundary Constraints:** `Camera.SetBounds(minX, minY, maxX, maxY, [clampEdges=true])`, `Camera.ClearBounds()`, `Camera.HasBounds()`, `Camera.GetBounds()`.
    - **Screen Shake:** `Camera.Shake(intensity, duration, [decay=true])`, `Camera.StopShake()`, `Camera.IsShaking()`.
    - **Coordinate Mapping:** `Camera.ScreenToWorld(screenX, screenY)`, `Camera.WorldToScreen(worldX, worldY)` converting seamlessly with view transformations.
  - Upgraded `AddCamera(e, [smoothSpeed], [offsetX], [offsetY], [zoom], [priority])` and added `GetCamera(e) -> CameraComponent`.
  - Added comprehensive test script: `assets/scripting/CameraTestScript.lua`.
- **Event-Driven Input System (`OnInputReceived` / `OnInputReceiced`):**
  - Added dedicated script callback `OnInputReceived(self, event)` (with `OnInputReceiced` alias) that triggers exclusively when hardware input arrives (keyboard, mouse, joystick, text) rather than polling every frame.
  - Added typed enum table `InputEvent` (with aliases `InputEventType`, `EventType`) supporting enum comparisons like `if event.type == InputEvent.KeyDown` or `if event.type == InputEvent.Keydown`.
  - Added `InputEventData` table fields: `type`, `typeName`, `typeStr`, `key`, `keyCode`, `keyName`, `button`, `buttonCode`, `buttonName`, `x`, `y`, `worldX` (camera-mapped), `worldY` (camera-mapped), `delta`, `wheel`, `text`, `unicode`, `alt`, `control`, `shift`, `system`, `joystickId`, `axis`, `position`, `isInput`, and `isPaused`.
  - Added input helper functions: `Input.HasInput()`, `Input.GetLastEvent()`, `Input.KeyToString(key)`, and `Input.MouseButtonToString(button)`.
  - Added Inspector SCRIPT section visual indicator for `OnInputReceived` callback hook.
  - Added test scripts: `assets/scripting/InputTestScript.lua` and `assets/scripting/InteractiveInputDemo.lua`.
- **2D Rigidbody Physics System & Impulse Solver:**
  - Added `Rigidbody2DComponent` with configurable `bodyType` (`Dynamic`, `Kinematic`, `Static`), `mass`, `gravityScale`, `restitution` (bounciness), `drag` (linear damping), and `freezeRotation`.
  - Added fixed-timestep physics simulation loop with sub-stepping (`PhysicsSystem`).
  - Added impulse-based physical collision resolution with coefficient of restitution and tangential surface friction.
  - Added trigger / sensor collider support (`isTrigger`) with zero-impulse passthrough and event dispatch.
  - Added physics lifecycle callbacks in Lua: `OnCollisionEnter(self, other, normalX, normalY)` and `OnTriggerEnter(self, other)`.
  - Added full Lua `Physics` library: `Physics.Raycast`, `Physics.ApplyForce`, `Physics.ApplyImpulse`, `Physics.SetVelocity`, `Physics.GetVelocity`, `Physics.SetGravity`, `Physics.GetGravity`, `Physics.SetFixedTimestep`, and `Physics.GetFixedTimestep`.
  - Added global entity Rigidbody Lua helpers: `AddRigidbody(entity, bodyType, mass, gravityScale)`, `GetRigidbody(entity)`, and `HasRigidbody(entity)`.
- **Velocity Component Overhaul & Editor Inspector Integration:**
  - Added full interactive Inspector UI for `VelocityComponent` with editable `Velocity X` and `Velocity Y` numeric fields.
  - Added convenient property aliases on Lua `Velocity` usertype: `.dx`, `.dy`, `.vx`, `.vy`, `.x`, and `.y`.
  - Added `RemoveVelocity(entity)` Lua binding.
  - Improved `SetVelocity(e, dx, dy)` in Lua to automatically attach `VelocityComponent` if not yet present.
  - Improved `AddVelocity(e, dx, dy)` to return a reference to the component.
  - Fixed component pool update semantics so adding or setting components on entities that already possess them updates their values in place instead of discarding changes.
  - Ensured automatic attachment of `VelocityComponent` whenever a `Rigidbody2D` is created (via Inspector, templates, scenes, or Lua `AddRigidbody`) to guarantee physics simulation reliability.
  - Added post-collision world transform synchronization so physics position corrections are immediately reflected in rendered positions.
- **Inspector Lock Feature:**
  - Added Unity-style lock toggle button in the top-right corner of the Inspector panel.
  - Freezes the active inspector view on the selected object so clicking other entities in the viewport or hierarchy does not deselect or alter the inspector view.
- **Script Export Variables Expansion & Visual Previews:**
  - Added first-class export variable constructors in Lua: `Image(path)`, `Vec2(x, y)`, `Color(r, g, b)`, `Entity(name)`, and `Template(path)`.
  - Added live thumbnail rendering directly inside the Inspector for `Image` texture assets and `Template` prefabs.
  - Added native Windows color picker integration (`ChooseColor`) for `Color` export properties.
  - Added drag-and-drop targeting: drag images and templates from the Content Browser, or entities from the Hierarchy tree, directly into matching export property slots.
  - Added complete JSON serialization and deserialization for all script export property types.
- **Inspector Scrolling & Content Clipping:**
  - Added smooth mouse-wheel scrolling for the Inspector panel when content exceeds viewport height.
  - Added viewport scissor clipping below the fixed title header to keep content cleanly contained within the panel.
- **Template (Prefab) System:**
  - Added support for saving entity hierarchies as reusable `.template` asset files.
  - Added template instantiation from Lua via `Instantiate(template, x, y, [parent])` and `Template(path):Instantiate(...)`.
  - Added circular reference detection and nested prefab dependency warnings.
- **Parent-Child Hierarchy System:**
  - Added complete transform parenting system (`HierarchySystem`) supporting local and world coordinate spaces.
  - Added Lua hierarchy APIs: `SetParent(child, parent, [keepWorldTransform])`, `GetParent(child)`, `GetChildren(parent)`, `GetWorldPosition(e)`, `GetWorldRotation(e)`, and `GetWorldScale(e)`.
  - Added drag-and-drop reparenting inside the Hierarchy tree with visual feedback.
  - Added unsaved changes dirty indicator (`*`) across scenes, UI layouts, and scripts.
- **UI Editor Enhancements & Layout Containers:**
  - Added `VerticalBox` and `HorizontalBox` layout elements with automatic element spacing and alignment.
  - Added progress bar elements with `SetProgressValue` / `GetProgressValue`.
  - Added slider min/max boundary configuration (`SetSliderMin`, `SetSliderMax`, `GetSliderMin`, `GetSliderMax`).
  - Added vertical text alignment (`SetTextVAlign`) and auto-wrapping labels in the UI inspector.
  - Added UI element focus management (`SetFocused`, `GetFocused`, `OnUIFocus`).
  - Added script method stub insertion, asset synchronization, and IDE workflow directly from the UI Editor.
- **Editor Tooling & Visual Feedback:**
  - Added viewport world coordinate axes and origin marker (0, 0).
  - Added automatic scene saving on Play Mode launch (`F5`).
  - Added hot-reloading and inspector property refreshing for exported script variables.
  - Added on-screen notification toasts via `Engine.LogToScreen` and `LogToScreen` during play mode.
  - Added color-coded console badges for `Engine.Log` (teal), `Engine.LogWarning` (amber), and `Engine.LogError` (red).
  - Added `.jfif` image format loading support.
  - Added read-only file attributes for engine content and template files in standalone builds.

### Fixed
- Fixed input event dispatch ordering in `Application::SetEvents` so `InputManager` processes events before scene event handling, ensuring `Input.HasInput()` and queries are immediately up-to-date during script callbacks.
- Fixed editor crash when opening the UI Editor for the first time in standalone builds.
- Fixed scene save issue where objects could occasionally reset to (0, 0) by committing active input fields and ensuring local transforms are preserved.
- Fixed `VelocityComponent` being non-editable in the Inspector and failing to update or simulate when attached to entities.
- Fixed text underline disappearing when bold style is active in UIManager text rendering.
- Fixed object rotation handle positioning and smooth angular dragging in the level editor viewport.
- Fixed hit detection and selection on scaled entities to prevent miss-clicks and inaccurate marquee selection.
- Fixed non-uniform edge scaling and ellipse stretching for circle primitives across both editor and runtime.
- Fixed IDE launch command to run detached without spawning extraneous console windows.
- Fixed UI button click handling in standalone builds and ensured all audio channels stop when exiting play mode.
- Fixed startup freeze/hang in `EditorScene` and revealed hidden scripting directories in build mode.
- Fixed template asset path resolution to reliably locate project root using `FindProjectRoot()`.

### Removed
- Removed requirement for magic string comparisons for input event types in Lua scripts in favor of `InputEvent` enum constants.
- Removed legacy non-Lua comment cruft from `EditorScene`, `ContentBrowser`, `PhysicsSystem`, `SplashScreen`, and scripting manager classes.
- Removed redundant `SetPathReadOnly` calls from `EditorScene` constructor.

---

## Architecture

The engine architecture is structured into decoupled modules under `src/Core/`:

```
RayneEngine
|-- Core/
|   |-- Application/     Main loop, DPI awareness, window management, engine versioning
|   |-- Audio/           AudioManager sound effect pool (32 channels) and music streaming
|   |-- ECS/             Registry, component pools, view iterators, entity handles,
|   |                    PhysicsSystem (Rigidbody2D solver), HierarchySystem (parenting)
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

- <img width="542" height="467" alt="image" src="https://github.com/user-attachments/assets/ea89523d-e5bf-4653-b7b2-d0dbef35a27b" />

These settings are serialized to `project_settings.json` which governs the standalone game's runtime behavior without interfering with the editor's fixed resolution.

<img width="1920" height="999" alt="StartupScreenRE (1)" src="https://github.com/user-attachments/assets/8e7e0c96-a6a6-4168-858f-dd590b5f1f5e" />

---

### Visual Level Editor (`EditorScene`)

The built-in level editor provides a real-time environment for constructing and previewing 2D game scenes:

- **Entity Hierarchy & Parenting:**
  - Scrollable hierarchy tree displaying all active scene entities.
  - **Transform Parenting:** Drag-and-drop entities onto other objects to establish parent-child relationships with automatic local/world coordinate propagation.
  - Context menu actions: Rename, duplicate, delete, and save entity subtrees as `.template` assets.
  - Multi-selection and group hierarchy operations.
 
<img width="238" height="203" alt="image" src="https://github.com/user-attachments/assets/672faf1c-f303-4a71-99f6-0c81ecd6a1e9" />

- **Property Inspector:**
  - **Inspector Lock:** Unity-style lock button in the header freezes inspection on the current entity, preventing accidental selection changes when clicking in the viewport or hierarchy.
  - **Smooth Scrolling & Viewport Scissor:** Mouse-wheel scrolling with view clipping ensures large component lists and extensive export variables remain completely accessible.
  - **Rigidbody 2D Section:** Real-time configuration of `BodyType` (Dynamic / Kinematic / Static), `Mass`, `Gravity Scale`, `Restitution` (bounciness), `Drag`, and `Freeze Rotation`.
  - **Script Export Variables:** Auto-generates UI fields for all exported script properties (`Int`, `Float`, `Bool`, `String`, `Image`, `Vec2`, `Color`, `Entity`, `Template`).
  - **Visual Asset Previews:** Real-time thumbnail rendering for `Image` texture assets and `Template` prefab files.
  - **Windows Color Picker:** Interactive color swatches that launch the native Windows `ChooseColor` dialog.
  - **Drag-and-Drop Slots:** Drop textures and templates from the Content Browser, or entities from the Hierarchy tree, directly into matching property slots.
  - Live numerical manipulation of position (`x`, `y`), size (`width`, `height`), rotation (degrees), and scale (`scaleX`, `scaleY`).
  - Entity Tag and Name assignment for easy query from Lua scripts.
  - Color palette tinting (`R`, `G`, `B`) for primitives and sprites.
  - Script path assignment with automatic Lua binding and hot-reload reflection.
  - Sprite asset assignment with aspect-correct scaling.
  - Collision channel, collision type (`Static` / `Solid`), collider shape (`Box` / `Circle`), and sensor trigger (`isTrigger`) configuration.
  - UI element text editing directly from the inspector (`UIText` field).
 
<img width="268" height="854" alt="image" src="https://github.com/user-attachments/assets/6b0dfa2e-04aa-4642-afe5-e0e79697f9d7" />

- **Content Browser:**
  - Integrated file browser with breadcrumb navigation and path history.
  - Asset category filters: All, Images, Scripts, Audio, Scenes.
  - Live search bar with instant filtering across all entries.
  - File management: Create scripts, scenes, or folders; rename, duplicate, or delete assets; copy asset path to clipboard; reveal file in OS file explorer.
  - Drag-and-drop: Drag textures or scripts from the browser directly onto viewport entities.
  - Scene loading: Double-click or trigger scene loading requests directly from the browser.
 
<img width="1410" height="156" alt="image" src="https://github.com/user-attachments/assets/16534f79-c2c0-476e-803a-e555d4ab1c9a" />

- **In-Editor Console Panel:**
  - Switchable bottom panel (Content Browser ↔ Console via tab bar).
  - Captures all `std::cout` and `std::cerr` output in real time via stream redirectors installed at engine boot.
  - Color-coded log lines: normal output in light grey, errors in red.
  - Scrollable log area with draggable scrollbar and mouse-wheel support; auto-scrolls to the newest message.
  - Command input field with blinking cursor, Ctrl+V paste support, and Enter-to-submit.
  - Built-in `clear` command to reset the log. Buffer capped at 2 000 messages.
 
<img width="1410" height="150" alt="image" src="https://github.com/user-attachments/assets/57ff8e46-a02c-498c-a938-61e5e90226e4" />

- **Interactive Viewport & Selection:**
  - Free camera panning using Middle Mouse Button (invertible and sensitivity-configurable).
  - Smooth camera zooming with user-defined min/max limits.
  - **Multi-Selection & Box-Select:** Drag in empty space to create a selection box (marquee select); hold `Shift` or `Ctrl` to toggle entities in the selection group.
  - **Group Manipulation:** Move or delete multiple selected entities simultaneously.
  - **Undo / Redo (Command Pattern):** Full history stack supporting `Ctrl + Z` and `Ctrl + Y` for entity creation, deletion, dragging, and resizing — including atomic `MacroCommand` grouping for multi-object operations.
  - 8-point interactive resize handles for live scaling of selected objects.
  - Grid rendering with configurable cell dimensions, color, and opacity.
  - Toggleable snap-to-grid alignment.

<img width="716" height="370" alt="image" src="https://github.com/user-attachments/assets/f9805d88-a114-471d-80cb-fb3bcd87d033" />

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

<img width="1918" height="1010" alt="image" src="https://github.com/user-attachments/assets/4df3464c-f75b-4e60-9eb1-230ffbbc3036" />

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
  - `TransformComponent`: 2D spatial position (`float x`, `float y`), rotation angle in degrees (`float rotation`), and scaling (`float scaleX`, `float scaleY`). Supports parent-child hierarchies with world coordinates.
  - `VelocityComponent`: Movement velocity delta (`float dx`, `float dy`).
  - `Rigidbody2DComponent`: Physics body attributes (`BodyType` dynamic/kinematic/static, `mass`, `gravityScale`, `restitution`, `drag`, `freezeRotation`).
  - `RenderComponent`: Visual representation (`sf::Color`, `sf::Vector2f size`, and `ShapeType`: Rectangle, Circle, Triangle, Pentagon, Hexagon).
  - `SpriteComponent`: Renderable SFML sprite with texture handle and dimensions; auto-loads via `ResourceManager` and computes scale on construction.
  - `CameraComponent`: Marks an entity as a camera focus (`bool active`, `float smoothSpeed`, `float offsetX`, `float offsetY`, `float zoom`, `int priority`, `CameraMultiFollowMode multiFollowMode`, `float minZoom`, `float maxZoom`, `float autoFramePadding`).
  - `CollisionComponent`: Configures collision filtering via integer `channel`, collision type (`Static`, `Solid`), collider shape (`Box`, `Circle`), and trigger flag (`isTrigger`).
  - `ScriptComponent`: Encapsulates a sol2 Lua environment, filesystem modification timestamp tracking for live hot-reloading, exported variables, and lifecycle hooks.

---

### Physics & Event Pipeline

RayneEngine features a unified **2D Rigidbody Physics System** alongside legacy simple AABB checks:

- **Physics Simulation (`PhysicsSystem`):**
  - **Fixed Timestep Loop:** Accumulator-based fixed-timestep integration with sub-stepping for deterministic physical behavior (`Physics.SetFixedTimestep(dt)`).
  - **Gravity Acceleration:** Configurable global gravity vector (`Physics.SetGravity(gx, gy)`). Default: `(0, 980)`.
  - **Linear Velocity Damping:** Applies aerodynamic/frictional drag per body (`drag`).
  - **Body Types:**
    - `BodyType::Dynamic` — Full physics simulation responding to gravity, forces, impulses, and contact collisions.
    - `BodyType::Kinematic` — Controlled programmatically via velocity; pushes dynamic objects without being displaced by them.
    - `BodyType::Static` — Immovable obstacles (walls, terrain, floors); infinite mass in collision responses.
  - **Rotation Lock:** Entities can toggle `freezeRotation` to prevent angular tumble on impact.
- **Impulse Solver & Friction:**
  - Physical collision resolution computes instantaneous contact normals and applies restitution-weighted impulses `j = -(1 + e) * v_rel / (invMassA + invMassB)`.
  - Realistic tangential friction damping (`frictionMu = 0.2`) stops sliding objects naturally.
- **Sensor / Trigger Colliders:**
  - Any collider flagged with `isTrigger = true` bypasses impulse resolution entirely and acts as a sensor zone, firing `OnTriggerEnter(self, other)`.
- **Collision Callbacks:**
  - `OnCollisionEnter(self, other, normalX, normalY)` dispatches once upon solid contact, supplying the collision normal.
  - `OnTriggerEnter(self, other)` dispatches when entering a sensor zone.
  - `OnCollision(self, other)` legacy fallback event dispatched on contact.
- **Raycasting:**
  - `Physics.Raycast(startX, startY, dirX, dirY, distance, [channel])` performs ray intersection against active scene colliders, returning hit status, hit entity, contact point, normal, and distance.

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

function OnInputReceived(self, event)
    -- Invoked strictly when hardware input arrives (keyboard, mouse, joystick, text)
    -- (OnInputReceiced is also accepted as an alias)
    if event.type == InputEvent.KeyDown and event.key == Key.Space then
        print("Space key pressed!")
    end
end

function OnCollisionEnter(self, other, normalX, normalY)
    -- Invoked upon physical contact with another entity, providing the collision normal
end

function OnTriggerEnter(self, other)
    -- Invoked when entering a trigger/sensor collider zone
end

function OnCollision(self, other)
    -- Legacy fallback invoked upon collision contact
end

function OnDestroy(self)
    -- Invoked when the entity or scene is destroyed
end
```

---

### Script Export Variables

RayneEngine allows scripts to expose typed variables to the Editor Inspector using an `Export` table and built-in type constructors:

```lua
-- ExportDemoScript.lua
characterTexture = Image("assets/sprites/character.png") -- Texture asset path (thumbnail preview)
spawnOffset      = Vec2(50.0, -20.0)                     -- 2D Vector (X, Y fields)
tintColor        = Color(255, 128, 64)                   -- RGB Color (Windows color picker)
targetObject     = Entity("Player")                      -- Entity reference (drag from hierarchy)
spawnPrefab      = Template("assets/bullet.template")    -- Prefab path (thumbnail preview)
moveSpeed        = 120.0                                 -- Float
maxHealth        = 100                                   -- Integer
enableGlow       = true                                  -- Boolean checkbox
greetingMessage  = "Hello RayneEngine!"                  -- String

Export = {
    "characterTexture",
    "spawnOffset",
    "tintColor",
    "targetObject",
    "spawnPrefab",
    "moveSpeed",
    "maxHealth",
    "enableGlow",
    "greetingMessage"
}
```

- **Visual Previews:** The Inspector displays real-time thumbnail previews for `Image` assets and `Template` prefabs.
- **Color Picker:** Clicking the color swatch opens the native Windows color selection dialog (`ChooseColor`).
- **Drag & Drop:** Drag files directly from the Content Browser onto Image/Template slots, or drag objects from the Hierarchy tree onto Entity slots.

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
| `AddVelocity` | `(e: Entity, dx: number, dy: number) -> Velocity` | Attaches or updates a `VelocityComponent` |
| `GetVelocity` | `(e: Entity) -> Velocity` | Returns a mutable reference to `{ dx, dy, vx, vy, x, y }` |
| `SetVelocity` | `(e: Entity, dx: number, dy: number)` | Sets velocity vector directly (attaches if not present) |
| `HasVelocity` | `(e: Entity) -> boolean` | Checks if entity has velocity |
| `RemoveVelocity` | `(e: Entity)` | Removes `VelocityComponent` from entity |
| `AddSprite` | `(e: Entity, path: string, w: number, h: number)` | Attaches a `SpriteComponent` |
| `SetSprite` | `(e: Entity, path: string)` | Updates or swaps the sprite texture |
| `SetSpriteSize` | `(e: Entity, w: number, h: number)` | Updates rendered dimensions of sprite |
| `HasSprite` | `(e: Entity) -> boolean` | Checks if entity has a sprite |
| `SetColor` | `(e: Entity, r: number, g: number, b: number, [a]: number)` | Sets color of `RenderComponent` (0–255) |
| `AddCamera` | `(e: Entity, [smoothSpeed=0.0], [ox=0.0], [oy=0.0], [zoom=1.0], [priority=0]) -> CameraComponent` | Attaches or updates camera tracking component |
| `GetCamera` | `(e: Entity) -> CameraComponent | nil` | Reads camera component reference |
| `RemoveCamera` | `(e: Entity)` | Removes camera component |
| `HasCamera` | `(e: Entity) -> boolean` | Checks if entity has camera tracking |
| `AddCollision` | `(e: Entity, [channel]: integer)` | Attaches a `CollisionComponent` (default channel `0`, type `Static`) |
| `RemoveCollision` | `(e: Entity)` | Removes collision component |
| `HasCollision` | `(e: Entity) -> boolean` | Checks if entity has collision enabled |
| `SetCollisionChannel` | `(e: Entity, channel: integer)` | Sets collision filter channel |
| `GetCollisionChannel` | `(e: Entity) -> integer` | Reads collision filter channel |
| `SetCollisionType` | `(e: Entity, type: string)` | Sets collision type: `"static"` or `"solid"` |
| `GetCollisionType` | `(e: Entity) -> string` | Returns current collision type as string |
| `SetParent` | `(child: Entity, parent: Entity, [keepWorldTransform=true]: boolean)` | Sets entity parent; `0` unparents |
| `GetParent` | `(child: Entity) -> Entity` | Returns parent entity ID, or `0` |
| `GetChildren` | `(parent: Entity) -> Entity[]` | Returns array of child entity IDs |
| `GetWorldPosition` | `(e: Entity) -> number, number` | Computes global world coordinates `x, y` |
| `GetWorldRotation` | `(e: Entity) -> number` | Computes global world rotation in degrees |
| `GetWorldScale` | `(e: Entity) -> number, number` | Computes global world scale factors `sx, sy` |
| `Template` | `(path: string) -> Template` | Creates template reference object |
| `Instantiate` | `(template: Template | string, x: number, y: number, [parent]: Entity) -> Entity` | Instantiates template prefab into scene |
| `LoadScene` | `(sceneName: string)` | Switches active scene to `assets/scenes/<sceneName>.json` |

> **Tip:** `GetTransform(e)` returns a mutable table — you can read and write `t.x`, `t.y`, `t.rotation`, `t.scaleX`, `t.scaleY` directly on the returned reference.

---

### In-Game Camera Library (`Camera`)

| Function | Signature | Description |
|---|---|---|
| `Camera.SetPosition` | `(x: number, y: number)` | Sets absolute world center of the camera |
| `Camera.GetPosition` | `() -> number, number` | Returns world center `x, y` of the camera |
| `Camera.GetX` | `() -> number` | Returns camera center world X |
| `Camera.GetY` | `() -> number` | Returns camera center world Y |
| `Camera.Move` | `(dx: number, dy: number)` | Translates camera position by relative world offsets |
| `Camera.SetZoom` | `(zoom: number)` | Sets camera zoom factor (`1.0` = normal, `>1` = in, `<1` = out) |
| `Camera.GetZoom` | `() -> number` | Returns current camera zoom factor |
| `Camera.Zoom` | `(factor: number)` | Multiplies camera zoom by a multiplier |
| `Camera.SetRotation` | `(deg: number)` | Sets camera rotation angle in degrees |
| `Camera.GetRotation` | `() -> number` | Returns camera rotation angle in degrees |
| `Camera.Rotate` | `(deltaDeg: number)` | Rotates camera by delta angle in degrees |
| `Camera.SetSize` | `(w: number, h: number)` | Sets base viewport dimensions |
| `Camera.GetSize` | `() -> number, number` | Returns base viewport dimensions `w, h` |
| `Camera.Reset` | `()` | Resets camera to default position, zoom (`1.0`), rotation (`0.0`), shake, and clears follow targets |
| `Camera.Follow` | `(e: Entity, [smoothSpeed=0.0], [ox=0.0], [oy=0.0])` | Follows entity target with optional smooth damping and offset |
| `Camera.StopFollow` | `()` | Disables following any entity target |
| `Camera.ResumeFollow` | `()` | Resumes following the active entity target |
| `Camera.IsFollowing` | `() -> boolean` | Returns true if the camera is actively tracking an entity |
| `Camera.GetFollowTarget` | `() -> Entity` | Returns tracked entity ID (or `0` if none) |
| `Camera.SetFollowSpeed` | `(speed: number)` | Sets follow smooth damping speed (`0` for instant snapping) |
| `Camera.GetFollowSpeed` | `() -> number` | Returns current follow damping speed |
| `Camera.SetFollowOffset` | `(ox: number, oy: number)` | Sets camera tracking offset in world units |
| `Camera.GetFollowOffset` | `() -> number, number` | Returns camera tracking offset `ox, oy` |
| `Camera.AddFollowTarget` | `(e: Entity)` | Adds an entity to the multi-target tracking group |
| `Camera.RemoveFollowTarget` | `(e: Entity)` | Removes an entity from multi-target tracking |
| `Camera.ClearFollowTargets` | `()` | Clears all tracked entities in the follow group |
| `Camera.FollowGroup` | `(targets: Entity[], [mode], [padding])` | Replaces follow targets with a list and sets optional follow mode & padding |
| `Camera.GetFollowTargets` | `() -> Entity[]` | Returns an array of all currently tracked entity IDs |
| `Camera.GetFollowTargetCount` | `() -> integer` | Returns the number of currently tracked follow targets |
| `Camera.SetMultiFollowMode` | `(mode: CameraMultiFollowMode \| integer \| string)` | Sets multi-follow mode: `Priority` (0), `Average` (1), `AutoFrame` (2) |
| `Camera.GetMultiFollowMode` | `() -> integer` | Returns the active `CameraMultiFollowMode` integer value |
| `Camera.SetAutoFramePadding` | `(padding: number)` | Sets world margin/padding added around tracked targets in AutoFrame mode |
| `Camera.GetAutoFramePadding` | `() -> number` | Returns the active AutoFrame world margin padding |
| `Camera.SetAutoFrameZoomLimits` | `(minZoom: number, maxZoom: number)` | Sets min and max zoom clamp boundaries for AutoFrame mode |
| `Camera.GetAutoFrameZoomLimits` | `() -> minZoom, maxZoom` | Returns the min and max zoom boundaries |
| `Camera.SetPrimary` | `(e: Entity)` | Sets the primary active camera entity |
| `Camera.GetPrimary` | `() -> Entity` | Returns the primary active camera entity ID |
| `Camera.SetBounds` | `(minX, minY, maxX, maxY, [clampEdges=true])` | Restricts camera movement within specified world bounding box |
| `Camera.ClearBounds` | `()` | Clears world boundary constraints |
| `Camera.HasBounds` | `() -> boolean` | Checks if world boundary constraints are active |
| `Camera.GetBounds` | `() -> minX, minY, maxX, maxY` | Returns active camera boundary box coordinates |
| `Camera.Shake` | `(intensity: number, duration: number, [decay=true])` | Triggers screenshake with optional decay over duration |
| `Camera.StopShake` | `()` | Immediately stops active screenshake |
| `Camera.IsShaking` | `() -> boolean` | Returns true if screenshake is currently active |
| `Camera.ScreenToWorld` | `(sx: number, sy: number) -> number, number` | Maps screen pixel coordinates to world coordinates |
| `Camera.WorldToScreen` | `(wx: number, wy: number) -> number, number` | Maps world coordinates to screen pixel coordinates |

#### `CameraMultiFollowMode` Enum Table

| Constant | Value | Description |
|---|---|---|
| `CameraMultiFollowMode.Priority` | `0` | Tracks solely the highest-priority entity (`priority` field in `CameraComponent`) |
| `CameraMultiFollowMode.Average` | `1` | Midpoint tracking: computes the average centroid position of all active entities |
| `CameraMultiFollowMode.AutoFrame` | `2` | Intelligent dynamic framing: centers on target centroid and adjusts zoom to frame all targets with padding |

---

### 2D Rigidbody Physics Library (`Physics`)

| Function | Signature | Description |
|---|---|---|
| `AddRigidbody` | `(e: Entity, [bodyType], [mass=1.0], [gravityScale=1.0]) -> Rigidbody2D` | Attaches a `Rigidbody2D` component |
| `GetRigidbody` | `(e: Entity) -> Rigidbody2D \| nil` | Returns the `Rigidbody2D` component or `nil` |
| `HasRigidbody` | `(e: Entity) -> boolean` | Checks if entity has a Rigidbody |
| `Physics.Raycast` | `(startX, startY, dirX, dirY, distance, [channel=-1]) -> RaycastResult` | Casts a ray and returns closest hit (`hit`, `entity`, `pointX`, `pointY`, `normalX`, `normalY`, `distance`) |
| `Physics.ApplyForce` | `(e: Entity, fx: number, fy: number)` | Applies a continuous force (accumulated for the next physics step) |
| `Physics.ApplyImpulse` | `(e: Entity, ix: number, iy: number)` | Applies an instantaneous impulse directly modifying velocity |
| `Physics.SetVelocity` | `(e: Entity, vx: number, vy: number)` | Sets linear velocity directly |
| `Physics.GetVelocity` | `(e: Entity) -> number, number` | Gets current linear velocity `vx, vy` |
| `Physics.SetGravity` | `(gx: number, gy: number)` | Sets global physics gravity vector (default: `0, 980`) |
| `Physics.GetGravity` | `() -> number, number` | Returns current global gravity vector |
| `Physics.SetFixedTimestep` | `(dt: number)` | Sets fixed simulation timestep in seconds (default: `1/60` ~ `0.01667`) |
| `Physics.GetFixedTimestep` | `() -> number` | Returns current fixed simulation timestep |

**Enums:**
- `BodyType`: `BodyType.Dynamic` (0), `BodyType.Kinematic` (1), `BodyType.Static` (2)
- `ColliderShape`: `ColliderShape.Box` (0), `ColliderShape.Circle` (1)

---

### Engine & System Library (`Engine`)

| Function | Signature | Description |
|---|---|---|
| `Engine.Log` / `print` | `(...)` | Prints highlighted message to in-editor console with teal `[LOG]` badge |
| `Engine.LogWarning` | `(msg: string)` | Prints warning message with amber `[WARN]` badge |
| `Engine.LogError` | `(msg: string)` | Prints error message with red `[ERROR]` badge |
| `Engine.LogToScreen` / `LogToScreen` | `(msg: string, [duration=3.5], [r], [g], [b])` | Displays on-screen notification toast during play mode |
| `Engine.SetPaused` / `PauseGame` | `(paused: boolean)` | Pauses or unpauses game simulation |
| `Engine.IsPaused` / `Engine.GetPaused` | `() -> boolean` | Returns true if simulation is currently paused |
| `Engine.TogglePause` | `()` | Toggles simulation pause state |
| `Engine.SetTimeScale` / `SetTimeScale` | `(scale: number)` | Sets simulation time scale factor |
| `Engine.GetTimeScale` | `() -> number` | Gets current simulation time scale factor |
| `Engine.SetFullscreen` | `(fullscreen: boolean)` | Sets window fullscreen mode |
| `Engine.ToggleFullscreen` | `()` | Toggles window fullscreen mode |
| `Engine.IsFullscreen` | `() -> boolean` | Returns true if window is currently fullscreen |
| `Engine.SetCursorVisible` | `(visible: boolean)` | Shows or hides the mouse cursor |
| `Engine.TakeScreenshot` | `([filename]: string) -> string` | Captures screenshot and saves to `screenshots/` |
| `Engine.OpenURL` | `(url: string)` | Opens URL or file path in default OS handler |
| `Engine.GetFPS` / `GetFPS` | `() -> number` | Returns current frames per second |
| `Engine.GetDeltaTime` | `() -> number` | Returns unscaled delta time of current frame |
| `Engine.ShowFPS` | `(show: boolean)` | Shows or hides on-screen FPS counter overlay |
| `Engine.IsFPSShown` | `() -> boolean` | Returns true if FPS counter is visible |
| `Engine.RestartScene` / `RestartScene` | `()` | Resets and reloads the active scene |
| `Engine.RestartCurrentScene` | `()` | Alias for `Engine.RestartScene()` |
| `Engine.LoadScene` / `LoadScene` | `(sceneName: string)` | Loads scene from `assets/scenes/<name>.json` |
| `Engine.Quit` / `QuitGame` | `()` | Exits standalone build or returns to editor |

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
| `Input.IsKeyJustPressed` | `(key: number \| string) -> boolean` | True during the frame key was pressed (alias for `IsKeyPressed`) |
| `Input.IsKeyReleased` | `(key: number \| string) -> boolean` | True during the frame key was released |
| `Input.IsMouseDown` | `(button: number \| string) -> boolean` | True while mouse button is held down |
| `Input.IsMousePressed` | `(button: number \| string) -> boolean` | True during the frame mouse button was pressed |
| `Input.IsMouseReleased` | `(button: number \| string) -> boolean` | True during the frame mouse button was released |
| `Input.MouseX` | `() -> number` | Mouse horizontal position in screen space |
| `Input.MouseY` | `() -> number` | Mouse vertical position in screen space |
| `Input.MouseScroll` | `() -> number` | Mouse wheel scroll delta for current frame |
| `Input.HasInput` | `() -> boolean` | True if any hardware input event occurred this frame |
| `Input.GetLastEvent` | `() -> InputEventData \| nil` | Returns the most recently processed input event data |
| `Input.KeyToString` | `(key: integer) -> string` | Converts a numeric key code to its human-readable name |
| `Input.MouseButtonToString` | `(button: integer) -> string` | Converts a numeric mouse button code to its name |

**Key Enums (`Key` / `KeyCode`):** `A` through `Z`, `Space`, `Enter`, `Escape`, `LShift`, `RShift`, `LCtrl`, `RCtrl`, `Left`, `Right`, `Up`, `Down`, `Tab`, `Delete`. (Both `Key.Space` and `KeyCode.Space` are supported).

**Key strings** (case-insensitive): single letters `"a"`–`"z"`, digits `"0"`–`"9"`, `"space"`, `"enter"` / `"return"`, `"escape"` / `"esc"`, `"shift"` / `"lshift"`, `"rshift"`, `"ctrl"` / `"lctrl"`, `"rctrl"`, `"alt"` / `"lalt"`, `"ralt"`, `"left"`, `"right"`, `"up"`, `"down"`, `"tab"`, `"delete"` / `"del"`, `"backspace"`.

**Mouse Enums (`Mouse`):** `Left`, `Right`, `Middle`.  
Strings `"left"` / `"0"`, `"right"` / `"1"`, `"middle"` / `"2"` are also accepted.

**Input Event Enums (`InputEvent` / `InputEventType` / `EventType`):**
`KeyDown`, `KeyUp`, `MouseDown`, `MouseUp`, `MouseMove`, `MouseWheel`, `TextEntered`, `JoystickPressed`, `JoystickReleased`, `JoystickMoved`, `Unknown` (with lowercase and legacy aliases like `Keydown`, `Mousedown`, `KeyPressed`, `MouseScroll`, etc.).

**InputEventData Fields:**
- `type` (integer): Enum code from `InputEvent` (e.g. `InputEvent.KeyDown`)
- `typeName` (string): Event name (e.g. `"KeyDown"`, `"MouseDown"`, `"MouseMove"`, `"TextEntered"`)
- `key` / `keyCode` (integer) & `keyName` (string): Key information
- `button` / `buttonCode` (integer) & `buttonName` (string): Mouse button information
- `x`, `y` (number): Screen mouse coordinates
- `worldX`, `worldY` (number): Camera-mapped world coordinates
- `delta` (number), `wheel` (string): Mouse wheel delta and orientation (`"vertical"` or `"horizontal"`)
- `text` (string), `unicode` (integer): Typed character and codepoint for `TextEntered`
- `alt`, `control`, `shift`, `system` (boolean): Modifier key states
- `joystickId`, `axis`, `position`: Gamepad / joystick values
- `isInput` (boolean): Always true
- `isPaused` (boolean): Current engine pause status

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
