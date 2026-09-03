#include "EventManager.h"

void EventManager::SubscribeCollision(std::function<void(CollisionEvent)> callback)
{
    m_CollisionCallbacks.push_back(std::move(callback));
}

void EventManager::FireCollision(Entity a, Entity b)
{
    for (auto &cb: m_CollisionCallbacks)
        cb({a, b});
}

void EventManager::Clear() { m_CollisionCallbacks.clear(); }
