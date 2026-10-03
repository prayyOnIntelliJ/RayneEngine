#include "HierarchyPanel.h"
#include "../Core/EditorScene.h"

HierarchyPanel::HierarchyPanel(EditorScene *scene)
    : m_Scene(scene) {}

void HierarchyPanel::Draw(sf::RenderWindow &window) { if (m_Scene) { m_Scene->DrawHierarchy(window); } }
