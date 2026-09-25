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
                sol::function cb;
                if (m_Tasks[i].callback.valid()) {
                    cb = std::move(m_Tasks[i].callback);
                }
                if (i < m_Tasks.size() - 1) {
                    m_Tasks[i] = std::move(m_Tasks.back());
                }
                m_Tasks.pop_back();
                if (cb.valid()) cb();
                if (m_Tasks.empty()) break;
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
