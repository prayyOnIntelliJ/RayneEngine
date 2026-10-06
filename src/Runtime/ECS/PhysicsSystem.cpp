#include "PhysicsSystem.h"
#include "Components.h"
#include "HierarchySystem.h"
#include "../Scripting/EventManager.h"
#include "../Scripting/ScriptComponent.h"
#include <cmath>
#include <limits>
#include <algorithm>
#include <unordered_set>
#include <iostream>

sf::Vector2f PhysicsSystem::s_Gravity = sf::Vector2f(0.f, 980.f);
float PhysicsSystem::s_FixedDeltaTime = 1.0f / 60.0f;
float PhysicsSystem::s_Accumulator = 0.0f;

bool PhysicsSystem::s_CollisionMatrix[PhysicsSystem::MAX_CHANNELS][PhysicsSystem::MAX_CHANNELS] = {
    {true, true, true, true, true, true, true, true},
    {true, true, true, true, true, true, true, true},
    {true, true, true, true, true, true, true, true},
    {true, true, true, true, true, true, true, true},
    {true, true, true, true, true, true, true, true},
    {true, true, true, true, true, true, true, true},
    {true, true, true, true, true, true, true, true},
    {true, true, true, true, true, true, true, true}
};

std::string PhysicsSystem::s_ChannelNames[PhysicsSystem::MAX_CHANNELS] = {
    "Default",
    "Player",
    "Enemy",
    "Projectile",
    "World",
    "Trigger",
    "Collectible",
    "Hazard"
};

struct PairHash
{
    size_t operator()(const std::pair<Entity, Entity> &p) const
    {
        return std::hash<Entity>()(p.first) ^ (std::hash<Entity>()(p.second) << 16);
    }
};

static std::unordered_set<std::pair<Entity, Entity>, PairHash> s_ActiveCollisions;

struct ColliderInfo
{
    Entity entity = 0;
    ColliderShape shape = ColliderShape::Box;
    CollisionType type = CollisionType::Solid;
    int channel = 0;
    bool isTrigger = false;
    sf::Vector2f center;
    sf::Vector2f halfSize;
    float radius = 0.f;
};

static bool RayAABB(float px, float py, float dx, float dy, float max_t,
                    float rx, float ry, float rw, float rh,
                    float &t, float &nx, float &ny)
{
    float invDx = (dx != 0.0f) ? 1.0f / dx : 0.0f;
    float invDy = (dy != 0.0f) ? 1.0f / dy : 0.0f;

    float t1 = (rx - px) * invDx;
    float t2 = (rx + rw - px) * invDx;
    if (dx == 0.0f)
    {
        if (px >= rx && px <= rx + rw)
        {
            t1 = -std::numeric_limits<float>::infinity();
            t2 = std::numeric_limits<float>::infinity();
        } else { return false; }
    } else if (t1 > t2) std::swap(t1, t2);

    float t3 = (ry - py) * invDy;
    float t4 = (ry + rh - py) * invDy;
    if (dy == 0.0f)
    {
        if (py >= ry && py <= ry + rh)
        {
            t3 = -std::numeric_limits<float>::infinity();
            t4 = std::numeric_limits<float>::infinity();
        } else { return false; }
    } else if (t3 > t4) std::swap(t3, t4);

    float tmin = std::max(t1, t3);
    float tmax = std::min(t2, t4);

    if (tmax < 0.0f) return false;
    if (tmin > tmax) return false;
    if (tmin > max_t) return false;

    t = tmin;
    if (t < 0.0f)
    {
        t = 0.0f;
        nx = 0.0f;
        ny = 0.0f;
        return true;
    }

    if (t == t1)
    {
        nx = (dx > 0) ? -1.0f : 1.0f;
        ny = 0.0f;
    } else
    {
        nx = 0.0f;
        ny = (dy > 0) ? -1.0f : 1.0f;
    }
    return true;
}

