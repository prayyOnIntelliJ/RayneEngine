#ifndef TRIANGLEPRIMITIVE_H
#define TRIANGLEPRIMITIVE_H
#include "Primitive.h"
#include "SFML/Graphics/ConvexShape.hpp"


class TrianglePrimitive : public Primitive {
public:
    sf::ConvexShape shape;

    TrianglePrimitive(float size, const sf::Color& col)
    {
        shape.setPointCount(3);
        shape.setPoint(0, {0.f, -size});
        shape.setPoint(1, {size, size});
        shape.setPoint(2, {-size, size});
        shape.setOrigin(0.f, size / 3.f);
        color = col;
        shape.setFillColor(color);
    }

    void Update(float deltaTime) override
    {
        shape.setPosition(position);
        shape.setRotation(rotation);
    }

    void Draw(sf::RenderWindow &window) override
    {
        window.draw(shape);
    }
};



#endif
