#ifndef RAYNEENGINE_APPLICATION_H
#define RAYNEENGINE_APPLICATION_H
#include "../Primitives/PrimitiveManager.h"
#include "../Scenes/SceneManager.h"
#include "../ViewPort/EditorCamera2D.h"
#include "SFML/Graphics/RenderWindow.hpp"

class Application {
public:
    Application();
    void Run();

private:
    void CreateWindow();
    void SetIcon();
    void Update(float deltaTime);
    void Render();
    void SetEvents();

    sf::RenderWindow m_renderWindow;
    EditorCamera2D m_editorCamera;
    PrimitiveManager m_primitiveManager;
    SceneManager m_sceneManager;
    sf::Clock m_deltaTimeClock;
};


#endif