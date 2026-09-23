#include "ConsolePanel.h"
#include <algorithm>
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

#include "../Application/Application.h"
#include "../Scripting/LuaState.h"

#include "SFML/Window/Clipboard.hpp"

static const sf::Color C_BG_PANEL = sf::Color(26, 29, 34);
static const sf::Color C_BG_INPUT = sf::Color(20, 23, 27);
static const sf::Color C_BORDER = sf::Color(42, 46, 53);
static const sf::Color C_BORDER_LIGHT = sf::Color(58, 63, 72);
static const sf::Color C_TEXT_PRIMARY = sf::Color(232, 234, 237);
static const sf::Color C_TEXT_MUTED = sf::Color(92, 97, 107);
static const sf::Color C_DANGER = sf::Color(241, 104, 94);
static const sf::Color C_ACCENT = sf::Color(124, 108, 240);
static const sf::Color C_ACCENT_HOV = sf::Color(146, 132, 245);

ConsoleRedirector::ConsoleRedirector(std::ostream& stream, std::function<void(const std::string&, bool)> callback, bool isError)
    : m_Stream(stream), m_Callback(std::move(callback)), m_IsError(isError)
{
    m_OldBuf = m_Stream.rdbuf(this);
}

ConsoleRedirector::~ConsoleRedirector()
{
    m_Stream.rdbuf(m_OldBuf);
}

std::streambuf::int_type ConsoleRedirector::overflow(int_type v)
{
    if (v == '\n')
    {
        if (m_Callback) m_Callback(m_Buffer, m_IsError);
        m_Buffer.clear();
    }
    else if (v != std::char_traits<char>::eof())
    {
        m_Buffer += static_cast<char>(v);
    }
    
    if (m_OldBuf && v != std::char_traits<char>::eof()) {
        m_OldBuf->sputc(v);
    }
    
    return v;
}

std::streamsize ConsoleRedirector::xsputn(const char* p, std::streamsize n)
{
    for (std::streamsize i = 0; i < n; ++i)
    {
        if (p[i] == '\n')
        {
            if (m_Callback) m_Callback(m_Buffer, m_IsError);
            m_Buffer.clear();
        }
        else
        {
            m_Buffer += p[i];
        }
    }
    
    if (m_OldBuf) {
        m_OldBuf->sputn(p, n);
    }
    
    return n;
}


std::vector<ConsoleMessage> ConsolePanel::s_Messages;
std::mutex ConsolePanel::s_Mutex;
std::unique_ptr<ConsoleRedirector> ConsolePanel::s_CoutRedirector;
std::unique_ptr<ConsoleRedirector> ConsolePanel::s_CerrRedirector;
bool ConsolePanel::s_Initialized = false;

void ConsolePanel::InitRedirectors()
{
    if (s_Initialized) return;
    
    auto callback = [](const std::string& msg, bool isError) {
        ConsolePanel::AddLogGlobal(msg, isError);
    };

    s_CoutRedirector = std::make_unique<ConsoleRedirector>(std::cout, callback, false);
    s_CerrRedirector = std::make_unique<ConsoleRedirector>(std::cerr, callback, true);
    
    s_Initialized = true;
    AddLogGlobal("Console initialized.", false);
}

void ConsolePanel::AddLogGlobal(const std::string& message, bool isError)
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_Messages.push_back({message, isError});
    if (s_Messages.size() > 2000)
    {
        s_Messages.erase(s_Messages.begin());
    }
}

ConsolePanel::ConsolePanel(const sf::Font& font)
    : m_Font(font)
{
    m_ScrollOffset = 999999.f; 
}

ConsolePanel::~ConsolePanel()
{
}

