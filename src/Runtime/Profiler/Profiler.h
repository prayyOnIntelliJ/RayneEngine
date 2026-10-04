#ifndef RAYNEENGINE_PROFILER_H
#define RAYNEENGINE_PROFILER_H

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <chrono>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Font.hpp>

struct SubsystemTimes
{
    float scriptsMs = 0.0f;
    float physicsMs = 0.0f;
    float renderingMs = 0.0f;
    float audioMs = 0.0f;
    float engineMs = 0.0f;
};

struct CustomSample
{
    std::string name;
    float durationMs = 0.0f;
};

struct FrameProfile
{
    float totalFrameTimeMs = 0.0f;
    float fps = 0.0f;
    SubsystemTimes subsystems;
    std::vector<CustomSample> customSamples;
    int drawCalls = 0;
    int entityCount = 0;
    float luaMemoryKb = 0.0f;
};

class Profiler
{
public:
    static Profiler &Get()
    {
        static Profiler instance;
        return instance;
    }

    void BeginFrame();
    void EndFrame();

    void BeginSubsystem(const std::string &name);
    void EndSubsystem(const std::string &name);

    void BeginSample(const std::string &name);
    void EndSample(const std::string &name);

    void RecordDrawCall(int count = 1);
    void SetEntityCount(int count);
    void SetLuaMemory(float kb);

    const std::vector<FrameProfile> &GetHistory() const { return m_History; }
    const FrameProfile &GetLatestFrame() const;

    float GetCurrentFPS() const { return m_CurrentFPS; }
    float GetAvgFPS() const { return m_AvgFPS; }
    float GetMinFPS() const { return m_MinFPS; }
    float GetMaxFPS() const { return m_MaxFPS; }

    float GetCurrentFrameTimeMs() const { return m_CurrentFrameTimeMs; }
    float GetAvgFrameTimeMs() const { return m_AvgFrameTimeMs; }
    float GetMinFrameTimeMs() const { return m_MinFrameTimeMs; }
    float GetMaxFrameTimeMs() const { return m_MaxFrameTimeMs; }

    void SetPaused(bool paused) { m_Paused = paused; }
    bool IsPaused() const { return m_Paused; }
    void ClearHistory();

    bool IsHudVisible() const { return m_ShowHud; }
    void SetHudVisible(bool show) { m_ShowHud = show; }
    void ToggleHud() { m_ShowHud = !m_ShowHud; }

    void RenderHud(sf::RenderWindow &window, const sf::Font &font);

    static constexpr size_t MAX_HISTORY = 240;

private:
    Profiler();
    ~Profiler() = default;

    Profiler(const Profiler &) = delete;
    Profiler &operator=(const Profiler &) = delete;

    bool m_Paused = false;
    bool m_ShowHud = false;

    std::chrono::high_resolution_clock::time_point m_FrameStartTime;
    FrameProfile m_CurrentFrame;
    std::vector<FrameProfile> m_History;
    FrameProfile m_EmptyProfile;

    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> m_ActiveSubsystems;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> m_ActiveSamples;

    float m_CurrentFPS = 0.0f;
    float m_AvgFPS = 0.0f;
    float m_MinFPS = 0.0f;
    float m_MaxFPS = 0.0f;

    float m_CurrentFrameTimeMs = 0.0f;
    float m_AvgFrameTimeMs = 0.0f;
    float m_MinFrameTimeMs = 0.0f;
    float m_MaxFrameTimeMs = 0.0f;

    int m_CurrentDrawCalls = 0;
};

struct ProfileScope
{
    std::string name;
    bool isSubsystem;

    explicit ProfileScope(std::string scopeName, bool subsystem = false);
    ~ProfileScope();
};

#define PROFILE_SCOPE(name) ProfileScope _prof_scope_##__LINE__(name, false)
#define PROFILE_SUBSYSTEM(name) ProfileScope _prof_subsys_##__LINE__(name, true)

#endif // RAYNEENGINE_PROFILER_H
