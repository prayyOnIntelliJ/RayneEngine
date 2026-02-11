#ifndef RAYNEENGINE_COMPONENTS_H
#define RAYNEENGINE_COMPONENTS_H

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

struct TransformComponent
{
    float x, y;
};

struct VelocityComponent
{
    float dx, dy;
};

struct RenderComponent
{
    sf::Color color;
    sf::Vector2f size;
};

#endif