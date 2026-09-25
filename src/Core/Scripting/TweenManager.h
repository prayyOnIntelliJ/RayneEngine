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
        for (size_t i = 0; i < m_Tasks.size(); ) {
            auto& task = m_Tasks[i];
            task.elapsed += dt;
            float t = task.elapsed / task.duration;
            if (t >= 1.f) t = 1.f;
            
            float eased = t;
            if (task.easeType == "EaseInQuad") {
                eased = t * t;
            } else if (task.easeType == "EaseOutQuad") {
                eased = t * (2.f - t);
            } else if (task.easeType == "EaseInOutQuad") {
                eased = t < 0.5f ? 2.f * t * t : -1.f + (4.f - 2.f * t) * t;
            }
            
            if (task.registry->HasComponent<TransformComponent>(task.entity)) {
                auto& tc = task.registry->GetComponent<TransformComponent>(task.entity);
                tc.x = task.startX + (task.targetX - task.startX) * eased;
                tc.y = task.startY + (task.targetY - task.startY) * eased;
            }
            
            if (task.elapsed >= task.duration) {
                // Swap-and-pop: O(1) removal
                if (i < m_Tasks.size() - 1) {
                    m_Tasks[i] = std::move(m_Tasks.back());
                }
                m_Tasks.pop_back();
            } else {
                ++i;
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
