#include "Application.h"

#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../ECS/Components.h"
#include "../Scenes/EditorScene.h"
#include "../Scripting/LuaState.h"
#include "../Scripting/ScriptComponent.h"
#include "SFML/Window/Event.hpp"

Application::Application()
{
    CreateEngineWindow();
    SetIcon();

    LuaState::Init(m_Registry);

    m_SceneManager.RegisterScene<EditorScene>("editor", m_RenderWindow, m_Registry);

    m_SceneManager.SwitchSceneTo("editor");

    Entity player = m_Registry.CreateEntity();
    m_Registry.AddComponent(player, TransformComponent{ 400.f, 400.f });
    m_Registry.AddComponent(player, RenderComponent{ sf::Color::Cyan, { 40, 40 } });

    auto& script = m_Registry.AddComponent(player, ScriptComponent(LuaState::GetLua(), ASSET_PATH "scripting/test.lua"));
    script.SetEntity(player);
    script.OnCreate();

    std::cout << "Initialized!\n";
}

void Application::CreateEngineWindow()
{
    std::cout << "Creating Window...\n";

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    m_RenderWindow.create(desktop, "RayneEngine");
    ShowWindow(m_RenderWindow.getSystemHandle(), SW_MAXIMIZE);

    std::cout << "Window created!\n";
}

void Application::Run()
{
    while (m_RenderWindow.isOpen())
    {
        sf::Time dt = m_DeltaTimeClock.restart();
        float deltaTime = dt.asSeconds();

        SetEvents();
        Update(deltaTime);
        Render();
    }

    std::cout << "Exited cleanly.\n";
}

void Application::SetIcon()
{
    sf::Image icon;

    if (const std::string& filePath = "assets/window/rayne_icon.png"; icon.loadFromFile(filePath))
        m_RenderWindow.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
    else
        std::cout << "Failed to load icon from " << filePath << std::endl;
}

void Application::Update(float deltaTime)
{
    m_SceneManager.Update(deltaTime);
    m_PrimitiveManager.Update(deltaTime);

    for (auto ScriptView = m_Registry.GetView<ScriptComponent>(); Entity entity : ScriptView)
    {
        auto& script = m_Registry.GetComponent<ScriptComponent>(entity);
        script.OnUpdate(deltaTime);
    }
}

void Application::Render()
{
    m_RenderWindow.clear(sf::Color(10, 18, 25));
    m_SceneManager.Render(m_RenderWindow);
    m_RenderWindow.display();
}

void Application::SetEvents()
{
    sf::Event event;

    while (m_RenderWindow.pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
            m_RenderWindow.close();

        m_SceneManager.HandleEvent(event);
    }
}