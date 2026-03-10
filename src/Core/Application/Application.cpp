#include "Application.h"

#include <iostream>

#include "../ECS/Components.h"
#include "../Scripting/LuaState.h"
#include "../Scripting/ScriptComponent.h"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Window/Event.hpp"

Application::Application()
{
    CreateWindow();
    SetIcon();


    LuaState::Init(m_Registry);

    m_EditorCamera = EditorCamera2D(sf::Vector2f(0.f, 0.f), 1.f, &m_RenderWindow);

    const sf::Vector2u size = m_RenderWindow.getSize();
    m_EditorCamera.GetView().setSize(static_cast<float>(size.x), static_cast<float>(size.y));
    m_EditorCamera.GetView().setCenter(0.f, 0.f);

    m_EditorCamera.GetView().setViewport(sf::FloatRect(0.2f, 0.15f, 0.5f, 0.6f));
    m_EditorCamera.Apply();

    std::cout << "Initialized!\n";

    Entity player = m_Registry.CreateEntity();
    m_Registry.AddComponent(player, TransformComponent{ 400.f, 400.f });
    m_Registry.AddComponent(player, RenderComponent{ sf::Color::Cyan, { 40, 40 } });

    auto& script = m_Registry.AddComponent(player, ScriptComponent(LuaState::GetLua(), ASSET_PATH "scripting/test.lua"));
    script.SetEntity(player);
    script.OnCreate();
}

void Application::CreateWindow()
{
    std::cout << "Starting...\n";

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    m_RenderWindow.create(desktop, "RayneEngine");

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

    if (const std::string filePath = "assets/window/rayne_icon.png"; icon.loadFromFile(filePath))
        m_RenderWindow.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
    else
        std::cout << "Failed to load icon from " << filePath << std::endl;
}

void Application::Update(float deltaTime)
{
    m_EditorCamera.Update(deltaTime);
    m_PrimitiveManager.Update(deltaTime);

    m_EditorCamera.Update(deltaTime);
    m_PrimitiveManager.Update(deltaTime);

    for (auto ScriptView = m_Registry.GetView<ScriptComponent>(); Entity entity : ScriptView)
    {
        auto& script = m_Registry.GetComponent<ScriptComponent>(entity);
        script.OnUpdate(deltaTime);
    }

    for (auto MoveView = m_Registry.GetView<TransformComponent, VelocityComponent>(); Entity entity : MoveView)
    {
        auto& transform = m_Registry.GetComponent<TransformComponent>(entity);
        auto& velocity = m_Registry.GetComponent<VelocityComponent>(entity);
        transform.x += velocity.dx * deltaTime;
        transform.y += velocity.dy * deltaTime;
    }
}

void Application::Render()
{
    m_RenderWindow.clear(sf::Color(10, 18, 25));
    m_EditorCamera.Apply();

    m_EditorCamera.DrawGrid(m_EditorCamera.GetView());

    // --- Border ---
    m_EditorCamera.DrawWorldBorder(3.f, sf::Color(200, 200, 250));

    // --- UI ---
    m_RenderWindow.setView(m_RenderWindow.getDefaultView());

    for (auto RenderView = m_Registry.GetView<TransformComponent, RenderComponent>(); Entity entity : RenderView)
    {
        auto& pos = m_Registry.GetComponent<TransformComponent>(entity);
        auto& gfx = m_Registry.GetComponent<RenderComponent>(entity);

        sf::RectangleShape shape(gfx.size);
        shape.setFillColor(gfx.color);
        shape.setPosition(pos.x, pos.y);
        m_RenderWindow.draw(shape);
    }

    m_RenderWindow.display();
}

void Application::SetEvents()
{
    sf::Event event;

    while (m_RenderWindow.pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
            m_RenderWindow.close();

        if (event.type == sf::Event::MouseWheelScrolled)
        {
            if (event.mouseWheelScroll.delta > 0)
                m_EditorCamera.GetView().zoom(0.9f);
            else
                m_EditorCamera.GetView().zoom(1.1f);
        }
    }
}