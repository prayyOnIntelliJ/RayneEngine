#include "GizmoManager.h"
#include "../Core/EditorScene.h"

GizmoManager::GizmoManager(EditorScene *scene)
    : m_Scene(scene) {}

void GizmoManager::DrawGrid() { if (m_Scene) { m_Scene->DrawGrid(); } }

void GizmoManager::DrawWorldAxes(sf::RenderWindow &window) { if (m_Scene) { m_Scene->DrawWorldAxes(window); } }

void GizmoManager::DrawGizmos(sf::RenderWindow &window) { if (m_Scene) { m_Scene->DrawGizmos(window); } }

void GizmoManager::DrawResizeHandles(sf::RenderWindow &window) { if (m_Scene) { m_Scene->DrawResizeHandles(window); } }
