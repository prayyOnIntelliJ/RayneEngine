#ifndef EDITOROBJECT_H
#define EDITOROBJECT_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <nlohmann/json.hpp>

#include "../../Runtime/ECS/Entity.h"
#include "../../Runtime/ECS/Components.h"
#include "../../Runtime/Scripting/ScriptComponent.h"

enum class ObjectType
{
    Rectangle, Circle, Triangle, Pentagon, Hexagon, Sprite, Camera,
    Empty, SpawnPoint, TriggerZone, PhysicsBox, PhysicsBall, StaticPlatform,
    WorldText, AudioSource, ParticleEmitter
};

struct EditorObject
{
    std::string id;
    std::string tag;
    Entity entity = 0;
    std::string parentId = "";

    sf::Vector2f localPosition = {0.f, 0.f};
    float rotation = 0.f;
    float scaleX = 1.f;
    float scaleY = 1.f;

    sf::Vector2f worldPosition = {0.f, 0.f};
    float worldRotation = 0.f;
    float worldScaleX = 1.f;
    float worldScaleY = 1.f;

    sf::RectangleShape shape;
    sf::CircleShape circleShape;
    sf::Color color;
    bool selected = false;
    std::string scriptPath;
    ObjectType objectType = ObjectType::Rectangle;
    std::string spritePath;
    std::shared_ptr<sf::Texture> previewTexture;
    sf::Sprite previewSprite;
    int zIndex = 0;
    bool visibleInGame = true;
    std::string templatePath = "";
    std::map<std::string, ScriptComponent::Property> scriptProperties;
    std::string textString = "World Text";
    unsigned int textFontSize = 28;
    sf::Color textColor = sf::Color::White;
    int textAlignment = 0;
    std::string audioClipPath = "";
    float audioVolume = 100.0f;
    float audioPitch = 1.0f;
    bool audioLoop = false;
    bool audioPlayOnStart = true;
    bool audioIsSpatial = false;
    bool particleEmitting = true;
    int particleMaxParticles = 120;
    float particleRate = 25.0f;
    float particleLifetime = 1.5f;
    float particleSpeed = 120.0f;
    float particleAngle = -90.0f;
    float particleSpread = 45.0f;
    float particleStartSize = 8.0f;
    float particleEndSize = 2.0f;
    sf::Color particleStartColor = sf::Color(255, 190, 50, 255);
    sf::Color particleEndColor = sf::Color(255, 50, 20, 0);
    float particleGravityX = 0.0f;
    float particleGravityY = 60.0f;
    std::vector<Particle> editorParticles;
    float particleTimer = 0.0f;
};

struct InspectorButton
{
    sf::FloatRect bounds;
    std::string action;
};

struct MenuItem
{
    std::string label;
    std::string action;
    bool isSeparator = false;
    std::string shortcut;
};

struct MenuEntry
{
    std::string label;
    std::vector<MenuItem> items;
    sf::FloatRect bounds;
};

#endif // EDITOROBJECT_H
