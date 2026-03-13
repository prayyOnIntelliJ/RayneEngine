#ifndef RAYNEENGINE_EVENTMANAGER_H
#define RAYNEENGINE_EVENTMANAGER_H
#include <functional>

#include "../ECS/Entity.h"

struct CollisionEvent
{
    Entity a;
    Entity b;
};

class EventManager
{
public:
    static EventManager& Get()
    {
        static EventManager instance;
        return instance;
    }

    void SubscribeCollision(std::function<void(CollisionEvent)> callback);
    void FireCollision(Entity a, Entity b);
    void Clear();

private:
    EventManager() = default;
    std::vector<std::function<void(CollisionEvent)>> m_CollisionCallbacks;
};


#endif //RAYNEENGINE_EVENTMANAGER_H