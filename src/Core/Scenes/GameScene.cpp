#include "GameScene.h"
#include "../Scenes/SceneManager.h"
#include <iostream>
#include "../Application/Application.h"

#include "../ECS/Components.h"
#include "../Scripting/EventManager.h"
#include "../UI/UIManager.h"
#include "../Scripting/ScriptComponent.h"
#include "../Scripting/TimerManager.h"
#include "../Scripting/TweenManager.h"
#include "../Application/EngineVersion.h"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/CircleShape.hpp"
#include "SFML/Window/Event.hpp"

GameScene::GameScene(SceneManager &manager, sf::RenderWindow &window, Registry &registry)
    : Scene(manager), m_Window(window), m_Registry(registry)
{
    m_Font = ResourceManager::Get().GetFont(ENGINE_ASSET_PATH "/fonts/Merriweather.ttf");

    m_DebugText.setFont(*m_Font);
    m_DebugText.setCharacterSize(12);
    m_DebugText.setFillColor(sf::Color(180, 180, 180));
    m_DebugText.setPosition(8.f, 8.f);
}

void GameScene::OnEnter()
{
    std::cout << "[INFO] [GameScene] Starting simulation...\n";

#ifndef RAYNE_STANDALONE
    m_Window.setTitle(
        (g_App ? g_App->GetProjectName() : std::string(Rayne::DEFAULT_PROJECT_NAME))
        + ": Play Mode"
        + " (" + Rayne::PlatformString() + ")"
        + " - RayneEngine " + Rayne::VersionString());
#endif

    m_Camera = m_Window.getDefaultView();
    m_LastCollisions.clear();

    EventManager::Get().SubscribeCollision([this](CollisionEvent e) {
        if (m_Registry.HasComponent<ScriptComponent>(e.a))
            m_Registry.GetComponent<ScriptComponent>(e.a).OnCollision(e.b);

        if (m_Registry.HasComponent<ScriptComponent>(e.b))
            m_Registry.GetComponent<ScriptComponent>(e.b).OnCollision(e.a);
    });

    std::cout << "[INFO] [GameScene] Firing OnCreate() for all active scripts...\n";
    m_Registry.ForEach<ScriptComponent>([](Entity, ScriptComponent &sc) { sc.OnCreate(); });
}

void GameScene::OnExit()
{
    m_LastCollisions.clear();
    TimerManager::Get().Clear();
    TweenManager::Get().Clear();
    EventManager::Get().Clear();
    std::cout << "[INFO] [GameScene] Stopping simulation, clearing collision state.\n";
}

