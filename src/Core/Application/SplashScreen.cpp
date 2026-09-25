#include "SplashScreen.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SplashScreen::SplashScreen(sf::RenderWindow& window)
    : m_Window(window)
{
}

void SplashScreen::Init(const std::string& bgPath, const std::string& fontPath,
                        const std::string& projectName, const std::string& versionStr)
{
    m_ProjectName = projectName;
    m_VersionStr  = versionStr;

    m_HasBg = m_BgTexture.loadFromFile(bgPath);
    if (m_HasBg) m_BgTexture.setSmooth(true);

    m_HasFont = m_Font.loadFromFile(fontPath);

    m_Clock.restart();
    m_TotalClock.restart();
    m_FadeAlpha = 255.f;
    m_FadeOutAlpha = 0.f;
    m_ElapsedTime = 0.f;
}

sf::ConvexShape SplashScreen::CreateRoundedRect(float width, float height, float radius, int cornerPoints)
{
    sf::ConvexShape shape;
    int totalPoints = cornerPoints * 4;
    shape.setPointCount(totalPoints);

    // Clamp radius to half of smallest dimension
    radius = std::min(radius, std::min(width, height) / 2.f);

    // Corners: top-left, top-right, bottom-right, bottom-left
    float cx[4] = { radius,          width - radius,  width - radius,  radius };
    float cy[4] = { radius,          radius,          height - radius, height - radius };
    float startAngle[4] = { 180.f, 270.f, 0.f, 90.f };

    int idx = 0;
    for (int corner = 0; corner < 4; ++corner)
    {
        for (int i = 0; i < cornerPoints; ++i)
        {
            float angle = startAngle[corner] + (90.f * i / (cornerPoints - 1));
            float rad = angle * static_cast<float>(M_PI) / 180.f;
            float x = cx[corner] + std::cos(rad) * radius;
            float y = cy[corner] + std::sin(rad) * radius;
            shape.setPoint(idx++, sf::Vector2f(x, y));
        }
    }

    return shape;
}

void SplashScreen::DrawBackground()
{
    m_Window.clear(sf::Color(BG_R, BG_G, BG_B));
}

void SplashScreen::DrawLogo()
{
    if (!m_HasBg) return;

    sf::Sprite sprite(m_BgTexture);

    float w = static_cast<float>(m_Window.getSize().x);
    float h = static_cast<float>(m_Window.getSize().y);

    // Logo takes up ~35% of window height, centered above the middle
    float targetHeight = h * 0.35f;
    float scale = targetHeight / m_BgTexture.getSize().y;

    if (m_BgTexture.getSize().x * scale > w * 0.6f)
    {
        scale = (w * 0.6f) / m_BgTexture.getSize().x;
    }

    sprite.setScale(scale, scale);

    float logoW = m_BgTexture.getSize().x * scale;
    float logoH = m_BgTexture.getSize().y * scale;

    // Position logo centered, slightly above center
    sprite.setPosition(
        (w - logoW) / 2.0f,
        (h - logoH) / 2.0f - h * 0.12f
    );

    // Apply fade-in alpha to logo
    float contentAlpha = std::max(0.f, 255.f - m_FadeAlpha);
    sf::Uint8 alpha = static_cast<sf::Uint8>(std::clamp(contentAlpha, 0.f, 255.f));
    sprite.setColor(sf::Color(255, 255, 255, alpha));

    m_Window.draw(sprite);
}

void SplashScreen::DrawTitle()
{
    if (!m_HasFont) return;

    float w = static_cast<float>(m_Window.getSize().x);
    float h = static_cast<float>(m_Window.getSize().y);

    // Calculate content alpha for fade-in (delayed slightly after logo)
    float fadeProgress = std::clamp(m_ElapsedTime / m_FadeInDuration, 0.f, 1.f);
    float titleFade = std::clamp((fadeProgress - 0.2f) / 0.8f, 0.f, 1.f); // Starts a bit later
    sf::Uint8 alpha = static_cast<sf::Uint8>(titleFade * 255.f);

    // Engine name - prominent title
    sf::Text title(m_ProjectName, m_Font, 28);
    title.setFillColor(sf::Color(230, 232, 240, alpha));
    title.setStyle(sf::Text::Bold);
    title.setLetterSpacing(2.5f);

    sf::FloatRect tb = title.getLocalBounds();
    title.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
    title.setPosition(w / 2.f, h * 0.58f);

    m_Window.draw(title);
}

