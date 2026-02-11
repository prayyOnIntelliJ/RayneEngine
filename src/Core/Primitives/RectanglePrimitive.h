#ifndef RECTANGLEPRIMITIVE_H
#define RECTANGLEPRIMITIVE_H
#include "Primitive.h"
#include "SFML/Graphics/RectangleShape.hpp"


class RectanglePrimitive : public Primitive {
public:
    sf::RectangleShape m_shape;

    RectanglePrimitive(const sf::Vector2f& size, const sf::Color& col)
    {
        m_shape.setSize(size);
        m_shape.setOrigin(size / 2.f);
        m_color = col;
        m_shape.setFillColor(m_color);
    }

    void Update(float deltaTime) override
    {
        m_shape.setPosition(m_position);
        m_shape.setRotation(m_rotation);
    }

    void Draw(sf::RenderWindow& window) override
    {
        window.draw(m_shape);
    }
};



#endif
