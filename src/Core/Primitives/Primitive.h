#ifndef PRIMITIVES_H
#define PRIMITIVES_H
#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/System/Vector2.hpp"


class Primitive {
public:
    sf::Vector2f position;
    float rotation = 0.f;
    sf::Color color = sf::Color::White;

    virtual void Update(float deltaTime) = 0;
    virtual void Draw(sf::RenderWindow& window) = 0;
    virtual ~Primitive() = default;
};



#endif
