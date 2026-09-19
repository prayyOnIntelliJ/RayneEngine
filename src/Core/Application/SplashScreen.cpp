#include "SplashScreen.h"
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <algorithm>

static float RandFloat(float lo, float hi)
{
    return lo + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / (hi - lo)));
}

float SplashScreen::SmoothStep(float edge0, float edge1, float x)
{
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

SplashScreen::SplashScreen(sf::RenderWindow& window)
    : m_Window(window)
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
}

void SplashScreen::Init(const std::string& bgPath, const std::string& fontPath,
                        const std::string& projectName, const std::string& versionStr)
{
    m_ProjectName = projectName;
    m_VersionStr  = versionStr;

    m_HasBg   = m_BgTexture.loadFromFile(bgPath);
    m_HasFont = m_Font.loadFromFile(fontPath);

    InitParticles(45);
    m_Clock.restart();
    m_GlobalClock.restart();
}

void SplashScreen::InitParticles(int count)
{
    float w = static_cast<float>(m_Window.getSize().x);
    float h = static_cast<float>(m_Window.getSize().y);

    m_Particles.resize(count);
    for (auto& p : m_Particles)
    {
        float px, py;
        int zone = std::rand() % 4;
        switch (zone)
        {
            case 0:
                px = RandFloat(0, w);
                py = RandFloat(0, h * 0.20f);
                break;
            case 1:
                px = RandFloat(0, w);
                py = RandFloat(h * 0.65f, h);
                break;
            case 2:
                px = RandFloat(0, w * 0.20f);
                py = RandFloat(0, h);
                break;
            default:
                px = RandFloat(w * 0.80f, w);
                py = RandFloat(0, h);
                break;
        }

        p.pos    = { px, py };
        p.vel    = { RandFloat(-10.f, 10.f), RandFloat(-18.f, -3.f) };
        p.radius = RandFloat(1.0f, 3.5f);
        p.alpha  = RandFloat(30.f, 120.f);
        p.alphaSpeed = RandFloat(0.5f, 2.0f);
        p.phase  = RandFloat(0.f, 6.28f);
    }
}

void SplashScreen::UpdateParticles(float dt)
{
    float w = static_cast<float>(m_Window.getSize().x);
    float h = static_cast<float>(m_Window.getSize().y);
    float time = m_GlobalClock.getElapsedTime().asSeconds();

    float cx = w / 2.f;
    float cy = h * 0.42f;
    float exclusionRadiusX = w * 0.18f;
    float exclusionRadiusY = h * 0.22f;

    for (auto& p : m_Particles)
    {
        p.pos += p.vel * dt;

        p.pos.x += std::sin(time * p.alphaSpeed + p.phase) * 6.0f * dt;

        if (p.pos.y < -10.f) { p.pos.y = h + 10.f; p.pos.x = RandFloat(0, w); }
        if (p.pos.x < -10.f) p.pos.x = w + 10.f;
        if (p.pos.x > w + 10.f) p.pos.x = -10.f;

        float dx = (p.pos.x - cx) / exclusionRadiusX;
        float dy = (p.pos.y - cy) / exclusionRadiusY;
        float distFromCenter = std::sqrt(dx * dx + dy * dy);
        float centerFade = std::clamp(distFromCenter - 0.5f, 0.f, 1.f);

        float baseAlpha = 50.f + 80.f * (0.5f + 0.5f * std::sin(time * p.alphaSpeed + p.phase));
        p.alpha = baseAlpha * centerFade;
    }
}

void SplashScreen::DrawParticles()
{
    for (const auto& p : m_Particles)
    {
        if (p.alpha < 2.f) continue;

        sf::CircleShape dot(p.radius);
        dot.setOrigin(p.radius, p.radius);
        dot.setPosition(p.pos);

        auto a = static_cast<sf::Uint8>(std::clamp(p.alpha, 0.f, 255.f));
        dot.setFillColor(sf::Color(ACCENT_R, ACCENT_G, ACCENT_B, a));
        m_Window.draw(dot);

        if (p.radius > 2.5f && a > 20)
        {
            float glowR = p.radius * 3.5f;
            sf::CircleShape glow(glowR);
            glow.setOrigin(glowR, glowR);
            glow.setPosition(p.pos);
            glow.setFillColor(sf::Color(ACCENT_R, ACCENT_G, ACCENT_B, static_cast<sf::Uint8>(a / 6)));
            m_Window.draw(glow);
        }
    }
}

