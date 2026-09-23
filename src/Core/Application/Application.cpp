#include "Application.h"
#include "SplashScreen.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include "../ECS/Components.h"
#include "../Input/InputManager.h"

#include "../Scenes/EditorScene.h"
#include "../Scenes/UIEditorScene.h"

#include "../Scenes/GameScene.h"
#include "../Scenes/SceneSerializer.h"
#include "../Scripting/LuaState.h"
#include "../Scripting/TimerManager.h"
#include "../Scripting/TweenManager.h"
#include "../Scripting/EventManager.h"
#include "SFML/Window/Event.hpp"
#include "../UI/UIManager.h"
#include "../Resources/ResourceManager.h"
#include "../Audio/AudioManager.h"
#include "SFML/Graphics/Texture.hpp"
#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/Text.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/System/Sleep.hpp"
#include "SFML/System/Time.hpp"

Application* g_App = nullptr;

BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT || dwCtrlType == CTRL_CLOSE_EVENT)
    {
        std::cout << "[INFO] [Application] Intercepted console stop signal (" << dwCtrlType << "). Forcing save...\n";
        if (g_App) g_App->GetSceneManager().Shutdown();
        return FALSE;
    }
    return FALSE;
}

Application::Application()
{
    g_App = this;
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    std::cout << "[INFO] [Application] Booting RayneEngine...\n";
    std::string projName = "RayneEngine";
    std::string projVersion = "1.0.0";
    std::string projAuthor = "";
    int winW = 1280;
    int winH = 720;
    bool vsync = true;
    int targetFPS = 60;
    bool fullscreen = false;
    sf::Color clearColor = sf::Color(18, 20, 23);
    float masterVol = 100.f;
    float musicVol = 100.f;
    std::string initialScene = "game";
    
    std::string path = std::string(ENGINE_ASSET_PATH) + "/project_settings.json";
    if (std::filesystem::exists(path)) {
        try {
            std::ifstream f(path);
            nlohmann::json j;
            f >> j;
            projName = j.value("ProjectName", projName);
            projVersion = j.value("Version", projVersion);
            projAuthor = j.value("Author", projAuthor);
            winW = j.value("WindowWidth", winW);
            winH = j.value("WindowHeight", winH);
            vsync = j.value("VSync", vsync);
            targetFPS = j.value("TargetFPS", targetFPS);
            fullscreen = j.value("Fullscreen", fullscreen);
            if (j.contains("ClearColor")) {
                std::string hex = j["ClearColor"].get<std::string>();
                if (hex.size() >= 7 && hex[0] == '#') {
                    unsigned int val = std::stoul(hex.substr(1), nullptr, 16);
                    clearColor = sf::Color((val >> 16) & 0xFF, (val >> 8) & 0xFF, val & 0xFF);
                }
            }
            masterVol = j.value("MasterVolume", masterVol);
            musicVol = j.value("MusicVolume", musicVol);
            initialScene = j.value("StartScene", initialScene);
            if (initialScene.find("scenes/") == 0) {
                initialScene = initialScene.substr(7);
            }
            if (initialScene.find(".json") != std::string::npos) {
                initialScene = initialScene.substr(0, initialScene.length() - 5);
            }
        } catch(...) {}
    }

    m_StartScene = initialScene;
    m_CurrentSceneName = initialScene;
    m_ProjectName = projName;
    m_ProjectVersion = projVersion;
    m_ProjectAuthor = projAuthor;
    m_WindowWidth = winW;
    m_WindowHeight = winH;
    m_ProjectVSync = vsync;
    m_ProjectTargetFPS = targetFPS;
    m_ProjectFullscreen = fullscreen;
    m_ClearColor = clearColor;
    m_MasterVolume = masterVol;
    m_MusicVolume = musicVol;

    AudioManager::Get().SetMasterVolume(masterVol);
    AudioManager::Get().SetMusicVolume(musicVol);
    
    SetProcessDPIAware();

#ifdef RAYNE_STANDALONE
    if (fullscreen) {
        m_RenderWindow.create(sf::VideoMode(winW, winH), projName, sf::Style::Fullscreen);
    } else {
        m_RenderWindow.create(sf::VideoMode(winW, winH), projName, sf::Style::Default);
    }
    m_RenderWindow.setVerticalSyncEnabled(vsync);
    if (targetFPS > 0) m_RenderWindow.setFramerateLimit(targetFPS);
#else
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    m_RenderWindow.create(desktop, projName + " - RayneEngine", sf::Style::Default);
    m_RenderWindow.setVerticalSyncEnabled(vsync);
    if (targetFPS > 0) m_RenderWindow.setFramerateLimit(targetFPS);

#ifdef _WIN32
    HWND hwnd = m_RenderWindow.getSystemHandle();
    ShowWindow(hwnd, SW_MAXIMIZE);

    sf::Event e;
    while (m_RenderWindow.pollEvent(e)) {}
#endif
#endif
    SetIcon();

    RunSplashSequence();

    std::cout << "[INFO] [Application] Engine Initialization Complete!\n";
}

