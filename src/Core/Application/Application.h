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
    void SetProjectName(const std::string& name) { m_ProjectName = name; }
    const std::string& GetProjectVersion() const { return m_ProjectVersion; }
    void SetProjectVersion(const std::string& ver) { m_ProjectVersion = ver; }
    const std::string& GetProjectAuthor() const { return m_ProjectAuthor; }
    void SetProjectAuthor(const std::string& author) { m_ProjectAuthor = author; }
    void SetVSync(bool vsync) { m_ProjectVSync = vsync; m_RenderWindow.setVerticalSyncEnabled(vsync); }
    void SetTargetFPS(unsigned int fps) { m_ProjectTargetFPS = fps; m_RenderWindow.setFramerateLimit(fps); }
    void SetClearColor(sf::Color color) { m_ClearColor = color; }
    sf::Color GetClearColor() const { return m_ClearColor; }
    void SetWindowSize(int w, int h);
    void SetMasterVolume(float vol);
    void SetMusicVolume(float vol);

    void Quit();
    void RestartCurrentScene();
    void LoadGameScene(const std::string& sceneName);

    void SetPaused(bool paused) { m_IsPaused = paused; }
    bool IsPaused() const { return m_IsPaused; }
    void TogglePause() { m_IsPaused = !m_IsPaused; }
    void SetTimeScale(float scale) { m_TimeScale = (scale < 0.f ? 0.f : scale); }
    float GetTimeScale() const { return m_TimeScale; }

    void SetFullscreen(bool fullscreen);
    void ToggleFullscreen() { SetFullscreen(!m_ProjectFullscreen); }
    bool IsFullscreen() const { return m_ProjectFullscreen; }
    void SetCursorVisible(bool visible);

    std::string TakeScreenshot(const std::string& customFilename = "");
    void OpenURL(const std::string& url);

    float GetFPS() const { return m_CurrentFPS; }
    float GetDeltaTime() const { return m_CurrentDeltaTime; }
    void SetShowFPSOverlay(bool show) { m_ShowFPSOverlay = show; }
    bool IsFPSOverlayShown() const { return m_ShowFPSOverlay; }
    const std::string& GetCurrentSceneName() const { return m_CurrentSceneName; }

private:
    void SetIcon();

    void Update(float deltaTime);

    void Render();

    void SetEvents();

    void RunSplashSequence();

    std::string m_ProjectName = "RayneEngine";
    std::string m_ProjectVersion = "1.0.0";
    std::string m_ProjectAuthor = "";
    std::string m_StartScene = "scenes/game.json";
    std::string m_CurrentSceneName = "game";
    int m_WindowWidth = 1280;
    int m_WindowHeight = 720;
    bool m_ProjectVSync = true;
    unsigned int m_ProjectTargetFPS = 60;
    bool m_ProjectFullscreen = false;
    sf::Color m_ClearColor = sf::Color(18, 20, 23);
    float m_MasterVolume = 100.f;
    float m_MusicVolume = 100.f;

    bool m_IsPaused = false;
    float m_TimeScale = 1.0f;
    float m_CurrentFPS = 0.f;
    float m_CurrentDeltaTime = 0.f;
    bool m_ShowFPSOverlay = false;
    bool m_IsFirstRun = false;

    sf::RenderWindow m_RenderWindow;
    SceneManager m_SceneManager;
    sf::Clock m_DeltaTimeClock;
};

#endif

#ifndef ASSET_PATH
#define ASSET_PATH "assets"
#endif

#ifndef ENGINE_ASSET_PATH
#define ENGINE_ASSET_PATH "engine_content"
#endif
