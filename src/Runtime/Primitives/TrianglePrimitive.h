#ifndef TRIANGLEPRIMITIVE_H
#define TRIANGLEPRIMITIVE_H
#include "Primitive.h"
#include "SFML/Graphics/ConvexShape.hpp"

class TrianglePrimitive : public Primitive
{
public:
    sf::ConvexShape m_shape;

    TrianglePrimitive(float size, const sf::Color &col)
    {
        m_shape.setPointCount(3);
        m_shape.setPoint(0, {0.f, -size});
        m_shape.setPoint(1, {size, size});
        m_shape.setPoint(2, {-size, size});
        m_shape.setOrigin(0.f, size / 3.f);
        m_color = col;
        m_shape.setFillColor(m_color);
    }

    void Update(float deltaTime) override
    {
        m_shape.setPosition(m_position);
        m_shape.setRotation(m_rotation);
    }

    void Draw(sf::RenderWindow &window) override { window.draw(m_shape); }
};

#endif
