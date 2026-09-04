#include "GameScene.h"
#include "../Scenes/SceneManager.h"
#include <iostream>

#include "../ECS/Components.h"
#include "../Scripting/EventManager.h"
#include "../UI/UIManager.h"
#include "../Scripting/ScriptComponent.h"
#include "../Application/EngineVersion.h"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/CircleShape.hpp"
#include "SFML/Window/Event.hpp"

GameScene::GameScene(SceneManager &manager, sf::RenderWindow &window, Registry &registry)
    : Scene(manager), m_Window(window), m_Registry(registry)
{
    m_Font = ResourceManager::Get().GetFont(ASSET_PATH "fonts/Merriweather.ttf");

    m_DebugText.setFont(*m_Font);
    m_DebugText.setCharacterSize(12);
    m_DebugText.setFillColor(sf::Color(180, 180, 180));
    m_DebugText.setPosition(8.f, 8.f);
}

void GameScene::OnEnter()
{
    std::cout << "[INFO] [GameScene] Starting simulation...\n";

    m_Window.setTitle(
        std::string(Rayne::DEFAULT_PROJECT_NAME)
        + ": Play Mode"
        + " (" + Rayne::PlatformString() + ")"
        + " - RayneEngine " + Rayne::VersionString());

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
    std::cout << "[INFO] [GameScene] Stopping simulation, clearing collision state.\n";
}

void GameScene::CheckCollisions()
{
    struct CollidableEntity
    {
        Entity id;
        float x, y, w, h;
        int channel;
    };
    std::vector<CollidableEntity> collidables;

    m_Registry.ForEach<TransformComponent, RenderComponent>(
        [&collidables, this](Entity e, TransformComponent &t, RenderComponent &r) {
            if (m_Registry.HasComponent<CollisionComponent>(e))
            {
                auto &col = m_Registry.GetComponent<CollisionComponent>(e);
                collidables.push_back({e, t.x, t.y, r.size.x, r.size.y, col.channel});
            }
        });

    std::vector<std::pair<Entity, Entity> > currentCollisions;

    for (size_t i = 0; i < collidables.size(); i++)
    {
        for (size_t j = i + 1; j < collidables.size(); j++)
        {
            const auto &a = collidables[i];
            const auto &b = collidables[j];

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
        }
    }

    m_LastCollisions = std::move(currentCollisions);
}

void GameScene::HandleEvent(const sf::Event &event)
{
    if (event.type == sf::Event::KeyPressed &&
        event.key.code == sf::Keyboard::Escape) { m_manager.SwitchSceneTo("editor"); }
}

void GameScene::Update(float deltaTime)
{
    m_Registry.ForEach<TransformComponent, VelocityComponent>(
        [deltaTime](Entity, TransformComponent &t, VelocityComponent &v) {
            t.x += v.dx * deltaTime;
            t.y += v.dy * deltaTime;
        });

    m_Registry.ForEach<ScriptComponent>([deltaTime](Entity, ScriptComponent &sc) { sc.OnUpdate(deltaTime); });

    CheckCollisions();

    m_Registry.ForEach<TransformComponent, CameraComponent>(
        [this](Entity, TransformComponent &t, CameraComponent &c) { if (c.active) { m_Camera.setCenter(t.x, t.y); } });

    bool mouseClicked = sf::Mouse::isButtonPressed(sf::Mouse::Left);
    // Simple way: Update is called per frame, UIManager tracks state
    // We actually need accurate mouse events, but for simplicity we'll just poll
    // In a real scenario we'd pass events from HandleEvent
    static bool wasClicked = false;
    bool justClicked = mouseClicked && !wasClicked;
    bool justReleased = !mouseClicked && wasClicked;
    wasClicked = mouseClicked;

    // We need window reference or input manager for mouse pos relative to window.
    // The engine's InputManager has GetMousePosition() which might be relative to desktop if not passed window.
    // Let's rely on sf::Mouse::getPosition(*m_RenderWindow) if we have it, but we don't.
    // So we'll update UIManager in HandleEvent, or pass window to Update.
    // Since GameScene doesn't get window in Update, let's just clear the clicked button state here:
    UIManager::Get().ClearClickedButton();
    // And actually, we can update it in Render since we get the window there, or just poll global mouse for now.
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
                window.draw(sc.sprite);
            } else
            {
                if (r.shapeType == ShapeType::Rectangle)
                {
                    sf::RectangleShape shape(r.size);
                    shape.setPosition(t.x, t.y);
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
                    circle.setFillColor(r.color);
                    window.draw(circle);
                }
            }
        });

    window.setView(uiView);
    UIManager::Get().Render(window);

    window.setView(window.getDefaultView());
    m_DebugText.setString("GAME  |  Esc: back to editor");
    window.draw(m_DebugText);
}
