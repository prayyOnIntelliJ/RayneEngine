#ifndef EDITORSCENE_H
#define EDITORSCENE_H

#include <vector>
#include <list>
#include <string>
#include <map>
#include <nlohmann/json.hpp>

#include "ContentBrowser.h"
#include "ConsolePanel.h"
#include "../UI/UIManager.h"
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
    std::string tag;
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
    float rotation = 0.f;
    float scaleX = 1.f;
    float scaleY = 1.f;
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

class EditorScene;

class EditorCommand {
public:
    virtual ~EditorCommand() = default;
    virtual void Execute(EditorScene* scene) = 0;
    virtual void Undo(EditorScene* scene) = 0;
};

class ObjectStateCommand : public EditorCommand {
public:
    std::string objectId;
    nlohmann::json beforeState;
    nlohmann::json afterState;

    ObjectStateCommand(std::string id, nlohmann::json before, nlohmann::json after) 
        : objectId(std::move(id)), beforeState(std::move(before)), afterState(std::move(after)) {}

    void Execute(EditorScene* scene) override;
    void Undo(EditorScene* scene) override;
};

class MacroCommand : public EditorCommand {
public:
    std::vector<std::shared_ptr<EditorCommand>> commands;
    void Execute(EditorScene* scene) override { for(auto& c : commands) c->Execute(scene); }
    void Undo(EditorScene* scene) override { for(auto it = commands.rbegin(); it != commands.rend(); ++it) (*it)->Undo(scene); }
};

class EditorScene : public Scene
{
    friend class EditorCommand;
    friend class ObjectStateCommand;

public:
    EditorScene(SceneManager &manager, sf::RenderWindow &window, Registry &registry);

    void HandleEvent(const sf::Event &event) override;

    void Update(float deltaTime) override;

    void Render(sf::RenderWindow &window) override;

    void OnEnter() override;

    void OnExit() override;
    void OnShutdown() override;

private:
    sf::RenderWindow &m_Window;
    Registry &m_Registry;

    std::list<EditorObject> m_Objects;
    EditorObject *m_Selected = nullptr;
    std::vector<EditorObject*> m_SelectedObjects;
    bool m_BoxSelecting = false;
    sf::Vector2f m_BoxSelectStart;
    sf::RectangleShape m_BoxSelectShape;
    int m_IdCounter = 0;

    sf::View m_camera;
    bool m_panning = false;
    sf::Vector2f m_panStart;

    bool m_Dragging = false;
    sf::Vector2f m_DragOffset;
    sf::Vector2f m_MouseScreenPos;

    bool m_Resizing = false;
    int m_ResizeHandle = -1;
    sf::Vector2f m_ResizeMouseStart;
    sf::Vector2f m_ResizeObjOrigin;
    sf::Vector2f m_ResizeObjSize;

    sf::RectangleShape m_Preview;
    sf::CircleShape m_CirclePreview;

    bool m_SnapToGrid = true;
    float m_GridSize = 40.f;

    float m_SaveFeedbackTimer = 0.f;

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
    std::vector<std::pair<sf::FloatRect, EditorObject *> > m_HierarchyHitboxes;


    std::unique_ptr<ContentBrowser> m_ContentBrowser;
    std::unique_ptr<ConsolePanel> m_ConsolePanel;

    enum class BottomPanelTab
    {
        ContentBrowser,
        Console
    };
    BottomPanelTab m_ActiveBottomPanelTab = BottomPanelTab::ContentBrowser;
    
    sf::FloatRect m_TabBrowserBounds;
    sf::FloatRect m_TabConsoleBounds;
    static constexpr float TabBarHeight = 24.f;

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
    EditorObject *m_ContextObject = nullptr;
    std::vector<std::pair<sf::FloatRect, std::string> > m_ContextHitboxes;

    enum class EditField
    {
        None,
        Name,
        Tag,
        TransformX,
        TransformY,
        Rotation,
        ScaleX,
        ScaleY,
        SizeW,
        SizeH,
        ColorR,
        ColorG,
        ColorB,
        Script,
        CollisionChannel,
        UIText
    };

    EditField m_ActiveField = EditField::None;
    std::string m_ActiveInputText;
    sf::FloatRect m_ActiveInputBounds;

    float m_AutoSaveTimer = 0.f;
    float m_AutoSavePopupTimer = 0.f;
    bool m_ShowAutoSavePopup = false;

    bool m_ShowSettings = false;
    int m_SettingsTab = 0;

    bool m_AutoSaveEnabled = true;
    float m_AutoSaveIntervalSeconds = 180.f;
    bool m_AutoSavePopupEnabled = true;
    float m_AutoSavePopupDuration = 10.f;
    std::string m_SceneSavePath = "scenes/game.json";

