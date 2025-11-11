#include "Application.h"

#include <iostream>

#include "../Primitives/CirclePrimitive.h"
#include "../Primitives/RectanglePrimitive.h"
#include "../Primitives/TrianglePrimitive.h"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Window/Event.hpp"

Application::Application()
{
    CreateWindow();
    SetIcon();

    editorCamera = EditorCamera2D(sf::Vector2f(0.f, 0.f), 1.f, &renderWindow);

    sf::Vector2u size = renderWindow.getSize();
    editorCamera.GetView().setSize(static_cast<float>(size.x), static_cast<float>(size.y));
    editorCamera.GetView().setCenter(0.f, 0.f);

    editorCamera.GetView().setViewport(sf::FloatRect(0.2f, 0.15f, 0.5f, 0.6f));
    editorCamera.Apply();

    auto cube = primitiveManager.Create<RectanglePrimitive>(sf::Vector2f(100, 100), sf::Color::Cyan);
    auto circle = primitiveManager.Create<CirclePrimitive>(50.f, sf::Color::Yellow);
    auto triangle = primitiveManager.Create<TrianglePrimitive>(60.f, sf::Color::Green);

    cube->position = {0, 0};
    circle->position = {200, 0};
    triangle->position = {0, 200};
}

void Application::CreateWindow()
{
    std::cout << "Starting RayneEngine...\n";

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    renderWindow.create(desktop, "RayneEngine");

    std::cout << "Window created!\n";
}

void Application::Run()
{

    while (renderWindow.isOpen())
    {
        sf::Time dt = deltaTimeClock.restart();
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
        renderWindow.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
    else
        std::cout << "Failed to load icon from " << filePath << std::endl;
}

void Application::Update(float deltaTime)
{
    editorCamera.Update(deltaTime);
    primitiveManager.Update(deltaTime);
}

void Application::Render()
{
    renderWindow.clear(sf::Color(10, 18, 25));

    // --- Scene ---
    editorCamera.Apply();
    editorCamera.DrawGrid(editorCamera.GetView());
    primitiveManager.Draw(renderWindow);

    // --- Border direkt hier ---
    editorCamera.DrawWorldBorder(3.f, sf::Color(200, 200, 250));

    // --- UI danach ---
    renderWindow.setView(renderWindow.getDefaultView());
    renderWindow.display();
}

void Application::SetEvents()
{
    sf::Event event;

    while (renderWindow.pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
            renderWindow.close();

        if (event.type == sf::Event::MouseWheelScrolled)
        {
            if (event.mouseWheelScroll.delta > 0)
                editorCamera.GetView().zoom(0.9f);
            else
                editorCamera.GetView().zoom(1.1f);
        }
    }
}