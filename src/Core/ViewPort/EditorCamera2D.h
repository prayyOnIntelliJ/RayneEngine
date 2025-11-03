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
    void DrawBorder(const sf::Color& color = sf::Color::White, float thickness = 2.f) const;

    sf::View& GetView();

private:
    sf::View view;
    float zoomLevel = 1.f;
    sf::Vector2i lastMousePosition;
    bool isDragging = false;
    sf::RenderWindow* renderWindow = nullptr;
};



#endif