void Application::RunSplashSequence()
{
    SplashScreen splash(m_RenderWindow);
    splash.Init(
        std::string(ENGINE_ASSET_PATH) + "/window/splash.jpg",
        std::string(ENGINE_ASSET_PATH) + "/fonts/Merriweather.ttf",
        m_ProjectName,
#ifdef RAYNE_STANDALONE
        "Standalone Build"
#else
        "Editor Build"
#endif
    );

    auto AnimateFrames = [&](int frames) {
        for (int i = 0; i < frames; ++i)
            splash.RenderFrame();
    };

    splash.SetProgress(0.05f, "Loading fonts...");
    AnimateFrames(30);

    auto font = ResourceManager::Get().GetFont(ENGINE_ASSET_PATH "/fonts/Merriweather.ttf");

    splash.SetProgress(0.15f, "Initializing UI Manager...");
    AnimateFrames(20);

    std::cout << "[INFO] [Application] Initializing UIManager...\n";
    UIManager::Get().Init(font);
    UIManager::Get().SetCurrentUIPath(std::string(ASSET_PATH) + "/ui.json");

    splash.SetProgress(0.25f, "Loading UI layout...");
    AnimateFrames(15);

    UIManager::Get().Load(std::string(ASSET_PATH) + "/ui.json");

    splash.SetProgress(0.35f, "Initializing Lua Scripting Engine...");
    AnimateFrames(20);

    std::cout << "[INFO] [Application] Initializing Lua Subsystem...\n";
    LuaState::Init(m_Registry, [this](const std::string &sceneName) {
        if (m_SceneManager.CurrentName() == "game")
        {
            LoadGameScene(sceneName);
        }
    });

    splash.SetProgress(0.50f, "Scripting engine ready");
    AnimateFrames(15);

    splash.SetProgress(0.60f, "Registering scenes...");
    AnimateFrames(15);

    std::cout << "[INFO] [Application] Registering Scenes...\n";
#ifndef RAYNE_STANDALONE
    m_SceneManager.RegisterScene<EditorScene>("editor", m_RenderWindow, m_Registry);
    splash.SetProgress(0.70f, "Registered: Editor Scene");
    AnimateFrames(10);

    m_SceneManager.RegisterScene<UIEditorScene>("ui_editor", m_RenderWindow);
    splash.SetProgress(0.78f, "Registered: UI Editor Scene");
    AnimateFrames(10);
#endif

    m_SceneManager.RegisterScene<GameScene>("game", m_RenderWindow, m_Registry);
    splash.SetProgress(0.85f, "Registered: Game Scene");
    AnimateFrames(10);

    splash.SetProgress(0.90f, "Loading project...");
    AnimateFrames(15);

#ifdef RAYNE_STANDALONE
    LoadGameScene(m_StartScene);

    std::cout << "[INFO] [Application] Switching to Game Scene (Standalone)...\n";
    m_SceneManager.SwitchSceneTo("game");
#else
    std::cout << "[INFO] [Application] Switching to Editor Scene...\n";
    m_SceneManager.SwitchSceneTo("editor");
#endif

    splash.SetProgress(1.0f, "Ready!");
    AnimateFrames(40);

    splash.BeginFadeOut();
    while (splash.RenderFrame()) {}
}


