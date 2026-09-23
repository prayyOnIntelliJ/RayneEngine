#ifndef RAYNEENGINE_TWEENMANAGER_H
#define RAYNEENGINE_TWEENMANAGER_H

#include <vector>
#include <string>
#include "../ECS/Registry.h"
#include "../ECS/Components.h"

struct TweenTask {
    Entity entity;
    float startX, startY;
    float targetX, targetY;
    float duration;
    float elapsed;
    std::string easeType;
    Registry* registry;
};

class TweenManager {
public:
    static TweenManager& Get() {
        static TweenManager instance;
        return instance;
    }
    
    void Position(Entity entity, float targetX, float targetY, float duration, const std::string& easeType, Registry& reg) {
        if (!reg.HasComponent<TransformComponent>(entity)) return;
        auto& t = reg.GetComponent<TransformComponent>(entity);
        m_Tasks.push_back({entity, t.x, t.y, targetX, targetY, duration, 0.f, easeType, &reg});
    }
    
    void Update(float dt) {
        for (auto it = m_Tasks.begin(); it != m_Tasks.end(); ) {
            it->elapsed += dt;
            float t = it->elapsed / it->duration;
            if (t >= 1.f) t = 1.f;
            
            float eased = t;
            if (it->easeType == "EaseInQuad") {
                eased = t * t;
            } else if (it->easeType == "EaseOutQuad") {
                eased = t * (2.f - t);
            } else if (it->easeType == "EaseInOutQuad") {
                eased = t < 0.5f ? 2.f * t * t : -1.f + (4.f - 2.f * t) * t;
            }
            
            if (it->registry->HasComponent<TransformComponent>(it->entity)) {
                auto& tc = it->registry->GetComponent<TransformComponent>(it->entity);
                tc.x = it->startX + (it->targetX - it->startX) * eased;
                tc.y = it->startY + (it->targetY - it->startY) * eased;
            }
            
            if (it->elapsed >= it->duration) {
                it = m_Tasks.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void Clear() {
        m_Tasks.clear();
    }
private:
    std::vector<TweenTask> m_Tasks;
};

#endif
