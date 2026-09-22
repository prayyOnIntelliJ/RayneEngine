#include "SplashScreen.h"

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
}

void SplashScreen::DrawBackground()
{
    m_Window.clear(sf::Color(35, 35, 38));
}

void SplashScreen::DrawImage()
{
    if (!m_HasBg) return;

    sf::Sprite sprite(m_BgTexture);
    
    float w = static_cast<float>(m_Window.getSize().x);
    float h = static_cast<float>(m_Window.getSize().y);
    
    float targetHeight = h * 0.4f;
    float scale = targetHeight / m_BgTexture.getSize().y;
    
    if (m_BgTexture.getSize().x * scale > w * 0.8f)
    {
        scale = (w * 0.8f) / m_BgTexture.getSize().x;
    }

    sprite.setScale(scale, scale);
    sprite.setPosition(
        (w - m_BgTexture.getSize().x * scale) / 2.0f,
        (h - m_BgTexture.getSize().y * scale) / 2.0f - h * 0.05f
    );
    
    m_Window.draw(sprite);
}

void SplashScreen::DrawProgressBar()
{
    float w = static_cast<float>(m_Window.getSize().x);
    float h = static_cast<float>(m_Window.getSize().y);

    float barWidth  = std::min(w * 0.4f, 400.f);
    float barHeight = 4.f;
    float barX = w / 2.f - barWidth / 2.f;
    float barY = h * 0.80f;

    sf::RectangleShape track({ barWidth, barHeight });
    track.setPosition(barX, barY);
    track.setFillColor(sf::Color(60, 60, 65));
    m_Window.draw(track);

    float fillW = barWidth * m_DisplayProgress;
    if (fillW > 0.f)
    {
        sf::RectangleShape fill({ fillW, barHeight });
        fill.setPosition(barX, barY);
        fill.setFillColor(sf::Color(ACCENT_R, ACCENT_G, ACCENT_B));
        m_Window.draw(fill);
    }

    if (m_HasFont)
    {
        int pct = static_cast<int>(m_DisplayProgress * 100.f);
        sf::Text pctText(std::to_string(pct) + "%", m_Font, 14);
        pctText.setFillColor(sf::Color(150, 150, 155));
        sf::FloatRect pb = pctText.getLocalBounds();
        pctText.setPosition(barX + barWidth - pb.width, barY + barHeight + 8.f);
        m_Window.draw(pctText);
    }
}

void SplashScreen::DrawStatusText()
{
    if (!m_HasFont) return;

    float w = static_cast<float>(m_Window.getSize().x);
    float h = static_cast<float>(m_Window.getSize().y);

    sf::Text status(m_StatusText, m_Font, 14);
    status.setFillColor(sf::Color(150, 150, 155));
    
    float barWidth  = std::min(w * 0.4f, 400.f);
    float barHeight = 4.f;
    float barX = w / 2.f - barWidth / 2.f;
    float barY = h * 0.80f;

    status.setPosition(barX, barY + barHeight + 8.f);
    m_Window.draw(status);
}

void SplashScreen::DrawVersionInfo()
{
    if (!m_HasFont) return;

    float w = static_cast<float>(m_Window.getSize().x);
    float h = static_cast<float>(m_Window.getSize().y);

    sf::Text ver(m_ProjectName + " " + m_VersionStr, m_Font, 12);
    ver.setFillColor(sf::Color(100, 100, 105));
    sf::FloatRect vb = ver.getLocalBounds();
    ver.setPosition(w - vb.width - 16.f, h - vb.height - 16.f);
    m_Window.draw(ver);
}

void SplashScreen::SetProgress(float target, const std::string& status)
{
    m_TargetProgress = std::clamp(target, 0.f, 1.f);
    m_StatusText = status;
}

void SplashScreen::BeginFadeOut()
{
    m_FadingOut = true;
}

bool SplashScreen::RenderFrame()
{
    float dt = m_Clock.restart().asSeconds();
    if (dt > 0.1f) dt = 0.1f;

    if (m_FadingOut)
    {
        return false;
    }

    float lerpSpeed = 5.0f;
    m_DisplayProgress += (m_TargetProgress - m_DisplayProgress) * lerpSpeed * dt;
    if (std::abs(m_DisplayProgress - m_TargetProgress) < 0.001f)
        m_DisplayProgress = m_TargetProgress;

    DrawBackground();
    DrawImage();
    DrawProgressBar();
    DrawStatusText();
    DrawVersionInfo();
    m_Window.display();

    sf::Event e;
    while (m_Window.pollEvent(e)) {}

    return true;
}
