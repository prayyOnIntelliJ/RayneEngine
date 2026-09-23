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
        for (size_t i = 0; i < m_Tasks.size(); ) {
            m_Tasks[i].timeLeft -= dt;
            if (m_Tasks[i].timeLeft <= 0.f) {
                if (m_Tasks[i].callback.valid()) {
                    m_Tasks[i].callback();
                }
                if (m_Tasks.empty()) break;
                if (i < m_Tasks.size() && m_Tasks[i].timeLeft <= 0.f) {
                    m_Tasks.erase(m_Tasks.begin() + i);
                }
            } else {
                ++i;
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
