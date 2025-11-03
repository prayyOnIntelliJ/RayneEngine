#include "EditorCamera2D.h"

#include <iostream>

#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/Text.hpp"
#include "SFML/Graphics/VertexArray.hpp"
#include "SFML/Window/Keyboard.hpp"
#include "SFML/Window/Mouse.hpp"

EditorCamera2D::EditorCamera2D(const sf::Vector2f &startPosition, float startZoom, sf::RenderWindow* window) : renderWindow(window)
{
    view.setCenter(startPosition);
    view.setSize(1920.f, 1080.f);
    zoomLevel = startZoom;
    view.zoom(zoomLevel);
}

void EditorCamera2D::Update(float deltaTime)
{
    sf::Vector2i mousePosition = sf::Mouse::getPosition(*renderWindow);

    const float MOVE_SPEED = 800.f * deltaTime;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
        view.move(0.f, -MOVE_SPEED);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
        view.move(0.f, MOVE_SPEED);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
        view.move(-MOVE_SPEED, 0.f);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
        view.move(MOVE_SPEED, 0.f);

    if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
    {
        if (!isDragging)
        {
            isDragging = true;
            lastMousePosition = mousePosition;
        }
        else
        {
            sf::Vector2i delta = mousePosition - lastMousePosition;
            view.move(-delta.x * deltaTime * 2000.f, -delta.y * deltaTime * 2000.f);
            lastMousePosition = mousePosition;
        }
    }
    else
    {
        isDragging = false;
    }
}


void EditorCamera2D::Apply() const
{
    renderWindow->setView(view);
}

void EditorCamera2D::DrawGrid(const sf::View &view) const
{
    const float GRID_SIZE = 100.f;
    const sf::Color gridColor(50, 50, 60);

    sf::FloatRect viewRect(
        view.getCenter().x - view.getSize().x / 2.f,
        view.getCenter().y - view.getSize().y / 2.f,
        view.getSize().x,
        view.getSize().y
        );

    sf::VertexArray lines(sf::Lines);

    float left = std::floor(viewRect.left / GRID_SIZE - 1) * GRID_SIZE;
    float right = std::ceil((viewRect.left + viewRect.width) / GRID_SIZE + 1) * GRID_SIZE;
    float top = std::floor(viewRect.top / GRID_SIZE - 1) * GRID_SIZE;
    float bottom = std::ceil((viewRect.top + viewRect.height) / GRID_SIZE + 1) * GRID_SIZE;

    for (float x = left; x <= right; x += GRID_SIZE)
    {
        lines.append(sf::Vertex(sf::Vector2f(x, top), gridColor));
        lines.append(sf::Vertex(sf::Vector2f(x, bottom), gridColor));
    }

    for (float y = top; y <= bottom; y += GRID_SIZE)
    {
        lines.append(sf::Vertex(sf::Vector2f(left, y), gridColor));
        lines.append(sf::Vertex(sf::Vector2f(right, y), gridColor));
    }


    renderWindow->draw(lines);

    sf::Color xColor(255, 80, 80);
    sf::Color yColor(80, 120, 255);

    sf::Vertex xAxis[] = {
        sf::Vertex(sf::Vector2f(left, 0.f), xColor),
        sf::Vertex(sf::Vector2f(right, 0.f), xColor)
    };
    renderWindow->draw(xAxis, 2, sf::Lines);

    sf::Vertex yAxis[] = {
        sf::Vertex(sf::Vector2f(0.f, top), yColor),
        sf::Vertex(sf::Vector2f(0.f, bottom), yColor)
    };
    renderWindow->draw(yAxis, 2, sf::Lines);

    sf::Vertex cross[] = {
        sf::Vertex({-10.f, 0.f}, sf::Color::White),
        sf::Vertex({10.f, 0.f}, sf::Color::White),
        sf::Vertex({0.f, -10.f}, sf::Color::White),
        sf::Vertex({0.f, 10.f}, sf::Color::White)
    };
    renderWindow->draw(cross, 4, sf::Lines);
}

void EditorCamera2D::DrawBorder(const sf::Color &color, float thickness) const
{
    auto previousView = renderWindow->getView();
    renderWindow->setView(renderWindow->getDefaultView());

    sf::Vector2u windowSize = renderWindow->getSize();
    sf::FloatRect viewport = view.getViewport();

    sf::FloatRect rect(
        viewport.left * windowSize.x,
        viewport.top * windowSize.y,
        viewport.width * windowSize.x,
        viewport.height * windowSize.y
    );

    sf::RectangleShape border;
    border.setPosition(rect.left, rect.top);
    border.setSize({rect.width, rect.height});
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineColor(color);
    border.setOutlineThickness(thickness);

    renderWindow->draw(border);

    renderWindow->setView(previousView);
}

sf::View & EditorCamera2D::GetView() { return view; }
