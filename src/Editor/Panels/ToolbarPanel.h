#ifndef TOOLBARPANEL_H
#define TOOLBARPANEL_H

#include <SFML/Graphics/RenderWindow.hpp>

class EditorScene;

class ToolbarPanel
{
public:
    explicit ToolbarPanel(EditorScene *scene);

    ~ToolbarPanel() = default;

    void Draw(sf::RenderWindow &window);

private:
    EditorScene *m_Scene = nullptr;
};

#endif
