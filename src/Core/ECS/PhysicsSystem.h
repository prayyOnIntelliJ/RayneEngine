#ifndef RAYNEENGINE_PHYSICSSYSTEM_H
#define RAYNEENGINE_PHYSICSSYSTEM_H

#include <sol/sol.hpp>
#include "Registry.h"

struct RaycastResult {
    bool hit = false;
    Entity entity = 0;
    float pointX = 0.f;
    float pointY = 0.f;
    float normalX = 0.f;
    float normalY = 0.f;
    float distance = 0.f;
};

class PhysicsSystem {
public:
    static RaycastResult Raycast(Registry& registry, float startX, float startY, float dirX, float dirY, float distance, int channel = -1);
    
    static void RegisterLua(sol::state& lua, Registry& registry);
};

#endif
