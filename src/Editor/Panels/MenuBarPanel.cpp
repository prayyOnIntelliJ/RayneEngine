#include "MenuBarPanel.h"
#include "../Core/EditorScene.h"

MenuBarPanel::MenuBarPanel(EditorScene *scene)
    : m_Scene(scene) {}

void MenuBarPanel::Init() { if (m_Scene) { m_Scene->InitMenus(); } }

void MenuBarPanel::Draw(sf::RenderWindow &window) { if (m_Scene) { m_Scene->DrawMenuBar(window); } }

void MenuBarPanel::HandleAction(const std::string &action) { if (m_Scene) { m_Scene->HandleMenuAction(action); } }