    sf::Color m_GridColor = sf::Color(38, 43, 51);
    int m_GridOpacity = 255;
    sf::Color m_EditorBgColor = sf::Color(18, 20, 23);
    float m_DefaultObjectSize = 40.f;
    sf::Color m_SelectionOutlineColor = sf::Color(124, 108, 240);
    float m_SelectionOutlineThickness = 2.f;

    bool m_ShowFPS = false;
    int m_FPSCapIndex = 1;
    float m_ZoomSensitivity = 0.1f;
    float m_ZoomMin = 0.2f;
    float m_ZoomMax = 5.f;

    bool m_PanOnMiddleButton = true;
    bool m_InvertPan = false;
    float m_ScrollSensitivity = 20.f;

    bool m_ShowEntityIDs = false;
    bool m_ShowColliderOutlines = false;
    int m_LogLevel = 2;
    bool m_ShowAutoSaveInTitle = false;

    enum class SettingsField
    {
        None,
        AutoSaveInterval, AutoSavePopupDuration, SceneSavePath,
        GridSize, DefaultObjectSize, SelectionThickness,
        GridOpacityVal,
        ZoomSensitivity, ZoomMin, ZoomMax,
        ScrollSensitivity
    };

    SettingsField m_ActiveSettingsField = SettingsField::None;
    std::string m_SettingsInputText;
    sf::FloatRect m_SettingsInputBounds;

    struct SettingsButton
    {
        sf::FloatRect bounds;
        std::string action;
    };

    std::vector<SettingsButton> m_SettingsButtons;

    sf::Clock m_FPSClock;
    float m_FPS = 0.f;
    int m_FrameCount = 0;

    void InitMenus();

    void HandleMenuAction(const std::string &action);

    void AddObject(sf::Vector2f pos, ObjectType type = ObjectType::Rectangle);

    void AddObjectWithSprite(sf::Vector2f pos, const std::string &spritePath);

    void ApplySpriteToObject(EditorObject &obj, const std::string &spritePath);

    void UpdateBounds();

    void DeleteSelected();
    
    void ClearSelection();
    void SelectObject(EditorObject* obj, bool multi);
    bool IsSelected(const EditorObject* obj) const;

    json SerializeObject(const EditorObject& obj) const;
    void DeserializeObject(const json& j);
    void ApplyState(EditorObject& obj, const json& j);
    void RemoveObject(const std::string& id);

    void ExecuteCommand(std::shared_ptr<EditorCommand> command);
    void UndoCommand();
    void RedoCommand();

    std::vector<std::shared_ptr<EditorCommand>> m_UndoStack;
    std::vector<std::shared_ptr<EditorCommand>> m_RedoStack;
    std::map<std::string, json> m_DragBeforeStates;

    void SaveToJson(const std::string &path);

    void LoadFromJson(const std::string &path);

    void SyncToRegistry();

    void SaveSettings();

    void LoadSettings();

    // Play-mode snapshot: stores editor state before entering play mode
    nlohmann::json m_PlayModeSnapshot;
    Entity m_SnapshotEntityCounter = 1;
    void SnapshotState();
    void RestoreSnapshot();

    sf::Vector2f SnapToGrid(sf::Vector2f pos) const;

    sf::Vector2f MouseWorldPos() const;

    EditorObject *ObjectAt(sf::Vector2f pos);
    EditorObject *ObjectById(const std::string& id);

    void DrawGrid();

    void DrawResizeHandles(sf::RenderWindow &window);

    int GetResizeHandle(sf::Vector2f worldPos) const;

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

    void DrawSettingsWindow(sf::RenderWindow &window);

    void HandleSettingsClick(sf::Vector2f pos);

    float DrawSettingsSectionHeader(sf::RenderWindow &window, const std::string &title,
                                    sf::Color accent, float x, float y, float winW);

    float DrawSettingsToggle(sf::RenderWindow &window, const std::string &label,
                             bool &value, const std::string &action,
                             float x, float y, float winW);

    float DrawSettingsSlider(sf::RenderWindow &window, const std::string &label,
                             float &value, float minVal, float maxVal,
                             const std::string &action,
                             float x, float y, float winW);

    float DrawSettingsInputField(sf::RenderWindow &window, const std::string &label,
                                 const std::string &currentVal, SettingsField field,
                                 float x, float y, float winW);

    float DrawSettingsDropdown(sf::RenderWindow &window, const std::string &label,
                               const std::vector<std::string> &options, int &currentIdx,
                               const std::string &action,
                               float x, float y, float winW);
};

#endif
