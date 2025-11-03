#include "Application.h"

#include <iostream>

#include "SFML/Window/Event.hpp"

Application::Application()
{
    CreateWindow();
    SetIcon();
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

        sf::Event event;

        while (renderWindow.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                renderWindow.close();
        }

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
}

void Application::Render()
{
    renderWindow.clear(sf::Color(10, 18, 25));
    renderWindow.display();
}
