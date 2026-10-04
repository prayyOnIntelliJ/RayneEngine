#include "ProfilerPanel.h"
#include <SFML/Graphics/VertexArray.hpp>
#include <iomanip>
#include <sstream>
#include <cmath>

static const sf::Color C_BG_DARK(18, 20, 25);
static const sf::Color C_BG_CARD(26, 29, 38);
static const sf::Color C_BG_HOVER(36, 40, 52);
static const sf::Color C_BORDER(46, 52, 68);
static const sf::Color C_BORDER_LIGHT(70, 78, 100);
static const sf::Color C_TEXT_PRI(240, 242, 248);
static const sf::Color C_TEXT_SEC(160, 168, 185);
static const sf::Color C_TEXT_MUT(110, 118, 135);
static const sf::Color C_ACCENT(59, 130, 246);

// Subsystem colors
static const sf::Color COL_SCRIPTS(74, 222, 128);  // Green
static const sf::Color COL_PHYSICS(251, 146, 60);  // Orange
static const sf::Color COL_RENDER(56, 189, 248);   // Cyan/Light Blue
static const sf::Color COL_AUDIO(192, 132, 252);   // Purple
static const sf::Color COL_ENGINE(148, 163, 184);  // Slate Gray

ProfilerPanel::ProfilerPanel(const sf::Font &font)
    : m_Font(font)
{
}

void ProfilerPanel::HandleEvent(const sf::Event &event, sf::Vector2f mouseScreenPos)
{
    m_MousePos = mouseScreenPos;

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
    {
        if (m_PauseBtnBounds.contains(mouseScreenPos))
        {
            Profiler::Get().SetPaused(!Profiler::Get().IsPaused());
            if (!Profiler::Get().IsPaused())
            {
                m_SelectedFrameIndex = -1;
            }
            return;
        }

        if (m_ClearBtnBounds.contains(mouseScreenPos))
        {
            Profiler::Get().ClearHistory();
            m_SelectedFrameIndex = -1;
            return;
        }

        if (m_LiveModeBtnBounds.contains(mouseScreenPos))
        {
            m_SelectedFrameIndex = -1;
            return;
        }

        if (m_HudToggleBtnBounds.contains(mouseScreenPos))
        {
            Profiler::Get().ToggleHud();
            return;
        }

        // Click on graph to select frame
        if (m_GraphBounds.contains(mouseScreenPos) && m_HoveredFrameIndex >= 0)
        {
            m_SelectedFrameIndex = m_HoveredFrameIndex;
            return;
        }
    }
}