void ConsolePanel::ExecuteCommand(const std::string& rawCommand)
{
    if (rawCommand.empty()) return;

    size_t first = rawCommand.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return;
    size_t last = rawCommand.find_last_not_of(" \t\r\n");
    std::string command = rawCommand.substr(first, (last - first + 1));
    if (command.empty()) return;

    AddLogGlobal("> " + command, false);

    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;
    std::string lowerCmd = cmd;
    std::transform(lowerCmd.begin(), lowerCmd.end(), lowerCmd.begin(), ::tolower);

    std::string args;
    std::getline(iss, args);
    size_t argsFirst = args.find_first_not_of(" \t");
    if (argsFirst != std::string::npos) {
        args = args.substr(argsFirst);
    } else {
        args.clear();
    }

    if (lowerCmd == "clear" || lowerCmd == "cls")
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        s_Messages.clear();
        m_ScrollOffset = 0.f;
    }
    else if (lowerCmd == "help")
    {
        AddLogGlobal("=== Available Commands ===", false);
        AddLogGlobal("  help                  - List all available commands", false);
        AddLogGlobal("  quit / exit           - Quit game (or return to editor)", false);
        AddLogGlobal("  restart / reload      - Restart the current active scene", false);
        AddLogGlobal("  scene load <name>     - Load a scene by name", false);
        AddLogGlobal("  pause [on|off]        - Toggle or set simulation pause", false);
        AddLogGlobal("  timescale <value>     - Set simulation time scale (e.g. 0.5, 1.0, 2.0)", false);
        AddLogGlobal("  fps [on|off|toggle]   - Show current FPS or toggle on-screen overlay", false);
        AddLogGlobal("  screenshot [name]     - Save screenshot to screenshots/", false);
        AddLogGlobal("  fullscreen [on|off]   - Toggle or set fullscreen mode", false);
        AddLogGlobal("  cursor [show|hide]    - Toggle or set mouse cursor visibility", false);
        AddLogGlobal("  openurl <url>         - Open URL in default browser", false);
        AddLogGlobal("  clear / cls           - Clear the console window", false);
        AddLogGlobal("  lua <code>            - Execute arbitrary Lua script code", false);
        AddLogGlobal("==========================", false);
    }
    else if (lowerCmd == "quit" || lowerCmd == "exit")
    {
        if (g_App) {
            AddLogGlobal("Quitting application / Play mode...", false);
            g_App->Quit();
        } else {
            AddLogGlobal("[Error] Engine application instance not available", true);
        }
    }
    else if (lowerCmd == "restart" || lowerCmd == "reload")
    {
        if (g_App) {
            AddLogGlobal("Restarting current scene...", false);
            g_App->RestartCurrentScene();
        } else {
            AddLogGlobal("[Error] Engine application instance not available", true);
        }
    }
    else if (lowerCmd == "scene")
    {
        std::istringstream aiss(args);
        std::string subCmd;
        aiss >> subCmd;
        std::transform(subCmd.begin(), subCmd.end(), subCmd.begin(), ::tolower);
        if (subCmd == "load")
        {
            std::string sceneName;
            aiss >> sceneName;
            if (!sceneName.empty())
            {
                if (g_App) {
                    AddLogGlobal("Loading scene: " + sceneName + "...", false);
                    g_App->LoadGameScene(sceneName);
                }
            }
            else
            {
                AddLogGlobal("Usage: scene load <scene_name>", true);
            }
        }
        else
        {
            AddLogGlobal("Usage: scene load <scene_name>", true);
        }
    }
    else if (lowerCmd == "pause")
    {
        if (g_App) {
            if (args == "on" || args == "1" || args == "true") {
                g_App->SetPaused(true);
            } else if (args == "off" || args == "0" || args == "false") {
                g_App->SetPaused(false);
            } else {
                g_App->TogglePause();
            }
            AddLogGlobal(std::string("Simulation paused: ") + (g_App->IsPaused() ? "ON" : "OFF"), false);
        }
    }
    else if (lowerCmd == "timescale")
    {
        if (g_App) {
            if (!args.empty()) {
                try {
                    float ts = std::stof(args);
                    g_App->SetTimeScale(ts);
                    AddLogGlobal("TimeScale set to " + std::to_string(g_App->GetTimeScale()), false);
                } catch (...) {
                    AddLogGlobal("Usage: timescale <float_value> (e.g. timescale 0.5)", true);
                }
            } else {
                AddLogGlobal("Current TimeScale: " + std::to_string(g_App->GetTimeScale()), false);
            }
        }
    }
    else if (lowerCmd == "fps")
    {
        if (g_App) {
            if (args == "on" || args == "1" || args == "true") {
                g_App->SetShowFPSOverlay(true);
                AddLogGlobal("FPS overlay: ON", false);
            } else if (args == "off" || args == "0" || args == "false") {
                g_App->SetShowFPSOverlay(false);
                AddLogGlobal("FPS overlay: OFF", false);
            } else if (args == "toggle") {
                g_App->SetShowFPSOverlay(!g_App->IsFPSOverlayShown());
                AddLogGlobal(std::string("FPS overlay: ") + (g_App->IsFPSOverlayShown() ? "ON" : "OFF"), false);
            } else {
                char buf[128];
                std::snprintf(buf, sizeof(buf), "FPS: %.1f | Frame Time: %.2f ms | Overlay: %s",
                              g_App->GetFPS(), g_App->GetDeltaTime() * 1000.f,
                              g_App->IsFPSOverlayShown() ? "ON" : "OFF");
                AddLogGlobal(buf, false);
            }
        }
    }
    else if (lowerCmd == "screenshot")
    {
        if (g_App) {
            std::string path = g_App->TakeScreenshot(args);
            if (!path.empty()) {
                AddLogGlobal("Screenshot saved: " + path, false);
            } else {
                AddLogGlobal("[Error] Failed to capture screenshot", true);
            }
        }
    }
    else if (lowerCmd == "fullscreen")
    {
        if (g_App) {
            if (args == "on" || args == "1" || args == "true") {
                g_App->SetFullscreen(true);
            } else if (args == "off" || args == "0" || args == "false") {
                g_App->SetFullscreen(false);
            } else {
                g_App->ToggleFullscreen();
            }
            AddLogGlobal(std::string("Fullscreen: ") + (g_App->IsFullscreen() ? "ON" : "OFF"), false);
        }
    }
    else if (lowerCmd == "cursor")
    {
        if (g_App) {
            if (args == "show" || args == "on" || args == "1" || args == "true") {
                g_App->SetCursorVisible(true);
                AddLogGlobal("Cursor: Visible", false);
            } else if (args == "hide" || args == "off" || args == "0" || args == "false") {
                g_App->SetCursorVisible(false);
                AddLogGlobal("Cursor: Hidden", false);
            } else {
                AddLogGlobal("Usage: cursor <show|hide>", true);
            }
        }
    }
    else if (lowerCmd == "openurl")
    {
        if (g_App) {
            if (!args.empty()) {
                g_App->OpenURL(args);
                AddLogGlobal("Opening URL: " + args, false);
            } else {
                AddLogGlobal("Usage: openurl <url>", true);
            }
        }
    }
    else
    {
        // Execute as Lua code fallback
        std::string luaCode = (lowerCmd == "lua") ? args : command;
        if (!luaCode.empty())
        {
            auto& lua = LuaState::GetLua();
            sol::protected_function_result result = lua.safe_script(luaCode, sol::script_pass_on_error);
            if (!result.valid())
            {
                sol::error err = result;
                AddLogGlobal(std::string("[Lua Error] ") + err.what(), true);
            }
            else
            {
                if (result.return_count() > 0)
                {
                    sol::object obj = result[0];
                    if (obj.is<std::string>()) {
                        AddLogGlobal(obj.as<std::string>(), false);
                    } else if (obj.is<double>()) {
                        AddLogGlobal(std::to_string(obj.as<double>()), false);
                    } else if (obj.is<bool>()) {
                        AddLogGlobal(obj.as<bool>() ? "true" : "false", false);
                    } else if (obj.is<int>()) {
                        AddLogGlobal(std::to_string(obj.as<int>()), false);
                    }
                }
            }
        }
    }
}

