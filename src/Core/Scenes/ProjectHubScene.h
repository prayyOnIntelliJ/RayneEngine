#ifndef PROJECTHUBSCENE_H
#define PROJECTHUBSCENE_H

#include "../Scenes/Scene.h"
#include "../Scenes/SceneManager.h"
#include "SFML/Graphics/RenderWindow.hpp"

class ProjectHubScene : public Scene
{
public:
    ProjectHubScene(SceneManager &manager, sf::RenderWindow &window);
    ~ProjectHubScene() override;

    void HandleEvent(const sf::Event &event) override;
    void Update(float deltaTime) override;
    void Render(sf::RenderWindow &window) override;

    void OnEnter() override;
    void OnExit() override;

private:
    void SetupUI();
    void CreateProject();

    sf::RenderWindow &m_Window;
    bool m_UIsCreated = false;
};

#endif
