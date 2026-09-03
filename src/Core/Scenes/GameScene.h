#ifndef RAYNEENGINE_GAMESCENE_H
#define RAYNEENGINE_GAMESCENE_H
#include "Scene.h"
#include "../ECS/Registry.h"
#include "../Resources/ResourceManager.h"
#include "SFML/Graphics/Text.hpp"

class GameScene : public Scene
{
public:
    GameScene(SceneManager &manager, sf::RenderWindow &window, Registry &registry);

    void HandleEvent(const sf::Event &event) override;

    void Update(float deltaTime) override;

    void Render(sf::RenderWindow &window) override;

    void OnEnter() override;

    void OnExit() override;

private:
    sf::RenderWindow &m_Window;
    Registry &m_Registry;

    sf::View m_Camera;
    std::shared_ptr<sf::Font> m_Font;
    sf::Text m_DebugText;

    void CheckCollisions();

    std::vector<std::pair<Entity, Entity> > m_LastCollisions;
};

#endif
