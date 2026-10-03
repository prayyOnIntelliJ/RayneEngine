#ifndef EDITORSCENE_H
#define EDITORSCENE_H

#include <vector>
#include <mutex>
#include <list>
#include <string>
#include <map>
#include <set>
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

enum class ObjectType {
    Rectangle, Circle, Triangle, Pentagon, Hexagon, Sprite, Camera,
    Empty, SpawnPoint, TriggerZone, PhysicsBox, PhysicsBall, StaticPlatform,
    WorldText, AudioSource, ParticleEmitter
};

struct EditorObject
{
    std::string id;
    std::string tag;
    Entity entity = 0;
    std::string parentId = "";

    sf::Vector2f localPosition = {0.f, 0.f};
    float rotation = 0.f;
    float scaleX = 1.f;
    float scaleY = 1.f;

    sf::Vector2f worldPosition = {0.f, 0.f};
    float worldRotation = 0.f;
    float worldScaleX = 1.f;
    float worldScaleY = 1.f;

    sf::RectangleShape shape;
    sf::CircleShape circleShape;
    sf::Color color;
    bool selected = false;
    std::string scriptPath;
    ObjectType objectType = ObjectType::Rectangle;
    std::string spritePath;
    std::shared_ptr<sf::Texture> previewTexture;
    sf::Sprite previewSprite;
    int zIndex = 0;
    std::string templatePath = "";
    std::map<std::string, ScriptComponent::Property> scriptProperties;

    // World Text fields
    std::string textString = "World Text";
    unsigned int textFontSize = 28;
    sf::Color textColor = sf::Color::White;
    int textAlignment = 0;

    // Audio Source fields
    std::string audioClipPath = "";
    float audioVolume = 100.0f;
    float audioPitch = 1.0f;
    bool audioLoop = false;
    bool audioPlayOnStart = true;
    bool audioIsSpatial = false;

    // Particle Emitter fields
    bool particleEmitting = true;
    int particleMaxParticles = 120;
    float particleRate = 25.0f;
    float particleLifetime = 1.5f;
    float particleSpeed = 120.0f;
    float particleAngle = -90.0f;
    float particleSpread = 45.0f;
    float particleStartSize = 8.0f;
    float particleEndSize = 2.0f;
    sf::Color particleStartColor = sf::Color(255, 190, 50, 255);
    sf::Color particleEndColor = sf::Color(255, 50, 20, 0);
    float particleGravityX = 0.0f;
    float particleGravityY = 60.0f;
    std::vector<Particle> editorParticles;
    float particleTimer = 0.0f;
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

    bool m_Rotating = false;
    float m_RotateMouseAngleStart = 0.f;
    float m_RotateObjAngleStart = 0.f;
    sf::Vector2f m_RotateHandlePos;

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
    float m_InspectorScrollY = 0.f;
    float m_InspectorContentHeight = 0.f;
    bool m_InspectorLocked = false;
    std::string m_LockedObjectId;
    EditorObject* GetInspectedObject();

    std::vector<InspectorButton> m_InspectorButtons;
    std::vector<std::pair<sf::FloatRect, EditorObject *> > m_HierarchyHitboxes;
    std::vector<std::pair<sf::FloatRect, std::string> > m_HierarchyFoldHitboxes;
    std::set<std::string> m_HierarchyCollapsed;
    bool m_HierarchyDragging = false;
    bool m_HierarchyPotentialDrag = false;
    sf::Vector2f m_HierarchyDragStartPos;
    std::string m_HierarchyDragSourceId;
    std::string m_HierarchyDragTargetId;
    sf::FloatRect m_HierarchyRootDropZone;

    bool m_ShowDeleteModal = false;
    std::string m_DeleteModalTargetId;
    std::vector<std::string> m_DeleteModalDescendantIds;
    sf::FloatRect m_DeleteModalCascadeBtn;
    sf::FloatRect m_DeleteModalUnparentBtn;
    sf::FloatRect m_DeleteModalCancelBtn;
    void DrawDeleteModal(sf::RenderWindow &window);
    void DeleteObjectWithPrompt(EditorObject* obj);
    void ConfirmDeleteCascade();
    void ConfirmDeleteUnparent();

    void UpdateWorldTransforms();
    bool IsDescendantOf(const std::string& childId, const std::string& ancestorId) const;
    std::vector<EditorObject*> GetChildren(const std::string& parentId);
    void SetParent(const std::string& childId, const std::string& newParentId, bool keepWorldTransform = true);

    void SaveAsTemplate(EditorObject *obj, const std::string &name = "");
    void ApplyToTemplate(EditorObject *obj);
    EditorObject* InstantiateTemplateOnCanvas(const std::string &templatePath, sf::Vector2f pos);

    bool m_ShowSaveTemplatePrompt = false;
    std::string m_SaveTemplateInputName;
    std::string m_SaveTemplateTargetId;
    void DrawSaveTemplateModal(sf::RenderWindow &window);

