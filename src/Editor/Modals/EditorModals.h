#ifndef EDITORMODALS_H
#define EDITORMODALS_H

#include <SFML/Graphics/RenderWindow.hpp>

class EditorScene;

class EditorModals
{
public:
    explicit EditorModals(EditorScene *scene);

    ~EditorModals() = default;

    void DrawSettings(sf::RenderWindow &window);

    void DrawProjectSettings(sf::RenderWindow &window);

    void DrawBuildPopup(sf::RenderWindow &window);

    void DrawDeleteModal(sf::RenderWindow &window);

    void DrawScriptErrorModal(sf::RenderWindow &window);

    void DrawSaveTemplateModal(sf::RenderWindow &window);

private:
    EditorScene *m_Scene = nullptr;
};

#endif // EDITORMODALS_H
