#include "GameScene.h"
#include "../Scenes/SceneManager.h"
#include <iostream>

#include "../ECS/Components.h"
#include "../Scripting/EventManager.h"
#include "../Scripting/ScriptComponent.h"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Window/Event.hpp"

GameScene::GameScene(SceneManager& manager, sf::RenderWindow& window, Registry& registry)
    : Scene(manager), m_Window(window), m_Registry(registry)
{
    m_Font.loadFromFile(ASSET_PATH "fonts/Merriweather.ttf");

    m_DebugText.setFont(m_Font);
    m_DebugText.setCharacterSize(12);
    m_DebugText.setFillColor(sf::Color(180, 180, 180));
    m_DebugText.setPosition(8.f, 8.f);
}

void GameScene::OnEnter()
{
    std::cout << "[GameScene] Started\n";

    m_Camera = m_Window.getDefaultView();

    EventManager::Get().SubscribeCollision([this](CollisionEvent e)
    {
        if (m_Registry.HasComponent<ScriptComponent>(e.a))
            m_Registry.GetComponent<ScriptComponent>(e.a).OnCollision(e.b);

        if (m_Registry.HasComponent<ScriptComponent>(e.b))
            m_Registry.GetComponent<ScriptComponent>(e.b).OnCollision(e.a);
    });

    m_Registry.ForEach<ScriptComponent>([](Entity, ScriptComponent& sc)
    {
        sc.OnCreate();
    });

    CheckCollisions();
}

void GameScene::OnExit()
{
    std::cout << "[GameScene] Stopped\n";
}

void GameScene::CheckCollisions()
{
    struct CollidableEntity { Entity id; float x, y, w, h; };
    std::vector<CollidableEntity> collidables;

    m_Registry.ForEach<TransformComponent, RenderComponent>(
        [&collidables](Entity e, TransformComponent& t, RenderComponent& r)
        {
            collidables.push_back({ e, t.x, t.y, r.size.x, r.size.y });
        });

    std::vector<std::pair<Entity, Entity>> currentCollisions;

    for (size_t i = 0; i < collidables.size(); i++)
    {
        for (size_t j = i + 1; j < collidables.size(); j++)
        {
            const auto& a = collidables[i];
            const auto& b = collidables[j];

            const bool overlapping =
                a.x < b.x + b.w && a.x + a.w > b.x &&
                    a.y < b.y + b.h && a.y + a.h > b.y;

            if (!overlapping) continue;

            currentCollisions.emplace_back(a.id, b.id);

            const bool wasColliding = std::find(
                m_LastCollisions.begin(), m_LastCollisions.end(),
                std::make_pair(a.id, b.id)) != m_LastCollisions.end();

            if (!wasColliding)
                EventManager::Get().FireCollision(a.id, b.id);
        }
    }
}

void GameScene::HandleEvent(const sf::Event& event)
{
    if (event.type == sf::Event::KeyPressed &&
        event.key.code == sf::Keyboard::Escape)
    {
        m_manager.SwitchSceneTo("editor");
    }
}

void GameScene::Update(float deltaTime)
{
    m_Registry.ForEach<TransformComponent, VelocityComponent>(
        [deltaTime](Entity, TransformComponent& t, VelocityComponent& v)
        {
            t.x += v.dx * deltaTime;
            t.y += v.dy * deltaTime;
        });

    m_Registry.ForEach<ScriptComponent>([deltaTime](Entity, ScriptComponent& sc)
    {
        sc.OnUpdate(deltaTime);
    });
}

void GameScene::Render(sf::RenderWindow& window)
{
    window.setView(m_Camera);

    m_Registry.ForEach<TransformComponent, RenderComponent>(
        [&window](Entity, TransformComponent& t, RenderComponent& r)
        {
            sf::RectangleShape shape(r.size);
            shape.setPosition(t.x, t.y);
            shape.setFillColor(r.color);
            window.draw(shape);
        });

    window.setView(window.getDefaultView());
    m_DebugText.setString("GAME  |  Esc: back to editor");
    window.draw(m_DebugText);
}