void SplashScreen::DrawBackground()
{
    if (!m_HasBg) return;

    sf::Sprite sprite(m_BgTexture);
    float scaleX = static_cast<float>(m_Window.getSize().x) / m_BgTexture.getSize().x;
    float scaleY = static_cast<float>(m_Window.getSize().y) / m_BgTexture.getSize().y;
    float scale  = std::max(scaleX, scaleY);
    sprite.setScale(scale, scale);
    sprite.setPosition(
        (m_Window.getSize().x - m_BgTexture.getSize().x * scale) / 2.0f,
        (m_Window.getSize().y - m_BgTexture.getSize().y * scale) / 2.0f
    );
    m_Window.draw(sprite);
}

void SplashScreen::DrawVignette()
{
    float w = static_cast<float>(m_Window.getSize().x);
    float h = static_cast<float>(m_Window.getSize().y);

    sf::RectangleShape botGrad({ w, h * 0.38f });
    botGrad.setPosition(0.f, h * 0.62f);
    botGrad.setFillColor(sf::Color(0, 0, 0, 100));
    m_Window.draw(botGrad);

    sf::RectangleShape botDark({ w, h * 0.15f });
    botDark.setPosition(0.f, h * 0.85f);
    botDark.setFillColor(sf::Color(0, 0, 0, 60));
    m_Window.draw(botDark);
}

void SplashScreen::DrawTitle(float dt)
{
    if (!m_HasFont) return;

    float w = static_cast<float>(m_Window.getSize().x);
    float h = static_cast<float>(m_Window.getSize().y);
    float time = m_GlobalClock.getElapsedTime().asSeconds();

    sf::Text title(m_ProjectName, m_Font, 42);
    title.setStyle(sf::Text::Bold);
    sf::FloatRect tb = title.getLocalBounds();
    title.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
    title.setPosition(w / 2.f, h * 0.70f);

    float pulse = 1.0f + 0.01f * std::sin(time * 1.5f);
    title.setScale(pulse, pulse);

    sf::Text titleGlow = title;
    float glowAlpha = 30.f + 20.f * std::sin(time * 2.0f);
    titleGlow.setFillColor(sf::Color(ACCENT_R, ACCENT_G, ACCENT_B, static_cast<sf::Uint8>(glowAlpha)));
    titleGlow.setScale(pulse * 1.015f, pulse * 1.015f);
    titleGlow.setOutlineColor(sf::Color(ACCENT_R, ACCENT_G, ACCENT_B, static_cast<sf::Uint8>(glowAlpha / 2)));
    titleGlow.setOutlineThickness(3.0f);
    m_Window.draw(titleGlow);

    title.setFillColor(sf::Color::White);
    title.setOutlineColor(sf::Color(0, 0, 0, 160));
    title.setOutlineThickness(1.5f);
    m_Window.draw(title);

    std::string subtitleStr = "Powered by RayneEngine";
#ifndef RAYNE_STANDALONE
    subtitleStr = "RayneEngine Editor";
#endif
    sf::Text subtitle(subtitleStr, m_Font, 15);
    subtitle.setFillColor(sf::Color(180, 180, 200, 160));
    subtitle.setLetterSpacing(2.5f);
    sf::FloatRect sb = subtitle.getLocalBounds();
    subtitle.setOrigin(sb.left + sb.width / 2.f, sb.top + sb.height / 2.f);
    subtitle.setPosition(w / 2.f, h * 0.70f + 38.f);
    m_Window.draw(subtitle);

    float lineW = std::min(tb.width * 0.5f, 250.f);
    float lineProgress = SmoothStep(0.f, 1.5f, time);
    float currentLineW = lineW * lineProgress;

    sf::RectangleShape line({ currentLineW, 1.5f });
    line.setOrigin(currentLineW / 2.f, 0.75f);
    line.setPosition(w / 2.f, h * 0.70f + 58.f);
    line.setFillColor(sf::Color(ACCENT_R, ACCENT_G, ACCENT_B, 100));
    m_Window.draw(line);

    sf::RectangleShape lineGlow({ currentLineW * 1.2f, 5.f });
    lineGlow.setOrigin(currentLineW * 1.2f / 2.f, 2.5f);
    lineGlow.setPosition(w / 2.f, h * 0.70f + 58.f);
    lineGlow.setFillColor(sf::Color(ACCENT_R, ACCENT_G, ACCENT_B, 18));
    m_Window.draw(lineGlow);
}