void SplashScreen::DrawProgressBar()
{
    float w = static_cast<float>(m_Window.getSize().x);
    float h = static_cast<float>(m_Window.getSize().y);

    float barWidth  = std::min(w * 0.35f, 350.f);
    float barX = w / 2.f - barWidth / 2.f;
    float barY = h * 0.72f;

    // Track (background) – rounded rectangle
    sf::ConvexShape track = CreateRoundedRect(barWidth, BAR_HEIGHT, BAR_RADIUS);
    track.setPosition(barX, barY);
    track.setFillColor(sf::Color(50, 52, 58));
    m_Window.draw(track);

    // Fill – rounded rectangle
    float fillW = barWidth * m_DisplayProgress;
    if (fillW > BAR_RADIUS * 2.f)
    {
        sf::ConvexShape fill = CreateRoundedRect(fillW, BAR_HEIGHT, BAR_RADIUS);
        fill.setPosition(barX, barY);
        fill.setFillColor(sf::Color(ACCENT_R, ACCENT_G, ACCENT_B));
        m_Window.draw(fill);

        // Subtle glow behind the fill bar
        sf::ConvexShape glow = CreateRoundedRect(fillW, BAR_HEIGHT + 4.f, BAR_RADIUS + 1.f);
        glow.setPosition(barX, barY - 2.f);
        glow.setFillColor(sf::Color(ACCENT_R, ACCENT_G, ACCENT_B, 40));
        m_Window.draw(glow);
    }
    else if (fillW > 0.f)
    {
        // For very small progress, draw a simple rect
        sf::RectangleShape fill({ fillW, BAR_HEIGHT });
        fill.setPosition(barX, barY);
        fill.setFillColor(sf::Color(ACCENT_R, ACCENT_G, ACCENT_B));
        m_Window.draw(fill);
    }
}

void SplashScreen::DrawStatusText()
{
    if (!m_HasFont) return;

    float w = static_cast<float>(m_Window.getSize().x);
    float h = static_cast<float>(m_Window.getSize().y);

    float barWidth  = std::min(w * 0.35f, 350.f);
    float barX = w / 2.f - barWidth / 2.f;
    float barY = h * 0.72f;

    // Status text – left-aligned below progress bar
    sf::Color textColor(160, 165, 175);
    sf::Text status(m_StatusText, m_Font, 13);
    status.setFillColor(textColor);
    status.setPosition(barX, barY + BAR_HEIGHT + 10.f);
    m_Window.draw(status);

    // Percentage – right-aligned below progress bar
    int pct = static_cast<int>(m_DisplayProgress * 100.f);
    sf::Text pctText(std::to_string(pct) + "%", m_Font, 13);
    pctText.setFillColor(sf::Color(ACCENT_R, ACCENT_G, ACCENT_B));
    sf::FloatRect pb = pctText.getLocalBounds();
    pctText.setPosition(barX + barWidth - pb.width - pb.left, barY + BAR_HEIGHT + 10.f);
    m_Window.draw(pctText);
}

void SplashScreen::DrawVersionInfo()
{
    if (!m_HasFont) return;

    float w = static_cast<float>(m_Window.getSize().x);
    float h = static_cast<float>(m_Window.getSize().y);

    // Version info – bottom right
    sf::Color verColor(90, 95, 105);
    sf::Text ver(m_ProjectName + "  " + m_VersionStr, m_Font, 11);
    ver.setFillColor(verColor);
    sf::FloatRect vb = ver.getLocalBounds();
    ver.setPosition(w - vb.width - vb.left - 20.f, h - vb.height - vb.top - 16.f);
    m_Window.draw(ver);
}

void SplashScreen::DrawFadeOverlay()
{
    float w = static_cast<float>(m_Window.getSize().x);
    float h = static_cast<float>(m_Window.getSize().y);

    sf::Uint8 alpha = 0;

    if (!m_FadingOut)
    {
        // Fade-in: black overlay fading from opaque to transparent
        alpha = static_cast<sf::Uint8>(std::clamp(m_FadeAlpha, 0.f, 255.f));
    }
    else
    {
        // Fade-out: black overlay fading from transparent to opaque
        alpha = static_cast<sf::Uint8>(std::clamp(m_FadeOutAlpha, 0.f, 255.f));
    }

    if (alpha > 0)
    {
        sf::RectangleShape overlay(sf::Vector2f(w, h));
        overlay.setFillColor(sf::Color(BG_R, BG_G, BG_B, alpha));
        m_Window.draw(overlay);
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

    m_ElapsedTime = m_TotalClock.getElapsedTime().asSeconds();

    // Update fade-in
    if (!m_FadingOut && m_FadeAlpha > 0.f)
    {
        m_FadeAlpha -= (255.f / m_FadeInDuration) * dt;
        if (m_FadeAlpha < 0.f) m_FadeAlpha = 0.f;
    }

    // Update fade-out
    if (m_FadingOut)
    {
        m_FadeOutAlpha += (255.f / m_FadeOutDuration) * dt;
        if (m_FadeOutAlpha >= 255.f)
        {
            m_FadeOutAlpha = 255.f;
            return false; // Fade-out complete
        }
    }

    // Smooth progress interpolation
    float lerpSpeed = 5.0f;
    m_DisplayProgress += (m_TargetProgress - m_DisplayProgress) * lerpSpeed * dt;
    if (std::abs(m_DisplayProgress - m_TargetProgress) < 0.001f)
        m_DisplayProgress = m_TargetProgress;

    // Draw everything
    DrawBackground();
    DrawLogo();
    DrawTitle();
    DrawProgressBar();
    DrawStatusText();
    DrawVersionInfo();
    DrawFadeOverlay();
    m_Window.display();

    sf::Event e;
    while (m_Window.pollEvent(e)) {}

    return true;
}
