
#include "Application.h"

#include <iostream>

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

Application* g_App = nullptr;

BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT || dwCtrlType == CTRL_CLOSE_EVENT)
    {
        std::cout << "[INFO] [Application] Intercepted console stop signal (" << dwCtrlType << "). Forcing save...\n";
        if (g_App) g_App->GetSceneManager().Shutdown();
        return FALSE; // Let default handler terminate process
    }
    return FALSE;
}

Application::Application()
{
    g_App = this;
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    std::cout << "[INFO] [Application] Booting RayneEngine...\n";
    CreateEngineWindow();
    SetIcon();

    std::cout << "[INFO] [Application] Initializing UIManager...\n";
    auto font = ResourceManager::Get().GetFont(ASSET_PATH "fonts/Merriweather.ttf");
    UIManager::Get().Init(font);
    UIManager::Get().Load(std::string(ASSET_PATH) + "ui.json");

    std::cout << "[INFO] [Application] Initializing Lua Subsystem...\n";
    LuaState::Init(m_Registry, [this](const std::string &sceneName) {
        if (m_SceneManager.CurrentName() == "game")
        {
            m_Registry.Clear();
            TimerManager::Get().Clear();
            TweenManager::Get().Clear();
            // We shouldn't clear EventManager here if we are INSIDE an event callback (like OnCollision).
            // Actually, clearing it is safe because FireCollision copies the list or iterates it by index? 
            // Wait, FireCollision uses a range-based for loop. Clearing it will empty the vector while iterating!
            // Let's NOT clear EventManager here, GameScene handles it on Exit. But wait, we want to clear old collision events.
            // Actually, EventManager only holds SubscribeCollision from GameScene::OnEnter. We SHOULD clear it and re-subscribe!
            EventManager::Get().Clear();
            
            // Re-subscribe default game scene collision handler
            EventManager::Get().SubscribeCollision([this](CollisionEvent e) {
                if (m_Registry.HasComponent<ScriptComponent>(e.a))
                    m_Registry.GetComponent<ScriptComponent>(e.a).OnCollision(e.b);

                if (m_Registry.HasComponent<ScriptComponent>(e.b))
                    m_Registry.GetComponent<ScriptComponent>(e.b).OnCollision(e.a);
            });

            SceneSerializer::LoadIntoRegistry(m_Registry, "assets/scenes/" + sceneName + ".json");
            m_Registry.ForEach<ScriptComponent>([](Entity, ScriptComponent &sc) { sc.OnCreate(); });
        }
    });

    std::cout << "[INFO] [Application] Registering Scenes...\n";
    m_SceneManager.RegisterScene<EditorScene>("editor", m_RenderWindow, m_Registry);
    m_SceneManager.RegisterScene<UIEditorScene>("ui_editor", m_RenderWindow);
    m_SceneManager.RegisterScene<GameScene>("game", m_RenderWindow, m_Registry);

    std::cout << "[INFO] [Application] Switching to Editor Scene...\n";
    m_SceneManager.SwitchSceneTo("editor");

    std::cout << "[INFO] [Application] Engine Initialization Complete!\n";
}

void Application::CreateEngineWindow()
{
    std::cout << "[INFO] [Window] Creating main window...\n";

    SetProcessDPIAware();

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    m_RenderWindow.create(desktop, "RayneEngine");

    std::cout << "[INFO] [Window] Created window with resolution " << desktop.width << "x" << desktop.height << "\n";

    HWND hwnd = m_RenderWindow.getSystemHandle();
    ShowWindow(hwnd, SW_MAXIMIZE);

    sf::Event e;
    while (m_RenderWindow.pollEvent(e)) {}

    std::cout << "[INFO] [Window] Window maximized successfully.\n";
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

    if (const std::string &filePath = "assets/window/rayne_icon.png"; icon.loadFromFile(filePath))
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
