#ifndef RAYNEENGINE_COMPONENTS_H
#define RAYNEENGINE_COMPONENTS_H

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <vector>

#include "Entity.h"
#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/Texture.hpp"
#include "../Resources/ResourceManager.h"

struct TransformComponent
{
    float x = 0.f;
    float y = 0.f;
    float rotation = 0.f;
    float scaleX = 1.f;
    float scaleY = 1.f;

    // Computed world transform
    float worldX = 0.f;
    float worldY = 0.f;
    float worldRotation = 0.f;
    float worldScaleX = 1.f;
    float worldScaleY = 1.f;
};

struct HierarchyComponent
{
    Entity parent = NULL_ENTITY;
    std::vector<Entity> children;
};

struct VelocityComponent
{
    float dx, dy;
};

enum class ShapeType { Rectangle, Circle, Triangle, Pentagon, Hexagon };

struct RenderComponent
{
    sf::Color color;
    sf::Vector2f size;
    ShapeType shapeType = ShapeType::Rectangle;
    int zIndex = 0;
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
            sprite.setTexture(*texture, true);
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

struct CameraComponent
{
    bool active = true;
};

enum class CollisionType { Static, Solid };
enum class BodyType { Dynamic, Kinematic, Static };
enum class ColliderShape { Box, Circle };

struct CollisionComponent
{
    int channel = 0;
    CollisionType type = CollisionType::Solid;
    bool isTrigger = false;
    ColliderShape shape = ColliderShape::Box;
};

struct Rigidbody2DComponent
{
    BodyType bodyType = BodyType::Dynamic;
    float mass = 1.0f;
    float gravityScale = 1.0f;
    float restitution = 0.0f; // Bounciness (0 = no bounce, 1 = elastic)
    float drag = 0.05f;       // Linear drag / damping
    bool freezeRotation = true;

    // Accumulated external continuous forces
    float forceX = 0.f;
    float forceY = 0.f;
};

struct TagComponent
{
    std::string tag;
};

#endif
