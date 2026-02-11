#include "EditorCamera2D.h"

#include <iostream>

#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/Text.hpp"
#include "SFML/Graphics/VertexArray.hpp"
#include "SFML/Window/Keyboard.hpp"
#include "SFML/Window/Mouse.hpp"

EditorCamera2D::EditorCamera2D(const sf::Vector2f &startPosition, float startZoom, sf::RenderWindow* window) : m_RenderWindow(window)
{
    m_View.setCenter(startPosition);
    m_View.setSize(1920.f, 1080.f);
    m_ZoomLevel = startZoom;
    m_View.zoom(m_ZoomLevel);
}

void EditorCamera2D::Update(float deltaTime)
{
    sf::Vector2i mousePosition = sf::Mouse::getPosition(*m_RenderWindow);

    const float MOVE_SPEED = 800.f * deltaTime;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
        m_View.move(0.f, -MOVE_SPEED);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
        m_View.move(0.f, MOVE_SPEED);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
        m_View.move(-MOVE_SPEED, 0.f);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
        m_View.move(MOVE_SPEED, 0.f);

    if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
    {
        if (!m_bIsDragging)
        {
            m_bIsDragging = true;
            m_LastMousePosition = mousePosition;
        }
        else
        {
            sf::Vector2i delta = mousePosition - m_LastMousePosition;
            m_View.move(-delta.x * deltaTime * 2000.f, -delta.y * deltaTime * 2000.f);
            m_LastMousePosition = mousePosition;
        }
    }
    else
    {
        m_bIsDragging = false;
    }
}


void EditorCamera2D::Apply() const
{
    m_RenderWindow->setView(m_View);
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


    m_RenderWindow->draw(lines);

    sf::Color xColor(255, 80, 80);
    sf::Color yColor(80, 120, 255);

    sf::Vertex xAxis[] = {
        sf::Vertex(sf::Vector2f(left, 0.f), xColor),
        sf::Vertex(sf::Vector2f(right, 0.f), xColor)
    };
    m_RenderWindow->draw(xAxis, 2, sf::Lines);

    sf::Vertex yAxis[] = {
        sf::Vertex(sf::Vector2f(0.f, top), yColor),
        sf::Vertex(sf::Vector2f(0.f, bottom), yColor)
    };
    m_RenderWindow->draw(yAxis, 2, sf::Lines);

    sf::Vertex cross[] = {
        sf::Vertex({-10.f, 0.f}, sf::Color::White),
        sf::Vertex({10.f, 0.f}, sf::Color::White),
        sf::Vertex({0.f, -10.f}, sf::Color::White),
        sf::Vertex({0.f, 10.f}, sf::Color::White)
    };
    m_RenderWindow->draw(cross, 4, sf::Lines);
}

void EditorCamera2D::DrawWorldBorder(float thicknessPixels, sf::Color color) const
{
    m_RenderWindow->setView(m_View);

    const sf::Vector2f c = m_View.getCenter();
    const sf::Vector2f s = m_View.getSize();
    const float l = c.x - s.x * 0.5f;
    const float r = c.x + s.x * 0.5f;
    const float t = c.y - s.y * 0.5f;
    const float b = c.y + s.y * 0.5f;

    sf::Vertex lines[] = {
        sf::Vertex({l, t}, color), sf::Vertex({r, t}, color),
        sf::Vertex({r, t}, color), sf::Vertex({r, b}, color),
        sf::Vertex({r, b}, color), sf::Vertex({l, b}, color),
        sf::Vertex({l, b}, color), sf::Vertex({l, t}, color),
    };

    m_RenderWindow->draw(lines, 8, sf::Lines);
}



sf::View & EditorCamera2D::GetView() { return m_View; }
