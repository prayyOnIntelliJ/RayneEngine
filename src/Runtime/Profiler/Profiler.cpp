#include "Profiler.h"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <sstream>

Profiler::Profiler()
{
    m_History.reserve(MAX_HISTORY);
    m_FrameStartTime = std::chrono::high_resolution_clock::now();
}

void Profiler::BeginFrame()
{
    m_FrameStartTime = std::chrono::high_resolution_clock::now();
    m_CurrentFrame = FrameProfile{};
    m_CurrentFrame.drawCalls = m_CurrentDrawCalls;
    m_CurrentDrawCalls = 0;
}

void Profiler::EndFrame()
{
    auto now = std::chrono::high_resolution_clock::now();
    float frameMs = std::chrono::duration<float, std::milli>(now - m_FrameStartTime).count();

    // Prevent 0 ms division
    if (frameMs < 0.0001f) frameMs = 0.0001f;

    m_CurrentFrame.totalFrameTimeMs = frameMs;
    m_CurrentFrame.fps = 1000.0f / frameMs;

    // Attribute unaccounted time to engine
    float accountedSubsystems = m_CurrentFrame.subsystems.scriptsMs +
                                m_CurrentFrame.subsystems.physicsMs +
                                m_CurrentFrame.subsystems.renderingMs +
                                m_CurrentFrame.subsystems.audioMs;

    if (accountedSubsystems < frameMs)
    {
        m_CurrentFrame.subsystems.engineMs = frameMs - accountedSubsystems;
    }

    m_CurrentFPS = m_CurrentFrame.fps;
    m_CurrentFrameTimeMs = frameMs;

    if (!m_Paused)
    {
        if (m_History.size() >= MAX_HISTORY)
        {
            m_History.erase(m_History.begin());
        }
        m_History.push_back(m_CurrentFrame);

        // Calculate statistics over history
        if (!m_History.empty())
        {
            float totalMs = 0.0f;
            float totalFps = 0.0f;
            m_MinFPS = 999999.0f;
            m_MaxFPS = 0.0f;
            m_MinFrameTimeMs = 999999.0f;
            m_MaxFrameTimeMs = 0.0f;

            for (const auto &f : m_History)
            {
                totalMs += f.totalFrameTimeMs;
                totalFps += f.fps;
                if (f.fps < m_MinFPS) m_MinFPS = f.fps;
                if (f.fps > m_MaxFPS) m_MaxFPS = f.fps;
                if (f.totalFrameTimeMs < m_MinFrameTimeMs) m_MinFrameTimeMs = f.totalFrameTimeMs;
                if (f.totalFrameTimeMs > m_MaxFrameTimeMs) m_MaxFrameTimeMs = f.totalFrameTimeMs;
            }

            m_AvgFPS = totalFps / static_cast<float>(m_History.size());
            m_AvgFrameTimeMs = totalMs / static_cast<float>(m_History.size());
        }
    }
}

void Profiler::BeginSubsystem(const std::string &name)
{
    m_ActiveSubsystems[name] = std::chrono::high_resolution_clock::now();
}

void Profiler::EndSubsystem(const std::string &name)
{
    auto it = m_ActiveSubsystems.find(name);
    if (it == m_ActiveSubsystems.end()) return;

    auto now = std::chrono::high_resolution_clock::now();
    float durMs = std::chrono::duration<float, std::milli>(now - it->second).count();
    m_ActiveSubsystems.erase(it);

    if (name == "Scripting" || name == "Scripts" || name == "Lua")
    {
        m_CurrentFrame.subsystems.scriptsMs += durMs;
    }
    else if (name == "Physics" || name == "Collision")
    {
        m_CurrentFrame.subsystems.physicsMs += durMs;
    }
    else if (name == "Rendering" || name == "Render")
    {
        m_CurrentFrame.subsystems.renderingMs += durMs;
    }
    else if (name == "Audio")
    {
        m_CurrentFrame.subsystems.audioMs += durMs;
    }
    else
    {
        m_CurrentFrame.subsystems.engineMs += durMs;
    }
}

