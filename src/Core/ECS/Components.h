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
    bool visibleInGame = true;
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

enum class CameraMultiFollowMode {
    Priority = 0,    // Follows highest priority active target (first if tied)
    Average = 1,     // Centers camera at midpoint of all active targets
    AutoFrame = 2    // Centers at midpoint AND dynamically adjusts zoom so all targets stay in view
};

struct CameraComponent
{
    bool active = true;
    float smoothSpeed = 0.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float zoom = 1.0f;
    int priority = 0;
    CameraMultiFollowMode multiFollowMode = CameraMultiFollowMode::Average;
    float minZoom = 0.3f;
    float maxZoom = 3.0f;
    float autoFramePadding = 200.0f;
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
    float restitution = 0.0f;
    float drag = 0.05f;
    bool freezeRotation = true;

    float forceX = 0.f;
    float forceY = 0.f;
};

struct TagComponent
{
    std::string tag;
};

struct AudioSourceComponent
{
    std::string soundPath = "";
    float volume = 100.0f;
    float pitch = 1.0f;
    bool loop = false;
    bool playOnStart = true;
    bool isSpatial = false;
    float minDistance = 150.0f;
    float attenuation = 1.0f;
    bool isPlaying = false;
    int channelIndex = -1;
};

struct TextComponent
{
    std::string text = "World Text";
    std::string fontPath = "";
    unsigned int characterSize = 28;
    sf::Color color = sf::Color::White;
    int alignment = 0; // 0 = Left, 1 = Center, 2 = Right
    sf::Color outlineColor = sf::Color::Black;
    float outlineThickness = 0.0f;
};

struct Particle
{
    sf::Vector2f position;
    sf::Vector2f velocity;
    float lifetime = 0.f;
    float maxLifetime = 1.f;
    float size = 8.f;
    sf::Color color = sf::Color::White;
};

struct ParticleEmitterComponent
{
    bool emitting = true;
    int maxParticles = 120;
    float emissionRate = 25.0f; // particles per second
    float lifetime = 1.5f;      // seconds
    float speed = 120.0f;
    float speedVariance = 40.0f;
    float angle = -90.0f;       // degrees, -90 is upwards
    float spreadAngle = 45.0f;  // degrees cone spread
    float startSize = 8.0f;
    float endSize = 2.0f;
    sf::Color startColor = sf::Color(255, 190, 50, 255);
    sf::Color endColor = sf::Color(255, 50, 20, 0);
    float gravityX = 0.0f;
    float gravityY = 60.0f;

    std::vector<Particle> particles;
    float spawnAccumulator = 0.0f;
};

#endif
