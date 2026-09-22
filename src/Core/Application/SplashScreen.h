#ifndef RAYNEENGINE_SPLASHSCREEN_H
#define RAYNEENGINE_SPLASHSCREEN_H

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <string>
#include <algorithm>

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
    void DrawImage();
    void DrawProgressBar();
    void DrawStatusText();
    void DrawVersionInfo();

    sf::RenderWindow& m_Window;
    sf::Clock         m_Clock;

    sf::Texture m_BgTexture;
    bool        m_HasBg = false;
    sf::Font    m_Font;
    bool        m_HasFont = false;

    std::string m_ProjectName = "RayneEngine";
    std::string m_VersionStr  = "v1.0";
    std::string m_StatusText  = "Starting...";

    float m_TargetProgress   = 0.0f;
    float m_DisplayProgress  = 0.0f;

    bool  m_FadingOut    = false;

    static constexpr sf::Uint8 ACCENT_R = 0;
    static constexpr sf::Uint8 ACCENT_G = 150;
    static constexpr sf::Uint8 ACCENT_B = 255;
};

#endif