void Profiler::BeginSample(const std::string &name)
{
    m_ActiveSamples[name] = std::chrono::high_resolution_clock::now();
}

void Profiler::EndSample(const std::string &name)
{
    auto it = m_ActiveSamples.find(name);
    if (it == m_ActiveSamples.end()) return;

    auto now = std::chrono::high_resolution_clock::now();
    float durMs = std::chrono::duration<float, std::milli>(now - it->second).count();
    m_ActiveSamples.erase(it);

    // Merge or add to custom samples
    bool found = false;
    for (auto &sample : m_CurrentFrame.customSamples)
    {
        if (sample.name == name)
        {
            sample.durationMs += durMs;
            found = true;
            break;
        }
    }
    if (!found)
    {
        m_CurrentFrame.customSamples.push_back({name, durMs});
    }
}

void Profiler::RecordDrawCall(int count)
{
    m_CurrentDrawCalls += count;
}

void Profiler::SetEntityCount(int count)
{
    m_CurrentFrame.entityCount = count;
}

void Profiler::SetLuaMemory(float kb)
{
    m_CurrentFrame.luaMemoryKb = kb;
}

const FrameProfile &Profiler::GetLatestFrame() const
{
    if (m_History.empty()) return m_EmptyProfile;
    return m_History.back();
}

void Profiler::ClearHistory()
{
    m_History.clear();
    m_MinFPS = 0.0f;
    m_MaxFPS = 0.0f;
    m_AvgFPS = 0.0f;
    m_MinFrameTimeMs = 0.0f;
    m_MaxFrameTimeMs = 0.0f;
    m_AvgFrameTimeMs = 0.0f;
}

