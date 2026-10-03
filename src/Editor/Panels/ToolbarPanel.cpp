#include "ToolbarPanel.h"
#include "../Core/EditorScene.h"

ToolbarPanel::ToolbarPanel(EditorScene *scene)
    : m_Scene(scene) {}

void ToolbarPanel::Draw(sf::RenderWindow &window) { if (m_Scene) { m_Scene->DrawToolbar(window); } }
