#ifndef RAYNEENGINE_COMPONENTS_H
#define RAYNEENGINE_COMPONENTS_H

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/Texture.hpp"
#include "../Resources/ResourceManager.h"

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

struct SpriteComponent
{
    std::string texturePath;
    std::shared_ptr<sf::Texture> texture;
    sf::Sprite sprite;
    sf::Vector2f size;

    SpriteComponent() = default;

    explicit SpriteComponent(const std::string &path, sf::Vector2f size)
        : texturePath(path), size(size)
    {
        texture = ResourceManager::Get().GetTexture(path);
        if (texture)
        {
            sprite.setTexture(*texture);
            const sf::Vector2u texSize = texture->getSize();
            if (texSize.x > 0 && texSize.y > 0)
            {
                sprite.setScale(
                    size.x / static_cast<float>(texSize.x),
                    size.y / static_cast<float>(texSize.y)
                );
            }
        }
    }
};

#endif
