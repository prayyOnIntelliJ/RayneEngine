#ifndef CIRCLEPRIMITIVE_H
#define CIRCLEPRIMITIVE_H
#include "Primitive.h"
#include "SFML/Graphics/CircleShape.hpp"


class CirclePrimitive : public Primitive {
public:
    sf::CircleShape shape;

    CirclePrimitive(float radius, const sf::Color& col)
    {
        shape.setRadius(radius);
        shape.setOrigin(radius, radius);
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
