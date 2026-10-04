#ifndef HIERARCHYPANEL_H
#define HIERARCHYPANEL_H

#include <SFML/Graphics/RenderWindow.hpp>

class EditorScene;

class HierarchyPanel
{
public:
    explicit HierarchyPanel(EditorScene *scene);

    ~HierarchyPanel() = default;

    void Draw(sf::RenderWindow &window);

private:
    EditorScene *m_Scene = nullptr;
};

#endif
