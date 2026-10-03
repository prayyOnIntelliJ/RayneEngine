#include "InspectorPanel.h"
#include "../Core/EditorScene.h"

InspectorPanel::InspectorPanel(EditorScene *scene)
    : m_Scene(scene) {}

void InspectorPanel::Draw(sf::RenderWindow &window) { if (m_Scene) { m_Scene->DrawInspector(window); } }

void InspectorPanel::HandleClick(sf::Vector2f pos) { if (m_Scene) { m_Scene->HandleInspectorClick(pos); } }
