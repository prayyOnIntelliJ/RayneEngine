#ifndef RAYNEENGINE_EVENTMANAGER_H
#define RAYNEENGINE_EVENTMANAGER_H
#include <functional>
#include <string>
#include <vector>

#include "../ECS/Entity.h"

struct CollisionEvent
{
    Entity a;
    Entity b;
};

class EventManager
{
public:
    static EventManager &Get()
    {
        static EventManager instance;
        return instance;
    }

    void SubscribeCollision(std::function<void(CollisionEvent)> callback);

    void FireCollision(Entity a, Entity b);

    void SubscribeButtonClick(std::function<void(const std::string &)> callback);

    void FireButtonClick(const std::string &buttonId);

    void Clear();

private:
    EventManager() = default;

    std::vector<std::function<void(CollisionEvent)> > m_CollisionCallbacks;
    std::vector<std::function<void(const std::string &)> > m_ButtonClickCallbacks;
};

#endif