    bool m_HasUnsavedChanges = false;
    void SetDirty(bool dirty = true);
    bool IsDirty() const { return m_HasUnsavedChanges; }


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

    struct SpotlightItem
    {
        std::string id;
        std::string name;
        std::string category;
        std::string desc;
        ObjectType type;
    };

    bool m_SpotlightOpen = false;
    std::string m_SpotlightQuery = "";
    int m_SpotlightCategory = 0; // 0=All, 1=Primitives, 2=Gameplay, 3=Physics, 4=Media & FX
    int m_SpotlightSelectedIndex = 0;
    float m_SpotlightScrollY = 0.0f;
    std::vector<SpotlightItem> m_AllSpotlightItems;
    std::vector<SpotlightItem> m_FilteredSpotlightItems;
    std::vector<std::pair<sf::FloatRect, int>> m_SpotlightItemHitboxes;
    std::vector<std::pair<sf::FloatRect, int>> m_SpotlightCategoryHitboxes;
    sf::FloatRect m_SpotlightSearchBoxBounds;
    sf::FloatRect m_SpotlightModalBounds;
    sf::FloatRect m_SpotlightItemsViewportBounds;
    sf::FloatRect m_SpotlightCloseBtnBounds;
    sf::FloatRect m_SpotlightClearSearchBtnBounds;
    sf::FloatRect m_SpotlightScrollbarThumbBounds;
    sf::FloatRect m_SpotlightScrollbarTrackBounds;
    bool m_SpotlightSearchFocused = false;
    bool m_SpotlightDraggingScrollbar = false;
    float m_SpotlightDragScrollStartMouseY = 0.0f;
    float m_SpotlightDragScrollStartScrollY = 0.0f;

    void InitSpotlightItems();
    void FilterSpotlightItems();
    void OpenSpotlight();
    void CloseSpotlight();
    void DrawSpotlightPalette(sf::RenderWindow &window);
    void DrawSpotlightItemIcon(sf::RenderWindow &window, ObjectType type, sf::Vector2f center, float size);
    void SelectSpotlightItem(const SpotlightItem &item);

    bool m_HierarchyContextMenuOpen = false;
    sf::Vector2f m_ContextMenuPos;
    EditorObject *m_ContextObject = nullptr;
    std::vector<std::pair<sf::FloatRect, std::string> > m_ContextHitboxes;

    enum class EditField
    {
        None,
        Name,
        Tag,
        ZIndex,
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
        RigidbodyMass,
        RigidbodyGravity,
        RigidbodyRestitution,
        RigidbodyDrag,
        VelocityDX,
        VelocityDY,
        CameraSmoothSpeed,
        CameraOffsetX,
        CameraOffsetY,
        CameraZoom,
        CameraPriority,
        CameraMinZoom,
        CameraMaxZoom,
        CameraAutoFramePadding,
        UIText,
        ScriptProperty,
        TextContent,
        TextFontSize,
        AudioPath,
        AudioVolume,
        AudioPitch,
        ParticleRate,
        ParticleLifetime,
        ParticleSpeed,
        ParticleAngle,
        ParticleSpread,
        ParticleStartSize,
        ParticleEndSize,
        ParticleGravityX,
        ParticleGravityY
    };

    EditField m_ActiveField = EditField::None;
    std::string m_ActiveScriptProperty;
    std::string m_ActiveInputText;
    sf::FloatRect m_ActiveInputBounds;

    int m_InputSelectionStart = -1;
    int m_InputSelectionEnd = -1;
    bool m_IsSelectingText = false;
    bool HasTextSelection() const { return m_InputSelectionStart >= 0 && m_InputSelectionEnd >= 0 && m_InputSelectionStart != m_InputSelectionEnd; }
    int GetSelectionMin() const { return std::min(m_InputSelectionStart, m_InputSelectionEnd); }
    int GetSelectionMax() const { return std::max(m_InputSelectionStart, m_InputSelectionEnd); }
    void DeleteActiveSelection();

    std::string m_ActiveTooltip;
    void DrawTooltip(sf::RenderWindow &window);

    float m_AutoSaveTimer = 0.f;
    float m_AutoSavePopupTimer = 0.f;
    bool m_ShowAutoSavePopup = false;

    bool m_ShowBuildPopup = false;
    bool m_BuildFinished = false;
    std::string m_BuildStatusText = "";
    float m_BuildProgress = 0.0f;
    bool m_CancelBuildRequested = false;
    std::mutex m_BuildMutex;

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
    std::string m_PreferredIDE = "code";

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

    bool m_ShowProjectSettings = false;
    int m_ProjectSettingsTab = 0;
    
    std::string m_ProjectName = "RayneGame";
    std::string m_ProjectVersion = "1.0.0";
    std::string m_ProjectAuthor = "";
    std::string m_ProjectStartScene = "scenes/game.json";
    int m_ProjectWindowWidth = 1280;
    int m_ProjectWindowHeight = 720;
    bool m_ProjectVSync = true;
    int m_ProjectTargetFPS = 60;
    bool m_ProjectFullscreen = false;
    sf::Color m_ProjectClearColor = sf::Color(18, 20, 23);
    float m_ProjectMasterVolume = 100.f;
    float m_ProjectMusicVolume = 100.f;

