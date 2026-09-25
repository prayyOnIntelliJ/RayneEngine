#include "EventManager.h"

void EventManager::SubscribeCollision(std::function<void(CollisionEvent)> callback)
{
    m_CollisionCallbacks.push_back(std::move(callback));
}

void EventManager::FireCollision(Entity a, Entity b)
{
    const size_t count = m_CollisionCallbacks.size();
    for (size_t i = 0; i < count && i < m_CollisionCallbacks.size(); ++i)
        m_CollisionCallbacks[i]({a, b});
}

void EventManager::Clear() { m_CollisionCallbacks.clear(); }
