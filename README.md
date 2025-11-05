# RayneEngine

**RayneEngine** is a custom 2D/3D game engine built using [SFML](https://www.sfml-dev.org/).  
It’s currently in early development and serves as a learning and experimentation project — aiming to grow into a flexible, lightweight engine for indie projects and technical exploration.

---

## 🚧 Development Status
RayneEngine is still in its **early stages**.  
Many systems are being prototyped and reworked as the architecture takes shape.  
Expect rapid iteration and breaking changes for now.

---

## ✨ Planned / Early Features

- **Core Engine Architecture**
  - Modular structure with clear separation between rendering, input, and game logic.
  - Basic update loop and time management.

- **Rendering System**
  - SFML-based 2D renderer.
  - Sprite and texture handling.
  - View and camera control.

- **Input System**
  - Keyboard and mouse input support.
  - Event-based system using SFML’s event handling.

- **Scene Management**
  - Early prototype of an entity/scene system.
  - Basic resource loading pipeline.

- **Utility Layer**
  - Math helpers (vectors, transforms, etc.).
  - Simple logging system.

- **Future Goals**
  - ECS (Entity Component System) architecture.
  - Physics integration.
  - Editor tooling.
  - 3D rendering extensions.

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