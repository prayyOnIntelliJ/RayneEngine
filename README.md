# RayneEngine

<img width="1536" height="600" alt="CoverRayne" src="https://github.com/user-attachments/assets/98536ef6-3e47-4e01-a0af-c2e264530ad6" />

**RayneEngine** is a small custom 2D game engine built with [SFML](https://www.sfml-dev.org/).  
It’s a **portfolio project** created as part of my training as a **Game Engineer** — focused on learning, experimenting, and exploring engine architecture in C++.

This engine is not meant to become a large-scale framework, but rather a compact, understandable system that demonstrates how core engine components work together.

---

## Development Status
RayneEngine is in **active development**.  
It has evolved from a basic prototype into a functional 2D engine featuring a built-in level editor, a custom Entity Component System (ECS), and Lua scripting support.

Here's an example of how a Lua script can interact with the engine:

```Lua
local speed = 100
local amplitude = 100
local timer = 0
local startY = nil

function OnCreate()
    print("Lua: Started Player Script")
end

function OnUpdate(dt)
    local pos = GetTransform(self)
    timer = timer + dt

    if startY == nil then
        startY = pos.y
    end

    local newX = pos.x + speed * dt
    local newY = startY + MathR.Sin(timer * 1) * amplitude

    SetPosition(self, newX, newY)
end

function OnCollision(other)
    print("Collided with Entity ID: " .. other)
end
```

![Engine](https://github.com/user-attachments/assets/939ad1b6-3de2-44d1-97d1-38adfb8ab955)

---

## Engine Features

- **Built-in Level Editor**
  - Interactive scene editing with an Inspector, Entity Hierarchy, and Content Browser.
  - Drag-and-drop support for sprites and Lua scripts.
  - Transform tools, shape placement, color pickers, and grid snapping.
  - In-engine scene serialization (saving/loading scenes to JSON).

- **Entity Component System (ECS)**
  - Fully custom ECS architecture (`Registry`, `Pool`, `View`).
  - Supports core components: `TransformComponent`, `RenderComponent`, `SpriteComponent`, `VelocityComponent`, `CameraComponent`, and `ScriptComponent`.

- **Lua Scripting Integration**
  - Embedded Lua 5.4 using `sol2`.
  - Entities can have scripts attached directly from the editor.
  - Bindings for math, transforms, and events (`OnCreate`, `OnUpdate`, `OnCollision`).

- **Rendering & Media**
  - SFML-based 2D graphics rendering.
  - Primitive shape rendering (Rectangles, Circles, Triangles, etc.) and Texture/Sprite handling.
  - Custom camera system with entity tracking (`CameraComponent`).
  - Audio and Input management structures.

- **Physics & Events**
  - Basic AABB Collision Detection that fires engine-wide events.
  - Event Manager for broadcasting engine and gameplay events (e.g., collisions sent to Lua scripts).

- **Asset Management**
  - Centralized `ResourceManager` for caching textures, fonts, and other assets.

---

## Technologies
- **Language:** C++20
- **Framework:** [SFML 2.6+](https://www.sfml-dev.org/) (Graphics, Window, Audio, System)
- **Scripting:** Lua 5.4 + [sol2](https://github.com/ThePhD/sol2)
- **Serialization:** [nlohmann_json](https://github.com/nlohmann/json)
- **Build System:** CMake

---

## Building the Project

```bash
git clone https://github.com/prayyOnIntelliJ/RayneEngine.git
cd RayneEngine
mkdir build && cd build
cmake ..
make
```
