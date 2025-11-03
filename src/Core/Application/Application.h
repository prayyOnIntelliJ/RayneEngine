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

    sf::RenderWindow renderWindow;
    EditorCamera2D editorCamera;
    PrimitiveManager primitiveManager;
    SceneManager sceneManager;
    sf::Clock deltaTimeClock;
};


#endif