void Profiler::RenderHud(sf::RenderWindow &window, const sf::Font &font)
{
    if (!m_ShowHud) return;

    const sf::View currentView = window.getView();
    window.setView(window.getDefaultView());

    const float pad = 10.f;
    const float w = 260.f;
    const float h = 180.f;
    const float x = window.getSize().x - w - pad;
    const float y = pad;

    // Background panel
    sf::RectangleShape panel({w, h});
    panel.setPosition(x, y);
    panel.setFillColor(sf::Color(16, 18, 24, 215));
    panel.setOutlineColor(sf::Color(60, 70, 90, 200));
    panel.setOutlineThickness(1.f);
    window.draw(panel);

    // Header
    sf::RectangleShape header({w, 22.f});
    header.setPosition(x, y);
    header.setFillColor(sf::Color(28, 32, 44, 240));
    window.draw(header);

    sf::Text headerText;
    headerText.setFont(font);
    headerText.setCharacterSize(10);
    headerText.setStyle(sf::Text::Bold);
    headerText.setFillColor(sf::Color(100, 200, 255));
    headerText.setString("PROFILER HUD [F3]");
    headerText.setPosition(x + 8.f, y + 4.f);
    window.draw(headerText);

    // FPS & Frametime
    sf::Color fpsColor = sf::Color(80, 230, 100);
    if (m_CurrentFPS < 30.0f) fpsColor = sf::Color(240, 70, 70);
    else if (m_CurrentFPS < 55.0f) fpsColor = sf::Color(245, 190, 40);

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << m_CurrentFPS << " FPS (" << m_CurrentFrameTimeMs << " ms)";

    sf::Text fpsText;
    fpsText.setFont(font);
    fpsText.setCharacterSize(13);
    fpsText.setStyle(sf::Text::Bold);
    fpsText.setFillColor(fpsColor);
    fpsText.setString(ss.str());
    fpsText.setPosition(x + 8.f, y + 26.f);
    window.draw(fpsText);

    // Mini Sparkline Graph
    const float graphX = x + 8.f;
    const float graphY = y + 48.f;
    const float graphW = w - 16.f;
    const float graphH = 34.f;

    sf::RectangleShape graphBg({graphW, graphH});
    graphBg.setPosition(graphX, graphY);
    graphBg.setFillColor(sf::Color(10, 12, 16, 200));
    graphBg.setOutlineColor(sf::Color(45, 52, 68));
    graphBg.setOutlineThickness(1.f);
    window.draw(graphBg);

    // 16.6ms target line (60 FPS)
    const float maxGraphMs = 35.0f;
    float line60Y = graphY + graphH - (16.66f / maxGraphMs) * graphH;
    if (line60Y >= graphY && line60Y <= graphY + graphH)
    {
        sf::RectangleShape line60({graphW, 1.f});
        line60.setPosition(graphX, line60Y);
        line60.setFillColor(sf::Color(60, 160, 80, 120));
        window.draw(line60);
    }

    if (m_History.size() >= 2)
    {
        size_t samples = std::min(m_History.size(), static_cast<size_t>(60));
        size_t startIdx = m_History.size() - samples;
        float stepX = graphW / static_cast<float>(samples - 1);

        sf::VertexArray line(sf::LineStrip, samples);
        for (size_t i = 0; i < samples; ++i)
        {
            float fMs = m_History[startIdx + i].totalFrameTimeMs;
            float clampedMs = std::min(fMs, maxGraphMs);
            float py = graphY + graphH - (clampedMs / maxGraphMs) * graphH;
            float px = graphX + static_cast<float>(i) * stepX;

            sf::Color ptColor = (fMs > 20.0f) ? sf::Color(240, 80, 80) : sf::Color(80, 200, 240);
            line[i].position = sf::Vector2f(px, py);
            line[i].color = ptColor;
        }
        window.draw(line);
    }

    // Subsystems
    const auto &latest = GetLatestFrame();
    float rowY = graphY + graphH + 6.f;

    auto drawSubRow = [&](const std::string &label, float valMs, sf::Color col) {
        sf::Text lText;
        lText.setFont(font);
        lText.setCharacterSize(10);
        lText.setFillColor(col);
        lText.setString(label);
        lText.setPosition(x + 8.f, rowY);
        window.draw(lText);

        std::ostringstream valStream;
        valStream << std::fixed << std::setprecision(2) << valMs << " ms";
        sf::Text vText;
        vText.setFont(font);
        vText.setCharacterSize(10);
        vText.setFillColor(sf::Color(220, 220, 230));
        vText.setString(valStream.str());
        vText.setPosition(x + 95.f, rowY);
        window.draw(vText);

        rowY += 13.f;
    };

    drawSubRow("Scripts / Lua:", latest.subsystems.scriptsMs, sf::Color(100, 220, 120));
    drawSubRow("Physics:", latest.subsystems.physicsMs, sf::Color(240, 160, 60));
    drawSubRow("Rendering:", latest.subsystems.renderingMs, sf::Color(80, 200, 255));
    drawSubRow("Engine / Other:", latest.subsystems.engineMs, sf::Color(160, 170, 190));

    // Stats footer
    rowY += 2.f;
    std::ostringstream statsStream;
    statsStream << "Draws: " << latest.drawCalls << " | Entities: " << latest.entityCount;
    if (latest.luaMemoryKb > 0.0f)
    {
        statsStream << " | Lua: " << static_cast<int>(latest.luaMemoryKb) << "KB";
    }

    sf::Text statsText;
    statsText.setFont(font);
    statsText.setCharacterSize(9);
    statsText.setFillColor(sf::Color(140, 150, 170));
    statsText.setString(statsStream.str());
    statsText.setPosition(x + 8.f, rowY);
    window.draw(statsText);

    window.setView(currentView);
}

// Scoped Timer Implementation
ProfileScope::ProfileScope(std::string scopeName, bool subsystem)
    : name(std::move(scopeName)), isSubsystem(subsystem)
{
    if (isSubsystem)
        Profiler::Get().BeginSubsystem(name);
    else
        Profiler::Get().BeginSample(name);
}

ProfileScope::~ProfileScope()
{
    if (isSubsystem)
        Profiler::Get().EndSubsystem(name);
    else
        Profiler::Get().EndSample(name);
}
