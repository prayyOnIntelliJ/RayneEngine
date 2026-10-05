#ifndef RAYNEENGINE_COMPONENTS_H
#define RAYNEENGINE_COMPONENTS_H

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <vector>
#include <string>
#include <algorithm>
#include <optional>

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

struct AnimationClip
{
    std::string name = "default";
    int startFrame = 0;
    int frameCount = 1;
    float fps = 10.0f;
    bool loop = true;
};

struct SpriteAnimationComponent
{
    int columns = 1;
    int rows = 1;
    int frameWidth = 0;
    int frameHeight = 0;

    std::string currentClip = "default";
    int currentFrame = 0;
    float elapsedTime = 0.0f;
    float playbackSpeed = 1.0f;
    bool isPlaying = true;

    std::vector<AnimationClip> clips;

    SpriteAnimationComponent()
    {
        clips.push_back(AnimationClip{"default", 0, 1, 10.0f, true});
    }

    SpriteAnimationComponent(int cols, int r, float fps = 10.0f, bool loop = true)
        : columns(std::max(1, cols)), rows(std::max(1, r))
    {
        clips.push_back(AnimationClip{"default", 0, columns * rows, fps, loop});
    }

    AnimationClip *GetClip(const std::string &clipName)
    {
        for (auto &c : clips)
        {
            if (c.name == clipName) return &c;
        }
        return nullptr;
    }

    const AnimationClip *GetClip(const std::string &clipName) const
    {
        for (const auto &c : clips)
        {
            if (c.name == clipName) return &c;
        }
        return nullptr;
    }

    void AddOrUpdateClip(const AnimationClip &clip)
    {
        for (auto &c : clips)
        {
            if (c.name == clip.name)
            {
                c = clip;
                return;
            }
        }
        clips.push_back(clip);
    }

    void RemoveClip(const std::string &clipName)
    {
        std::erase_if(clips, [&](const AnimationClip &c) { return c.name == clipName; });
        if (currentClip == clipName && !clips.empty())
        {
            currentClip = clips.front().name;
            currentFrame = 0;
            elapsedTime = 0.0f;
        }
    }

    void Play(const std::string &clipName = "")
    {
        Play(clipName, -1);
    }

    void Play(const std::string &clipName, int loopOverride)
    {
        if (!clipName.empty())
        {
            if (currentClip != clipName)
            {
                currentClip = clipName;
                currentFrame = 0;
                elapsedTime = 0.0f;
            }
        }
        AnimationClip *c = GetClip(currentClip);
        if (!c && !clips.empty())
        {
            currentClip = clips.front().name;
            c = &clips.front();
        }
        if (c && loopOverride >= 0)
        {
            c->loop = (loopOverride != 0);
        }
        isPlaying = true;
    }

    void Stop()
    {
        isPlaying = false;
        currentFrame = 0;
        elapsedTime = 0.0f;
    }

    void Pause()
    {
        isPlaying = false;
    }

    void Resume()
    {
        isPlaying = true;
    }

    void SetPlaybackSpeed(float speed)
    {
        playbackSpeed = speed;
    }

    void SetFrame(int frame)
    {
        const AnimationClip *c = GetClip(currentClip);
        int maxFrames = c ? c->frameCount : (columns * rows);
        if (maxFrames > 0)
        {
            currentFrame = std::clamp(frame, 0, maxFrames - 1);
        }
        else
        {
            currentFrame = 0;
        }
        elapsedTime = 0.0f;
    }

    int GetGlobalFrameIndex() const
    {
        const AnimationClip *c = GetClip(currentClip);
        if (!c && !clips.empty()) c = &clips.front();
        int base = c ? c->startFrame : 0;
        int count = c ? c->frameCount : (columns * rows);
        int local = (count > 0) ? std::clamp(currentFrame, 0, count - 1) : 0;
        int total = std::max(1, columns * rows);
        return (base + local) % total;
    }

    sf::FloatRect GetCurrentTextureRect(sf::Vector2u textureSize) const
    {
        if (columns <= 0 || rows <= 0 || textureSize.x == 0 || textureSize.y == 0)
        {
            return sf::FloatRect(0.f, 0.f, static_cast<float>(textureSize.x), static_cast<float>(textureSize.y));
        }

        float fw = (frameWidth > 0) ? static_cast<float>(frameWidth) : (static_cast<float>(textureSize.x) / static_cast<float>(columns));
        float fh = (frameHeight > 0) ? static_cast<float>(frameHeight) : (static_cast<float>(textureSize.y) / static_cast<float>(rows));

        int globalFrame = GetGlobalFrameIndex();
        int col = globalFrame % columns;
        int row = globalFrame / columns;

        return sf::FloatRect(static_cast<float>(col) * fw, static_cast<float>(row) * fh, fw, fh);
    }

    void Update(float dt)
    {
        if (!isPlaying || playbackSpeed == 0.0f || dt <= 0.0f) return;

        const AnimationClip *c = GetClip(currentClip);
        if (!c && !clips.empty()) c = &clips.front();
        if (!c) return;

        if (c->frameCount <= 1 || c->fps <= 0.0f) return;

        float frameDuration = 1.0f / (c->fps * std::abs(playbackSpeed));
        elapsedTime += dt;
        while (elapsedTime >= frameDuration)
        {
            elapsedTime -= frameDuration;
            currentFrame++;
            if (currentFrame >= c->frameCount)
            {
                if (c->loop)
                {
                    currentFrame = 0;
                }
                else
                {
                    currentFrame = c->frameCount - 1;
                    isPlaying = false;
                    break;
                }
            }
        }
    }
};

enum class CameraMultiFollowMode
{
    Priority = 0,
    Average = 1,
    AutoFrame = 2
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

    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float sizeX = 0.0f;   // 0.0f = auto-fit entity bounds
    float sizeY = 0.0f;   // 0.0f = auto-fit entity bounds
    float radius = 0.0f;  // 0.0f = auto-fit entity bounds (min(w, h) * 0.5f)

    CollisionComponent() = default;
    CollisionComponent(int ch, CollisionType t = CollisionType::Solid, bool trig = false, ColliderShape sh = ColliderShape::Box)
        : channel(ch), type(t), isTrigger(trig), shape(sh) {}

    sf::Vector2f GetEffectiveSize(sf::Vector2f defaultEntitySize) const
    {
        return sf::Vector2f(
            (sizeX > 0.001f) ? sizeX : defaultEntitySize.x,
            (sizeY > 0.001f) ? sizeY : defaultEntitySize.y
        );
    }

    float GetEffectiveRadius(sf::Vector2f defaultEntitySize) const
    {
        if (radius > 0.001f) return radius;
        sf::Vector2f eff = GetEffectiveSize(defaultEntitySize);
        return std::min(eff.x, eff.y) * 0.5f;
    }
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

struct NameComponent
{
    std::string name;
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
    int alignment = 0;
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
    float emissionRate = 25.0f;
    float lifetime = 1.5f;
    float speed = 120.0f;
    float speedVariance = 40.0f;
    float angle = -90.0f;
    float spreadAngle = 45.0f;
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
