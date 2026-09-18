#include "EventManager.h"

void EventManager::SubscribeCollision(std::function<void(CollisionEvent)> callback)
{
    m_CollisionCallbacks.push_back(std::move(callback));
}

void EventManager::FireCollision(Entity a, Entity b)
{
    auto callbacks = m_CollisionCallbacks; // Copy to prevent iterator invalidation if cleared during callback
    for (auto &cb: callbacks)
        cb({a, b});
}

void EventManager::Clear() { m_CollisionCallbacks.clear(); }
