#ifndef RAYNEENGINE_APPLICATION_H
#define RAYNEENGINE_APPLICATION_H
#include "../ECS/Registry.h"
#include "../Scenes/SceneManager.h"
#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/Graphics/Font.hpp"
#include "SFML/Graphics/Texture.hpp"
#include <string>

extern class Application* g_App;

class Application
{
public:
    Application();

    void Run();

    Registry m_Registry;
    SceneManager& GetSceneManager() { return m_SceneManager; }
    const std::string& GetProjectName() const { return m_ProjectName; }

private:
    void SetIcon();

    void Update(float deltaTime);

    void Render();

    void SetEvents();

    void RunSplashSequence();

    std::string m_ProjectName = "RayneEngine";
    std::string m_StartScene = "scenes/game.json";
    sf::RenderWindow m_RenderWindow;
    SceneManager m_SceneManager;
    sf::Clock m_DeltaTimeClock;
};

#endif

#ifndef ASSET_PATH
#define ASSET_PATH "assets/"
#endif

#ifndef ENGINE_ASSET_PATH
#define ENGINE_ASSET_PATH "engine_content"
#endif
