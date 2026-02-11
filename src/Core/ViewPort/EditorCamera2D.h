#ifndef VIEWPORT_H
#define VIEWPORT_H
#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/System/Vector2.hpp"


class EditorCamera2D {
public:
    EditorCamera2D() = default;
    EditorCamera2D(const sf::Vector2f &startPosition, float startZoom, sf::RenderWindow* window);

    void Update(float deltaTime);
    void Apply() const;
    void DrawGrid(const sf::View& view) const;
    void DrawWorldBorder(float thicknessPixels = 2.f, sf::Color color = sf::Color::White) const;

    sf::View& GetView();

private:
    sf::View m_View;
    float m_ZoomLevel = 1.f;
    sf::Vector2i m_LastMousePosition;
    bool m_bIsDragging = false;
    sf::RenderWindow* m_RenderWindow = nullptr;
};



#endif