void SplashScreen::DrawProgressBar()
{
    float w = static_cast<float>(m_Window.getSize().x);
    float h = static_cast<float>(m_Window.getSize().y);
    float time = m_GlobalClock.getElapsedTime().asSeconds();

    float barWidth  = std::min(w * 0.45f, 500.f);
    float barHeight = 6.f;
    float barX = w / 2.f - barWidth / 2.f;
    float barY = h * 0.84f;

    sf::RectangleShape track({ barWidth, barHeight });
    track.setPosition(barX, barY);
    track.setFillColor(sf::Color(255, 255, 255, 12));
    track.setOutlineColor(sf::Color(255, 255, 255, 25));
    track.setOutlineThickness(1.0f);
    m_Window.draw(track);

    float fillW = barWidth * m_DisplayProgress;
    if (fillW > 0.5f)
    {
        sf::RectangleShape fill({ fillW, barHeight });
        fill.setPosition(barX, barY);
        fill.setFillColor(sf::Color(ACCENT_R, ACCENT_G, ACCENT_B, 200));
        m_Window.draw(fill);

        float shimmerPos = std::fmod(time * 100.f, fillW + 60.f) - 30.f;
        if (shimmerPos > 0 && shimmerPos < fillW)
        {
            float shimmerW = 30.f;
            float sx = barX + shimmerPos;
            float sw = std::min(shimmerW, fillW - shimmerPos);
            if (sw > 0)
            {
                sf::RectangleShape shimmer({ sw, barHeight });
                shimmer.setPosition(sx, barY);
                shimmer.setFillColor(sf::Color(255, 255, 255, 45));
                m_Window.draw(shimmer);
            }
        }

        sf::RectangleShape glow({ fillW + 8.f, barHeight + 12.f });
        glow.setPosition(barX - 4.f, barY - 6.f);
        glow.setFillColor(sf::Color(ACCENT_R, ACCENT_G, ACCENT_B, 14));
        m_Window.draw(glow);

        float dotR = 3.f;
        sf::CircleShape edge(dotR);
        edge.setOrigin(dotR, dotR);
        edge.setPosition(barX + fillW, barY + barHeight / 2.f);
        float edgeAlpha = 100.f + 80.f * std::sin(time * 6.f);
        edge.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(edgeAlpha)));
        m_Window.draw(edge);

        float glowR = 10.f;
        sf::CircleShape edgeGlow(glowR);
        edgeGlow.setOrigin(glowR, glowR);
        edgeGlow.setPosition(barX + fillW, barY + barHeight / 2.f);
        edgeGlow.setFillColor(sf::Color(ACCENT_R, ACCENT_G, ACCENT_B, static_cast<sf::Uint8>(edgeAlpha / 5)));
        m_Window.draw(edgeGlow);
    }

    if (m_HasFont)
    {
        int pct = static_cast<int>(m_DisplayProgress * 100.f);
        sf::Text pctText(std::to_string(pct) + "%", m_Font, 16);
        pctText.setFillColor(sf::Color(ACCENT_R, ACCENT_G, ACCENT_B, 180));
        sf::FloatRect pb = pctText.getLocalBounds();
        pctText.setOrigin(pb.left, pb.top + pb.height / 2.f);
        pctText.setPosition(barX + barWidth + 14.f, barY + barHeight / 2.f);
        m_Window.draw(pctText);
    }
}