void ConsolePanel::HandleEvent(const sf::Event& event, sf::Vector2f mouseScreenPos)
{
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
    {
        if (m_ScrollbarBounds.contains(mouseScreenPos))
        {
            m_ScrollbarDragging = true;
            m_DragStartY = mouseScreenPos.y;
            m_DragStartScroll = m_ScrollOffset;
        }
        else if (m_InputBounds.contains(mouseScreenPos))
        {
            m_InputActive = true;
        }
        else if (m_Bounds.contains(mouseScreenPos))
        {
            m_InputActive = false;
        }
    }

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
    {
        m_ScrollbarDragging = false;
    }

    if (event.type == sf::Event::MouseMoved && m_ScrollbarDragging)
    {
        float dy = mouseScreenPos.y - m_DragStartY;
        float logAreaHeight = m_Bounds.height - 30.f;
        float handleHeight = std::max(20.f, (logAreaHeight / (m_MaxScroll + logAreaHeight)) * logAreaHeight);
        float trackHeight = logAreaHeight - handleHeight;
        
        if (trackHeight > 0.f)
        {
            float scrollDelta = (dy / trackHeight) * m_MaxScroll;
            m_ScrollOffset = std::max(0.f, std::min(m_DragStartScroll + scrollDelta, m_MaxScroll));
            m_AutoScroll = (m_ScrollOffset >= m_MaxScroll - 1.f);
        }
    }

    if (event.type == sf::Event::MouseWheelScrolled && m_Bounds.contains(mouseScreenPos))
    {
        m_ScrollOffset -= event.mouseWheelScroll.delta * 40.f;
        m_ScrollOffset = std::max(0.f, std::min(m_ScrollOffset, m_MaxScroll));
        m_AutoScroll = (m_ScrollOffset >= m_MaxScroll - 1.f);
    }

    if (m_InputActive)
    {
        if (event.type == sf::Event::TextEntered)
        {
            if (event.text.unicode == 8)
            {
                if (!m_InputBuffer.empty())
                    m_InputBuffer.pop_back();
            }
            else if (event.text.unicode == 22)
            {
                m_InputBuffer += sf::Clipboard::getString().toAnsiString();
            }
            else if (event.text.unicode == 13)
            {
                ExecuteCommand(m_InputBuffer);
                m_InputBuffer.clear();
            }
            else if (event.text.unicode >= 32 && event.text.unicode < 127)
            {
                m_InputBuffer += static_cast<char>(event.text.unicode);
            }
        }
    }
}

