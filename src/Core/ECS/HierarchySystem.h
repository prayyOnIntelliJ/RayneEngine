#ifndef RAYNEENGINE_HIERARCHYSYSTEM_H
#define RAYNEENGINE_HIERARCHYSYSTEM_H

#include "Registry.h"
#include "Components.h"
#include <SFML/Graphics/Transform.hpp>
#include <algorithm>
#include <vector>
#include <cmath>

namespace HierarchySystem {

inline bool IsDescendantOf(Registry &registry, Entity child, Entity ancestor)
{
    if (child == NULL_ENTITY || ancestor == NULL_ENTITY) return false;
    if (child == ancestor) return true;
    Entity cur = child;
    while (cur != NULL_ENTITY && registry.HasComponent<HierarchyComponent>(cur)) {
        Entity p = registry.GetComponent<HierarchyComponent>(cur).parent;
        if (p == ancestor) return true;
        cur = p;
    }
    return false;
}

inline Entity GetParent(Registry &registry, Entity child)
{
    if (child == NULL_ENTITY || !registry.HasComponent<HierarchyComponent>(child)) return NULL_ENTITY;
    return registry.GetComponent<HierarchyComponent>(child).parent;
}

inline std::vector<Entity> GetChildren(Registry &registry, Entity parent)
{
    if (parent == NULL_ENTITY || !registry.HasComponent<HierarchyComponent>(parent)) return {};
    return registry.GetComponent<HierarchyComponent>(parent).children;
}

inline void UpdateWorldTransforms(Registry &registry)
{
    auto updateSubtree = [&](auto& self, Entity e, const sf::Transform& parentTransform, float parentRot, float parentScaleX, float parentScaleY) -> void {
        if (!registry.HasComponent<TransformComponent>(e)) return;
        auto &tc = registry.GetComponent<TransformComponent>(e);

        bool hasParent = registry.HasComponent<HierarchyComponent>(e) && registry.GetComponent<HierarchyComponent>(e).parent != NULL_ENTITY;
        if (!hasParent) {
            tc.worldX = tc.x;
            tc.worldY = tc.y;
            tc.worldRotation = tc.rotation;
            tc.worldScaleX = tc.scaleX;
            tc.worldScaleY = tc.scaleY;
        } else {
            sf::Vector2f worldPos = parentTransform.transformPoint({tc.x, tc.y});
            tc.worldX = worldPos.x;
            tc.worldY = worldPos.y;
            tc.worldRotation = parentRot + tc.rotation;
            tc.worldScaleX = parentScaleX * tc.scaleX;
            tc.worldScaleY = parentScaleY * tc.scaleY;
        }

        sf::Transform myTr;
        myTr.translate(tc.worldX, tc.worldY);
        myTr.rotate(tc.worldRotation);
        myTr.scale(tc.worldScaleX, tc.worldScaleY);

        if (registry.HasComponent<HierarchyComponent>(e)) {
            for (Entity childE : registry.GetComponent<HierarchyComponent>(e).children) {
                self(self, childE, myTr, tc.worldRotation, tc.worldScaleX, tc.worldScaleY);
            }
        }
    };

    sf::Transform ident;
    registry.ForEach<TransformComponent>([&](Entity e, TransformComponent &tc) {
        bool hasParent = registry.HasComponent<HierarchyComponent>(e) && registry.GetComponent<HierarchyComponent>(e).parent != NULL_ENTITY;
        if (!hasParent) {
            updateSubtree(updateSubtree, e, ident, 0.f, 1.f, 1.f);
        }
    });
}

inline void SetParent(Registry &registry, Entity child, Entity newParent, bool keepWorldTransform = true)
{
    if (child == NULL_ENTITY || child == newParent) return;
    if (!registry.HasComponent<TransformComponent>(child)) return;

    if (newParent != NULL_ENTITY && IsDescendantOf(registry, newParent, child)) {
        return;
    }

    if (!registry.HasComponent<HierarchyComponent>(child)) {
        registry.AddComponent(child, HierarchyComponent{});
    }

    auto &childH = registry.GetComponent<HierarchyComponent>(child);
    if (childH.parent == newParent) return;

    if (childH.parent != NULL_ENTITY && registry.HasComponent<HierarchyComponent>(childH.parent)) {
        auto &oldParentH = registry.GetComponent<HierarchyComponent>(childH.parent);
        std::erase(oldParentH.children, child);
    }

    auto &tc = registry.GetComponent<TransformComponent>(child);

    if (keepWorldTransform) {
        if (newParent == NULL_ENTITY) {
            tc.x = tc.worldX;
            tc.y = tc.worldY;
            tc.rotation = tc.worldRotation;
            tc.scaleX = tc.worldScaleX;
            tc.scaleY = tc.worldScaleY;
            childH.parent = NULL_ENTITY;
        } else {
            if (!registry.HasComponent<HierarchyComponent>(newParent)) {
                registry.AddComponent(newParent, HierarchyComponent{});
            }
            if (registry.HasComponent<TransformComponent>(newParent)) {
                auto &parentTc = registry.GetComponent<TransformComponent>(newParent);
                sf::Transform parentWorldTr;
                parentWorldTr.translate(parentTc.worldX, parentTc.worldY);
                parentWorldTr.rotate(parentTc.worldRotation);
                parentWorldTr.scale(parentTc.worldScaleX, parentTc.worldScaleY);

                sf::Vector2f newLocalPos = parentWorldTr.getInverse().transformPoint({tc.worldX, tc.worldY});
                tc.x = newLocalPos.x;
                tc.y = newLocalPos.y;
                tc.rotation = tc.worldRotation - parentTc.worldRotation;
                tc.scaleX = (std::abs(parentTc.worldScaleX) > 0.0001f) ? (tc.worldScaleX / parentTc.worldScaleX) : tc.worldScaleX;
                tc.scaleY = (std::abs(parentTc.worldScaleY) > 0.0001f) ? (tc.worldScaleY / parentTc.worldScaleY) : tc.worldScaleY;
            }
            childH.parent = newParent;
            registry.GetComponent<HierarchyComponent>(newParent).children.push_back(child);
        }
    } else {
        childH.parent = newParent;
        if (newParent != NULL_ENTITY) {
            if (!registry.HasComponent<HierarchyComponent>(newParent)) {
                registry.AddComponent(newParent, HierarchyComponent{});
            }
            registry.GetComponent<HierarchyComponent>(newParent).children.push_back(child);
        }
    }

    UpdateWorldTransforms(registry);
}

} // namespace HierarchySystem

#endif // RAYNEENGINE_HIERARCHYSYSTEM_H
