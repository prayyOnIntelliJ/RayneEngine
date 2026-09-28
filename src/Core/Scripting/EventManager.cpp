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

void EventManager::SubscribeButtonClick(std::function<void(const std::string &)> callback)
{
    m_ButtonClickCallbacks.push_back(std::move(callback));
}

void EventManager::FireButtonClick(const std::string &buttonId)
{
    const size_t count = m_ButtonClickCallbacks.size();
    for (size_t i = 0; i < count && i < m_ButtonClickCallbacks.size(); ++i)
        m_ButtonClickCallbacks[i](buttonId);
}

void EventManager::Clear()
{
    m_CollisionCallbacks.clear();
    m_ButtonClickCallbacks.clear();
}