    enum class ProjectSettingsField {
        None,
        ProjectName,
        Version,
        Author,
        StartScene,
        WindowWidth,
        WindowHeight,
        TargetFPS,
        ClearColorHex,
        MasterVolume,
        MusicVolume
    };

    ProjectSettingsField m_ActiveProjectSettingsField = ProjectSettingsField::None;
    std::string m_ProjectSettingsInputText;
    sf::FloatRect m_ProjectSettingsInputBounds;

    struct ProjectSettingsButton
    {
        sf::FloatRect bounds;
        std::string action;
    };
    std::vector<ProjectSettingsButton> m_ProjectSettingsButtons;

    sf::Clock m_FPSClock;
    float m_FPS = 0.f;
    int m_FrameCount = 0;

    void InitMenus();
    void DrawBuildPopup(sf::RenderWindow &window);

    void HandleMenuAction(const std::string &action);
    void ExportStandaloneGame();
    void PackageEngineZip();

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

    void CopySelection();
    void PasteClipboard();

    std::vector<std::shared_ptr<EditorCommand>> m_UndoStack;
    std::vector<std::shared_ptr<EditorCommand>> m_RedoStack;
    std::map<std::string, json> m_DragBeforeStates;
    std::vector<json> m_ClipboardObjects;

    void SaveToJson(const std::string &path);

    void LoadFromJson(const std::string &path);

    void SyncToRegistry();
    void CommitActiveField();

    void SaveSettings();

    void LoadSettings();

    void OpenScriptInIDE(const std::string &scriptPath);

    nlohmann::json m_PlayModeSnapshot;
    Entity m_SnapshotEntityCounter = 1;
    void SnapshotState();
    void RestoreSnapshot();

    sf::Vector2f SnapToGrid(sf::Vector2f pos) const;

    sf::Vector2f MouseWorldPos() const;

    EditorObject *ObjectAt(sf::Vector2f pos);
    EditorObject *ObjectById(const std::string& id);

    void DrawGrid();
    void DrawWorldAxes(sf::RenderWindow &window);

    void DrawGizmos(sf::RenderWindow &window);

    void DrawResizeHandles(sf::RenderWindow &window);

    int GetResizeHandle(sf::Vector2f worldPos) const;

    sf::Vector2f GetRotateHandlePos(const EditorObject* obj) const;
    bool GetRotateHandle(sf::Vector2f worldPos) const;

    void UpdateStatusText();

    std::string NextId();

    void DrawMenuBar(sf::RenderWindow &window);


    void DrawToolbar(sf::RenderWindow &window);

    void DrawAddDropdown(sf::RenderWindow &window);

    void DrawInspector(sf::RenderWindow &window);
    void DrawInspectorHeader(sf::RenderWindow &window, float panelX, float panelY);

    void DrawHierarchy(sf::RenderWindow &window);

    float DrawSectionHeader(sf::RenderWindow &window, const std::string &title, sf::Color accent, float x, float y);

    float DrawRow(sf::RenderWindow &window, const std::string &key, const std::string &val, float x, float y);

    float DrawEditableRow(sf::RenderWindow &window, const std::string &key, const std::string &val,
                          const std::string &action, float x, float y);

    float DrawCheckboxRow(sf::RenderWindow &window, const std::string &key, bool value,
                          const std::string &action, float x, float y);

    float DrawAddButton(sf::RenderWindow &window, const std::string &label, const std::string &action, float x,
                        float y);

    float DrawRemoveButton(sf::RenderWindow &window, const std::string &label, const std::string &action, float x,
                           float y);

    float DrawActionButton(sf::RenderWindow &window, const std::string &label, const std::string &action, float x,
                           float y, sf::Color fillColor, sf::Color borderColor);

    float DrawImagePreview(sf::RenderWindow &window, const std::string &path, float x, float y,
                           float w = -1.f, float h = 54.f, const std::string &action = "");

    float DrawTemplatePreview(sf::RenderWindow &window, const std::string &templatePath, float x, float y,
                              float w = -1.f, float h = 54.f, const std::string &action = "");

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

    void DrawProjectSettingsWindow(sf::RenderWindow &window);
    void HandleProjectSettingsClick(sf::Vector2f pos);
    void CommitActiveProjectSettingsField();
    void SaveProjectSettings();
    void LoadProjectSettings();
    float DrawProjectSettingsInputField(sf::RenderWindow &window, const std::string &label,
                                 const std::string &currentVal, ProjectSettingsField field,
                                 float x, float y, float winW);
    float DrawProjectSettingsToggle(sf::RenderWindow &window, const std::string &label,
                                    bool value, const std::string &action,
                                    float x, float y, float winW);
};

#endif