void Application::Run()
{
    std::cout << "[INFO] [Application] Entering main application loop...\n";
    while (m_RenderWindow.isOpen())
    {
        sf::Time dt = m_DeltaTimeClock.restart();
        float deltaTime = dt.asSeconds();

        m_CurrentDeltaTime = deltaTime;
        if (deltaTime > 0.0001f)
        {
            float instantFPS = 1.0f / deltaTime;
            m_CurrentFPS = (m_CurrentFPS <= 0.f) ? instantFPS : (m_CurrentFPS * 0.9f + instantFPS * 0.1f);
        }

        SetEvents();
        if (!m_RenderWindow.isOpen()) break;
        
        Update(deltaTime);
        Render();
        InputManager::Get().EndFrame();
    }

    m_SceneManager.Shutdown();
    std::cout << "[INFO] [Application] Exited cleanly.\n";
}

void Application::SetIcon()
{
    sf::Image icon;

    if (const std::string &filePath = std::string(ENGINE_ASSET_PATH) + "/window/rayne_icon.png"; icon.loadFromFile(filePath))
    {
        m_RenderWindow.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
        std::cout << "[INFO] [Window] Loaded window icon from " << filePath << "\n";
    } else { std::cout << "[WARN] [Window] Failed to load window icon from " << filePath << "\n"; }
}

void Application::Update(float deltaTime)
{
    m_SceneManager.Update(deltaTime);
}

void Application::Render()
{
    m_RenderWindow.clear(m_ClearColor);
    m_SceneManager.Render(m_RenderWindow);

    if (m_ShowFPSOverlay)
    {
        auto font = ResourceManager::Get().GetFont(std::string(ENGINE_ASSET_PATH) + "/fonts/Merriweather.ttf");
        if (font)
        {
            sf::View defaultView = m_RenderWindow.getDefaultView();
            m_RenderWindow.setView(defaultView);

            char buf[64];
            std::snprintf(buf, sizeof(buf), "FPS: %.1f (%.1f ms)", m_CurrentFPS, m_CurrentDeltaTime * 1000.f);

            sf::Text fpsText;
            fpsText.setFont(*font);
            fpsText.setCharacterSize(13);
            fpsText.setString(buf);
            fpsText.setFillColor(sf::Color(120, 240, 120));

            sf::FloatRect bounds = fpsText.getLocalBounds();
            float x = static_cast<float>(m_RenderWindow.getSize().x) - bounds.width - 16.f;
            float y = 8.f;
            fpsText.setPosition(x, y);

            sf::RectangleShape bg({bounds.width + 12.f, bounds.height + 10.f});
            bg.setPosition(x - 6.f, y - 2.f);
            bg.setFillColor(sf::Color(0, 0, 0, 160));

            m_RenderWindow.draw(bg);
            m_RenderWindow.draw(fpsText);
        }
    }

    m_RenderWindow.display();
}

void Application::SetEvents()
{
    sf::Event event{};

    while (m_RenderWindow.pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
        {
            std::cout << "[INFO] [Application] Window closed event received. Shutting down...\n";
            m_RenderWindow.close();
        }

        m_SceneManager.HandleEvent(event);
        InputManager::Get().HandleEvent(event);
    }
}

void Application::SetMasterVolume(float vol)
{
    m_MasterVolume = vol;
    AudioManager::Get().SetMasterVolume(vol);
}

void Application::SetMusicVolume(float vol)
{
    m_MusicVolume = vol;
    AudioManager::Get().SetMusicVolume(vol);
}

void Application::Quit()
{
#ifdef RAYNE_STANDALONE
    std::cout << "[INFO] [Application] Quit called: closing window...\n";
    m_RenderWindow.close();
#else
    if (m_SceneManager.CurrentName() == "game")
    {
        std::cout << "[INFO] [Application] Quit called in Play Mode: returning to editor...\n";
        m_SceneManager.SwitchSceneTo("editor");
    }
    else
    {
        std::cout << "[INFO] [Application] Quit called: closing window...\n";
        m_RenderWindow.close();
    }
#endif
}

void Application::RestartCurrentScene()
{
    std::cout << "[INFO] [Application] Restarting current scene: " << m_CurrentSceneName << "...\n";
    LoadGameScene(m_CurrentSceneName);
}

