#ifndef RAYNEENGINE_SPLASHSCREEN_H
#define RAYNEENGINE_SPLASHSCREEN_H

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <string>
#include <algorithm>
#include <cmath>

class SplashScreen
{
public:
    SplashScreen(sf::RenderWindow& window);

    void Init(const std::string& bgPath, const std::string& fontPath,
              const std::string& projectName, const std::string& versionStr);

    void SetProgress(float target, const std::string& status);

    bool RenderFrame();

    void BeginFadeOut();

private:
    void DrawBackground();
    void DrawLogo();
    void DrawTitle();
    void DrawProgressBar();
    void DrawStatusText();
    void DrawVersionInfo();
    void DrawFadeOverlay();

    sf::ConvexShape CreateRoundedRect(float width, float height, float radius, int cornerPoints = 8);

    sf::RenderWindow& m_Window;
    sf::Clock         m_Clock;
    sf::Clock         m_TotalClock;

    sf::Texture m_BgTexture;
    bool        m_HasBg = false;
    sf::Font    m_Font;
    bool        m_HasFont = false;

    std::string m_ProjectName = "RayneEngine";
    std::string m_VersionStr  = "v1.0";
    std::string m_StatusText  = "Starting...";

    float m_TargetProgress   = 0.0f;
    float m_DisplayProgress  = 0.0f;

    bool  m_FadingOut     = false;
    float m_FadeAlpha     = 255.f;
    float m_FadeOutAlpha  = 0.f;

    float m_FadeInDuration  = 0.8f;
    float m_FadeOutDuration = 0.5f;
    float m_ElapsedTime     = 0.f;

    static constexpr sf::Uint8 BG_R = 26;
    static constexpr sf::Uint8 BG_G = 26;
    static constexpr sf::Uint8 BG_B = 30;

    static constexpr sf::Uint8 ACCENT_R = 0;
    static constexpr sf::Uint8 ACCENT_G = 150;
    static constexpr sf::Uint8 ACCENT_B = 255;

    static constexpr float BAR_HEIGHT = 6.f;
    static constexpr float BAR_RADIUS = 3.f;
};

#endif
