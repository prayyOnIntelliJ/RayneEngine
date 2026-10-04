#ifndef PROFILERPANEL_H
#define PROFILERPANEL_H

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>
#include "../../Runtime/Profiler/Profiler.h"

class ProfilerPanel
{
public:
    explicit ProfilerPanel(const sf::Font &font);
    ~ProfilerPanel() = default;

    void HandleEvent(const sf::Event &event, sf::Vector2f mouseScreenPos);
    void Render(sf::RenderWindow &window, float x, float y, float width, float height);

    bool IsInputActive() const { return false; }

private:
    const sf::Font &m_Font;

    sf::FloatRect m_Bounds;
    sf::FloatRect m_PauseBtnBounds;
    sf::FloatRect m_ClearBtnBounds;
    sf::FloatRect m_HudToggleBtnBounds;
    sf::FloatRect m_LiveModeBtnBounds;
    sf::FloatRect m_GraphBounds;

    int m_SelectedFrameIndex = -1; // -1 means live latest frame
    int m_HoveredFrameIndex = -1;
    sf::Vector2f m_MousePos;

    float m_MaxGraphMs = 35.0f; // Scale for graph (35ms covers down to ~28 FPS)
};

#endif // PROFILERPANEL_H