void SplashScreen::DrawStatusText()
{
    if (!m_HasFont) return;

    float w = static_cast<float>(m_Window.getSize().x);
    float h = static_cast<float>(m_Window.getSize().y);

    sf::Text status(m_StatusText, m_Font, 14);
    status.setFillColor(sf::Color(190, 190, 200, 180));
    status.setLetterSpacing(1.2f);
    sf::FloatRect sb = status.getLocalBounds();
    status.setOrigin(sb.left + sb.width / 2.f, sb.top + sb.height / 2.f);
    status.setPosition(w / 2.f, h * 0.84f - 18.f);
    m_Window.draw(status);
}

void SplashScreen::DrawVersionInfo()
{
    if (!m_HasFont) return;

    float w = static_cast<float>(m_Window.getSize().x);
    float h = static_cast<float>(m_Window.getSize().y);

    sf::Text ver(m_VersionStr, m_Font, 12);
    ver.setFillColor(sf::Color(130, 130, 150, 100));
    sf::FloatRect vb = ver.getLocalBounds();
    ver.setPosition(w - vb.width - 16.f, h - vb.height - 16.f);
    m_Window.draw(ver);

    sf::Text copy("(C) 2026 RayneEngine", m_Font, 12);
    copy.setFillColor(sf::Color(130, 130, 150, 100));
    copy.setPosition(16.f, h - vb.height - 16.f);
    m_Window.draw(copy);
}

void SplashScreen::DrawFadeOverlay()
{
    float w = static_cast<float>(m_Window.getSize().x);
    float h = static_cast<float>(m_Window.getSize().y);

    sf::Uint8 alpha = 0;
    if (!m_FadeInDone)
        alpha = static_cast<sf::Uint8>(std::clamp(m_FadeAlpha, 0.f, 255.f));
    else if (m_FadingOut)
        alpha = static_cast<sf::Uint8>(std::clamp(m_FadeOutAlpha, 0.f, 255.f));

    if (alpha > 0)
    {
        sf::RectangleShape fade({ w, h });
        fade.setFillColor(sf::Color(10, 12, 15, alpha));
        m_Window.draw(fade);
    }
}

void SplashScreen::SetProgress(float target, const std::string& status)
{
    m_TargetProgress = std::clamp(target, 0.f, 1.f);
    m_StatusText = status;
}

void SplashScreen::BeginFadeOut()
{
    m_FadingOut = true;
    m_FadeOutAlpha = 0.f;
}

bool SplashScreen::RenderFrame()
{
    float dt = m_Clock.restart().asSeconds();
    if (dt > 0.1f) dt = 0.1f;

    float lerpSpeed = 4.0f;
    m_DisplayProgress += (m_TargetProgress - m_DisplayProgress) * lerpSpeed * dt;
    if (std::abs(m_DisplayProgress - m_TargetProgress) < 0.001f)
        m_DisplayProgress = m_TargetProgress;

    if (!m_FadeInDone)
    {
        m_FadeAlpha -= 300.f * dt;
        if (m_FadeAlpha <= 0.f)
        {
            m_FadeAlpha = 0.f;
            m_FadeInDone = true;
        }
    }

    if (m_FadingOut)
    {
        m_FadeOutAlpha += 400.f * dt;
        if (m_FadeOutAlpha >= 255.f)
        {
            m_FadeOutAlpha = 255.f;
            m_Window.clear(sf::Color(10, 12, 15));
            m_Window.display();
            return false;
        }
    }

    UpdateParticles(dt);

    m_Window.clear(sf::Color(10, 12, 15));
    DrawBackground();
    DrawVignette();
    DrawParticles();
    DrawTitle(dt);
    DrawStatusText();
    DrawProgressBar();
    DrawVersionInfo();
    DrawFadeOverlay();
    m_Window.display();

    sf::Event e;
    while (m_Window.pollEvent(e)) {}

    return true;
}