void GameScene::CheckCollisions()
{
    struct CollidableEntity
    {
        Entity id;
        float x, y, w, h;
        int channel;
        CollisionType type;
    };
    std::vector<CollidableEntity> collidables;

    m_Registry.ForEach<TransformComponent, RenderComponent>(
        [&collidables, this](Entity e, TransformComponent &t, RenderComponent &r) {
            if (m_Registry.HasComponent<CollisionComponent>(e))
            {
                auto &col = m_Registry.GetComponent<CollisionComponent>(e);
                collidables.push_back({e, t.x, t.y, r.size.x, r.size.y, col.channel, col.type});
            }
        });

    std::vector<std::pair<Entity, Entity> > currentCollisions;

    for (size_t i = 0; i < collidables.size(); i++)
    {
        for (size_t j = i + 1; j < collidables.size(); j++)
        {
            auto &a = collidables[i];
            auto &b = collidables[j];

            if (a.channel != b.channel) continue;

            const bool overlapping =
                    a.x < b.x + b.w && a.x + a.w > b.x &&
                    a.y < b.y + b.h && a.y + a.h > b.y;

            if (!overlapping) continue;

            Entity e1 = std::min(a.id, b.id);
            Entity e2 = std::max(a.id, b.id);

            currentCollisions.emplace_back(e1, e2);

            const bool wasColliding = std::find(
                                          m_LastCollisions.begin(), m_LastCollisions.end(),
                                          std::make_pair(e1, e2)) != m_LastCollisions.end();

            if (!wasColliding)
                EventManager::Get().FireCollision(a.id, b.id);
                
            if (a.type == CollisionType::Solid && b.type == CollisionType::Solid)
            {
                float aCenterX = a.x + a.w / 2.0f;
                float aCenterY = a.y + a.h / 2.0f;
                float bCenterX = b.x + b.w / 2.0f;
                float bCenterY = b.y + b.h / 2.0f;

                float dx = aCenterX - bCenterX;
                float dy = aCenterY - bCenterY;
                
                float overlapX = (a.w / 2.0f + b.w / 2.0f) - std::abs(dx);
                float overlapY = (a.h / 2.0f + b.h / 2.0f) - std::abs(dy);

                if (overlapX > 0 && overlapY > 0)
                {
                    if (!m_Registry.HasComponent<TransformComponent>(a.id) || !m_Registry.HasComponent<TransformComponent>(b.id))
                        continue; // Entities might have been destroyed by OnCollision scripts

                    bool aMovable = m_Registry.HasComponent<VelocityComponent>(a.id);
                    bool bMovable = m_Registry.HasComponent<VelocityComponent>(b.id);
                    
                    if (overlapX < overlapY)
                    {
                        float pushX = (dx > 0) ? overlapX : -overlapX;
                        if (aMovable && !bMovable) {
                            m_Registry.GetComponent<TransformComponent>(a.id).x += pushX;
                            a.x += pushX;
                        } else if (!aMovable && bMovable) {
                            m_Registry.GetComponent<TransformComponent>(b.id).x -= pushX;
                            b.x -= pushX;
                        } else {
                            m_Registry.GetComponent<TransformComponent>(a.id).x += pushX / 2.0f;
                            a.x += pushX / 2.0f;
                            m_Registry.GetComponent<TransformComponent>(b.id).x -= pushX / 2.0f;
                            b.x -= pushX / 2.0f;
                        }
                    }
                    else
                    {
                        float pushY = (dy > 0) ? overlapY : -overlapY;
                        if (aMovable && !bMovable) {
                            m_Registry.GetComponent<TransformComponent>(a.id).y += pushY;
                            a.y += pushY;
                        } else if (!aMovable && bMovable) {
                            m_Registry.GetComponent<TransformComponent>(b.id).y -= pushY;
                            b.y -= pushY;
                        } else {
                            m_Registry.GetComponent<TransformComponent>(a.id).y += pushY / 2.0f;
                            a.y += pushY / 2.0f;
                            m_Registry.GetComponent<TransformComponent>(b.id).y -= pushY / 2.0f;
                            b.y -= pushY / 2.0f;
                        }
                    }
                }
            }
        }
    }

    m_LastCollisions = std::move(currentCollisions);
}

void GameScene::HandleEvent(const sf::Event &event)
{
#ifndef RAYNE_STANDALONE
    if (event.type == sf::Event::KeyPressed &&
        event.key.code == sf::Keyboard::Escape) { m_manager.SwitchSceneTo("editor"); }
#else
    if (event.type == sf::Event::KeyPressed &&
        event.key.code == sf::Keyboard::Escape) { m_Window.close(); }
#endif
}