void ConsolePanel::Render(sf::RenderWindow& window, float x, float y, float width, float height)
{
    m_Bounds = {x, y, width, height};

    sf::RectangleShape bg({width, height});
    bg.setPosition(x, y);
    bg.setFillColor(C_BG_PANEL);
    window.draw(bg);

    sf::RectangleShape topBorder({width, 1.f});
    topBorder.setPosition(x, y);
    topBorder.setFillColor(C_BORDER);
    window.draw(topBorder);

    float inputHeight = 30.f;
    m_InputBounds = {x, y + height - inputHeight, width, inputHeight};
    
    sf::RectangleShape inputBg({width, inputHeight});
    inputBg.setPosition(m_InputBounds.left, m_InputBounds.top);
    inputBg.setFillColor(C_BG_INPUT);
    window.draw(inputBg);
    
    sf::RectangleShape inputBorderTop({width, 1.f});
    inputBorderTop.setPosition(m_InputBounds.left, m_InputBounds.top);
    inputBorderTop.setFillColor(C_BORDER);
    window.draw(inputBorderTop);

    sf::Text inputText;
    inputText.setFont(m_Font);
    inputText.setCharacterSize(12);
    inputText.setFillColor(m_InputActive ? C_TEXT_PRIMARY : C_TEXT_MUTED);
    
    std::string dispText = "> " + m_InputBuffer;
    if (m_InputActive)
    {
        if (static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() / 500) % 2 == 0)
        {
            dispText += "|";
        }
    }
    else if (m_InputBuffer.empty())
    {
        dispText = "Type command here...";
        inputText.setFillColor(C_TEXT_MUTED);
    }
    
    inputText.setString(dispText);
    inputText.setPosition(x + 10.f, y + height - inputHeight + 8.f);
    window.draw(inputText);

    float logAreaHeight = height - inputHeight;
    
    std::lock_guard<std::mutex> lock(s_Mutex);
    
    float lh = 18.f;
    float contentHeight = 5.f + s_Messages.size() * lh;
    m_MaxScroll = std::max(0.f, contentHeight - logAreaHeight + 10.f);
    
    if (m_AutoScroll) {
        m_ScrollOffset = m_MaxScroll;
    }
    m_ScrollOffset = std::max(0.f, std::min(m_ScrollOffset, m_MaxScroll));
    m_LastMessageCount = s_Messages.size();

    sf::View oldView = window.getView();
    sf::View logView(sf::FloatRect(0.f, 0.f, width, logAreaHeight));
    logView.setViewport(sf::FloatRect(
        x / window.getSize().x, 
        y / window.getSize().y, 
        width / window.getSize().x, 
        logAreaHeight / window.getSize().y));
    window.setView(logView);

    float currentY = 5.f - m_ScrollOffset;

    sf::Text logText;
    logText.setFont(m_Font);
    logText.setCharacterSize(12);
    
    for (const auto& msg : s_Messages)
    {
        if (currentY + lh > 0 && currentY < logAreaHeight)
        {
            logText.setString(msg.text);
            logText.setFillColor(msg.isError ? C_DANGER : C_TEXT_PRIMARY);
            logText.setPosition(10.f, currentY);
            window.draw(logText);
        }
        
        currentY += lh;
    }
    
    window.setView(oldView);

    if (m_MaxScroll > 0.f)
    {
        float sbWidth = 10.f;
        float sbX = x + width - sbWidth - 2.f;
        float sbY = y + 2.f;
        float sbH = logAreaHeight - 4.f;
        
        m_ScrollbarBounds = {sbX - 5.f, sbY, sbWidth + 10.f, sbH};

        sf::RectangleShape track({sbWidth, sbH});
        track.setPosition(sbX, sbY);
        track.setFillColor(C_BG_INPUT);
        window.draw(track);
        
        float handleHeight = std::max(20.f, (logAreaHeight / (m_MaxScroll + logAreaHeight)) * sbH);
        float trackHeight = sbH - handleHeight;
        float handleY = sbY + (m_ScrollOffset / m_MaxScroll) * trackHeight;
        
        sf::RectangleShape handle({sbWidth, handleHeight});
        handle.setPosition(sbX, handleY);
        handle.setFillColor(m_ScrollbarDragging ? C_ACCENT : C_BORDER_LIGHT);
        handle.setOutlineColor(C_BORDER);
        handle.setOutlineThickness(1.f);
        window.draw(handle);
    }
}