static bool TestBoxBox(const ColliderInfo &a, const ColliderInfo &b, sf::Vector2f &normal, float &penetration)
{
    float dx = b.center.x - a.center.x;
    float dy = b.center.y - a.center.y;
    float px = (a.halfSize.x + b.halfSize.x) - std::abs(dx);
    float py = (a.halfSize.y + b.halfSize.y) - std::abs(dy);

    if (px <= 0.0f || py <= 0.0f) return false;

    if (px < py)
    {
        penetration = px;
        normal = sf::Vector2f(dx > 0.f ? 1.0f : -1.0f, 0.0f);
    } else
    {
        penetration = py;
        normal = sf::Vector2f(0.0f, dy > 0.f ? 1.0f : -1.0f);
    }
    return true;
}

static bool TestCircleCircle(const ColliderInfo &a, const ColliderInfo &b, sf::Vector2f &normal, float &penetration)
{
    float dx = b.center.x - a.center.x;
    float dy = b.center.y - a.center.y;
    float distSq = dx * dx + dy * dy;
    float radSum = a.radius + b.radius;

    if (distSq >= radSum * radSum) return false;

    float dist = std::sqrt(distSq);
    if (dist > 0.0001f)
    {
        penetration = radSum - dist;
        normal = sf::Vector2f(dx / dist, dy / dist);
    } else
    {
        penetration = a.radius;
        normal = sf::Vector2f(0.0f, 1.0f);
    }
    return true;
}

static bool TestCircleBox(const ColliderInfo &circle, const ColliderInfo &box, sf::Vector2f &normal, float &penetration)
{
    float dx = circle.center.x - box.center.x;
    float dy = circle.center.y - box.center.y;
    float clampedX = std::clamp(dx, -box.halfSize.x, box.halfSize.x);
    float clampedY = std::clamp(dy, -box.halfSize.y, box.halfSize.y);

    float diffX = dx - clampedX;
    float diffY = dy - clampedY;
    float distSq = diffX * diffX + diffY * diffY;

    if (distSq > circle.radius * circle.radius) return false;

    float dist = std::sqrt(distSq);
    if (dist > 0.0001f)
    {
        penetration = circle.radius - dist;
        normal = sf::Vector2f(diffX / dist, diffY / dist);
    } else
    {
        float overlapX = box.halfSize.x - std::abs(dx);
        float overlapY = box.halfSize.y - std::abs(dy);
        if (overlapX < overlapY)
        {
            penetration = circle.radius + overlapX;
            normal = sf::Vector2f(dx >= 0.f ? 1.0f : -1.0f, 0.0f);
        } else
        {
            penetration = circle.radius + overlapY;
            normal = sf::Vector2f(0.0f, dy >= 0.f ? 1.0f : -1.0f);
        }
    }
    return true;
}

