#ifndef INSPECTORPANEL_H
#define INSPECTORPANEL_H

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>

class EditorScene;

class InspectorPanel
{
public:
    explicit InspectorPanel(EditorScene *scene);

    ~InspectorPanel() = default;

    void Draw(sf::RenderWindow &window);

    void HandleClick(sf::Vector2f pos);

private:
    EditorScene *m_Scene = nullptr;
};

#endif
