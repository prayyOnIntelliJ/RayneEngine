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
#include "SFML/Window/Event.hpp"
#include "../UI/UIManager.h"
#include "../Resources/ResourceManager.h"

Application::Application()
{
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
        InputManager::Get().EndFrame();
        Update(deltaTime);
        Render();
    }

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
    m_PrimitiveManager.Update(deltaTime);
}

void Application::Render()
{
    m_RenderWindow.clear(sf::Color(10, 18, 25));
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