RaycastResult PhysicsSystem::Raycast(Registry &registry, float startX, float startY, float dirX, float dirY,
                                     float distance, int channel)
{
    RaycastResult closest;
    closest.hit = false;
    closest.distance = distance;

    float len = std::sqrt(dirX * dirX + dirY * dirY);
    if (len > 0.0f)
    {
        dirX /= len;
        dirY /= len;
    } else { return closest; }

    registry.ForEach<TransformComponent, CollisionComponent>(
        [&](Entity e, TransformComponent &t, CollisionComponent &col) {
            if (channel != -1 && !CanCollide(channel, col.channel)) return;

            float baseW = 0.0f, baseH = 0.0f;
            if (registry.HasComponent<RenderComponent>(e))
            {
                auto &r = registry.GetComponent<RenderComponent>(e);
                baseW = r.size.x;
                baseH = r.size.y;
            } else if (registry.HasComponent<SpriteComponent>(e))
            {
                auto &s = registry.GetComponent<SpriteComponent>(e);
                baseW = s.size.x;
                baseH = s.size.y;
            } else { return; }

            sf::Vector2f effSize = col.GetEffectiveSize(sf::Vector2f(baseW, baseH));
            float effW = effSize.x * t.worldScaleX;
            float effH = effSize.y * t.worldScaleY;
            float offX = col.offsetX * t.worldScaleX;
            float offY = col.offsetY * t.worldScaleY;
            float boxX = t.worldX + (baseW * t.worldScaleX - effW) * 0.5f + offX;
            float boxY = t.worldY + (baseH * t.worldScaleY - effH) * 0.5f + offY;

            float hit_t = 0.0f;
            float hit_nx = 0.0f, hit_ny = 0.0f;

            if (RayAABB(startX, startY, dirX, dirY, closest.distance, boxX, boxY, effW, effH, hit_t, hit_nx, hit_ny))
            {
                if (hit_t < closest.distance)
                {
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

    registry.ForEach<TransformComponent, TilemapComponent>(
        [&](Entity e, TransformComponent &t, TilemapComponent &tm) {
            if (!tm.generateCollisions) return;
            if (channel != -1 && !CanCollide(channel, tm.collisionChannel)) return;

            const auto &boxes = tm.GetCollisionBoxes();
            for (const auto &box : boxes)
            {
                float effW = box.width * t.worldScaleX;
                float effH = box.height * t.worldScaleY;
                float boxX = t.worldX + box.left * t.worldScaleX;
                float boxY = t.worldY + box.top * t.worldScaleY;

                float hit_t = 0.0f;
                float hit_nx = 0.0f, hit_ny = 0.0f;
                if (RayAABB(startX, startY, dirX, dirY, closest.distance, boxX, boxY, effW, effH, hit_t, hit_nx, hit_ny))
                {
                    if (hit_t < closest.distance)
                    {
                        closest.hit = true;
                        closest.entity = e;
                        closest.distance = hit_t;
                        closest.normalX = hit_nx;
                        closest.normalY = hit_ny;
                        closest.pointX = startX + dirX * hit_t;
                        closest.pointY = startY + dirY * hit_t;
                    }
                }
            }
        });

    return closest;
}

void PhysicsSystem::Step(Registry &registry, float dt)
{
    s_Accumulator += dt;
    if (s_Accumulator > 0.2f) s_Accumulator = 0.2f;

    while (s_Accumulator >= s_FixedDeltaTime)
    {
        FixedUpdate(registry, s_FixedDeltaTime);
        s_Accumulator -= s_FixedDeltaTime;
    }
}

void PhysicsSystem::FixedUpdate(Registry &registry, float fixedDt)
{
    registry.ForEach<TransformComponent, Rigidbody2DComponent>(
        [&registry](Entity e, TransformComponent &, Rigidbody2DComponent &) {
            if (!registry.HasComponent<VelocityComponent>(e)) { registry.AddComponent(e, VelocityComponent{0.f, 0.f}); }
        });

    registry.ForEach<TransformComponent, VelocityComponent>(
        [&registry, fixedDt](Entity e, TransformComponent &t, VelocityComponent &v) {
            if (registry.HasComponent<Rigidbody2DComponent>(e))
            {
                auto &rb = registry.GetComponent<Rigidbody2DComponent>(e);
                if (rb.bodyType == BodyType::Dynamic)
                {
                    float invMass = 1.0f / std::max(0.0001f, rb.mass);
                    float ax = (rb.forceX * invMass) + (s_Gravity.x * rb.gravityScale);
                    float ay = (rb.forceY * invMass) + (s_Gravity.y * rb.gravityScale);

                    v.dx += ax * fixedDt;
                    v.dy += ay * fixedDt;

                    float damping = std::max(0.0f, 1.0f - rb.drag * fixedDt);
                    v.dx *= damping;
                    v.dy *= damping;

                    t.x += v.dx * fixedDt;
                    t.y += v.dy * fixedDt;

                    rb.forceX = 0.f;
                    rb.forceY = 0.f;
                } else if (rb.bodyType == BodyType::Kinematic)
                {
                    t.x += v.dx * fixedDt;
                    t.y += v.dy * fixedDt;
                } else
                {
                    v.dx = 0.f;
                    v.dy = 0.f;
                }
            } else
            {
                t.x += v.dx * fixedDt;
                t.y += v.dy * fixedDt;
            }
        });

    HierarchySystem::UpdateWorldTransforms(registry);
    std::vector<ColliderInfo> colliders;
    registry.ForEach<TransformComponent, CollisionComponent>(
        [&registry, &colliders](Entity e, TransformComponent &t, CollisionComponent &col) {
            ColliderInfo info;
            info.entity = e;
            info.shape = col.shape;
            info.type = col.type;
            info.channel = col.channel;
            info.isTrigger = col.isTrigger;

            float baseW = 32.f;
            float baseH = 32.f;
            if (registry.HasComponent<RenderComponent>(e))
            {
                auto &r = registry.GetComponent<RenderComponent>(e);
                baseW = r.size.x;
                baseH = r.size.y;
                if (r.shapeType == ShapeType::Circle) { info.shape = ColliderShape::Circle; }
            } else if (registry.HasComponent<SpriteComponent>(e))
            {
                auto &s = registry.GetComponent<SpriteComponent>(e);
                baseW = s.size.x;
                baseH = s.size.y;
            }

            sf::Vector2f effSize = col.GetEffectiveSize(sf::Vector2f(baseW, baseH));
            float effW = effSize.x * t.worldScaleX;
            float effH = effSize.y * t.worldScaleY;
            float offX = col.offsetX * t.worldScaleX;
            float offY = col.offsetY * t.worldScaleY;

            info.center = sf::Vector2f(t.worldX + (baseW * t.worldScaleX) * 0.5f + offX,
                                       t.worldY + (baseH * t.worldScaleY) * 0.5f + offY);
            info.halfSize = sf::Vector2f(std::abs(effW) * 0.5f, std::abs(effH) * 0.5f);
            info.radius = (info.shape == ColliderShape::Circle)
                ? (col.GetEffectiveRadius(sf::Vector2f(baseW, baseH)) * std::max(std::abs(t.worldScaleX), std::abs(t.worldScaleY)))
                : std::max(info.halfSize.x, info.halfSize.y);

            colliders.push_back(info);
        });

    registry.ForEach<TransformComponent, TilemapComponent>(
        [&](Entity e, TransformComponent &t, TilemapComponent &tm) {
            if (!tm.generateCollisions) return;
            const auto &boxes = tm.GetCollisionBoxes();
            for (const auto &box : boxes)
            {
                ColliderInfo info;
                info.entity = e;
                info.shape = ColliderShape::Box;
                info.type = CollisionType::Solid;
                info.channel = tm.collisionChannel;
                info.isTrigger = false;
                float halfW = box.width * 0.5f * std::abs(t.worldScaleX);
                float halfH = box.height * 0.5f * std::abs(t.worldScaleY);
                info.center = sf::Vector2f(t.worldX + (box.left + box.width * 0.5f) * t.worldScaleX,
                                           t.worldY + (box.top + box.height * 0.5f) * t.worldScaleY);
                info.halfSize = sf::Vector2f(halfW, halfH);
                info.radius = std::max(halfW, halfH);
                colliders.push_back(info);
            }
        });

    std::unordered_set<std::pair<Entity, Entity>, PairHash> currentCollisions;

    for (size_t i = 0; i < colliders.size(); ++i)
    {
        for (size_t j = i + 1; j < colliders.size(); ++j)
        {
            auto &a = colliders[i];
            auto &b = colliders[j];

            if (!CanCollide(a.channel, b.channel)) continue;

            sf::Vector2f normal(0.f, 0.f);
            float penetration = 0.0f;
            bool hit = false;

            if (a.shape == ColliderShape::Box && b.shape == ColliderShape::Box)
            {
                hit = TestBoxBox(a, b, normal, penetration);
            } else if (a.shape == ColliderShape::Circle && b.shape == ColliderShape::Circle)
            {
                hit = TestCircleCircle(a, b, normal, penetration);
            } else if (a.shape == ColliderShape::Circle && b.shape == ColliderShape::Box)
            {
                hit = TestCircleBox(a, b, normal, penetration);
                normal = -normal;
            } else if (a.shape == ColliderShape::Box && b.shape == ColliderShape::Circle)
            {
                hit = TestCircleBox(b, a, normal, penetration);
            }

            if (!hit) continue;

            Entity e1 = std::min(a.entity, b.entity);
            Entity e2 = std::max(a.entity, b.entity);
            currentCollisions.emplace(e1, e2);

            bool wasColliding = s_ActiveCollisions.count({e1, e2}) > 0;

            const bool isTriggerContact = a.isTrigger || b.isTrigger || (
                                              a.type == CollisionType::Static && b.type == CollisionType::Static);

            if (!wasColliding)
            {
                EventManager::Get().FireCollision(a.entity, b.entity);

                if (isTriggerContact)
                {
                    if (registry.HasComponent<ScriptComponent>(a.entity))
                        registry.GetComponent<ScriptComponent>(a.entity).OnTriggerEnter(b.entity);
                    if (registry.HasComponent<ScriptComponent>(b.entity))
                        registry.GetComponent<ScriptComponent>(b.entity).OnTriggerEnter(a.entity);
                } else
                {
                    if (registry.HasComponent<ScriptComponent>(a.entity))
                        registry.GetComponent<ScriptComponent>(a.entity).OnCollisionEnter(
                            b.entity, -normal.x, -normal.y);
                    if (registry.HasComponent<ScriptComponent>(b.entity))
                        registry.GetComponent<ScriptComponent>(b.entity).OnCollisionEnter(a.entity, normal.x, normal.y);
                }
            }
            if (!isTriggerContact && (a.type == CollisionType::Solid || b.type == CollisionType::Solid))
            {
                Rigidbody2DComponent *rbA = registry.HasComponent<Rigidbody2DComponent>(a.entity)
                                                ? &registry.GetComponent<Rigidbody2DComponent>(a.entity)
                                                : nullptr;
                Rigidbody2DComponent *rbB = registry.HasComponent<Rigidbody2DComponent>(b.entity)
                                                ? &registry.GetComponent<Rigidbody2DComponent>(b.entity)
                                                : nullptr;

                bool aDynamic = (rbA && rbA->bodyType == BodyType::Dynamic) || (
                                    !rbA && registry.HasComponent<VelocityComponent>(a.entity));
                bool bDynamic = (rbB && rbB->bodyType == BodyType::Dynamic) || (
                                    !rbB && registry.HasComponent<VelocityComponent>(b.entity));

                float massA = (rbA && rbA->bodyType == BodyType::Dynamic) ? std::max(0.0001f, rbA->mass) : 0.f;
                float massB = (rbB && rbB->bodyType == BodyType::Dynamic) ? std::max(0.0001f, rbB->mass) : 0.f;

                float invMassA = (aDynamic && massA > 0.f) ? 1.0f / massA : (aDynamic ? 1.0f : 0.0f);
                float invMassB = (bDynamic && massB > 0.f) ? 1.0f / massB : (bDynamic ? 1.0f : 0.0f);
                float invMassSum = invMassA + invMassB;

                if (invMassSum > 0.00001f)
                {
                    const float percent = 0.8f;
                    const float slop = 0.01f;
                    float correctionMag = std::max(penetration - slop, 0.0f) / invMassSum * percent;
                    sf::Vector2f correction = normal * correctionMag;

                    if (aDynamic && registry.HasComponent<TransformComponent>(a.entity))
                    {
                        auto &tA = registry.GetComponent<TransformComponent>(a.entity);
                        tA.x -= correction.x * invMassA;
                        tA.y -= correction.y * invMassA;
                    }
                    if (bDynamic && registry.HasComponent<TransformComponent>(b.entity))
                    {
                        auto &tB = registry.GetComponent<TransformComponent>(b.entity);
                        tB.x += correction.x * invMassB;
                        tB.y += correction.y * invMassB;
                    }
                    VelocityComponent *vA = registry.HasComponent<VelocityComponent>(a.entity)
                                                ? &registry.GetComponent<VelocityComponent>(a.entity)
                                                : nullptr;
                    VelocityComponent *vB = registry.HasComponent<VelocityComponent>(b.entity)
                                                ? &registry.GetComponent<VelocityComponent>(b.entity)
                                                : nullptr;

                    sf::Vector2f velA = vA ? sf::Vector2f(vA->dx, vA->dy) : sf::Vector2f(0.f, 0.f);
                    sf::Vector2f velB = vB ? sf::Vector2f(vB->dx, vB->dy) : sf::Vector2f(0.f, 0.f);

                    sf::Vector2f relVel = velB - velA;
                    float velAlongNormal = relVel.x * normal.x + relVel.y * normal.y;

                    if (velAlongNormal < 0.0f)
                    {
                        float eA = rbA ? rbA->restitution : 0.0f;
                        float eB = rbB ? rbB->restitution : 0.0f;
                        float restitution = std::max(eA, eB);

                        float j = -(1.0f + restitution) * velAlongNormal / invMassSum;
                        sf::Vector2f impulse = normal * j;

                        if (vA)
                        {
                            vA->dx -= impulse.x * invMassA;
                            vA->dy -= impulse.y * invMassA;
                        }
                        if (vB)
                        {
                            vB->dx += impulse.x * invMassB;
                            vB->dy += impulse.y * invMassB;
                        }

                        sf::Vector2f tangent = relVel - normal * velAlongNormal;
                        float tangentLen = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
                        if (tangentLen > 0.0001f)
                        {
                            tangent.x /= tangentLen;
                            tangent.y /= tangentLen;
                            float jt = -(relVel.x * tangent.x + relVel.y * tangent.y) / invMassSum;
                            float frictionMu = (rbA || rbB) ? 0.2f : 0.0f;
                            float maxFriction = j * frictionMu;
                            jt = std::clamp(jt, -maxFriction, maxFriction);
                            sf::Vector2f frictionImpulse = tangent * jt;

                            if (vA)
                            {
                                vA->dx -= frictionImpulse.x * invMassA;
                                vA->dy -= frictionImpulse.y * invMassA;
                            }
                            if (vB)
                            {
                                vB->dx += frictionImpulse.x * invMassB;
                                vB->dy += frictionImpulse.y * invMassB;
                            }
                        }
                    }
                }
            }
        }
    }

    s_ActiveCollisions = std::move(currentCollisions);
    HierarchySystem::UpdateWorldTransforms(registry);
}

void PhysicsSystem::ApplyForce(Registry &registry, Entity entity, float fx, float fy)
{
    if (registry.HasComponent<Rigidbody2DComponent>(entity))
    {
        auto &rb = registry.GetComponent<Rigidbody2DComponent>(entity);
        rb.forceX += fx;
        rb.forceY += fy;
    }
}

void PhysicsSystem::ApplyImpulse(Registry &registry, Entity entity, float ix, float iy)
{
    if (registry.HasComponent<Rigidbody2DComponent>(entity))
    {
        auto &rb = registry.GetComponent<Rigidbody2DComponent>(entity);
        float invMass = 1.0f / std::max(0.0001f, rb.mass);
        if (registry.HasComponent<VelocityComponent>(entity))
        {
            auto &v = registry.GetComponent<VelocityComponent>(entity);
            v.dx += ix * invMass;
            v.dy += iy * invMass;
        } else { registry.AddComponent(entity, VelocityComponent{ix * invMass, iy * invMass}); }
    } else if (registry.HasComponent<VelocityComponent>(entity))
    {
        auto &v = registry.GetComponent<VelocityComponent>(entity);
        v.dx += ix;
        v.dy += iy;
    }
}

void PhysicsSystem::SetVelocity(Registry &registry, Entity entity, float vx, float vy)
{
    if (registry.HasComponent<VelocityComponent>(entity))
    {
        auto &v = registry.GetComponent<VelocityComponent>(entity);
        v.dx = vx;
        v.dy = vy;
    } else { registry.AddComponent(entity, VelocityComponent{vx, vy}); }
}

sf::Vector2f PhysicsSystem::GetVelocity(Registry &registry, Entity entity)
{
    if (registry.HasComponent<VelocityComponent>(entity))
    {
        auto &v = registry.GetComponent<VelocityComponent>(entity);
        return sf::Vector2f(v.dx, v.dy);
    }
    return sf::Vector2f(0.f, 0.f);
}

void PhysicsSystem::SetGravity(float gx, float gy)
{
    s_Gravity = sf::Vector2f(gx, gy);
    std::cout << "[INFO] [Physics] Gravity set to (" << gx << ", " << gy << ")\n";
}

sf::Vector2f PhysicsSystem::GetGravity() { return s_Gravity; }

void PhysicsSystem::SetFixedTimestep(float fixedDt)
{
    if (fixedDt > 0.001f)
    {
        s_FixedDeltaTime = fixedDt;
        std::cout << "[INFO] [Physics] Fixed timestep set to " << fixedDt << "s\n";
    }
}

float PhysicsSystem::GetFixedTimestep() { return s_FixedDeltaTime; }

void PhysicsSystem::Reset()
{
    s_Accumulator = 0.0f;
    s_ActiveCollisions.clear();
}

bool PhysicsSystem::CanCollide(int channelA, int channelB)
{
    if (channelA < 0 || channelA >= MAX_CHANNELS || channelB < 0 || channelB >= MAX_CHANNELS)
    {
        return true;
    }
    return s_CollisionMatrix[channelA][channelB];
}

void PhysicsSystem::SetCanCollide(int channelA, int channelB, bool canCollide)
{
    if (channelA >= 0 && channelA < MAX_CHANNELS && channelB >= 0 && channelB < MAX_CHANNELS)
    {
        s_CollisionMatrix[channelA][channelB] = canCollide;
        s_CollisionMatrix[channelB][channelA] = canCollide;
    }
}

const std::string &PhysicsSystem::GetChannelName(int channel)
{
    static const std::string s_Unknown = "Unknown";
    if (channel >= 0 && channel < MAX_CHANNELS)
    {
        return s_ChannelNames[channel];
    }
    return s_Unknown;
}

void PhysicsSystem::SetChannelName(int channel, const std::string &name)
{
    if (channel >= 0 && channel < MAX_CHANNELS)
    {
        s_ChannelNames[channel] = name;
    }
}

void PhysicsSystem::ResetCollisionMatrix()
{
    for (int i = 0; i < MAX_CHANNELS; ++i)
    {
        for (int j = 0; j < MAX_CHANNELS; ++j)
        {
            s_CollisionMatrix[i][j] = true;
        }
    }
    s_ChannelNames[0] = "Default";
    s_ChannelNames[1] = "Player";
    s_ChannelNames[2] = "Enemy";
    s_ChannelNames[3] = "Projectile";
    s_ChannelNames[4] = "World";
    s_ChannelNames[5] = "Trigger";
    s_ChannelNames[6] = "Collectible";
    s_ChannelNames[7] = "Hazard";
}

void PhysicsSystem::LoadCollisionSettings(const nlohmann::json &j)
{
    if (j.contains("collisionChannels") && j["collisionChannels"].is_array())
    {
        const auto &arr = j["collisionChannels"];
        for (size_t i = 0; i < arr.size() && i < MAX_CHANNELS; ++i)
        {
            if (arr[i].is_string())
            {
                s_ChannelNames[i] = arr[i].get<std::string>();
            }
        }
    }
    if (j.contains("collisionMatrix") && j["collisionMatrix"].is_array())
    {
        const auto &mat = j["collisionMatrix"];
        for (size_t r = 0; r < mat.size() && r < MAX_CHANNELS; ++r)
        {
            if (mat[r].is_array())
            {
                const auto &row = mat[r];
                for (size_t c = 0; c < row.size() && c < MAX_CHANNELS; ++c)
                {
                    if (row[c].is_boolean())
                    {
                        s_CollisionMatrix[r][c] = row[c].get<bool>();
                    }
                }
            }
        }
    }
}

void PhysicsSystem::SaveCollisionSettings(nlohmann::json &j)
{
    nlohmann::json chArr = nlohmann::json::array();
    for (int i = 0; i < MAX_CHANNELS; ++i)
    {
        chArr.push_back(s_ChannelNames[i]);
    }
    j["collisionChannels"] = chArr;

    nlohmann::json matArr = nlohmann::json::array();
    for (int r = 0; r < MAX_CHANNELS; ++r)
    {
        nlohmann::json rowArr = nlohmann::json::array();
        for (int c = 0; c < MAX_CHANNELS; ++c)
        {
            rowArr.push_back(s_CollisionMatrix[r][c]);
        }
        matArr.push_back(rowArr);
    }
    j["collisionMatrix"] = matArr;
}

void PhysicsSystem::RegisterLua(sol::state &lua, Registry &registry)
{
    lua.new_usertype<RaycastResult>("RaycastResult",
                                    "hit", &RaycastResult::hit,
                                    "entity", &RaycastResult::entity,
                                    "pointX", &RaycastResult::pointX,
                                    "pointY", &RaycastResult::pointY,
                                    "normalX", &RaycastResult::normalX,
                                    "normalY", &RaycastResult::normalY,
                                    "distance", &RaycastResult::distance
    );

    lua.new_enum("BodyType",
                 "Dynamic", BodyType::Dynamic,
                 "Kinematic", BodyType::Kinematic,
                 "Static", BodyType::Static
    );

    lua.new_enum("ColliderShape",
                 "Box", ColliderShape::Box,
                 "Circle", ColliderShape::Circle
    );

    lua.new_usertype<Rigidbody2DComponent>("Rigidbody2D",
                                           "bodyType", &Rigidbody2DComponent::bodyType,
                                           "mass", &Rigidbody2DComponent::mass,
                                           "gravityScale", &Rigidbody2DComponent::gravityScale,
                                           "restitution", &Rigidbody2DComponent::restitution,
                                           "drag", &Rigidbody2DComponent::drag,
                                           "freezeRotation", &Rigidbody2DComponent::freezeRotation
    );

    sol::table physicsTable = lua.create_named_table("Physics");

    physicsTable.set_function(
        "Raycast",
        [&registry](float startX, float startY, float dirX, float dirY, float distance,
                    sol::optional<int> channel) -> RaycastResult {
            return Raycast(registry, startX, startY, dirX, dirY, distance, channel.value_or(-1));
        });

    physicsTable.set_function("ApplyForce", [&registry](Entity entity, float fx, float fy) {
        ApplyForce(registry, entity, fx, fy);
    });

    physicsTable.set_function("ApplyImpulse", [&registry](Entity entity, float ix, float iy) {
        ApplyImpulse(registry, entity, ix, iy);
    });

    physicsTable.set_function("SetVelocity", [&registry](Entity entity, float vx, float vy) {
        SetVelocity(registry, entity, vx, vy);
    });

    physicsTable.set_function("GetVelocity", [&registry](Entity entity) -> std::tuple<float, float> {
        sf::Vector2f v = GetVelocity(registry, entity);
        return {v.x, v.y};
    });

    physicsTable.set_function("SetGravity", [](float gx, float gy) { SetGravity(gx, gy); });

    physicsTable.set_function("GetGravity", []() -> std::tuple<float, float> {
        sf::Vector2f g = GetGravity();
        return {g.x, g.y};
    });

    physicsTable.set_function("SetFixedTimestep", [](float dt) { SetFixedTimestep(dt); });

    physicsTable.set_function("GetFixedTimestep", []() -> float { return GetFixedTimestep(); });

    physicsTable.set_function("CanCollide", [](int a, int b) -> bool { return CanCollide(a, b); });
    physicsTable.set_function("SetCanCollide", [](int a, int b, bool enable) { SetCanCollide(a, b, enable); });
    physicsTable.set_function("GetChannelName", [](int ch) -> std::string { return GetChannelName(ch); });
    physicsTable.set_function("SetChannelName", [](int ch, const std::string &name) { SetChannelName(ch, name); });

    lua.set_function("AddRigidbody",
                     [&registry](Entity e, sol::optional<int> bodyType, sol::optional<float> mass,
                                 sol::optional<float> gravityScale) -> Rigidbody2DComponent & {
                         Rigidbody2DComponent rb;
                         if (bodyType.has_value()) rb.bodyType = static_cast<BodyType>(bodyType.value());
                         if (mass.has_value()) rb.mass = mass.value();
                         if (gravityScale.has_value()) rb.gravityScale = gravityScale.value();
                         return registry.AddComponent(e, rb);
                     });

    lua.set_function("GetRigidbody", [&registry](Entity e) -> Rigidbody2DComponent * {
        if (!registry.HasComponent<Rigidbody2DComponent>(e)) return nullptr;
        return &registry.GetComponent<Rigidbody2DComponent>(e);
    });

    lua.set_function("HasRigidbody", [&registry](Entity e) -> bool {
        return registry.HasComponent<Rigidbody2DComponent>(e);
    });
}
