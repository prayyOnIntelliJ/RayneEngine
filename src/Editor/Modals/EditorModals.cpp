#include "EditorModals.h"
#include "../Core/EditorScene.h"

EditorModals::EditorModals(EditorScene *scene)
    : m_Scene(scene) {}

void EditorModals::DrawSettings(sf::RenderWindow &window) { if (m_Scene) { m_Scene->DrawSettingsWindow(window); } }

void EditorModals::DrawProjectSettings(sf::RenderWindow &window)
{
    if (m_Scene) { m_Scene->DrawProjectSettingsWindow(window); }
}

void EditorModals::DrawBuildPopup(sf::RenderWindow &window) { if (m_Scene) { m_Scene->DrawBuildPopup(window); } }

void EditorModals::DrawDeleteModal(sf::RenderWindow &window) { if (m_Scene) { m_Scene->DrawDeleteModal(window); } }

void EditorModals::DrawScriptErrorModal(sf::RenderWindow &window)
{
    if (m_Scene) { m_Scene->DrawScriptErrorModal(window); }
}

void EditorModals::DrawSaveTemplateModal(sf::RenderWindow &window)
{
    if (m_Scene) { m_Scene->DrawSaveTemplateModal(window); }
}
