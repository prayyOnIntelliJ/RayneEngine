#ifndef RAYNEENGINE_APPLICATION_H
#define RAYNEENGINE_APPLICATION_H
#include "../ECS/Registry.h"
#include "../Primitives/PrimitiveManager.h"
#include "../Scenes/SceneManager.h"
#include "../ViewPort/EditorCamera2D.h"
#include "SFML/Graphics/RenderWindow.hpp"

class Application {
public:
    Application();
    void Run();

    Registry m_Registry;

private:
    void CreateWindow();
    void SetIcon();
    void Update(float deltaTime);
    void Render();
    void SetEvents();

    sf::RenderWindow m_RenderWindow;
    EditorCamera2D m_EditorCamera;
    PrimitiveManager m_PrimitiveManager;
    SceneManager m_SceneManager;
    sf::Clock m_DeltaTimeClock;
};


#endif