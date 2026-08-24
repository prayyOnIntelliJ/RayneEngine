#ifndef EDITORSCENE_H
#define EDITORSCENE_H

#include <vector>
#include <string>
#include <nlohmann/json.hpp>

#include "ContentBrowser.h"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/CircleShape.hpp"
#include "SFML/Graphics/Text.hpp"
#include "SFML/Graphics/Font.hpp"

#include "../Scenes/Scene.h"
#include "../ECS/Registry.h"
#include "../ECS/Components.h"
#include "../Scripting/ScriptComponent.h"

using json = nlohmann::json;

enum class ObjectType { Rectangle, Circle, Triangle, Pentagon, Hexagon, Sprite };

struct EditorObject
{
    std::string id;
    Entity entity = 0;
    sf::RectangleShape shape;
    sf::CircleShape circleShape;
    sf::Color color;
    bool selected = false;
    std::string scriptPath;
    ObjectType objectType = ObjectType::Rectangle;
    std::string spritePath;
    std::shared_ptr<sf::Texture> previewTexture;
    sf::Sprite previewSprite;
};

struct InspectorButton
{
    sf::FloatRect bounds;
    std::string action;
};

struct MenuItem
{
    std::string label;
    std::string action;
    bool isSeparator = false;
    std::string shortcut;
};

struct MenuEntry
{
    std::string label;
    std::vector<MenuItem> items;
    sf::FloatRect bounds;
};

class EditorScene : public Scene
{
public:
    EditorScene(SceneManager &manager, sf::RenderWindow &window, Registry &registry);

    void HandleEvent(const sf::Event &event) override;

    void Update(float deltaTime) override;

    void Render(sf::RenderWindow &window) override;

    void OnEnter() override;

    void OnExit() override;

private:
    sf::RenderWindow &m_Window;
    Registry &m_Registry;

    std::vector<EditorObject> m_Objects;
    EditorObject *m_Selected = nullptr;
    int m_IdCounter = 0;

    sf::View m_camera;
    bool m_panning = false;
    sf::Vector2f m_panStart;

    bool m_Dragging = false;
    sf::Vector2f m_DragOffset;
    sf::Vector2f m_MouseScreenPos;

    sf::RectangleShape m_Preview;
    sf::CircleShape m_CirclePreview;

    bool m_SnapToGrid = true;
    float m_GridSize = 40.f;

    std::shared_ptr<sf::Font> m_Font;
    sf::Text m_StatusText;

    static constexpr float MenuBarHeight = 30.f;
    static constexpr float ToolbarHeight = 34.f;
    static constexpr float TopBarHeight = MenuBarHeight + ToolbarHeight;
    static constexpr float InspectorWidth = 270.f;
    static constexpr float HierarchyWidth = 240.f;
    static constexpr float InspectorPad = 10.f;
    static constexpr float BrowserHeight = 180.f;

    sf::RectangleShape m_InspectorPanel;
    sf::RectangleShape m_HierarchyPanel;
    
    sf::FloatRect m_InspectorBounds;
    sf::FloatRect m_BrowserBounds;
    sf::FloatRect m_HierarchyBounds;
    float m_HierarchyScrollY = 0.f;

    std::vector<InspectorButton> m_InspectorButtons;
    std::vector<std::pair<sf::FloatRect, EditorObject*>> m_HierarchyHitboxes;

    std::unique_ptr<ContentBrowser> m_ContentBrowser;


    ObjectType m_PlacementType = ObjectType::Rectangle;
    std::string m_PlacementSpritePath;
    std::shared_ptr<sf::Texture> m_PlacementTexture;

    std::vector<MenuEntry> m_Menus;
    int m_OpenMenuIndex = -1;
    bool m_AddDropdownOpen = false;
    sf::FloatRect m_AddBtnBounds;
    std::vector<std::pair<sf::FloatRect, std::string> > m_MenuItemHitboxes;
    std::vector<std::pair<sf::FloatRect, std::string> > m_AddDropdownHitboxes;
    std::vector<std::pair<sf::FloatRect, std::string> > m_ToolbarHitboxes;

    bool m_HierarchyContextMenuOpen = false;
    sf::Vector2f m_ContextMenuPos;
    EditorObject* m_ContextObject = nullptr;
    std::vector<std::pair<sf::FloatRect, std::string>> m_ContextHitboxes;

    enum class EditField
    {
        None, Name, Script,
        TransformX, TransformY,
        SizeW, SizeH,
        ColorR, ColorG, ColorB
    };

    EditField m_ActiveField = EditField::None;
    std::string m_ActiveInputText;
    sf::FloatRect m_ActiveInputBounds;

    void InitMenus();

    void HandleMenuAction(const std::string &action);

    void AddObject(sf::Vector2f pos, ObjectType type = ObjectType::Rectangle);

    void AddObjectWithSprite(sf::Vector2f pos, const std::string &spritePath);

    void ApplySpriteToObject(EditorObject &obj, const std::string &spritePath);

    void UpdateBounds();

    void DeleteSelected();

    void SaveToJson(const std::string &path);

    void LoadFromJson(const std::string &path);

    void SyncToRegistry();

    sf::Vector2f SnapToGrid(sf::Vector2f pos) const;

    sf::Vector2f MouseWorldPos() const;

    EditorObject *ObjectAt(sf::Vector2f pos);

    void DrawGrid();

    void UpdateStatusText();

    std::string NextId();

    void DrawMenuBar(sf::RenderWindow &window);

    void DrawToolbar(sf::RenderWindow &window);

    void DrawAddDropdown(sf::RenderWindow &window);

    void DrawInspector(sf::RenderWindow &window);
    
    void DrawHierarchy(sf::RenderWindow &window);

    float DrawSectionHeader(sf::RenderWindow &window, const std::string &title, sf::Color accent, float x, float y);

    float DrawRow(sf::RenderWindow &window, const std::string &key, const std::string &val, float x, float y);

    float DrawEditableRow(sf::RenderWindow &window, const std::string &key, const std::string &val,
                          const std::string &action, float x, float y);

    float DrawAddButton(sf::RenderWindow &window, const std::string &label, const std::string &action, float x,
                        float y);

    float DrawRemoveButton(sf::RenderWindow &window, const std::string &label, const std::string &action, float x,
                           float y);

    float DrawActionButton(sf::RenderWindow &window, const std::string &label, const std::string &action, float x,
                           float y, sf::Color fillColor, sf::Color borderColor);

    float DrawScriptInput(sf::RenderWindow &window, float x, float y);

    void HandleInspectorClick(sf::Vector2f pos);
};

#endif
