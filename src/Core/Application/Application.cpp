#include "Application.h"

#include <iostream>

#include "../Math/MathR.h"
#include "../Math/Vector.h"
#include "../Primitives/CirclePrimitive.h"
#include "../Primitives/RectanglePrimitive.h"
#include "../Primitives/TrianglePrimitive.h"
#include "../Scripting/LuaState.h"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Window/Event.hpp"

Application::Application()
{
    CreateWindow();
    SetIcon();

    m_editorCamera = EditorCamera2D(sf::Vector2f(0.f, 0.f), 1.f, &m_renderWindow);

    sf::Vector2u size = m_renderWindow.getSize();
    m_editorCamera.GetView().setSize(static_cast<float>(size.x), static_cast<float>(size.y));
    m_editorCamera.GetView().setCenter(0.f, 0.f);

    m_editorCamera.GetView().setViewport(sf::FloatRect(0.2f, 0.15f, 0.5f, 0.6f));
    m_editorCamera.Apply();

    const auto cube = m_primitiveManager.Create<RectanglePrimitive>(sf::Vector2f(100, 100), sf::Color::Cyan);
    const auto circle = m_primitiveManager.Create<CirclePrimitive>(50.f, sf::Color::Yellow);
    const auto triangle = m_primitiveManager.Create<TrianglePrimitive>(60.f, sf::Color::Green);

    cube->position = {0, 0};
    circle->position = {200, 0};
    triangle->position = {0, 200};

    std::cout << "Initialized!" << std::endl;

    LuaState::Init();
}

void Application::CreateWindow()
{
    std::cout << "Starting...\n";

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    m_renderWindow.create(desktop, "RayneEngine");

    std::cout << "Window created!\n";
}

void Application::Run()
{

    while (m_renderWindow.isOpen())
    {
        sf::Time dt = m_deltaTimeClock.restart();
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
    std::string filePath = "assets/window/rayne_icon.png";

    if (icon.loadFromFile(filePath))
        m_renderWindow.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
    else
        std::cout << "Failed to load icon from " << filePath << std::endl;
}

void Application::Update(float deltaTime)
{
    m_editorCamera.Update(deltaTime);
    m_primitiveManager.Update(deltaTime);
}

void Application::Render()
{
    m_renderWindow.clear(sf::Color(10, 18, 25));

    // --- Scene ---
    m_editorCamera.Apply();
    m_editorCamera.DrawGrid(m_editorCamera.GetView());
    m_primitiveManager.Draw(m_renderWindow);

    // --- Border direkt hier ---
    m_editorCamera.DrawWorldBorder(3.f, sf::Color(200, 200, 250));

    // --- UI danach ---
    m_renderWindow.setView(m_renderWindow.getDefaultView());
    m_renderWindow.display();
}

void Application::SetEvents()
{
    sf::Event event;

    while (m_renderWindow.pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
            m_renderWindow.close();

        if (event.type == sf::Event::MouseWheelScrolled)
        {
            if (event.mouseWheelScroll.delta > 0)
                m_editorCamera.GetView().zoom(0.9f);
            else
                m_editorCamera.GetView().zoom(1.1f);
        }
    }
}