void GameScene::Update(float deltaTime)
{
    m_Registry.ForEach<TransformComponent, VelocityComponent>(
        [deltaTime](Entity, TransformComponent &t, VelocityComponent &v) {
            t.x += v.dx * deltaTime;
            t.y += v.dy * deltaTime;
        });

#ifndef RAYNE_STANDALONE
    m_HotReloadTimer += deltaTime;
    if (m_HotReloadTimer >= 0.5f)
    {
        m_HotReloadTimer = 0.f;
        m_Registry.ForEach<ScriptComponent>([](Entity, ScriptComponent &sc) { sc.ReloadIfNeeded(); });
    }
#endif

    m_Registry.ForEach<ScriptComponent>([deltaTime](Entity, ScriptComponent &sc) { sc.OnUpdate(deltaTime); });

    TimerManager::Get().Update(deltaTime);
    TweenManager::Get().Update(deltaTime);

    CheckCollisions();

    m_Registry.ForEach<TransformComponent, CameraComponent>(
        [this](Entity, TransformComponent &t, CameraComponent &c) { if (c.active) { m_Camera.setCenter(t.x, t.y); } });

    bool mouseClicked = sf::Mouse::isButtonPressed(sf::Mouse::Left);
    static bool wasClicked = false;
    bool justClicked = mouseClicked && !wasClicked;
    bool justReleased = !mouseClicked && wasClicked;
    wasClicked = mouseClicked;

    UIManager::Get().ClearClickedButton();
}

void GameScene::Render(sf::RenderWindow &window)
{
    static bool wasClicked = false;
    bool mouseClicked = sf::Mouse::isButtonPressed(sf::Mouse::Left);
    bool justClicked = mouseClicked && !wasClicked;
    bool justReleased = !mouseClicked && wasClicked;
    wasClicked = mouseClicked;

    sf::View uiView(sf::FloatRect(0.f, 0.f, 1920.f, 1080.f));
    uiView.setViewport(sf::FloatRect(0.f, 0.f, 1.f, 1.f));

    sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos, uiView);

    UIManager::Get().Update(0.016f, mousePos, justClicked, justReleased);
    window.setView(m_Camera);

    m_Registry.ForEach<TransformComponent, RenderComponent>(
        [&](Entity e, TransformComponent &t, RenderComponent &r) {
            if (m_Registry.HasComponent<SpriteComponent>(e))
            {
                auto &sc = m_Registry.GetComponent<SpriteComponent>(e);
                sc.sprite.setPosition(t.x, t.y);
                sc.sprite.setRotation(t.rotation);
                
                float baseScaleX = 1.f, baseScaleY = 1.f;
                if (sc.texture) {
                    auto texSize = sc.texture->getSize();
                    if (texSize.x > 0 && texSize.y > 0) {
                        baseScaleX = sc.size.x / static_cast<float>(texSize.x);
                        baseScaleY = sc.size.y / static_cast<float>(texSize.y);
                    }
                }
                sc.sprite.setScale(baseScaleX * t.scaleX, baseScaleY * t.scaleY);
                
                window.draw(sc.sprite);
            } else
            {
                if (r.shapeType == ShapeType::Rectangle)
                {
                    sf::RectangleShape shape(r.size);
                    shape.setPosition(t.x, t.y);
                    shape.setRotation(t.rotation);
                    shape.setScale(t.scaleX, t.scaleY);
                    shape.setFillColor(r.color);
                    window.draw(shape);
                } else
                {
                    sf::CircleShape circle;
                    switch (r.shapeType)
                    {
                        case ShapeType::Triangle: circle.setPointCount(3);
                            break;
                        case ShapeType::Pentagon: circle.setPointCount(5);
                            break;
                        case ShapeType::Hexagon: circle.setPointCount(6);
                            break;
                        case ShapeType::Circle: default: circle.setPointCount(30);
                            break;
                    }
                    circle.setRadius(r.size.x / 2.f);
                    circle.setPosition(t.x, t.y);
                    circle.setRotation(t.rotation);
                    circle.setScale(t.scaleX, t.scaleY);
                    circle.setFillColor(r.color);
                    window.draw(circle);
                }
            }
        });

    window.setView(uiView);
    UIManager::Get().Render(window);

    window.setView(window.getDefaultView());
#ifndef RAYNE_STANDALONE
    m_DebugText.setString("GAME  |  Esc: back to editor");
    window.draw(m_DebugText);
#endif
}
