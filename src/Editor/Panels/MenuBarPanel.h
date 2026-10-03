#ifndef MENUBARPANEL_H
#define MENUBARPANEL_H

#include <string>
#include <SFML/Graphics/RenderWindow.hpp>

class EditorScene;

class MenuBarPanel
{
public:
    explicit MenuBarPanel(EditorScene *scene);

    ~MenuBarPanel() = default;

    void Init();

    void Draw(sf::RenderWindow &window);

    void HandleAction(const std::string &action);

private:
    EditorScene *m_Scene = nullptr;
};

#endif // MENUBARPANEL_H
