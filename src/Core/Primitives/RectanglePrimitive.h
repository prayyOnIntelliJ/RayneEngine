#ifndef RECTANGLEPRIMITIVE_H
#define RECTANGLEPRIMITIVE_H
#include "Primitive.h"
#include "SFML/Graphics/RectangleShape.hpp"


class RectanglePrimitive : public Primitive {
public:
    sf::RectangleShape shape;

    RectanglePrimitive(const sf::Vector2f& size, const sf::Color& col)
    {
        shape.setSize(size);
        shape.setOrigin(size / 2.f);
        color = col;
        shape.setFillColor(color);
    }

    void Update(float deltaTime) override
    {
        shape.setPosition(position);
        shape.setRotation(rotation);
    }

    void Draw(sf::RenderWindow& window) override
    {
        window.draw(shape);
    }
};



#endif