void ProfilerPanel::Render(sf::RenderWindow &window, float x, float y, float width, float height)
{
    m_Bounds = sf::FloatRect(x, y, width, height);

    // Main background
    sf::RectangleShape bg({width, height});
    bg.setPosition(x, y);
    bg.setFillColor(C_BG_DARK);
    bg.setOutlineColor(C_BORDER);
    bg.setOutlineThickness(1.f);
    window.draw(bg);

    const auto &history = Profiler::Get().GetHistory();
    const bool isPaused = Profiler::Get().IsPaused();

    // 1. Toolbar (Top ~32px)
    const float toolbarH = 32.f;
    sf::RectangleShape toolbar({width, toolbarH});
    toolbar.setPosition(x, y);
    toolbar.setFillColor(C_BG_CARD);
    window.draw(toolbar);

    sf::RectangleShape toolbarBorder({width, 1.f});
    toolbarBorder.setPosition(x, y + toolbarH);
    toolbarBorder.setFillColor(C_BORDER);
    window.draw(toolbarBorder);

    float curX = x + 10.f;
    const float curY = y + 5.f;

    // Status pill
    std::string statusText = isPaused ? "PAUSED" : "RECORDING";
    sf::Color statusColor = isPaused ? sf::Color(245, 158, 11) : sf::Color(34, 197, 94);

    sf::RectangleShape statusPill({74.f, 22.f});
    statusPill.setPosition(curX, curY);
    statusPill.setFillColor(sf::Color(statusColor.r, statusColor.g, statusColor.b, 35));
    statusPill.setOutlineColor(statusColor);
    statusPill.setOutlineThickness(1.f);
    window.draw(statusPill);

    sf::Text sText;
    sText.setFont(m_Font);
    sText.setCharacterSize(10);
    sText.setStyle(sf::Text::Bold);
    sText.setFillColor(statusColor);
    sText.setString(statusText);
    sText.setPosition(curX + (74.f - sText.getLocalBounds().width) / 2.f, curY + 4.f);
    window.draw(sText);

    curX += 82.f;

    // Action buttons helper
    auto drawBtn = [&](const std::string &label, sf::FloatRect &outBounds, bool active, float btnW = 68.f) {
        outBounds = sf::FloatRect(curX, curY, btnW, 22.f);
        bool hov = outBounds.contains(m_MousePos);

        sf::RectangleShape btn({btnW, 22.f});
        btn.setPosition(curX, curY);
        btn.setFillColor(active ? sf::Color(C_ACCENT.r, C_ACCENT.g, C_ACCENT.b, 60) :
                         hov ? C_BG_HOVER : C_BG_CARD);
        btn.setOutlineColor(active ? C_ACCENT : hov ? C_BORDER_LIGHT : C_BORDER);
        btn.setOutlineThickness(1.f);
        window.draw(btn);

        sf::Text txt;
        txt.setFont(m_Font);
        txt.setCharacterSize(10);
        txt.setFillColor(active ? sf::Color::White : hov ? sf::Color::White : C_TEXT_SEC);
        txt.setString(label);
        txt.setPosition(curX + (btnW - txt.getLocalBounds().width) / 2.f, curY + 4.f);
        window.draw(txt);

        curX += btnW + 6.f;
    };

    drawBtn(isPaused ? "Resume" : "Pause", m_PauseBtnBounds, isPaused, 62.f);
    drawBtn("Live", m_LiveModeBtnBounds, m_SelectedFrameIndex == -1, 46.f);
    drawBtn("Clear", m_ClearBtnBounds, false, 50.f);

    bool hudActive = Profiler::Get().IsHudVisible();
    drawBtn(hudActive ? "F3 HUD: ON" : "F3 HUD: OFF", m_HudToggleBtnBounds, hudActive, 80.f);

    // Summary Metrics in toolbar (right side)
    std::ostringstream statsStream;
    statsStream << std::fixed << std::setprecision(1);
    statsStream << "FPS: " << Profiler::Get().GetCurrentFPS()
                << "  |  Avg: " << Profiler::Get().GetAvgFPS()
                << " (Min: " << Profiler::Get().GetMinFPS() << ", Max: " << Profiler::Get().GetMaxFPS() << ")"
                << "  |  Frame: " << Profiler::Get().GetCurrentFrameTimeMs() << " ms"
                << " (Avg: " << Profiler::Get().GetAvgFrameTimeMs() << " ms)";

    sf::Text summaryText;
    summaryText.setFont(m_Font);
    summaryText.setCharacterSize(10);
    summaryText.setFillColor(C_TEXT_PRI);
    summaryText.setString(statsStream.str());

    float summaryW = summaryText.getLocalBounds().width;
    float sumX = std::max(curX + 12.f, x + width - summaryW - 12.f);
    summaryText.setPosition(sumX, curY + 4.f);
    window.draw(summaryText);

    // 2. Main Area Layout: Left = Graph (58%), Right = Subsystem & Stats Details (42%)
    const float contentY = y + toolbarH + 8.f;
    const float contentH = height - toolbarH - 14.f;
    const float leftW = (width - 24.f) * 0.58f;
    const float rightW = (width - 24.f) * 0.42f;
    const float leftX = x + 8.f;
    const float rightX = leftX + leftW + 8.f;

    // --- LEFT PANEL: FRAME TIME GRAPH ---
    sf::RectangleShape graphBox({leftW, contentH});
    graphBox.setPosition(leftX, contentY);
    graphBox.setFillColor(C_BG_CARD);
    graphBox.setOutlineColor(C_BORDER);
    graphBox.setOutlineThickness(1.f);
    window.draw(graphBox);

    const float graphPadX = 36.f;
    const float graphPadY = 16.f;
    const float gInnerX = leftX + graphPadX;
    const float gInnerY = contentY + graphPadY;
    const float gInnerW = leftW - graphPadX - 10.f;
    const float gInnerH = contentH - graphPadY * 2.f;

    m_GraphBounds = sf::FloatRect(gInnerX, gInnerY, gInnerW, gInnerH);

    // Graph background area
    sf::RectangleShape innerGraph({gInnerW, gInnerH});
    innerGraph.setPosition(gInnerX, gInnerY);
    innerGraph.setFillColor(sf::Color(12, 14, 18, 220));
    window.draw(innerGraph);

    // Target reference lines: 16.6ms (60 FPS) and 33.3ms (30 FPS)
    auto drawTargetLine = [&](float msVal, const std::string &lbl, sf::Color col) {
        float lineY = gInnerY + gInnerH - (msVal / m_MaxGraphMs) * gInnerH;
        if (lineY >= gInnerY && lineY <= gInnerY + gInnerH)
        {
            sf::RectangleShape line({gInnerW, 1.f});
            line.setPosition(gInnerX, lineY);
            line.setFillColor(sf::Color(col.r, col.g, col.b, 100));
            window.draw(line);

            sf::Text t;
            t.setFont(m_Font);
            t.setCharacterSize(9);
            t.setFillColor(col);
            t.setString(lbl);
            t.setPosition(leftX + 4.f, lineY - 6.f);
            window.draw(t);
        }
    };

    drawTargetLine(16.66f, "16.6ms", sf::Color(34, 197, 94));
    drawTargetLine(33.33f, "33.3ms", sf::Color(245, 158, 11));

    // Zero line
    sf::Text zeroText;
    zeroText.setFont(m_Font);
    zeroText.setCharacterSize(9);
    zeroText.setFillColor(C_TEXT_MUT);
    zeroText.setString("0ms");
    zeroText.setPosition(leftX + 10.f, gInnerY + gInnerH - 6.f);
    window.draw(zeroText);

    // Render bars / waveform for frames
    m_HoveredFrameIndex = -1;
    if (!history.empty())
    {
        const size_t numFrames = history.size();
        const float colW = std::max(1.f, gInnerW / static_cast<float>(numFrames));

        for (size_t i = 0; i < numFrames; ++i)
        {
            const auto &frame = history[i];
            float fMs = std::min(frame.totalFrameTimeMs, m_MaxGraphMs);
            float barH = (fMs / m_MaxGraphMs) * gInnerH;
            float bx = gInnerX + static_cast<float>(i) * colW;
            float by = gInnerY + gInnerH - barH;

            // Check hover
            sf::FloatRect colRect(bx, gInnerY, colW, gInnerH);
            if (colRect.contains(m_MousePos))
            {
                m_HoveredFrameIndex = static_cast<int>(i);
            }

            bool isSelected = (m_SelectedFrameIndex == static_cast<int>(i));
            bool isHovered = (m_HoveredFrameIndex == static_cast<int>(i));

            sf::Color col = (frame.totalFrameTimeMs > 25.0f) ? sf::Color(239, 68, 68) :
                            (frame.totalFrameTimeMs > 17.5f) ? sf::Color(245, 158, 11) :
                            sf::Color(56, 189, 248);

            if (isSelected) col = sf::Color(255, 255, 255);
            else if (isHovered) col = sf::Color(col.r + 30, col.g + 30, col.b + 30);

            sf::RectangleShape bar({std::max(1.f, colW - 0.5f), barH});
            bar.setPosition(bx, by);
            bar.setFillColor(col);
            window.draw(bar);

            if (isSelected || isHovered)
            {
                // Vertical selection needle
                sf::RectangleShape needle({1.f, gInnerH});
                needle.setPosition(bx + colW / 2.f, gInnerY);
                needle.setFillColor(isSelected ? sf::Color::White : sf::Color(200, 200, 200, 160));
                window.draw(needle);
            }
        }
    }

    // Graph Title / Header
    sf::Text gTitle;
    gTitle.setFont(m_Font);
    gTitle.setCharacterSize(10);
    gTitle.setStyle(sf::Text::Bold);
    gTitle.setFillColor(C_TEXT_SEC);
    std::string inspectInfo = (m_SelectedFrameIndex >= 0) ? " (INSPECTING FRAME #" + std::to_string(m_SelectedFrameIndex) + ")" : " (LIVE STREAM)";
    gTitle.setString("FRAME TIME HISTORY (ms)" + inspectInfo);
    gTitle.setPosition(leftX + 8.f, contentY + 3.f);
    window.draw(gTitle);

    // --- RIGHT PANEL: SUBSYSTEM BREAKDOWN & INSPECTED DETAILS ---
    sf::RectangleShape rightBox({rightW, contentH});
    rightBox.setPosition(rightX, contentY);
    rightBox.setFillColor(C_BG_CARD);
    rightBox.setOutlineColor(C_BORDER);
    rightBox.setOutlineThickness(1.f);
    window.draw(rightBox);

    // Determine which frame to inspect:
    // If hovering, inspect hovered. Else if selected, inspect selected. Else latest.
    int activeFrameIdx = -1;
    if (m_HoveredFrameIndex >= 0) activeFrameIdx = m_HoveredFrameIndex;
    else if (m_SelectedFrameIndex >= 0 && m_SelectedFrameIndex < static_cast<int>(history.size())) activeFrameIdx = m_SelectedFrameIndex;
    else if (!history.empty()) activeFrameIdx = static_cast<int>(history.size()) - 1;

    FrameProfile inspectFrame = (activeFrameIdx >= 0 && activeFrameIdx < static_cast<int>(history.size())) ? history[activeFrameIdx] : Profiler::Get().GetLatestFrame();

    float rY = contentY + 6.f;
    const float rPadX = rightX + 12.f;

    sf::Text bTitle;
    bTitle.setFont(m_Font);
    bTitle.setCharacterSize(11);
    bTitle.setStyle(sf::Text::Bold);
    bTitle.setFillColor(C_TEXT_PRI);
    bTitle.setString("SUBSYSTEM BREAKDOWN");
    bTitle.setPosition(rPadX, rY);
    window.draw(bTitle);

    rY += 20.f;

    // Stacked Horizontal Bar Chart
    const float barChartW = rightW - 24.f;
    const float barChartH = 14.f;

    sf::RectangleShape barChartBg({barChartW, barChartH});
    barChartBg.setPosition(rPadX, rY);
    barChartBg.setFillColor(sf::Color(10, 12, 16));
    barChartBg.setOutlineColor(C_BORDER);
    barChartBg.setOutlineThickness(1.f);
    window.draw(barChartBg);

    float totalMs = std::max(0.001f, inspectFrame.totalFrameTimeMs);
    float curBarX = rPadX;

    auto drawBarSegment = [&](float valMs, sf::Color col) {
        if (valMs <= 0.0f) return;
        float segW = (valMs / totalMs) * barChartW;
        sf::RectangleShape seg({segW, barChartH});
        seg.setPosition(curBarX, rY);
        seg.setFillColor(col);
        window.draw(seg);
        curBarX += segW;
    };

    drawBarSegment(inspectFrame.subsystems.scriptsMs, COL_SCRIPTS);
    drawBarSegment(inspectFrame.subsystems.physicsMs, COL_PHYSICS);
    drawBarSegment(inspectFrame.subsystems.renderingMs, COL_RENDER);
    drawBarSegment(inspectFrame.subsystems.audioMs, COL_AUDIO);
    drawBarSegment(inspectFrame.subsystems.engineMs, COL_ENGINE);

    rY += barChartH + 10.f;

    // Breakdown Table Rows
    auto drawTableRow = [&](const std::string &catName, float valMs, sf::Color col) {
        // Color dot
        sf::CircleShape dot(3.5f);
        dot.setPosition(rPadX, rY + 4.f);
        dot.setFillColor(col);
        window.draw(dot);

        // Name
        sf::Text nText;
        nText.setFont(m_Font);
        nText.setCharacterSize(10);
        nText.setFillColor(C_TEXT_PRI);
        nText.setString(catName);
        nText.setPosition(rPadX + 14.f, rY);
        window.draw(nText);

        // Value & Percentage
        float pct = (valMs / totalMs) * 100.0f;
        std::ostringstream vs;
        vs << std::fixed << std::setprecision(2) << valMs << " ms (" << std::setprecision(1) << pct << "%)";

        sf::Text vText;
        vText.setFont(m_Font);
        vText.setCharacterSize(10);
        vText.setFillColor(col);
        vText.setString(vs.str());
        vText.setPosition(rPadX + 110.f, rY);
        window.draw(vText);

        rY += 15.f;
    };

    drawTableRow("Scripts / Lua", inspectFrame.subsystems.scriptsMs, COL_SCRIPTS);
    drawTableRow("Physics", inspectFrame.subsystems.physicsMs, COL_PHYSICS);
    drawTableRow("Rendering", inspectFrame.subsystems.renderingMs, COL_RENDER);
    drawTableRow("Audio", inspectFrame.subsystems.audioMs, COL_AUDIO);
    drawTableRow("Engine / Other", inspectFrame.subsystems.engineMs, COL_ENGINE);

    // Separator line
    rY += 4.f;
    sf::RectangleShape sep({barChartW, 1.f});
    sep.setPosition(rPadX, rY);
    sep.setFillColor(C_BORDER);
    window.draw(sep);
    rY += 6.f;

    // Stats Grid
    sf::Text sHeader;
    sHeader.setFont(m_Font);
    sHeader.setCharacterSize(10);
    sHeader.setStyle(sf::Text::Bold);
    sHeader.setFillColor(C_TEXT_SEC);
    sHeader.setString("FRAME METRICS");
    sHeader.setPosition(rPadX, rY);
    window.draw(sHeader);

    rY += 15.f;

    auto drawStatField = [&](const std::string &key, const std::string &val, float offX = 0.f) {
        sf::Text k;
        k.setFont(m_Font);
        k.setCharacterSize(10);
        k.setFillColor(C_TEXT_MUT);
        k.setString(key + ":");
        k.setPosition(rPadX + offX, rY);
        window.draw(k);

        sf::Text v;
        v.setFont(m_Font);
        v.setCharacterSize(10);
        v.setStyle(sf::Text::Bold);
        v.setFillColor(C_TEXT_PRI);
        v.setString(val);
        v.setPosition(rPadX + offX + k.getLocalBounds().width + 6.f, rY);
        window.draw(v);
    };

    drawStatField("Draw Calls", std::to_string(inspectFrame.drawCalls), 0.f);
    drawStatField("Entities", std::to_string(inspectFrame.entityCount), 110.f);
    if (inspectFrame.luaMemoryKb > 0.0f)
    {
        drawStatField("Lua Mem", std::to_string(static_cast<int>(inspectFrame.luaMemoryKb)) + " KB", 200.f);
    }

    rY += 16.f;

    // Custom samples (if any)
    if (!inspectFrame.customSamples.empty())
    {
        sf::Text csHeader;
        csHeader.setFont(m_Font);
        csHeader.setCharacterSize(9);
        csHeader.setStyle(sf::Text::Bold);
        csHeader.setFillColor(sf::Color(250, 204, 21));
        csHeader.setString("CUSTOM SAMPLES:");
        csHeader.setPosition(rPadX, rY);
        window.draw(csHeader);
        rY += 12.f;

        for (const auto &cs : inspectFrame.customSamples)
        {
            if (rY > contentY + contentH - 12.f) break;

            std::ostringstream csStream;
            csStream << cs.name << ": " << std::fixed << std::setprecision(2) << cs.durationMs << " ms";
            sf::Text csText;
            csText.setFont(m_Font);
            csText.setCharacterSize(9);
            csText.setFillColor(sf::Color(254, 240, 138));
            csText.setString(csStream.str());
            csText.setPosition(rPadX + 6.f, rY);
            window.draw(csText);
            rY += 11.f;
        }
    }
}
