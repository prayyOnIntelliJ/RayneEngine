#ifndef EDITORSCENE_H
#define EDITORSCENE_H

#include <vector>
#include <string>
#include <nlohmann/json.hpp>

#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/Text.hpp"
#include "SFML/Graphics/Font.hpp"
#include "SFML/Window/Event.hpp"

#include "../Scenes/Scene.h"
#include "../ECS/Registry.h"
#include "../ECS/Components.h"
#include "../Scripting/ScriptComponent.h"

using json = nlohmann::json;

struct EditorObject
{
    std::string id;
    Entity entity = 0;
    sf::RectangleShape shape;
    sf::Color color;
    bool selected = false;
    std::string scriptPath;
};

struct InspectorButton
{
    sf::FloatRect bounds;
    std::string action;
};

class EditorScene : public Scene
{
public:
    EditorScene(SceneManager& manager, sf::RenderWindow& window, Registry& registry);

    void HandleEvent(const sf::Event& event) override;
    void Update(float deltaTime) override;
    void Render(sf::RenderWindow& window) override;

    void OnEnter() override;
    void OnExit() override;

private:
    sf::RenderWindow& m_Window;
    Registry& m_Registry;

    std::vector<EditorObject> m_Objects;
    EditorObject* m_Selected = nullptr;
    int m_IdCounter = 0;

    sf::View m_camera;
    bool m_panning = false;
    sf::Vector2f m_panStart;

    bool m_Dragging = false;
    sf::Vector2f m_DragOffset;
    sf::Vector2f m_MouseScreenPos;

    sf::RectangleShape m_Preview;

    bool m_SnapToGrid = true;
    float m_GridSize = 40.f;
    sf::Vector2f m_ToolboxSize = { 0.f, 50.f };

    sf::Font m_Font;
    sf::Text m_StatusText;
    sf::Text m_HelpText;
    sf::RectangleShape m_Toolbar;

    static constexpr float InspectorWidth = 270.f;
    static constexpr float InspectorPad = 10.f;
    sf::RectangleShape m_InspectorPanel;
    std::vector<InspectorButton> m_InspectorButtons;

    bool m_ScriptInputActive = false;
    std::string m_ScriptInputText;
    sf::FloatRect m_ScriptInputBounds;

    bool m_NameInputActive = false;
    std::string m_NameInputText;
    sf::FloatRect m_NameInputBounds;

    void AddObject(sf::Vector2f pos);
    void DeleteSelected();
    void SaveToJson(const std::string& path);
    void LoadFromJson(const std::string& path);
    void SyncToRegistry();
    sf::Vector2f SnapToGrid(sf::Vector2f pos) const;
    sf::Vector2f MouseWorldPos() const;
    EditorObject* ObjectAt(sf::Vector2f pos);
    void DrawGrid();
    void UpdateStatusText();
    std::string NextId();

    void DrawInspector(sf::RenderWindow& window);
    float DrawSectionHeader(sf::RenderWindow& window, const std::string& title, sf::Color accent, float x, float y);
    float DrawRow(sf::RenderWindow& window, const std::string& key, const std::string& val, float x, float y);
    float DrawEditableRow(sf::RenderWindow& window, const std::string& key, const std::string& val, const std::string& action, sf::FloatRect& boundsOut, float x, float y);
    float DrawAddButton(sf::RenderWindow& window, const std::string& label, const std::string& action, float x, float y);
    float DrawScriptInput(sf::RenderWindow& window, float x, float y);
    void HandleInspectorClick(sf::Vector2f pos);
};

#endif