void Application::LoadGameScene(const std::string& sceneName)
{
    if (sceneName.empty()) return;
    m_CurrentSceneName = sceneName;
    m_IsPaused = false;
    m_TimeScale = 1.0f;

    m_Registry.Clear();
    TimerManager::Get().Clear();
    TweenManager::Get().Clear();
    EventManager::Get().Clear();
    
    EventManager::Get().SubscribeCollision([this](CollisionEvent e) {
        if (m_Registry.HasComponent<ScriptComponent>(e.a))
            m_Registry.GetComponent<ScriptComponent>(e.a).OnCollision(e.b);

        if (m_Registry.HasComponent<ScriptComponent>(e.b))
            m_Registry.GetComponent<ScriptComponent>(e.b).OnCollision(e.a);
    });

    std::string scenePath = std::string(ASSET_PATH) + "/scenes/" + sceneName + ".json";
    if (std::filesystem::exists(scenePath))
    {
        SceneSerializer::LoadIntoRegistry(m_Registry, scenePath);
    }
    else
    {
        std::cout << "[WARN] [Application] Scene file not found: " << scenePath << "\n";
    }

    std::string uiPath = std::string(ASSET_PATH) + "/scenes/" + sceneName + "_ui.json";
    UIManager::Get().SetCurrentUIPath(uiPath);
    if (std::filesystem::exists(uiPath))
    {
        UIManager::Get().Load(uiPath);
    }
    else
    {
        UIManager::Get().GetElements().clear();
    }

    m_Registry.ForEach<ScriptComponent>([](Entity, ScriptComponent &sc) { sc.OnCreate(); });
}

void Application::SetFullscreen(bool fullscreen)
{
    if (m_ProjectFullscreen == fullscreen) return;
    m_ProjectFullscreen = fullscreen;

    if (m_ProjectFullscreen)
    {
        m_RenderWindow.create(sf::VideoMode(m_WindowWidth, m_WindowHeight), m_ProjectName, sf::Style::Fullscreen);
    }
    else
    {
        m_RenderWindow.create(sf::VideoMode(m_WindowWidth, m_WindowHeight), m_ProjectName, sf::Style::Close | sf::Style::Titlebar);
    }

    m_RenderWindow.setVerticalSyncEnabled(m_ProjectVSync);
    if (!m_ProjectVSync && m_ProjectTargetFPS > 0)
    {
        m_RenderWindow.setFramerateLimit(m_ProjectTargetFPS);
    }
    SetIcon();
}

void Application::SetCursorVisible(bool visible)
{
    m_RenderWindow.setMouseCursorVisible(visible);
}

std::string Application::TakeScreenshot(const std::string& customFilename)
{
    std::filesystem::create_directories("screenshots");
    std::string filename = customFilename;
    if (filename.empty())
    {
        auto now = std::chrono::system_clock::now();
        std::time_t timeNow = std::chrono::system_clock::to_time_t(now);
        std::tm tmStruct{};
#if defined(_WIN32)
        localtime_s(&tmStruct, &timeNow);
#else
        localtime_r(&timeNow, &tmStruct);
#endif
        char buf[64];
        std::strftime(buf, sizeof(buf), "screenshot_%Y-%m-%d_%H-%M-%S.png", &tmStruct);
        filename = std::string("screenshots/") + buf;
    }
    else if (filename.find('/') == std::string::npos && filename.find('\\') == std::string::npos)
    {
        filename = "screenshots/" + filename;
        if (filename.find(".png") == std::string::npos && filename.find(".jpg") == std::string::npos)
        {
            filename += ".png";
        }
    }

    sf::Vector2u winSize = m_RenderWindow.getSize();
    sf::Texture tex;
    if (tex.create(winSize.x, winSize.y))
    {
        tex.update(m_RenderWindow);
        sf::Image img = tex.copyToImage();
        if (img.saveToFile(filename))
        {
            std::cout << "[INFO] [Application] Screenshot saved to " << filename << "\n";
            return filename;
        }
    }

    std::cout << "[WARN] [Application] Failed to save screenshot to " << filename << "\n";
    return "";
}

void Application::OpenURL(const std::string& url)
{
    if (url.empty()) return;
#ifdef _WIN32
    ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#endif
}
