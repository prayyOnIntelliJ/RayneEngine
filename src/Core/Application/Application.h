#ifndef RAYNEENGINE_APPLICATION_H
#define RAYNEENGINE_APPLICATION_H
#include "../ECS/Registry.h"
#include "../Primitives/PrimitiveManager.h"
#include "../Scenes/SceneManager.h"
#include "SFML/Graphics/RenderWindow.hpp"

class Application
{
public:
    Application();

    void Run();

    Registry m_Registry;

private:
    void CreateEngineWindow();

    void SetIcon();

    void Update(float deltaTime);

    void Render();

    void SetEvents();

    sf::RenderWindow m_RenderWindow;
    PrimitiveManager m_PrimitiveManager;
    SceneManager m_SceneManager;
    sf::Clock m_DeltaTimeClock;
};


#endif

#ifndef ASSET_PATH
#define ASSET_PATH "assets/"
#endif
