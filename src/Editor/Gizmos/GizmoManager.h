#ifndef GIZMOMANAGER_H
#define GIZMOMANAGER_H

#include <SFML/Graphics/RenderWindow.hpp>

class EditorScene;

class GizmoManager
{
public:
    explicit GizmoManager(EditorScene *scene);

    ~GizmoManager() = default;

    void DrawGrid();

    void DrawWorldAxes(sf::RenderWindow &window);

    void DrawGizmos(sf::RenderWindow &window);

    void DrawResizeHandles(sf::RenderWindow &window);

private:
    EditorScene *m_Scene = nullptr;
};

#endif
