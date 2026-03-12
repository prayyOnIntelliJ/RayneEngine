# RayneEngine

<img width="1536" height="600" alt="CoverRayne" src="https://github.com/user-attachments/assets/98536ef6-3e47-4e01-a0af-c2e264530ad6" />

**RayneEngine** is a small custom 2D game engine built with [SFML](https://www.sfml-dev.org/).  
It’s a **portfolio project** created as part of my training as a **Game Engineer** — focused on learning, experimenting, and exploring engine architecture in C++.

This engine is not meant to become a large-scale framework, but rather a compact, understandable system that demonstrates how core engine components work together.

---

## Development Status
RayneEngine is still in **early development**.  
Most systems are experimental and will change frequently as I refine the structure and design.

The following Code...

```Lua
local speed = 100
local amplitude = 100
local timer = 0
local startY = nil

function OnCreate()
    print(&quot;Lua: Started Player Script&quot;)
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

    print(&quot;Player is now on Position: &quot; .. pos.x .. &quot;, &quot; .. pos.y)
end
```

results in the following pictures...

![Engine](https://github.com/user-attachments/assets/939ad1b6-3de2-44d1-97d1-38adfb8ab955)


NOTE: This is experimental and needs further development to be completely functional.

---

## Planned / Early Features

- **Core Engine Structure**
  - Main loop with delta time
  - Modular design (rendering, input, scene management)
  - Basic ECS (Entity Component System)

- **Rendering**
  - SFML-based 2D renderer
  - Sprite and texture handling
  - Basic camera system

- **Input**
  - Keyboard and mouse input via SFML events, but planned to be in Lua events

- **Scene Management**
  - Early prototype of an entity and scene system
  - Simple resource loader - PLANNED

- **Utility Layer**
  - Logging system - PLANNED
  - Math helpers (vectors, transforms, etc.) with Lua integration

- **Future Goals**
  - Physics prototype
  - Simple in-engine debugging tools

---

## Technologies
- **Language:** C++
- **Framework:** [SFML 2.6+](https://www.sfml-dev.org/)
- **Build System:** CMake

---

## Building the Project

```bash
git clone https://github.com/prayyOnIntelliJ/RayneEngine.git
cd RayneEngine
mkdir build && cd build
cmake ..
make
