#include "SpotlightPalette.h"
#include "../Core/EditorScene.h"

SpotlightPalette::SpotlightPalette(EditorScene *scene)
    : m_Scene(scene) {}

void SpotlightPalette::Init() { if (m_Scene) { m_Scene->InitSpotlightItems(); } }

void SpotlightPalette::Open() { if (m_Scene) { m_Scene->OpenSpotlight(); } }

void SpotlightPalette::Close() { if (m_Scene) { m_Scene->CloseSpotlight(); } }

void SpotlightPalette::Draw(sf::RenderWindow &window) { if (m_Scene) { m_Scene->DrawSpotlightPalette(window); } }
