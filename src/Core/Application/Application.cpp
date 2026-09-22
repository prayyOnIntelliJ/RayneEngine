#include "Application.h"
#include "SplashScreen.h"

#include <iostream>
#include <fstream>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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
    int winW = 1280;
    int winH = 720;
    bool vsync = true;
    std::string initialScene = "game";
    
    std::string path = std::string(ENGINE_ASSET_PATH) + "/project_settings.json";
    if (std::filesystem::exists(path)) {
        try {
            std::ifstream f(path);
            nlohmann::json j;
            f >> j;
            projName = j.value("ProjectName", projName);
            winW = j.value("WindowWidth", winW);
            winH = j.value("WindowHeight", winH);
            vsync = j.value("VSync", vsync);
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
    m_ProjectName = projName;
    
    SetProcessDPIAware();

#ifdef RAYNE_STANDALONE
    m_RenderWindow.create(sf::VideoMode(winW, winH), projName, sf::Style::Default);
    m_RenderWindow.setVerticalSyncEnabled(vsync);
#else
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    m_RenderWindow.create(desktop, projName + " - RayneEngine", sf::Style::Default);
    m_RenderWindow.setVerticalSyncEnabled(vsync);

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

            SceneSerializer::LoadIntoRegistry(m_Registry, std::string(ASSET_PATH) + "/scenes/" + sceneName + ".json");
            std::string uiPath = std::string(ASSET_PATH) + "/scenes/" + sceneName + "_ui.json";
            UIManager::Get().SetCurrentUIPath(uiPath);
            UIManager::Get().Load(uiPath);
            m_Registry.ForEach<ScriptComponent>([](Entity, ScriptComponent &sc) { sc.OnCreate(); });
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
    std::cout << "[INFO] [Application] Switching to Game Scene (Standalone)...\n";
    m_SceneManager.SwitchSceneTo("game");
    
    SceneSerializer::LoadIntoRegistry(m_Registry, std::string(ASSET_PATH) + "/scenes/" + m_StartScene + ".json");
    std::string uiPath = std::string(ASSET_PATH) + "/ui.json";
    UIManager::Get().SetCurrentUIPath(uiPath);
    UIManager::Get().Load(uiPath);
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
    m_RenderWindow.clear(sf::Color(18, 20, 23));
    m_SceneManager.Render(m_RenderWindow);
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
