#include "EditorSerializer.h"
#include "../Core/EditorScene.h"

EditorSerializer::EditorSerializer(EditorScene *scene)
    : m_Scene(scene) {}

void EditorSerializer::SaveToJson(const std::string &path) { if (m_Scene) { m_Scene->SaveToJson(path); } }

void EditorSerializer::LoadFromJson(const std::string &path) { if (m_Scene) { m_Scene->LoadFromJson(path); } }
