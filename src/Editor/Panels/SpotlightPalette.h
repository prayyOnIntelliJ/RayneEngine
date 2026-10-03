#ifndef SPOTLIGHTPALETTE_H
#define SPOTLIGHTPALETTE_H

#include <SFML/Graphics/RenderWindow.hpp>

class EditorScene;

class SpotlightPalette
{
public:
    explicit SpotlightPalette(EditorScene *scene);

    ~SpotlightPalette() = default;

    void Init();

    void Open();

    void Close();

    void Draw(sf::RenderWindow &window);

private:
    EditorScene *m_Scene = nullptr;
};

#endif // SPOTLIGHTPALETTE_H
