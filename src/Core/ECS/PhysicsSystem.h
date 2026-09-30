#ifndef RAYNEENGINE_PHYSICSSYSTEM_H
#define RAYNEENGINE_PHYSICSSYSTEM_H

#include <sol/sol.hpp>
#include <SFML/System/Vector2.hpp>
#include "Registry.h"
#include "Components.h"

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
    static void Step(Registry& registry, float dt);
    static void FixedUpdate(Registry& registry, float fixedDt);
    static void ApplyForce(Registry& registry, Entity entity, float fx, float fy);
    static void ApplyImpulse(Registry& registry, Entity entity, float ix, float iy);
    static void SetVelocity(Registry& registry, Entity entity, float vx, float vy);
    static sf::Vector2f GetVelocity(Registry& registry, Entity entity);
    static void SetGravity(float gx, float gy);
    static sf::Vector2f GetGravity();
    static void SetFixedTimestep(float fixedDt);
    static float GetFixedTimestep();
    static void Reset();

    static void RegisterLua(sol::state& lua, Registry& registry);

private:
    static sf::Vector2f s_Gravity;
    static float s_FixedDeltaTime;
    static float s_Accumulator;
};

#endif
