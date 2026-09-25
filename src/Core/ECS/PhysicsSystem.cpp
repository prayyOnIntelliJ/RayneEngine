#include "PhysicsSystem.h"
#include "Components.h"
#include <cmath>
#include <limits>
#include <algorithm>
#include <iostream>

static bool RayAABB(float px, float py, float dx, float dy, float max_t, 
                    float rx, float ry, float rw, float rh, 
                    float& t, float& nx, float& ny) 
{
    float invDx = (dx != 0.0f) ? 1.0f / dx : 0.0f;
    float invDy = (dy != 0.0f) ? 1.0f / dy : 0.0f;

    float t1 = (rx - px) * invDx;
    float t2 = (rx + rw - px) * invDx;
    if (dx == 0.0f) {
        if (px >= rx && px <= rx + rw) {
            t1 = -std::numeric_limits<float>::infinity();
            t2 = std::numeric_limits<float>::infinity();
        } else {
            return false;
        }
    } else if (t1 > t2) std::swap(t1, t2);

    float t3 = (ry - py) * invDy;
    float t4 = (ry + rh - py) * invDy;
    if (dy == 0.0f) {
        if (py >= ry && py <= ry + rh) {
            t3 = -std::numeric_limits<float>::infinity();
            t4 = std::numeric_limits<float>::infinity();
        } else {
            return false;
        }
    } else if (t3 > t4) std::swap(t3, t4);

    float tmin = std::max(t1, t3);
    float tmax = std::min(t2, t4);

    if (tmax < 0.0f) return false;
    if (tmin > tmax) return false;
    if (tmin > max_t) return false;

    t = tmin;
    if (t < 0.0f) {
        t = 0.0f;
        nx = 0.0f;
        ny = 0.0f;
        return true;
    }

    if (t == t1) { nx = (dx > 0) ? -1.0f : 1.0f; ny = 0.0f; }
    else { nx = 0.0f; ny = (dy > 0) ? -1.0f : 1.0f; }
    return true;
}

RaycastResult PhysicsSystem::Raycast(Registry& registry, float startX, float startY, float dirX, float dirY, float distance, int channel)
{
    RaycastResult closest;
    closest.hit = false;
    closest.distance = distance;

    // Normalize direction
    float len = std::sqrt(dirX * dirX + dirY * dirY);
    if (len > 0.0f) {
        dirX /= len;
        dirY /= len;
    } else {
        return closest;
    }

    // We check against all entities that have a Transform and a Collision component
    registry.ForEach<TransformComponent, CollisionComponent>(
        [&](Entity e, TransformComponent& t, CollisionComponent& col) {
            if (channel != -1 && col.channel != channel) return;
            
            float w = 0.0f, h = 0.0f;
            if (registry.HasComponent<RenderComponent>(e)) {
                auto& r = registry.GetComponent<RenderComponent>(e);
                w = r.size.x;
                h = r.size.y;
            } else if (registry.HasComponent<SpriteComponent>(e)) {
                auto& s = registry.GetComponent<SpriteComponent>(e);
                w = s.size.x;
                h = s.size.y;
            } else {
                return; // No size to collide against
            }

            float hit_t = 0.0f;
            float hit_nx = 0.0f, hit_ny = 0.0f;

            if (RayAABB(startX, startY, dirX, dirY, closest.distance, t.x, t.y, w, h, hit_t, hit_nx, hit_ny)) {
                if (hit_t < closest.distance) {
                    closest.hit = true;
                    closest.entity = e;
                    closest.distance = hit_t;
                    closest.normalX = hit_nx;
                    closest.normalY = hit_ny;
                    closest.pointX = startX + dirX * hit_t;
                    closest.pointY = startY + dirY * hit_t;
                }
            }
        });

    return closest;
}

void PhysicsSystem::RegisterLua(sol::state& lua, Registry& registry)
{
    sol::usertype<RaycastResult> hitType = lua.new_usertype<RaycastResult>("RaycastResult",
        "hit", &RaycastResult::hit,
        "entity", &RaycastResult::entity,
        "pointX", &RaycastResult::pointX,
        "pointY", &RaycastResult::pointY,
        "normalX", &RaycastResult::normalX,
        "normalY", &RaycastResult::normalY,
        "distance", &RaycastResult::distance
    );

    sol::table physicsTable = lua.create_named_table("Physics");
    
    physicsTable.set_function("Raycast", [&registry](float startX, float startY, float dirX, float dirY, float distance, sol::optional<int> channel) -> RaycastResult {
        return Raycast(registry, startX, startY, dirX, dirY, distance, channel.value_or(-1));
    });
}
