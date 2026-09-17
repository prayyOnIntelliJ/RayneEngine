#ifndef RAYNEENGINE_TIMERMANAGER_H
#define RAYNEENGINE_TIMERMANAGER_H

#include <vector>
#include <sol/sol.hpp>

struct TimerTask {
    float timeLeft;
    sol::function callback;
};

class TimerManager {
public:
    static TimerManager& Get() {
        static TimerManager instance;
        return instance;
    }
    
    void After(float seconds, sol::function callback) {
        m_Tasks.push_back({seconds, callback});
    }
    
    void Update(float dt) {
        for (auto it = m_Tasks.begin(); it != m_Tasks.end(); ) {
            it->timeLeft -= dt;
            if (it->timeLeft <= 0.f) {
                if (it->callback.valid()) {
                    it->callback();
                }
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
    std::vector<TimerTask> m_Tasks;
};

#endif
