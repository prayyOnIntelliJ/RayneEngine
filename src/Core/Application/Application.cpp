#include "Application.h"

#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../ECS/Components.h"
#include "../Input/InputManager.h"
#include "../Scenes/EditorScene.h"
#include "../Scenes/GameScene.h"
#include "../Scripting/LuaState.h"
#include "../Scripting/ScriptComponent.h"
#include "SFML/Window/Event.hpp"

Application::Application()
{
    CreateEngineWindow();
    SetIcon();

    LuaState::Init(m_Registry);

    m_SceneManager.RegisterScene<EditorScene>("editor", m_RenderWindow, m_Registry);
    m_SceneManager.RegisterScene<GameScene>("game", m_RenderWindow, m_Registry);

    m_SceneManager.SwitchSceneTo("editor");

    std::cout << "Initialized!\n";
}

void Application::CreateEngineWindow()
{
    std::cout << "Creating Window...\n";

    SetProcessDPIAware();

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    m_RenderWindow.create(desktop, "RayneEngine");

    std::cout << "Created window" << std::endl;

    HWND hwnd = m_RenderWindow.getSystemHandle();
    ShowWindow(hwnd, SW_MAXIMIZE);

    sf::Event e;
    while (m_RenderWindow.pollEvent(e)) {}

    std::cout << "Maximized Window" << std::endl;
}

void Application::Run()
{
    while (m_RenderWindow.isOpen())
    {
        sf::Time dt = m_DeltaTimeClock.restart();
        float deltaTime = dt.asSeconds();

        SetEvents();
        InputManager::Get().EndFrame();
        Update(deltaTime);
        Render();
    }

    std::cout << "Exited cleanly.\n";
}

void Application::SetIcon()
{
    sf::Image icon;

    if (const std::string &filePath = "assets/window/rayne_icon.png"; icon.loadFromFile(filePath))
    {
        m_RenderWindow.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
        std::cout << "Icon taken from: " << filePath << std::endl;
    } else
        std::cout << "Failed to load icon from " << filePath << std::endl;
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
            m_RenderWindow.close();

        m_SceneManager.HandleEvent(event);
        InputManager::Get().HandleEvent(event);
    }
}
