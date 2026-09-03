#ifndef PRIMITIVES_H
#define PRIMITIVES_H
#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/System/Vector2.hpp"

class Primitive
{
public:
    sf::Vector2f m_position;
    float m_rotation = 0.f;
    sf::Color m_color = sf::Color::White;

    virtual void Update(float deltaTime) = 0;

    virtual void Draw(sf::RenderWindow &window) = 0;

    virtual ~Primitive() = default;
};

#endif
