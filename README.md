# RayneEngine

**RayneEngine** is a small custom game engine built with [SFML](https://www.sfml-dev.org/).  
It’s a **portfolio project** created as part of my training as a **Game Engineer** — focused on learning, experimenting, and exploring engine architecture in C++.

This engine is not meant to become a large-scale framework, but rather a compact, understandable system that demonstrates how core engine components work together.

---

## 🚧 Development Status
RayneEngine is still in **early development**.  
Most systems are experimental and will change frequently as I refine the structure and design.

---

## ✨ Planned / Early Features

- **Core Engine Structure**
  - Main loop with delta time.
  - Modular design (rendering, input, scene management).

- **Rendering**
  - SFML-based 2D renderer.
  - Sprite and texture handling.
  - Basic camera system.

- **Input**
  - Keyboard and mouse input via SFML events.

- **Scene Management**
  - Early prototype of an entity and scene system.
  - Simple resource loader.

- **Utility Layer**
  - Logging system.
  - Math helpers (vectors, transforms, etc.).

- **Future Goals**
  - Basic ECS (Entity Component System).
  - Physics prototype.
  - Simple in-engine debugging tools.

---

## 🛠️ Technologies
- **Language:** C++
- **Framework:** [SFML 2.6+](https://www.sfml-dev.org/)
- **Build System:** CMake

---

## 📦 Building the Project

```bash
git clone https://github.com/<your-username>/RayneEngine.git
cd RayneEngine
mkdir build && cd build
cmake ..
make