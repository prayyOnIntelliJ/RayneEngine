#ifndef RAYNEENGINE_SPLASHSCREEN_H
#define RAYNEENGINE_SPLASHSCREEN_H

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <string>
#include <vector>
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
    struct Particle
    {
        sf::Vector2f pos;
        sf::Vector2f vel;
        float radius;
        float alpha;
        float alphaSpeed;
        float phase;
    };

    void InitParticles(int count);
    void UpdateParticles(float dt);
    void DrawParticles();

    void DrawBackground();
    void DrawVignette();
    void DrawTitle(float dt);
    void DrawProgressBar();
    void DrawStatusText();
    void DrawVersionInfo();
    void DrawFadeOverlay();

    static float SmoothStep(float edge0, float edge1, float x);

    sf::RenderWindow& m_Window;
    sf::Clock         m_Clock;
    sf::Clock         m_GlobalClock;

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
    float m_FadeAlpha    = 255.0f;
    bool  m_FadeInDone   = false;
    float m_FadeOutAlpha = 0.0f;

    std::vector<Particle> m_Particles;

    static constexpr sf::Uint8 ACCENT_R = 0;
    static constexpr sf::Uint8 ACCENT_G = 200;
    static constexpr sf::Uint8 ACCENT_B = 255;
};

#endif
