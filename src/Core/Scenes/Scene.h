#ifndef SCENE_H
#define SCENE_H

#include "SFML/Graphics/RenderWindow.hpp"

class SceneManager;

class Scene {
public:
    explicit Scene(SceneManager& manager) : m_manager(manager) {}

    virtual ~Scene() = default;

    virtual void HandleEvent(const sf::Event& event) = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render(sf::RenderWindow& window) = 0;

    virtual void OnEnter() {};
    virtual void OnExit() {}
    virtual void OnPause() {}
    virtual void OnResume() {}

protected:
    SceneManager& m_manager;
};

#endif
