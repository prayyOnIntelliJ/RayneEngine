#include "EditorScene.h"
#include "../Scenes/SceneManager.h"
#include "../Application/Application.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <map>
#include <chrono>
#include <ctime>

#include "../ECS/Components.h"
#include "../Scripting/ScriptComponent.h"
#include "../Scripting/LuaState.h"
#include "../Resources/ResourceManager.h"
#include "../Application/EngineVersion.h"
#include "SFML/Window/Event.hpp"
#include <SFML/Graphics/ConvexShape.hpp>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#undef CreateWindow
#endif

static sf::Vector2f RotatePoint(sf::Vector2f point, sf::Vector2f center, float angleDegrees);

static const sf::Color C_PANEL_BG = sf::Color(30, 32, 38);

static ShapeType MapToShapeType(ObjectType type)
{
    switch (type)
    {
        case ObjectType::Circle: return ShapeType::Circle;
        case ObjectType::Triangle: return ShapeType::Triangle;
        case ObjectType::Pentagon: return ShapeType::Pentagon;
        case ObjectType::Hexagon: return ShapeType::Hexagon;
        default: return ShapeType::Rectangle;
    }
}

static bool IsPolygonType(ObjectType type)
{
    return type == ObjectType::Circle || type == ObjectType::Triangle ||
           type == ObjectType::Pentagon || type == ObjectType::Hexagon;
}

static size_t GetPolygonPointCount(ObjectType type)
{
    switch (type)
    {
        case ObjectType::Triangle: return 3;
        case ObjectType::Pentagon: return 5;
        case ObjectType::Hexagon: return 6;
        case ObjectType::Circle: default: return 30;
    }
}

namespace fs = std::filesystem;

static const sf::Color C_BG_CANVAS = sf::Color(18, 20, 23);
static const sf::Color C_BG_PANEL = sf::Color(26, 29, 34);
static const sf::Color C_BG_ELEVATED = sf::Color(33, 37, 43);
static const sf::Color C_BG_INPUT = sf::Color(20, 23, 27);
static const sf::Color C_BORDER = sf::Color(42, 46, 53);
static const sf::Color C_BORDER_LIGHT = sf::Color(58, 63, 72);
static const sf::Color C_TEXT_PRIMARY = sf::Color(232, 234, 237);
static const sf::Color C_TEXT_SECONDARY = sf::Color(154, 160, 172);
static const sf::Color C_TEXT_MUTED = sf::Color(92, 97, 107);

static const sf::Color C_ACCENT = sf::Color(124, 108, 240);
static const sf::Color C_ACCENT_HOV = sf::Color(146, 132, 245);
static const sf::Color C_ACCENT_ACT = sf::Color(100, 85, 217);
static const sf::Color C_ACCENT_DIM = sf::Color(40, 35, 80, 200);
static const sf::Color C_ACCENT_BRIGHT = sf::Color(146, 132, 245);

static const sf::Color C_ACCENT2 = sf::Color(67, 217, 200);

static const sf::Color C_SUCCESS = sf::Color(74, 222, 128);
static const sf::Color C_SUCCESS_DIM = sf::Color(20, 55, 35, 200);
static const sf::Color C_WARNING = sf::Color(245, 185, 77);
static const sf::Color C_DANGER = sf::Color(241, 104, 94);
static const sf::Color C_DANGER_DIM = sf::Color(70, 20, 18, 200);

static const sf::Color C_GRID_MINOR = sf::Color(38, 43, 51);
static const sf::Color C_GRID_MAJOR = sf::Color(51, 58, 69);

EditorScene::EditorScene(SceneManager &manager, sf::RenderWindow &window, Registry &registry)
    : Scene(manager), m_Window(window), m_Registry(registry)
{
    m_Font = ResourceManager::Get().GetFont(ENGINE_ASSET_PATH "/fonts/Merriweather.ttf");
    m_ContentBrowser = std::make_unique<ContentBrowser>(*m_Font, ASSET_PATH);
    m_ConsolePanel = std::make_unique<ConsolePanel>(*m_Font);
    m_ContentBrowser->onSceneLoadRequest = [this](const std::string &path) {
        this->LoadFromJson(path);
        std::error_code ec;
        std::filesystem::path p(path);
        std::filesystem::path root = std::filesystem::absolute(ASSET_PATH, ec);
        std::string relPath = std::filesystem::proximate(p, root, ec).generic_string();
        if (ec) relPath = p.filename().string();
        
        this->m_SceneSavePath = relPath;
        this->SaveSettings();
        std::cout << "[INFO] [EditorScene] Loaded scene from browser. Set active path to: " << relPath << "\n";
    };

    m_camera = window.getDefaultView();

    const float winW = static_cast<float>(window.getSize().x);
    const float winH = static_cast<float>(window.getSize().y);

    sf::View adjustedView = m_camera;
    adjustedView.setViewport({
        HierarchyWidth / winW,
        TopBarHeight / winH,
        1.f - ((InspectorWidth + HierarchyWidth) / winW),
        1.f - (BrowserHeight / winH) - (TopBarHeight / winH)
    });
    m_camera = adjustedView;

    UpdateBounds();

    m_Preview.setSize({m_GridSize, m_GridSize});
    m_Preview.setFillColor(sf::Color(C_ACCENT.r, C_ACCENT.g, C_ACCENT.b, 45));
    m_Preview.setOutlineColor(sf::Color(C_ACCENT_HOV.r, C_ACCENT_HOV.g, C_ACCENT_HOV.b, 160));
    m_Preview.setOutlineThickness(1.f);

    m_CirclePreview.setRadius(m_GridSize / 2.f);
    m_CirclePreview.setFillColor(sf::Color(C_ACCENT.r, C_ACCENT.g, C_ACCENT.b, 45));
    m_CirclePreview.setOutlineColor(sf::Color(C_ACCENT_HOV.r, C_ACCENT_HOV.g, C_ACCENT_HOV.b, 160));
    m_CirclePreview.setOutlineThickness(1.f);

    m_StatusText.setFont(*m_Font);
    m_StatusText.setCharacterSize(11);
    m_StatusText.setFillColor(C_TEXT_MUTED);

    m_InspectorPanel.setFillColor(C_BG_PANEL);
    m_HierarchyPanel.setFillColor(C_BG_PANEL);

    InitMenus();
    UpdateStatusText();

    LoadSettings();
    LoadProjectSettings();

    const std::string scenesDir = ASSET_PATH "/scenes";
    const std::string defaultScenePath = std::string(ASSET_PATH) + "/" + m_SceneSavePath;

    if (!std::filesystem::exists(scenesDir)) { std::filesystem::create_directories(scenesDir); }

    if (std::filesystem::exists(defaultScenePath))
    {
        std::cout << "[INFO] [EditorScene] Auto-loading default scene at startup...\n";
        LoadFromJson(defaultScenePath);
    } else
    {
        std::cout << "[INFO] [EditorScene] Default scene not found, creating new empty scene at " << defaultScenePath <<
                "...\n";
        SaveToJson(defaultScenePath);
    }
}

void EditorScene::InitMenus()
{
    MenuEntry datei;
    datei.label = "File";
    datei.items = {
        {"Save", "save", false, "Ctrl+S"},
        {"Load", "load", false, "Ctrl+L"},
        {"", "", true, ""},
        {"Project Settings", "project_settings", false, ""},
        {"Quit", "quit", false, ""}
    };

    MenuEntry buildMenu;
    buildMenu.label = "Build";
    buildMenu.items = {
        {"Build Game (Standalone)", "build_game", false, ""},
        {"Package Engine (ZIP)", "package_engine", false, ""}
    };

    MenuEntry edit;
    edit.label = "Edit";
    edit.items = {
        {"Settings", "settings", false, "Ctrl+,"},
        {"", "", true, ""},
        {"Duplicate", "duplicate", false, "Ctrl+D"},
        {"Delete", "delete", false, "Del"},
        {"", "", true, ""},
        {"Deselect everything", "deselect", false, "Esc"},
        {"", "", true, ""},
        {"Clear Scene", "reset_scene", false, ""}
    };

    MenuEntry ansicht;
    ansicht.label = "View";
    ansicht.items = {
        {"Toggle Grid", "toggle_grid", false, "G"},
        {"Center Camera", "center_camera", false, ""}
    };

    MenuEntry tools;
    tools.label = "Tools";
    tools.items = {
        {"Run Scene", "run", false, "F5"},
        {"", "", true, ""},
        {"Save", "save", false, "Ctrl+S"}
    };

    m_Menus = {datei, buildMenu, edit, ansicht, tools};
}

void EditorScene::OnEnter()
{
    if (!m_PlayModeSnapshot.empty())
    {
        RestoreSnapshot();
        m_PlayModeSnapshot = json{};
    }

    std::cout << "[INFO] [EditorScene] Activated Editor Layout\n";
    UpdateBounds();
    UpdateStatusText();
}

void EditorScene::OnExit() { SyncToRegistry(); }

void EditorScene::OnShutdown()
{
    std::cout << "[INFO] [EditorScene] Shutting down, saving scene state...\n";
    if (!m_PlayModeSnapshot.empty())
    {
        RestoreSnapshot();
        m_PlayModeSnapshot = json{};
    }

    SaveToJson(std::string(ASSET_PATH) + "/" + m_SceneSavePath);
    SaveSettings();
}

void EditorScene::UpdateBounds()
{
    const float w = static_cast<float>(m_Window.getSize().x);
    const float h = static_cast<float>(m_Window.getSize().y);

    m_HierarchyBounds = {0.f, TopBarHeight, HierarchyWidth, h - TopBarHeight};
    m_BrowserBounds = {HierarchyWidth, h - BrowserHeight, w - InspectorWidth - HierarchyWidth, BrowserHeight};
    
    m_TabBrowserBounds = {HierarchyWidth, h - BrowserHeight - TabBarHeight, 100.f, TabBarHeight};
    m_TabConsoleBounds = {HierarchyWidth + 100.f, h - BrowserHeight - TabBarHeight, 100.f, TabBarHeight};
    
    m_InspectorBounds = {w - InspectorWidth, TopBarHeight, InspectorWidth, h - TopBarHeight};
}

void EditorScene::HandleMenuAction(const std::string &action)
{
    if (action == "settings")
    {
        m_ShowSettings = !m_ShowSettings;
        m_ActiveSettingsField = SettingsField::None;
        m_SettingsInputText.clear();
    } else if (action == "save")
    {
        SyncToRegistry();
        SaveToJson(std::string(ASSET_PATH) + "/" + m_SceneSavePath);
        std::cout << "[INFO] [EditorScene] Scene saved successfully to " << m_SceneSavePath << "\n";
        m_SaveFeedbackTimer = 2.0f;
    } else if (action == "load")
    {
        LoadFromJson(std::string(ASSET_PATH) + "/" + m_SceneSavePath);
        std::cout << "[INFO] [EditorScene] Scene loaded successfully from " << m_SceneSavePath << "\n";
    } else if (action == "build_game")
    {
        ExportStandaloneGame();
    } else if (action == "package_engine")
    {
        PackageEngineZip();
    } else if (action == "project_settings")
    {
        m_ShowProjectSettings = !m_ShowProjectSettings;
        m_ActiveProjectSettingsField = ProjectSettingsField::None;
        m_ProjectSettingsInputText.clear();
    } else if (action == "quit") { m_Window.close(); } else if (action == "delete")
    {
        DeleteSelected();
        UpdateStatusText();
    } else if (action == "deselect")
    {
        ClearSelection();
        m_ActiveField = EditField::None;
        m_ActiveInputText.clear();
        UpdateStatusText();
    } else if (action == "toggle_grid")
    {
        m_SnapToGrid = !m_SnapToGrid;
        UpdateStatusText();
    } else if (action == "center_camera") { m_camera.setCenter(0.f, 0.f); } else if (action == "run")
    {
        SyncToRegistry();
        SnapshotState();
        m_manager.SwitchSceneTo("game");
    } else if (action == "reset_scene")
    {
        for (auto &obj: m_Objects)
            if (obj.entity != 0) m_Registry.DestroyEntity(obj.entity);
        m_Objects.clear();
        m_Selected = nullptr;
        UpdateStatusText();
    } else if (action == "duplicate")
    {
        if (m_Selected)
        {
            const ObjectType t = (m_Selected->objectType == ObjectType::Sprite)
                                     ? ObjectType::Rectangle
                                     : m_Selected->objectType;
            AddObject(m_Selected->shape.getPosition() + sf::Vector2f(m_GridSize, 0.f), t);
        }
    } else if (action == "open_ui_editor") { m_manager.SwitchSceneTo("ui_editor"); }
}

void EditorScene::HandleEvent(const sf::Event &event)
{
    UpdateBounds();

    if (m_ShowBuildPopup)
    {
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
        {
            if (m_BuildFinished || !m_CancelBuildRequested) {
                m_ShowBuildPopup = false;
            }
            return;
        }

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
        {
            float pW = 500.f;
            float pH = 180.f;
            float pX = (m_Window.getSize().x - pW) / 2.f;
            float pY = (m_Window.getSize().y - pH) / 2.f;
            
            float btnW = 100.f;
            float btnH = 30.f;
            float btnX = pX + pW / 2.f - btnW / 2.f;
            float btnY = pY + pH - 20.f - btnH;
            
            sf::FloatRect btnBounds(btnX, btnY, btnW, btnH);
            if (btnBounds.contains((float)event.mouseButton.x, (float)event.mouseButton.y)) {
                if (m_BuildFinished) {
                    m_ShowBuildPopup = false;
                } else {
                    std::lock_guard<std::mutex> lock(m_BuildMutex);
                    m_CancelBuildRequested = true;
                }
            }
            return;
        }
        
        if (event.type == sf::Event::MouseMoved ||
            event.type == sf::Event::MouseButtonPressed ||
            event.type == sf::Event::MouseButtonReleased ||
            event.type == sf::Event::MouseWheelScrolled ||
            event.type == sf::Event::KeyPressed ||
            event.type == sf::Event::KeyReleased)
        {
            return;
        }
    }

    if (m_ShowProjectSettings)
    {
        if (event.type == sf::Event::TextEntered && m_ActiveProjectSettingsField != ProjectSettingsField::None)
        {
            if (event.text.unicode == '\b') {
                if (!m_ProjectSettingsInputText.empty()) m_ProjectSettingsInputText.pop_back();
            } else if (event.text.unicode == '\r' || event.text.unicode == '\n') {
                CommitActiveProjectSettingsField();
            } else if (event.text.unicode == 27) {
                m_ActiveProjectSettingsField = ProjectSettingsField::None;
                m_ProjectSettingsInputText.clear();
            } else if (event.text.unicode < 128) {
                m_ProjectSettingsInputText += static_cast<char>(event.text.unicode);
            }
            return;
        }

        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
        {
            if (m_ActiveProjectSettingsField != ProjectSettingsField::None)
            {
                m_ActiveProjectSettingsField = ProjectSettingsField::None;
                m_ProjectSettingsInputText.clear();
            } else {
                CommitActiveProjectSettingsField();
                m_ShowProjectSettings = false;
            }
            return;
        }

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
        {
            HandleProjectSettingsClick({(float) event.mouseButton.x, (float) event.mouseButton.y});
            return;
        }

        return;
    }

    if (m_ShowSettings)
    {
        if (event.type == sf::Event::TextEntered && m_ActiveSettingsField != SettingsField::None)
        {
            if (event.text.unicode ==
                '\b') { if (!m_SettingsInputText.empty()) m_SettingsInputText.pop_back(); } else if (
                event.text.unicode == '\r' || event.text.unicode == '\n')
            {
                try
                {
                    float v = std::stof(m_SettingsInputText);
                    if (m_ActiveSettingsField == SettingsField::AutoSaveInterval)
                        m_AutoSaveIntervalSeconds = std::max(10.f, v);
                    else if (m_ActiveSettingsField == SettingsField::AutoSavePopupDuration)
                        m_AutoSavePopupDuration = std::clamp(v, 1.f, 60.f);
                    else if (m_ActiveSettingsField == SettingsField::GridSize)
                        m_GridSize = m_DefaultObjectSize = std::clamp(v, 8.f, 256.f);
                    else if (m_ActiveSettingsField == SettingsField::DefaultObjectSize)
                        m_DefaultObjectSize = std::clamp(v, 8.f, 512.f);
                    else if (m_ActiveSettingsField == SettingsField::SelectionThickness)
                        m_SelectionOutlineThickness = std::clamp(v, 0.f, 8.f);
                    else if (m_ActiveSettingsField == SettingsField::GridOpacityVal)
                        m_GridOpacity = std::clamp(static_cast<int>(v), 0, 255);
                    else if (m_ActiveSettingsField == SettingsField::ZoomSensitivity)
                        m_ZoomSensitivity = std::clamp(v, 0.01f, 0.5f);
                    else if (m_ActiveSettingsField == SettingsField::ZoomMin)
                        m_ZoomMin = std::clamp(v, 0.05f, 1.f);
                    else if (m_ActiveSettingsField == SettingsField::ZoomMax)
                        m_ZoomMax = std::clamp(v, 1.f, 20.f);
                    else if (m_ActiveSettingsField == SettingsField::ScrollSensitivity)
                        m_ScrollSensitivity = std::clamp(v, 1.f, 100.f);
                } catch (...) {}

                if (m_ActiveSettingsField == SettingsField::SceneSavePath)
                    m_SceneSavePath = m_SettingsInputText;

                m_ActiveSettingsField = SettingsField::None;
                m_SettingsInputText.clear();
                SaveSettings();
            } else if (event.text.unicode == 27)
            {
                m_ActiveSettingsField = SettingsField::None;
                m_SettingsInputText.clear();
            } else if (event.text.unicode < 128) { m_SettingsInputText += static_cast<char>(event.text.unicode); }
            return;
        }

        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
        {
            if (m_ActiveSettingsField != SettingsField::None)
            {
                m_ActiveSettingsField = SettingsField::None;
                m_SettingsInputText.clear();
            } else { m_ShowSettings = false; }
            return;
        }

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
        {
            HandleSettingsClick({(float) event.mouseButton.x, (float) event.mouseButton.y});
            return;
        }

        return;
    }

    if (event.type == sf::Event::MouseMoved)
    {
        m_MouseScreenPos = {(float) event.mouseMove.x, (float) event.mouseMove.y};

        if (m_panning)
        {
            sf::Vector2f current = m_Window.mapPixelToCoords(
                {event.mouseMove.x, event.mouseMove.y}, m_camera);
            m_camera.move(m_panStart - current);
        }

        if (m_MouseScreenPos.y > TopBarHeight)
        {
            sf::Vector2f pos = MouseWorldPos();
            if (IsPolygonType(m_PlacementType))
            {
                m_CirclePreview.setPointCount(GetPolygonPointCount(m_PlacementType));
                m_CirclePreview.setPosition(SnapToGrid(pos));
            } else
                m_Preview.setPosition(SnapToGrid(pos));
        }

        if (m_Dragging) { 
            sf::Vector2f primaryPos = SnapToGrid(MouseWorldPos() - m_DragOffset);
            if (m_Selected) {
                sf::Vector2f delta = primaryPos - m_Selected->shape.getPosition();
                for (auto* obj : m_SelectedObjects) {
                    obj->shape.setPosition(obj->shape.getPosition() + delta);
                }
            }
        }

        if (m_Resizing && m_Selected)
        {
            sf::Vector2f mouseWorld = MouseWorldPos();
            sf::Vector2f deltaWorld = mouseWorld - m_ResizeMouseStart;
            sf::Vector2f deltaLocal = RotatePoint(deltaWorld, {0.f, 0.f}, -m_Selected->rotation);

            sf::Vector2f newSize = m_ResizeObjSize;
            sf::Vector2f localPosOffset = {0.f, 0.f};
            const float minSize = 4.f;

            switch (m_ResizeHandle)
            {
                case 0:
                    localPosOffset.x = std::min(deltaLocal.x, m_ResizeObjSize.x - minSize);
                    localPosOffset.y = std::min(deltaLocal.y, m_ResizeObjSize.y - minSize);
                    newSize.x = m_ResizeObjSize.x - localPosOffset.x;
                    newSize.y = m_ResizeObjSize.y - localPosOffset.y;
                    break;
                case 1:
                    localPosOffset.y = std::min(deltaLocal.y, m_ResizeObjSize.y - minSize);
                    newSize.y = m_ResizeObjSize.y - localPosOffset.y;
                    break;
                case 2:
                    localPosOffset.y = std::min(deltaLocal.y, m_ResizeObjSize.y - minSize);
                    newSize.x = std::max(m_ResizeObjSize.x + deltaLocal.x, minSize);
                    newSize.y = m_ResizeObjSize.y - localPosOffset.y;
                    break;
                case 3:
                    localPosOffset.x = std::min(deltaLocal.x, m_ResizeObjSize.x - minSize);
                    newSize.x = m_ResizeObjSize.x - localPosOffset.x;
                    break;
                case 4:
                    newSize.x = std::max(m_ResizeObjSize.x + deltaLocal.x, minSize);
                    break;
                case 5:
                    localPosOffset.x = std::min(deltaLocal.x, m_ResizeObjSize.x - minSize);
                    newSize.x = m_ResizeObjSize.x - localPosOffset.x;
                    newSize.y = std::max(m_ResizeObjSize.y + deltaLocal.y, minSize);
                    break;
                case 6:
                    newSize.y = std::max(m_ResizeObjSize.y + deltaLocal.y, minSize);
                    break;
                case 7:
                    newSize.x = std::max(m_ResizeObjSize.x + deltaLocal.x, minSize);
                    newSize.y = std::max(m_ResizeObjSize.y + deltaLocal.y, minSize);
                    break;
                default: break;
            }

            sf::Vector2f worldPosOffset = RotatePoint(localPosOffset, {0.f, 0.f}, m_Selected->rotation);
            sf::Vector2f newPos = m_ResizeObjOrigin + worldPosOffset;

            m_Selected->shape.setPosition(newPos);
            m_Selected->shape.setSize(newSize);

            if (IsPolygonType(m_Selected->objectType))
            {
                m_Selected->circleShape.setPosition(newPos);
                m_Selected->circleShape.setRadius(newSize.x / 2.f);
            }

            if (m_Selected->previewTexture)
            {
                const sf::Vector2u ts = m_Selected->previewTexture->getSize();
                if (ts.x > 0 && ts.y > 0)
                    m_Selected->previewSprite.setScale(newSize.x / ts.x, newSize.y / ts.y);
            }
        }

        if (m_Rotating && m_Selected)
        {
            sf::Vector2f center = m_Selected->shape.getPosition() + m_Selected->shape.getSize() * 0.5f;
            sf::Vector2f pos = MouseWorldPos();
            float currentAngle = std::atan2(pos.y - center.y, pos.x - center.x) * 180.f / 3.14159265f;
            
            float newRotation = m_RotateObjAngleStart + (currentAngle - m_RotateMouseAngleStart);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
            {
                newRotation = std::round(newRotation / 15.f) * 15.f;
            }

            m_Selected->rotation = newRotation;
            m_Selected->shape.setRotation(newRotation);
            if (IsPolygonType(m_Selected->objectType))
                m_Selected->circleShape.setRotation(newRotation);
            if (m_Selected->previewTexture)
                m_Selected->previewSprite.setRotation(newRotation);
        }

        if (m_ContentBrowser->HasDraggedAsset())
            m_ContentBrowser->HandleEvent(event, m_MouseScreenPos);

        return;
    }

    if (event.type == sf::Event::MouseButtonReleased)
    {
        m_MouseScreenPos = {(float) event.mouseButton.x, (float) event.mouseButton.y};
    }

    const bool inTopBars = m_MouseScreenPos.y < TopBarHeight;
    const bool inMenuBar = m_MouseScreenPos.y < MenuBarHeight;
    const bool inBrowser = m_BrowserBounds.contains(m_MouseScreenPos);
    const bool inInspector = m_InspectorBounds.contains(m_MouseScreenPos);
    const bool inTabs = m_TabBrowserBounds.contains(m_MouseScreenPos) || m_TabConsoleBounds.contains(m_MouseScreenPos);

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
    {
        if (m_TabBrowserBounds.contains(m_MouseScreenPos)) {
            m_ActiveBottomPanelTab = BottomPanelTab::ContentBrowser;
            return;
        } else if (m_TabConsoleBounds.contains(m_MouseScreenPos)) {
            m_ActiveBottomPanelTab = BottomPanelTab::Console;
            return;
        }
    }

    if (event.type == sf::Event::MouseButtonReleased &&
        event.mouseButton.button == sf::Mouse::Left &&
        m_ContentBrowser->HasDraggedAsset())
    {
        DraggedAsset drag = m_ContentBrowser->GetDraggedAsset();

        m_ContentBrowser->ClearDrag();

        const bool inHierarchy = m_HierarchyBounds.contains(m_MouseScreenPos);

        if (drag.type == AssetType::Image)
        {
            if (inInspector || inHierarchy)
            {
                if (m_Selected)
                    ApplySpriteToObject(*m_Selected, drag.path);
            } else if (!inBrowser && !inTopBars && !inTabs)
            {
                EditorObject *hit = ObjectAt(MouseWorldPos());
                if (hit)
                    ApplySpriteToObject(*hit, drag.path);
                else
                {
                    m_Selected = nullptr;
                    AddObjectWithSprite(MouseWorldPos(), drag.path);
                }
            }
        } else if (drag.type == AssetType::Script)
        {
            if (!inBrowser && !inTopBars && !inTabs)
            {
                EditorObject *target = nullptr;
                if (inInspector || inHierarchy)
                    target = m_Selected;
                else
                    target = ObjectAt(MouseWorldPos());

                if (target && target->entity != 0 && !m_Registry.HasComponent<ScriptComponent>(target->entity))
                {
                    auto &sc = m_Registry.AddComponent(target->entity, ScriptComponent(LuaState::GetLua(), drag.path));
                    sc.SetEntity(target->entity);
                    target->scriptPath = drag.path;
                    std::cout << "[INFO] [ContentBrowser] Script dropped onto Entity " << target->id << "\n";
                }
            }
        }

        return;
    }

    if (m_ActiveBottomPanelTab == BottomPanelTab::Console)
    {
        if (inBrowser || m_ConsolePanel->IsInputActive())
        {
            m_ConsolePanel->HandleEvent(event, m_MouseScreenPos);
        }
    }
    else
    {
        if (inBrowser || m_ContentBrowser->HasDraggedAsset() ||
            m_ContentBrowser->IsInputActive() || m_ContentBrowser->IsContextMenuOpen())
        {
            std::string prevSel = m_ContentBrowser->GetSelectedPath();
            m_ContentBrowser->HandleEvent(event, m_MouseScreenPos);

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
            {
                if (m_ContentBrowser->GetSelectedPath() != prevSel && !m_ContentBrowser->GetSelectedPath().empty())
                {
                    if (m_Selected) m_Selected->selected = false;
                    m_Selected = nullptr;
                    UpdateStatusText();
                }
            }
        }
    }

    if ((m_ContentBrowser->IsInputActive() || m_ConsolePanel->IsInputActive()) &&
        (event.type == sf::Event::TextEntered || event.type == sf::Event::KeyPressed))
        return;

    if (event.type == sf::Event::TextEntered && m_ActiveField != EditField::None)
    {
        if (event.text.unicode == '\b') { if (!m_ActiveInputText.empty()) m_ActiveInputText.pop_back(); } else if (
            event.text.unicode == '\r' || event.text.unicode == '\n')
        {
            if (m_Selected && !m_ActiveInputText.empty())
            {
                if (m_ActiveField == EditField::Name)
                {
                    m_Selected->id = m_ActiveInputText;
                    UpdateStatusText();
                } else if (m_ActiveField == EditField::Tag)
                {
                    m_Selected->tag = m_ActiveInputText;
                    if (m_Selected->entity != 0)
                    {
                        if (m_Registry.HasComponent<TagComponent>(m_Selected->entity))
                            m_Registry.GetComponent<TagComponent>(m_Selected->entity).tag = m_Selected->tag;
                        else
                            m_Registry.AddComponent(m_Selected->entity, TagComponent{m_Selected->tag});
                    }
                    UpdateStatusText();
                } else if (m_ActiveField == EditField::Script)
                {
                    std::string fullPath = std::string(ASSET_PATH) + "/" + m_ActiveInputText + ".lua";
                    std::ifstream check(fullPath);
                    if (!check.is_open())
                    {
                        std::ofstream newFile(fullPath);
                        newFile << "function OnCreate()\n\nend\n\n";
                        newFile << "function OnUpdate(dt)\n\nend\n";
                        newFile.close();
                        std::cout << "[INFO] [Inspector] Created new Lua script file: " << fullPath << "\n";
                    }
                    check.close();
                    auto &sc = m_Registry.AddComponent(m_Selected->entity,
                                                       ScriptComponent(LuaState::GetLua(), fullPath));
                    sc.SetEntity(m_Selected->entity);
                    m_Selected->scriptPath = fullPath;
                    std::cout << "[INFO] [Inspector] Script assigned to entity: " << fullPath << "\n";
                } else if (m_ActiveField == EditField::TransformX || m_ActiveField == EditField::TransformY)
                {
                    try
                    {
                        float val = std::stof(m_ActiveInputText);
                        sf::Vector2f pos = m_Selected->shape.getPosition();
                        if (m_ActiveField == EditField::TransformX) pos.x = val;
                        else pos.y = val;
                        m_Selected->shape.setPosition(pos);
                    } catch (...) {}
                } else if (m_ActiveField == EditField::Rotation)
                {
                    try
                    {
                        float val = std::stof(m_ActiveInputText);
                        m_Selected->rotation = val;
                        m_Selected->shape.setRotation(val);
                        if (IsPolygonType(m_Selected->objectType))
                            m_Selected->circleShape.setRotation(val);
                        if (m_Selected->previewTexture)
                            m_Selected->previewSprite.setRotation(val);
                    } catch (...) {}
                } else if (m_ActiveField == EditField::ScaleX || m_ActiveField == EditField::ScaleY)
                {
                    try
                    {
                        float val = std::stof(m_ActiveInputText);
                        if (m_ActiveField == EditField::ScaleX) m_Selected->scaleX = val;
                        else m_Selected->scaleY = val;
                        m_Selected->shape.setScale(m_Selected->scaleX, m_Selected->scaleY);
                        if (IsPolygonType(m_Selected->objectType))
                            m_Selected->circleShape.setScale(m_Selected->scaleX, m_Selected->scaleY);
                        if (m_Selected->previewTexture)
                        {
                            const sf::Vector2u ts = m_Selected->previewTexture->getSize();
                            if (ts.x > 0 && ts.y > 0)
                            {
                                sf::Vector2f size = m_Selected->shape.getSize();
                                m_Selected->previewSprite.setScale((size.x / ts.x) * m_Selected->scaleX, (size.y / ts.y) * m_Selected->scaleY);
                            }
                        }
                    } catch (...) {}
                } else if (m_ActiveField == EditField::SizeW || m_ActiveField == EditField::SizeH)
                {
                    try
                    {
                        float val = std::max(4.f, std::stof(m_ActiveInputText));
                        sf::Vector2f size = m_Selected->shape.getSize();
                        if (m_ActiveField == EditField::SizeW) size.x = val;
                        else size.y = val;
                        m_Selected->shape.setSize(size);

                        if (IsPolygonType(m_Selected->objectType))
                            m_Selected->circleShape.setRadius(size.x / 2.f);

                        if (m_Selected->previewTexture)
                        {
                            const sf::Vector2u ts = m_Selected->previewTexture->getSize();
                            if (ts.x > 0 && ts.y > 0)
                                m_Selected->previewSprite.setScale((size.x / ts.x) * m_Selected->scaleX, (size.y / ts.y) * m_Selected->scaleY);
                        }

                        if (m_Selected->entity != 0 && m_Registry.HasComponent<RenderComponent>(m_Selected->entity))
                            m_Registry.GetComponent<RenderComponent>(m_Selected->entity).size = size;

                        if (!m_Selected->spritePath.empty() && m_Selected->entity != 0 &&
                            m_Registry.HasComponent<SpriteComponent>(m_Selected->entity))
                        {
                            m_Registry.GetComponent<SpriteComponent>(m_Selected->entity) =
                                    SpriteComponent(m_Selected->spritePath, size);
                        }
                    } catch (...) {}
                } else if (m_ActiveField == EditField::ColorR || m_ActiveField == EditField::ColorG ||
                           m_ActiveField == EditField::ColorB)
                {
                    try
                    {
                        int val = std::clamp(std::stoi(m_ActiveInputText), 0, 255);
                        if (m_ActiveField == EditField::ColorR) m_Selected->color.r = val;
                        else if (m_ActiveField == EditField::ColorG) m_Selected->color.g = val;
                        else m_Selected->color.b = val;
                        m_Selected->shape.setFillColor(m_Selected->color);
                    } catch (...) {}
                }
            }

            m_ActiveField = EditField::None;
            m_ActiveInputText.clear();
        } else if (event.text.unicode < 128)
        {
            char c = static_cast<char>(event.text.unicode);
            if (m_ActiveField == EditField::Name || m_ActiveField == EditField::Tag || m_ActiveField == EditField::Script || m_ActiveField ==
                EditField::UIText)
                m_ActiveInputText += c;
            else if (std::isdigit(c) || c == '-')
                m_ActiveInputText += c;
        }
        return;
    }

    if (event.type == sf::Event::Resized)
    {
        const float nW = static_cast<float>(event.size.width);
        const float nH = static_cast<float>(event.size.height);
        sf::FloatRect vp = m_camera.getViewport();
        vp.left = HierarchyWidth / nW;
        vp.top = TopBarHeight / nH;
        vp.width = 1.f - ((InspectorWidth + HierarchyWidth) / nW);
        vp.height = 1.f - (BrowserHeight / nH) - (TopBarHeight / nH);
        m_camera.setViewport(vp);
        UpdateBounds();
    }

    const bool inHierarchy = m_HierarchyBounds.contains(m_MouseScreenPos);

    if (event.type == sf::Event::MouseWheelScrolled)
    {
        if (inHierarchy)
        {
            m_HierarchyScrollY -= event.mouseWheelScroll.delta * m_ScrollSensitivity;
            if (m_HierarchyScrollY < 0.f) m_HierarchyScrollY = 0.f;
        } else if (!inTopBars && !inInspector && !inBrowser)
        {
            float delta = m_InvertPan ? -event.mouseWheelScroll.delta : event.mouseWheelScroll.delta;
            const float factor = delta > 0 ? (1.f - m_ZoomSensitivity) : (1.f + m_ZoomSensitivity);

            const float currentW = m_camera.getSize().x;
            const float newW = currentW * factor;
            constexpr float MinZoomSize = 80.f;
            constexpr float MaxZoomSize = 12000.f;

            if (newW >= MinZoomSize && newW <= MaxZoomSize)
                m_camera.zoom(factor);
        }
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        (event.mouseButton.button == sf::Mouse::Middle || event.mouseButton.button == sf::Mouse::Right))
    {
        if (event.mouseButton.button == sf::Mouse::Right && inHierarchy)
        {
            m_HierarchyContextMenuOpen = false;
            for (auto &[rect, obj]: m_HierarchyHitboxes)
            {
                if (rect.contains(m_MouseScreenPos))
                {
                    m_ContextObject = obj;
                    m_HierarchyContextMenuOpen = true;
                    m_ContextMenuPos = m_MouseScreenPos;
                    if (m_Selected) m_Selected->selected = false;
                    m_Selected = obj;
                    m_Selected->selected = true;
                    UpdateStatusText();
                    break;
                }
            }
        } else
        {
            m_panning = true;
            m_panStart = m_Window.mapPixelToCoords(sf::Mouse::getPosition(m_Window), m_camera);
        }
    }

    if (event.type == sf::Event::MouseButtonReleased &&
        (event.mouseButton.button == sf::Mouse::Middle || event.mouseButton.button == sf::Mouse::Right))
        m_panning = false;

    if (event.type == sf::Event::MouseButtonReleased &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        if (m_ShowSettings) { SaveSettings(); }

        if (m_Dragging && !m_SelectedObjects.empty())
        {
            auto macroCmd = std::make_shared<MacroCommand>();
            bool movedAny = false;
            
            for (auto* obj : m_SelectedObjects) {
                if (obj->entity != 0 && m_Registry.HasComponent<TransformComponent>(obj->entity)) {
                    auto &t = m_Registry.GetComponent<TransformComponent>(obj->entity);
                    t.x = obj->shape.getPosition().x;
                    t.y = obj->shape.getPosition().y;
                    
                    json after = SerializeObject(*obj);
                    json before = m_DragBeforeStates[obj->id];
                    if (before != after) {
                        macroCmd->commands.push_back(std::make_shared<ObjectStateCommand>(obj->id, before, after));
                        movedAny = true;
                    }
                }
            }
            if (movedAny) {
                m_UndoStack.push_back(macroCmd);
                m_RedoStack.clear();
            }
        }
        m_Dragging = false;
        
        if (m_BoxSelecting) {
            sf::Vector2f pos = MouseWorldPos();
            sf::Vector2f diff = pos - m_BoxSelectStart;
            if (std::abs(diff.x) < 2.f && std::abs(diff.y) < 2.f) {
                AddObject(pos, m_PlacementType);
            } else {
                sf::FloatRect selectRect(
                    std::min(m_BoxSelectStart.x, pos.x),
                    std::min(m_BoxSelectStart.y, pos.y),
                    std::abs(diff.x),
                    std::abs(diff.y)
                );
                
                bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl);
                bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift);
                if (!ctrl && !shift) ClearSelection();
                
                for (auto& obj : m_Objects) {
                    if (obj.shape.getGlobalBounds().intersects(selectRect)) {
                        SelectObject(&obj, true);
                    }
                }
            }
            m_BoxSelecting = false;
        }

        if (m_Resizing && m_Selected && m_Selected->entity != 0)
        {
            const sf::Vector2f newSize = m_Selected->shape.getSize();
            const sf::Vector2f newPos = m_Selected->shape.getPosition();

            if (m_Registry.HasComponent<TransformComponent>(m_Selected->entity))
            {
                auto &t = m_Registry.GetComponent<TransformComponent>(m_Selected->entity);
                t.x = newPos.x;
                t.y = newPos.y;
            }

            if (m_Registry.HasComponent<RenderComponent>(m_Selected->entity))
            {
                auto &rc = m_Registry.GetComponent<RenderComponent>(m_Selected->entity);
                rc.size = newSize;
            }

            if (m_Selected->previewTexture && m_Registry.HasComponent<SpriteComponent>(m_Selected->entity))
            {
                m_Registry.GetComponent<SpriteComponent>(m_Selected->entity) =
                        SpriteComponent(m_Selected->spritePath, newSize);
            }

            json after = SerializeObject(*m_Selected);
            json before = m_DragBeforeStates[m_Selected->id];
            if (before != after) {
                auto cmd = std::make_shared<ObjectStateCommand>(m_Selected->id, before, after);
                m_UndoStack.push_back(cmd);
                m_RedoStack.clear();
            }
        }
        m_Resizing = false;
        m_ResizeHandle = -1;

        if (m_Rotating && m_Selected && m_Selected->entity != 0)
        {
            if (m_Registry.HasComponent<TransformComponent>(m_Selected->entity))
            {
                auto &t = m_Registry.GetComponent<TransformComponent>(m_Selected->entity);
                t.rotation = m_Selected->rotation;
            }

            json after = SerializeObject(*m_Selected);
            json before = m_DragBeforeStates[m_Selected->id];
            if (before != after) {
                auto cmd = std::make_shared<ObjectStateCommand>(m_Selected->id, before, after);
                m_UndoStack.push_back(cmd);
                m_RedoStack.clear();
            }
        }
        m_Rotating = false;
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        if (m_HierarchyContextMenuOpen)
        {
            m_HierarchyContextMenuOpen = false;
            bool hitContext = false;
            for (auto &[rect, action]: m_ContextHitboxes)
            {
                if (rect.contains(m_MouseScreenPos))
                {
                    hitContext = true;
                    if (action == "rename" && m_ContextObject)
                    {
                        SelectObject(m_ContextObject, false);
                        m_ActiveField = EditField::Name;
                        m_ActiveInputText = m_ContextObject->id;
                    } else if (action == "delete" && m_ContextObject)
                    {
                        SelectObject(m_ContextObject, false);
                        DeleteSelected();
                    } else if (action == "duplicate" && m_ContextObject)
                    {
                        EditorObject newObj = *m_ContextObject;
                        newObj.id = NextId();
                        newObj.selected = false;
                        newObj.shape.setPosition(m_ContextObject->shape.getPosition() + sf::Vector2f(20.f, 20.f));
                        if (newObj.objectType == ObjectType::Circle)
                            newObj.circleShape.setPosition(
                                newObj.shape.getPosition());
                        if (newObj.entity != 0)
                        {
                            newObj.entity = m_Registry.CreateEntity();
                            m_Registry.AddComponent(newObj.entity, TransformComponent{
                                                        newObj.shape.getPosition().x, newObj.shape.getPosition().y
                                                    });
                            m_Registry.AddComponent(newObj.entity,
                                                    RenderComponent{newObj.color, newObj.shape.getSize(), MapToShapeType(newObj.objectType)});
                            if (!newObj.spritePath.empty()) ApplySpriteToObject(newObj, newObj.spritePath);
                        }
                        m_Objects.push_back(std::move(newObj));
                    }
                    break;
                }
            }
            if (hitContext) return;
        }
        if (m_OpenMenuIndex >= 0)
        {
            for (auto &[r, a]: m_MenuItemHitboxes)
            {
                if (r.contains(m_MouseScreenPos))
                {
                    HandleMenuAction(a);
                    m_OpenMenuIndex = -1;
                    return;
                }
            }
            bool onHeader = false;
            for (int i = 0; i < (int) m_Menus.size(); i++)
            {
                if (m_Menus[i].bounds.contains(m_MouseScreenPos))
                {
                    m_OpenMenuIndex = (m_OpenMenuIndex == i) ? -1 : i;
                    onHeader = true;
                    break;
                }
            }
            if (!onHeader) m_OpenMenuIndex = -1;
            return;
        }

        if (m_AddDropdownOpen)
        {
            for (auto &[r, a]: m_AddDropdownHitboxes)
            {
                if (r.contains(m_MouseScreenPos))
                {
                    if (a == "add_rect") m_PlacementType = ObjectType::Rectangle;
                    else if (a == "add_circle") m_PlacementType = ObjectType::Circle;
                    else if (a == "add_triangle") m_PlacementType = ObjectType::Triangle;
                    else if (a == "add_pentagon") m_PlacementType = ObjectType::Pentagon;
                    else if (a == "add_hexagon") m_PlacementType = ObjectType::Hexagon;
                    m_AddDropdownOpen = false;
                    return;
                }
            }
            if (!m_AddBtnBounds.contains(m_MouseScreenPos))
                m_AddDropdownOpen = false;
        }

        if (inMenuBar)
        {
            bool found = false;
            for (int i = 0; i < (int) m_Menus.size(); i++)
            {
                if (m_Menus[i].bounds.contains(m_MouseScreenPos))
                {
                    m_OpenMenuIndex = (m_OpenMenuIndex == i) ? -1 : i;
                    m_AddDropdownOpen = false;
                    found = true;
                    break;
                }
            }
            if (!found) m_OpenMenuIndex = -1;
            return;
        }

        if (inTopBars)
        {
            for (auto &[r, a]: m_ToolbarHitboxes)
            {
                if (r.contains(m_MouseScreenPos))
                {
                    if (a == "add_dropdown")
                    {
                        m_AddDropdownOpen = !m_AddDropdownOpen;
                        m_OpenMenuIndex = -1;
                    } else
                    {
                        HandleMenuAction(a);
                        m_AddDropdownOpen = false;
                    }
                    return;
                }
            }
            return;
        }

        if (inInspector)
        {
            HandleInspectorClick(m_MouseScreenPos);
            return;
        }

        if (inHierarchy)
        {
            for (auto &[rect, obj]: m_HierarchyHitboxes)
            {
                if (rect.contains(m_MouseScreenPos))
                {
                    if (m_Selected) m_Selected->selected = false;
                    m_Selected = obj;
                    m_Selected->selected = true;
                    UpdateStatusText();
                    return;
                }
            }
            return;
        }

        if (inBrowser) return;

        m_ActiveField = EditField::None;

        sf::Vector2f pos = MouseWorldPos();


        if (m_Selected)
        {
            const int handle = GetResizeHandle(pos);
            if (handle >= 0)
            {
                m_Resizing = true;
                m_DragBeforeStates.clear();
                m_DragBeforeStates[m_Selected->id] = SerializeObject(*m_Selected);
                m_ResizeHandle = handle;
                m_ResizeMouseStart = pos;
                m_ResizeObjOrigin = m_Selected->shape.getPosition();
                m_ResizeObjSize = m_Selected->shape.getSize();
                UpdateStatusText();
                return;
            }
            if (GetRotateHandle(pos))
            {
                m_Rotating = true;
                m_DragBeforeStates.clear();
                m_DragBeforeStates[m_Selected->id] = SerializeObject(*m_Selected);
                sf::Vector2f center = m_Selected->shape.getPosition() + m_Selected->shape.getSize() * 0.5f;
                m_RotateMouseAngleStart = std::atan2(pos.y - center.y, pos.x - center.x) * 180.f / 3.14159265f;
                m_RotateObjAngleStart = m_Selected->rotation;
                return;
            }
        }

        EditorObject *hit = ObjectAt(pos);

        bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl);
        bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift);

        if (hit)
        {
            if (ctrl || shift) {
                if (IsSelected(hit)) {
                    hit->selected = false;
                    std::erase(m_SelectedObjects, hit);
                    m_Selected = m_SelectedObjects.empty() ? nullptr : m_SelectedObjects.back();
                } else {
                    SelectObject(hit, true);
                }
            } else {
                if (!IsSelected(hit)) {
                    SelectObject(hit, false);
                }
            }

            m_Dragging = true;
            m_DragBeforeStates.clear();
            for (auto* obj : m_SelectedObjects) {
                m_DragBeforeStates[obj->id] = SerializeObject(*obj);
            }
            m_DragOffset = pos - hit->shape.getPosition();
            
        } else
        {
            SelectObject(nullptr, false);
            if (!ctrl && !shift) {
                m_BoxSelecting = true;
                m_BoxSelectStart = pos;
            }
        }


        UpdateStatusText();
    }

    if (event.type == sf::Event::KeyPressed)
    {
        const bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl);

        if (ctrl && event.key.code == sf::Keyboard::S) HandleMenuAction("save");
        if (ctrl && event.key.code == sf::Keyboard::L) HandleMenuAction("load");
        if (ctrl && event.key.code == sf::Keyboard::D) HandleMenuAction("duplicate");
        if (ctrl && event.key.code == sf::Keyboard::Z) UndoCommand();
        if (ctrl && event.key.code == sf::Keyboard::Y) RedoCommand();
        if (ctrl && event.key.code == sf::Keyboard::Comma) HandleMenuAction("settings");
        if (event.key.code == sf::Keyboard::G) HandleMenuAction("toggle_grid");

        if (event.key.code == sf::Keyboard::Delete)
        {
            DeleteSelected();
            UpdateStatusText();
        }

        if (event.key.code == sf::Keyboard::Escape)
        {
            if (m_OpenMenuIndex >= 0 || m_AddDropdownOpen)
            {
                m_OpenMenuIndex = -1;
                m_AddDropdownOpen = false;
                return;
            }
            if (m_ActiveField != EditField::None)
            {
                m_ActiveField = EditField::None;
                m_ActiveInputText.clear();
                return;
            }
            if (m_Selected) m_Selected->selected = false;
            m_Selected = nullptr;

            UpdateStatusText();
        }

        if (event.key.code == sf::Keyboard::F5)
        {
            SyncToRegistry();
            SnapshotState();
            m_manager.SwitchSceneTo("game");
        }
    }
}

void EditorScene::Update(float deltaTime)
{
    if (m_SaveFeedbackTimer > 0.f)
        m_SaveFeedbackTimer -= deltaTime;

    m_FrameCount++;
    float elapsed = m_FPSClock.getElapsedTime().asSeconds();
    if (elapsed >= 0.5f)
    {
        m_FPS = static_cast<float>(m_FrameCount) / elapsed;
        m_FrameCount = 0;
        m_FPSClock.restart();

        std::string sceneName = m_SceneSavePath;
        const size_t slash = sceneName.find_last_of("/\\");
        if (slash != std::string::npos) sceneName = sceneName.substr(slash + 1);
        const size_t dot = sceneName.rfind('.');
        if (dot != std::string::npos) sceneName = sceneName.substr(0, dot);

        std::string title =
                (g_App ? g_App->GetProjectName() : std::string(Rayne::DEFAULT_PROJECT_NAME))
                + ": " + sceneName
                + " (" + Rayne::PlatformString() + ")"
                + " - RayneEngine " + Rayne::VersionString();
        if (m_ShowAutoSaveInTitle)
        {
            int remaining = static_cast<int>(m_AutoSaveIntervalSeconds - m_AutoSaveTimer);
            title += "  |  AutoSave in " + std::to_string(remaining) + "s";
        }
        m_Window.setTitle(title);
    }

    if (m_AutoSaveEnabled)
    {
        if (m_ShowAutoSavePopup)
        {
            m_AutoSavePopupTimer -= deltaTime;
            if (m_AutoSavePopupTimer <= 0.f)
            {
                SaveToJson(std::string(ASSET_PATH) + "/" + m_SceneSavePath);
                std::cout << "[INFO] [EditorScene] AutoSaved scene\n";
                m_ShowAutoSavePopup = false;
            }
        } else
        {
            m_AutoSaveTimer += deltaTime;
            if (m_AutoSaveTimer >= m_AutoSaveIntervalSeconds)
            {
                if (m_AutoSavePopupEnabled)
                {
                    m_ShowAutoSavePopup = true;
                    m_AutoSavePopupTimer = m_AutoSavePopupDuration;
                } else
                {
                    SaveToJson(std::string(ASSET_PATH) + "/" + m_SceneSavePath);
                    std::cout << "[INFO] [EditorScene] AutoSaved scene (silent)\n";
                }
                m_AutoSaveTimer = 0.f;
            }
        }
    } else
    {
        m_ShowAutoSavePopup = false;
        m_AutoSaveTimer = 0.f;
    }
}

void EditorScene::Render(sf::RenderWindow &window)
{
    window.setView(m_camera);
    DrawGrid();

    for (auto &obj: m_Objects)
    {
        if (IsPolygonType(obj.objectType))
        {
            obj.circleShape.setPointCount(GetPolygonPointCount(obj.objectType));
            obj.circleShape.setPosition(obj.shape.getPosition());
            obj.circleShape.setRadius(obj.shape.getSize().x / 2.f);
            obj.circleShape.setFillColor(obj.color);
            obj.circleShape.setOutlineColor(obj.selected ? m_SelectionOutlineColor : sf::Color::Transparent);
            obj.circleShape.setOutlineThickness(obj.selected ? m_SelectionOutlineThickness : 0.f);
            window.draw(obj.circleShape);
        } else
        {
            obj.shape.setOutlineColor(obj.selected ? m_SelectionOutlineColor : sf::Color::Transparent);
            obj.shape.setOutlineThickness(obj.selected ? m_SelectionOutlineThickness : 0.f);
            window.draw(obj.shape);
        }

        if (m_ShowColliderOutlines)
        {
            sf::RectangleShape colBox(obj.shape.getSize());
            colBox.setPosition(obj.shape.getPosition());
            colBox.setFillColor(sf::Color::Transparent);
            colBox.setOutlineColor(sf::Color(C_DANGER.r, C_DANGER.g, C_DANGER.b, 160));
            colBox.setOutlineThickness(1.f);
            window.draw(colBox);
        }

        if (m_ShowEntityIDs && obj.entity != 0)
        {
            sf::Text idText;
            idText.setFont(*m_Font);
            idText.setCharacterSize(9);
            idText.setFillColor(sf::Color(255, 200, 60, 200));
            idText.setString("#" + std::to_string(obj.entity));
            idText.setPosition(obj.shape.getPosition() + sf::Vector2f(2.f, 2.f));
            window.draw(idText);
        }
    }

    for (auto &obj: m_Objects)
    {
        if (obj.previewTexture)
        {
            obj.previewSprite.setTexture(*obj.previewTexture);
            obj.previewSprite.setPosition(obj.shape.getPosition());
            window.draw(obj.previewSprite);
        }
    }

    bool canPlace = true;
    if (m_Dragging || m_Resizing || m_Rotating || m_BoxSelecting || m_panning) canPlace = false;
    
    if (canPlace)
    {
        if (m_MouseScreenPos.y < TopBarHeight) canPlace = false;
        else if (m_BrowserBounds.contains(m_MouseScreenPos)) canPlace = false;
        else if (m_InspectorBounds.contains(m_MouseScreenPos)) canPlace = false;
        else if (m_HierarchyBounds.contains(m_MouseScreenPos)) canPlace = false;
        else if (m_TabBrowserBounds.contains(m_MouseScreenPos)) canPlace = false;
        else if (m_TabConsoleBounds.contains(m_MouseScreenPos)) canPlace = false;
    }
    
    if (canPlace)
    {
        sf::Vector2f wPos = MouseWorldPos();
        if (m_Selected && GetResizeHandle(wPos) >= 0) canPlace = false;
        else if (m_Selected && GetRotateHandle(wPos)) canPlace = false;
        else if (ObjectAt(wPos) != nullptr) canPlace = false;
    }

    if (canPlace)
    {
        if (IsPolygonType(m_PlacementType))
            window.draw(m_CirclePreview);
        else
            window.draw(m_Preview);
    }

    if (m_BoxSelecting) {
        sf::Vector2f pos = MouseWorldPos();
        sf::RectangleShape box;
        box.setPosition(std::min(m_BoxSelectStart.x, pos.x), std::min(m_BoxSelectStart.y, pos.y));
        box.setSize({std::abs(pos.x - m_BoxSelectStart.x), std::abs(pos.y - m_BoxSelectStart.y)});
        box.setFillColor(sf::Color(m_SelectionOutlineColor.r, m_SelectionOutlineColor.g, m_SelectionOutlineColor.b, 50));
        box.setOutlineColor(m_SelectionOutlineColor);
        box.setOutlineThickness(1.f);
        window.draw(box);
    }

    DrawGizmos(window);

    const sf::View uiView(sf::FloatRect(
        0.f, 0.f,
        static_cast<float>(m_Window.getSize().x),
        static_cast<float>(m_Window.getSize().y)));
    window.setView(uiView);
    UpdateBounds();

    auto drawTab = [&](const std::string& label, const sf::FloatRect& bounds, bool active) {
        sf::RectangleShape tabRect({bounds.width, bounds.height});
        tabRect.setPosition(bounds.left, bounds.top);
        tabRect.setFillColor(active ? C_BG_PANEL : C_BG_ELEVATED);
        window.draw(tabRect);
        
        sf::Text tabText;
        tabText.setFont(*m_Font);
        tabText.setCharacterSize(12);
        tabText.setFillColor(active ? C_TEXT_PRIMARY : C_TEXT_MUTED);
        tabText.setString(label);
        tabText.setPosition(bounds.left + 10.f, bounds.top + 4.f);
        window.draw(tabText);
    };

    drawTab("Files", m_TabBrowserBounds, m_ActiveBottomPanelTab == BottomPanelTab::ContentBrowser);
    drawTab("Console", m_TabConsoleBounds, m_ActiveBottomPanelTab == BottomPanelTab::Console);

    if (m_ActiveBottomPanelTab == BottomPanelTab::ContentBrowser) {
        m_ContentBrowser->Render(window,
                                 m_BrowserBounds.left, m_BrowserBounds.top,
                                 m_BrowserBounds.width, m_BrowserBounds.height);
    } else {
        m_ConsolePanel->Render(window,
                               m_BrowserBounds.left, m_BrowserBounds.top,
                               m_BrowserBounds.width, m_BrowserBounds.height);
    }

    DrawInspector(window);
    DrawHierarchy(window); {
        sf::Text info;
        info.setFont(*m_Font);
        info.setCharacterSize(10);
        info.setFillColor(C_TEXT_MUTED);
        const sf::Vector2f wp = m_Window.mapPixelToCoords(sf::Mouse::getPosition(m_Window), m_camera);
        info.setString("x:" + std::to_string((int) wp.x) + " y:" + std::to_string((int) wp.y));
        info.setPosition(8.f, m_BrowserBounds.top - 16.f);
        window.draw(info);
    }

    m_StatusText.setPosition(8.f, m_BrowserBounds.top - 30.f);
    window.draw(m_StatusText);

    DrawToolbar(window);
    DrawMenuBar(window);

    if (m_ShowFPS)
    {
        sf::Text fpsText;
        fpsText.setFont(*m_Font);
        fpsText.setCharacterSize(11);
        fpsText.setFillColor(sf::Color(80, 255, 120, 220));
        fpsText.setString("FPS: " + std::to_string(static_cast<int>(m_FPS)));
        fpsText.setPosition(m_Window.getSize().x - InspectorWidth - 60.f, TopBarHeight + 6.f);
        window.draw(fpsText);
    }

    if (m_ShowAutoSavePopup && m_AutoSavePopupEnabled)
    {
        sf::Text asText;
        asText.setFont(*m_Font);
        asText.setCharacterSize(14);
        asText.setFillColor(C_TEXT_PRIMARY);
        int secondsLeft = static_cast<int>(m_AutoSavePopupTimer + 0.99f);
        asText.setString("AutoSave in " + std::to_string(secondsLeft) + " seconds...");

        float tw = asText.getLocalBounds().width;
        float th = asText.getLocalBounds().height;
        float pW = tw + 40.f;
        float pH = 40.f;
        float pX = (m_Window.getSize().x - pW) / 2.f;
        float pY = m_Window.getSize().y - 100.f;

        sf::RectangleShape asBg({pW, pH});
        asBg.setFillColor(C_BG_ELEVATED);
        asBg.setOutlineColor(C_BORDER_LIGHT);
        asBg.setOutlineThickness(1.f);
        asBg.setPosition(pX, pY);

        sf::RectangleShape warningBar({4.f, pH});
        warningBar.setFillColor(C_WARNING);
        warningBar.setPosition(pX, pY);

        asText.setPosition(pX + 20.f, pY + (pH - th) / 2.f - 4.f);

        window.draw(asBg);
        window.draw(warningBar);
        window.draw(asText);
    }

    if (m_SaveFeedbackTimer > 0.f)
    {
        sf::Text asText;
        asText.setFont(*m_Font);
        asText.setCharacterSize(14);
        asText.setFillColor(C_TEXT_PRIMARY);
        asText.setString("Scene Saved successfully!");

        float tw = asText.getLocalBounds().width;
        float th = asText.getLocalBounds().height;
        float pW = tw + 40.f;
        float pH = 40.f;
        float pX = (m_Window.getSize().x - pW) / 2.f;
        float pY = m_Window.getSize().y - 100.f;

        sf::RectangleShape asBg({pW, pH});
        asBg.setFillColor(C_BG_ELEVATED);
        asBg.setOutlineColor(C_BORDER_LIGHT);
        asBg.setOutlineThickness(1.f);
        asBg.setPosition(pX, pY);

        sf::RectangleShape successBar({4.f, pH});
        successBar.setFillColor(C_SUCCESS);
        successBar.setPosition(pX, pY);

        asText.setPosition(pX + 20.f, pY + (pH - th) / 2.f - 4.f);

        float alpha = std::clamp(m_SaveFeedbackTimer / 0.5f, 0.f, 1.f) * 255.f;
        asBg.setFillColor(sf::Color(C_BG_ELEVATED.r, C_BG_ELEVATED.g, C_BG_ELEVATED.b, alpha));
        asBg.setOutlineColor(sf::Color(C_BORDER_LIGHT.r, C_BORDER_LIGHT.g, C_BORDER_LIGHT.b, alpha));
        successBar.setFillColor(sf::Color(C_SUCCESS.r, C_SUCCESS.g, C_SUCCESS.b, alpha));
        asText.setFillColor(sf::Color(C_TEXT_PRIMARY.r, C_TEXT_PRIMARY.g, C_TEXT_PRIMARY.b, alpha));

        window.draw(asBg);
        window.draw(successBar);
        window.draw(asText);
    }

    if (m_AddDropdownOpen) DrawAddDropdown(window);

    if (m_ShowSettings) DrawSettingsWindow(window);
    if (m_ShowProjectSettings) DrawProjectSettingsWindow(window);
    if (m_ShowBuildPopup) DrawBuildPopup(window);

    if (m_ContentBrowser->HasDraggedAsset() &&
        m_ContentBrowser->GetDraggedAsset().type == AssetType::Image)
    {
        const bool overInspector = m_InspectorBounds.contains(m_MouseScreenPos);
        const bool overHierarchy = m_HierarchyBounds.contains(m_MouseScreenPos);

        if (overInspector && m_Selected)
        {
            sf::RectangleShape glow(sf::Vector2f(m_InspectorBounds.width, m_InspectorBounds.height));
            glow.setPosition(m_InspectorBounds.left, m_InspectorBounds.top);
            glow.setFillColor(sf::Color(C_ACCENT.r, C_ACCENT.g, C_ACCENT.b, 18));
            glow.setOutlineColor(sf::Color(C_ACCENT.r, C_ACCENT.g, C_ACCENT.b, 160));
            glow.setOutlineThickness(2.f);
            window.draw(glow);

            sf::Text dropHint;
            dropHint.setFont(*m_Font);
            dropHint.setCharacterSize(11);
            dropHint.setFillColor(sf::Color(C_ACCENT_BRIGHT.r, C_ACCENT_BRIGHT.g, C_ACCENT_BRIGHT.b, 230));
            dropHint.setString("Drop to set Sprite");
            const float hw = dropHint.getLocalBounds().width;
            dropHint.setPosition(
                m_InspectorBounds.left + (m_InspectorBounds.width - hw) / 2.f,
                m_InspectorBounds.top + 6.f);
            window.draw(dropHint);
        } else if (overHierarchy && m_Selected)
        {
            sf::RectangleShape glow(sf::Vector2f(m_HierarchyBounds.width, m_HierarchyBounds.height));
            glow.setPosition(m_HierarchyBounds.left, m_HierarchyBounds.top);
            glow.setFillColor(sf::Color(C_ACCENT.r, C_ACCENT.g, C_ACCENT.b, 18));
            glow.setOutlineColor(sf::Color(C_ACCENT.r, C_ACCENT.g, C_ACCENT.b, 140));
            glow.setOutlineThickness(2.f);
            window.draw(glow);
        }
    }

    m_ContentBrowser->RenderDragGhost(window);
}

static void DrawPill(sf::RenderWindow &window, sf::FloatRect r, sf::Color fill, sf::Color outline)
{
    const float radius = 4.f;
    const int cornerPoints = 8;
    sf::ConvexShape shape;
    shape.setPointCount(cornerPoints * 4);

    const float PI = 3.141592654f;
    auto addArc = [&](int startIndex, float cx, float cy, float startAngle, float endAngle) {
        for (int i = 0; i < cornerPoints; ++i)
        {
            float t = static_cast<float>(i) / (cornerPoints - 1);
            float angle = startAngle + (endAngle - startAngle) * t;
            shape.setPoint(startIndex + i, sf::Vector2f(cx + std::cos(angle) * radius, cy + std::sin(angle) * radius));
        }
    };

    addArc(0, r.left + radius, r.top + radius, PI, PI * 1.5f);
    addArc(cornerPoints, r.left + r.width - radius, r.top + radius, PI * 1.5f, PI * 2.f);
    addArc(cornerPoints * 2, r.left + r.width - radius, r.top + r.height - radius, 0.f, PI * 0.5f);
    addArc(cornerPoints * 3, r.left + radius, r.top + r.height - radius, PI * 0.5f, PI);

    if (fill.a > 0)
    {
        sf::ConvexShape shadow = shape;
        shadow.setFillColor(sf::Color(0, 0, 0, 60));
        shadow.move(0.f, 2.f);
        window.draw(shadow);
    }

    shape.setFillColor(fill);
    shape.setOutlineColor(outline);
    shape.setOutlineThickness(1.f);
    window.draw(shape);

    if (fill.a > 0)
    {
        sf::ConvexShape highlight = shape;
        highlight.setFillColor(sf::Color::Transparent);
        highlight.setOutlineColor(sf::Color(255, 255, 255, 25));
        highlight.setOutlineThickness(1.f);
        highlight.move(0.f, -1.f);
        window.draw(highlight);
    }
}

void EditorScene::DrawMenuBar(sf::RenderWindow &window)
{
    const float w = static_cast<float>(window.getSize().x);

    sf::RectangleShape bg({w, MenuBarHeight});
    bg.setFillColor(C_BG_PANEL);
    bg.setPosition(0.f, 0.f);
    window.draw(bg);

    sf::RectangleShape border({w, 1.f});
    border.setFillColor(C_BORDER);
    border.setPosition(0.f, MenuBarHeight - 1.f);
    window.draw(border);

    m_MenuItemHitboxes.clear();

    float x = 6.f;
    for (int i = 0; i < (int) m_Menus.size(); i++)
    {
        auto &menu = m_Menus[i];
        const bool open = (m_OpenMenuIndex == i);
        const bool hovered = menu.bounds.contains(m_MouseScreenPos) || open;

        sf::Text lbl;
        lbl.setFont(*m_Font);
        lbl.setCharacterSize(12);
        lbl.setString(menu.label);

        const float itemW = lbl.getLocalBounds().width + 22.f;
        menu.bounds = sf::FloatRect(x, 0.f, itemW, MenuBarHeight);

        if (hovered)
        {
            sf::RectangleShape hbg({itemW - 2.f, MenuBarHeight - 6.f});
            hbg.setFillColor(open ? C_ACCENT_DIM : C_BG_ELEVATED);
            hbg.setPosition(x + 1.f, 3.f);
            window.draw(hbg);
        }

        lbl.setFillColor(hovered ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
        lbl.setPosition(x + 10.f, 8.f);
        window.draw(lbl);

        if (open)
        {
            float maxLabelW = 0.f;
            float maxShortW = 0.f;
            for (auto &item: menu.items)
            {
                if (item.isSeparator) continue;
                sf::Text tmp;
                tmp.setFont(*m_Font);
                tmp.setCharacterSize(12);
                tmp.setString(item.label);
                maxLabelW = std::max(maxLabelW, tmp.getLocalBounds().width);
                if (!item.shortcut.empty())
                {
                    tmp.setCharacterSize(11);
                    tmp.setString(item.shortcut);
                    maxShortW = std::max(maxShortW, tmp.getLocalBounds().width);
                }
            }

            const float dropW = std::max(maxLabelW + maxShortW + 52.f, 190.f);
            const float dropX = x;
            const float dropY = MenuBarHeight;

            float dropH = 8.f;
            for (auto &item: menu.items)
                dropH += item.isSeparator ? 8.f : 28.f;

            sf::RectangleShape dbg({dropW, dropH});
            dbg.setFillColor(C_BG_ELEVATED);
            dbg.setOutlineColor(C_BORDER_LIGHT);
            dbg.setOutlineThickness(1.f);
            dbg.setPosition(dropX, dropY);
            window.draw(dbg);

            float iy = dropY + 4.f;
            for (auto &item: menu.items)
            {
                if (item.isSeparator)
                {
                    sf::RectangleShape sep({dropW - 12.f, 1.f});
                    sep.setFillColor(C_BORDER);
                    sep.setPosition(dropX + 6.f, iy + 3.f);
                    window.draw(sep);
                    iy += 8.f;
                    continue;
                }

                const sf::FloatRect ir(dropX, iy, dropW, 28.f);
                const bool ih = ir.contains(m_MouseScreenPos);

                if (ih)
                {
                    sf::RectangleShape ibg({dropW - 6.f, 26.f});
                    ibg.setFillColor(C_ACCENT_DIM);
                    ibg.setPosition(dropX + 3.f, iy + 1.f);
                    window.draw(ibg);
                }

                sf::Text il;
                il.setFont(*m_Font);
                il.setCharacterSize(12);
                il.setFillColor(ih ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
                il.setString(item.label);
                il.setPosition(dropX + 14.f, iy + 7.f);
                window.draw(il);

                if (!item.shortcut.empty())
                {
                    sf::Text sl;
                    sl.setFont(*m_Font);
                    sl.setCharacterSize(10);
                    sl.setFillColor(C_TEXT_MUTED);
                    sl.setString(item.shortcut);
                    sl.setPosition(dropX + dropW - sl.getLocalBounds().width - 10.f, iy + 8.f);
                    window.draw(sl);
                }

                m_MenuItemHitboxes.push_back({ir, item.action});
                iy += 28.f;
            }
        }

        x += itemW;
    }

    sf::Text watermark;
    watermark.setFont(*m_Font);
    watermark.setCharacterSize(11);
    watermark.setFillColor(C_TEXT_MUTED);
    watermark.setString("RayneEngine");
    watermark.setPosition(w - watermark.getLocalBounds().width - 12.f, 9.f);
    window.draw(watermark);
}

void EditorScene::DrawToolbar(sf::RenderWindow &window)
{
    const float w = static_cast<float>(window.getSize().x);
    const float ty = MenuBarHeight;

    sf::RectangleShape bg({w, ToolbarHeight});
    bg.setFillColor(C_BG_PANEL);
    bg.setPosition(0.f, ty);
    window.draw(bg);

    sf::RectangleShape border({w, 1.f});
    border.setFillColor(C_BORDER);
    border.setPosition(0.f, ty + ToolbarHeight - 1.f);
    window.draw(border);

    m_ToolbarHitboxes.clear();

    const auto drawSep = [&](float sx) {
        sf::RectangleShape s({1.f, ToolbarHeight - 16.f});
        s.setFillColor(C_BORDER);
        s.setPosition(sx, ty + 8.f);
        window.draw(s);
    };

    const auto drawBtn = [&](sf::FloatRect r, const std::string &label, bool active,
                             sf::Color accentFill = C_ACCENT, sf::Color accentBorder = C_ACCENT) {
        const bool hov = r.contains(m_MouseScreenPos);
        sf::Color fill = active
                             ? C_ACCENT_DIM
                             : hov
                                   ? C_BG_ELEVATED
                                   : sf::Color(0, 0, 0, 0);
        sf::Color bdr = active
                            ? sf::Color(accentBorder.r, accentBorder.g, accentBorder.b, 200)
                            : hov
                                  ? C_BORDER_LIGHT
                                  : sf::Color(0, 0, 0, 0);
        DrawPill(window, r, fill, bdr);

        sf::Text t;
        t.setFont(*m_Font);
        t.setCharacterSize(12);
        t.setFillColor(active
                           ? sf::Color(accentBorder.r, accentBorder.g, accentBorder.b, 255)
                           : hov
                                 ? C_TEXT_PRIMARY
                                 : C_TEXT_SECONDARY);
        t.setString(label);
        t.setPosition(r.left + (r.width - t.getLocalBounds().width) / 2.f,
                      r.top + (r.height - t.getLocalBounds().height) / 2.f - 2.f);
        window.draw(t);
    };

    float cx = 10.f; {
        const sf::FloatRect ar(cx, ty + 4.f, 86.f, ToolbarHeight - 8.f);
        const bool hov = ar.contains(m_MouseScreenPos);
        const bool act = m_AddDropdownOpen;

        sf::Color fill = act ? C_ACCENT_DIM : hov ? C_BG_ELEVATED : C_BG_ELEVATED;
        sf::Color bdr = act ? C_ACCENT : hov ? C_BORDER_LIGHT : C_BORDER;
        DrawPill(window, ar, fill, bdr);

        sf::Text at;
        at.setFont(*m_Font);
        at.setCharacterSize(12);
        at.setFillColor(act ? C_ACCENT_BRIGHT : hov ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
        at.setString("+ Add");
        at.setPosition(ar.left + (ar.width - at.getLocalBounds().width) / 2.f,
                       ar.top + (ar.height - at.getLocalBounds().height) / 2.f - 2.f);
        window.draw(at);

        m_AddBtnBounds = ar;
        m_ToolbarHitboxes.push_back({ar, "add_dropdown"});
        cx += ar.width + 8.f;
    }

    drawSep(cx);
    cx += 12.f; {
        const sf::FloatRect dr(cx, ty + 4.f, 56.f, ToolbarHeight - 8.f);
        drawBtn(dr, "Delete", false, C_DANGER, C_DANGER);
        m_ToolbarHitboxes.push_back({dr, "delete"});
        cx += dr.width + 8.f;
    }

    drawSep(cx);
    cx += 12.f; {
        const sf::FloatRect ur(cx, ty + 4.f, 76.f, ToolbarHeight - 8.f);
        drawBtn(ur, "UI Editor", false, C_ACCENT, C_ACCENT);
        m_ToolbarHitboxes.push_back({ur, "open_ui_editor"});
        cx += ur.width + 8.f;
    }

    drawSep(cx);
    cx += 12.f; {
        const sf::FloatRect gr(cx, ty + 4.f, 70.f, ToolbarHeight - 8.f);
        drawBtn(gr, m_SnapToGrid ? "Grid ON" : "Grid", m_SnapToGrid);
        m_ToolbarHitboxes.push_back({gr, "toggle_grid"});
    } {
        const float runW = 92.f;
        const float runX = w - InspectorWidth - runW - 10.f;
        const sf::FloatRect rr(runX, ty + 4.f, runW, ToolbarHeight - 8.f);
        const bool hov = rr.contains(m_MouseScreenPos);

        sf::Color fill = hov
                             ? sf::Color(C_ACCENT_ACT.r, C_ACCENT_ACT.g, C_ACCENT_ACT.b, 240)
                             : C_ACCENT_DIM;
        sf::Color runBdr = hov ? C_ACCENT_HOV : C_ACCENT;
        DrawPill(window, rr, fill, runBdr);

        sf::Text rt;
        rt.setFont(*m_Font);
        rt.setCharacterSize(12);
        rt.setFillColor(hov ? C_TEXT_PRIMARY : C_ACCENT_BRIGHT);
        rt.setString("Run  F5");
        rt.setPosition(rr.left + (rr.width - rt.getLocalBounds().width) / 2.f,
                       rr.top + (rr.height - rt.getLocalBounds().height) / 2.f - 2.f);
        window.draw(rt);

        drawSep(runX - 8.f);
        m_ToolbarHitboxes.push_back({rr, "run"});
    }
}

void EditorScene::DrawAddDropdown(sf::RenderWindow &window)
{
    m_AddDropdownHitboxes.clear();

    const float dropX = m_AddBtnBounds.left;
    const float dropY = TopBarHeight;
    const float dropW = 180.f;
    const float itemH = 38.f;
    const float headerH = 24.f;

    struct DropItem
    {
        std::string label;
        std::string action;
        std::string desc;
        bool isHeader = false;
    };

    std::vector<DropItem> items = {
        {"Primitives", "", "", true},
        {"Rectangle", "add_rect", "Rectangle primitive", false},
        {"Circle", "add_circle", "Circle primitive", false},
        {"Triangle", "add_triangle", "Triangle primitive", false},
        {"Pentagon", "add_pentagon", "Pentagon primitive", false},
        {"Hexagon", "add_hexagon", "Hexagon primitive", false}
    };

    float dropH = 12.f;
    for (const auto &item: items) { dropH += item.isHeader ? headerH : itemH; }

    sf::RectangleShape bg({dropW, dropH});
    bg.setFillColor(C_BG_ELEVATED);
    bg.setOutlineColor(C_BORDER_LIGHT);
    bg.setOutlineThickness(1.f);
    bg.setPosition(dropX, dropY);
    window.draw(bg);

    float iy = dropY + 6.f;
    for (auto &item: items)
    {
        if (item.isHeader)
        {
            sf::Text ht;
            ht.setFont(*m_Font);
            ht.setCharacterSize(11);
            ht.setFillColor(C_TEXT_SECONDARY);
            ht.setString(item.label);
            ht.setPosition(dropX + 8.f, iy + 6.f);
            window.draw(ht);
            iy += headerH;
            continue;
        }

        const sf::FloatRect ir(dropX, iy, dropW, itemH);
        const bool hov = ir.contains(m_MouseScreenPos);
        const bool cur = (item.action == "add_rect" && m_PlacementType == ObjectType::Rectangle) ||
                         (item.action == "add_circle" && m_PlacementType == ObjectType::Circle) ||
                         (item.action == "add_triangle" && m_PlacementType == ObjectType::Triangle) ||
                         (item.action == "add_pentagon" && m_PlacementType == ObjectType::Pentagon) ||
                         (item.action == "add_hexagon" && m_PlacementType == ObjectType::Hexagon);

        if (hov)
        {
            sf::RectangleShape ibg({dropW - 8.f, itemH - 4.f});
            ibg.setFillColor(C_ACCENT_DIM);
            ibg.setPosition(dropX + 4.f, iy + 2.f);
            window.draw(ibg);
        }

        if (cur)
        {
            sf::RectangleShape accent({3.f, itemH - 14.f});
            accent.setFillColor(C_ACCENT);
            accent.setPosition(dropX + 6.f, iy + 7.f);
            window.draw(accent);
        }

        sf::Text lt;
        lt.setFont(*m_Font);
        lt.setCharacterSize(12);
        lt.setFillColor((hov || cur) ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
        lt.setString(item.label);
        lt.setPosition(dropX + 16.f, iy + 6.f);
        window.draw(lt);

        sf::Text dt;
        dt.setFont(*m_Font);
        dt.setCharacterSize(10);
        dt.setFillColor(C_TEXT_MUTED);
        dt.setString(item.desc);
        dt.setPosition(dropX + 16.f, iy + 21.f);
        window.draw(dt);

        m_AddDropdownHitboxes.push_back({ir, item.action});
        iy += itemH;
    }
}

void EditorScene::DrawInspector(sf::RenderWindow &window)
{
    m_InspectorButtons.clear();

    const float panelX = m_InspectorBounds.left;
    const float panelY = m_InspectorBounds.top;
    const float panelH = m_InspectorBounds.height;

    m_InspectorPanel.setPosition(panelX, panelY);
    m_InspectorPanel.setSize({InspectorWidth, panelH});
    window.draw(m_InspectorPanel);

    sf::RectangleShape leftBorder({1.f, panelH});
    leftBorder.setFillColor(C_BORDER);
    leftBorder.setPosition(panelX, panelY);
    window.draw(leftBorder); {
        sf::RectangleShape header({InspectorWidth, 36.f});
        header.setPosition(panelX, panelY);
        header.setFillColor(C_BG_ELEVATED);
        window.draw(header);

        sf::RectangleShape headerLine({InspectorWidth, 1.f});
        headerLine.setFillColor(C_BORDER);
        headerLine.setPosition(panelX, panelY + 35.f);
        window.draw(headerLine);

        sf::Text title;
        title.setFont(*m_Font);
        title.setCharacterSize(11);
        title.setFillColor(C_TEXT_SECONDARY);
        title.setStyle(sf::Text::Bold);
        title.setString("INSPECTOR");
        title.setPosition(panelX + InspectorPad + 2.f, panelY + 12.f);
        window.draw(title);
    }


    if (!m_Selected)
    {
        std::string selPath = m_ContentBrowser->GetSelectedPath();
        if (!selPath.empty())
        {
            float y = panelY + 42.f;
            y = DrawSectionHeader(window, "FILE PROPERTIES", sf::Color(180, 180, 220), panelX, y);

            std::error_code ec;
            std::filesystem::path p(selPath);

            if (std::filesystem::exists(p, ec))
            {
                std::string filename = p.filename().string();
                std::string ext = p.extension().string();
                std::string typeName = "Unknown File";

                std::string lowerExt = ext;
                std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::tolower);

                if (lowerExt == ".lua") typeName = "Lua Script";
                else if (lowerExt == ".json") typeName = "JSON Data";
                else if (lowerExt == ".png" || lowerExt == ".jpg" || lowerExt == ".jpeg") typeName = "Image Asset";
                else if (lowerExt == ".wav" || lowerExt == ".ogg") typeName = "Audio Asset";
                else if (lowerExt == ".ttf") typeName = "Font Asset";
                else if (std::filesystem::is_directory(p, ec)) typeName = "Directory";

                uintmax_t size = std::filesystem::file_size(p, ec);
                std::string sizeStr = "-";
                if (!ec)
                {
                    if (size < 1024) sizeStr = std::to_string(size) + " B";
                    else if (size < 1024 * 1024) sizeStr = std::to_string(size / 1024) + " KB";
                    else sizeStr = std::to_string(size / (1024 * 1024)) + " MB";
                }

                auto ftime = std::filesystem::last_write_time(p, ec);
                std::string timeStr = "Unknown";
                if (!ec)
                {
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
                    std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
                    char timeBuf[64];
                    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&cftime));
                    timeStr = timeBuf;
                }

                y = DrawRow(window, "Name", filename, panelX, y);
                y = DrawRow(window, "Type", typeName, panelX, y);
                if (typeName != "Directory") { y = DrawRow(window, "Size", sizeStr, panelX, y); }
                y = DrawRow(window, "Modified", timeStr, panelX, y);

                if (typeName == "Image Asset")
                {
                    y += 10.f;
                    y = DrawSectionHeader(window, "PREVIEW", sf::Color(220, 150, 220), panelX, y);

                    auto tex = ResourceManager::Get().GetTexture(selPath);
                    if (tex)
                    {
                        const sf::Vector2u ts = tex->getSize();
                        y = DrawRow(window, "Dimensions", std::to_string(ts.x) + " x " + std::to_string(ts.y), panelX,
                                    y);

                        float previewW = InspectorWidth - 20.f;
                        float previewH = previewW * ((float) ts.y / (float) ts.x);
                        if (previewH > 200.f)
                        {
                            previewH = 200.f;
                            previewW = previewH * ((float) ts.x / (float) ts.y);
                        }

                        sf::Sprite sprite(*tex);
                        sprite.setScale(previewW / ts.x, previewH / ts.y);
                        sprite.setPosition(panelX + 10.f + (InspectorWidth - 20.f - previewW) / 2.f, y + 10.f);
                        window.draw(sprite);

                        y += previewH + 20.f;
                    }
                } else if (typeName == "Lua Script" || typeName == "JSON Data")
                {
                    y += 10.f;
                    y = DrawSectionHeader(window, "FILE CONTENT", sf::Color(150, 220, 150), panelX, y);

                    std::ifstream ifs(selPath);
                    if (ifs.is_open())
                    {
                        int lineCount = 0;
                        std::string line;
                        std::string previewContent;
                        while (std::getline(ifs, line) && lineCount < 20)
                        {
                            previewContent += line + "\n";
                            lineCount++;
                        }
                        if (std::getline(ifs, line)) { previewContent += "..."; }
                        ifs.close();

                        sf::Text contentText;
                        contentText.setFont(*m_Font);
                        contentText.setCharacterSize(9);
                        contentText.setFillColor(sf::Color(180, 190, 200));
                        contentText.setString(previewContent);
                        contentText.setPosition(panelX + 10.f, y + 5.f);
                        window.draw(contentText);

                        y += contentText.getLocalBounds().height + 15.f;
                    }
                }
            }
        } else
        {
            sf::Text empty;
            empty.setFont(*m_Font);
            empty.setCharacterSize(12);
            empty.setFillColor(C_TEXT_MUTED);
            empty.setString("No selection");
            empty.setPosition(panelX + InspectorPad + 2.f, panelY + 52.f);
            window.draw(empty);
        }
        return;
    }

    float y = panelY + 42.f;

    y = DrawSectionHeader(window, "OBJECT", C_TEXT_SECONDARY, panelX, y);

    std::string nameDisplay = (m_ActiveField == EditField::Name && !m_ActiveInputText.empty())
                                  ? m_ActiveInputText + "|"
                                  : (m_ActiveField == EditField::Name ? "|" : m_Selected->id);
    y = DrawEditableRow(window, "Name", nameDisplay, "edit_name", panelX, y);

    std::string tagDisplay = (m_ActiveField == EditField::Tag && !m_ActiveInputText.empty())
                                  ? m_ActiveInputText + "|"
                                  : (m_ActiveField == EditField::Tag ? "|" : m_Selected->tag);
    y = DrawEditableRow(window, "Tag", tagDisplay, "edit_tag", panelX, y);
    y = DrawRow(window, "Entity", std::to_string(m_Selected->entity), panelX, y); {
        std::string typeLabel = "rectangle";
        if (m_Selected->objectType == ObjectType::Circle) typeLabel = "circle";
        else if (m_Selected->objectType == ObjectType::Triangle) typeLabel = "triangle";
        else if (m_Selected->objectType == ObjectType::Pentagon) typeLabel = "pentagon";
        else if (m_Selected->objectType == ObjectType::Hexagon) typeLabel = "hexagon";
        else if (m_Selected->objectType == ObjectType::Sprite) typeLabel = "sprite";
        y = DrawRow(window, "Type", typeLabel, panelX, y);
    }

    y += 8.f;
    y = DrawSectionHeader(window, "TRANSFORM", C_TEXT_SECONDARY, panelX, y);

    std::string txDisplay = (m_ActiveField == EditField::TransformX && !m_ActiveInputText.empty())
                                ? m_ActiveInputText + "|"
                                : (m_ActiveField == EditField::TransformX
                                       ? "|"
                                       : std::to_string((int) m_Selected->shape.getPosition().x));
    y = DrawEditableRow(window, "X", txDisplay, "edit_x", panelX, y);
    std::string tyDisplay = (m_ActiveField == EditField::TransformY && !m_ActiveInputText.empty())
                                ? m_ActiveInputText + "|"
                                : (m_ActiveField == EditField::TransformY
                                       ? "|"
                                       : std::to_string((int) m_Selected->shape.getPosition().y));
    y = DrawEditableRow(window, "Y", tyDisplay, "edit_y", panelX, y);

    std::string rotDisplay = (m_ActiveField == EditField::Rotation && !m_ActiveInputText.empty())
                                 ? m_ActiveInputText + "|"
                                 : (m_ActiveField == EditField::Rotation
                                        ? "|"
                                        : std::to_string(m_Selected->rotation));
    y = DrawEditableRow(window, "Rotation", rotDisplay, "edit_rot", panelX, y);

    std::string sxDisplay = (m_ActiveField == EditField::ScaleX && !m_ActiveInputText.empty())
                                ? m_ActiveInputText + "|"
                                : (m_ActiveField == EditField::ScaleX
                                       ? "|"
                                       : std::to_string(m_Selected->scaleX));
    y = DrawEditableRow(window, "Scale X", sxDisplay, "edit_scalex", panelX, y);

    std::string syDisplay = (m_ActiveField == EditField::ScaleY && !m_ActiveInputText.empty())
                                ? m_ActiveInputText + "|"
                                : (m_ActiveField == EditField::ScaleY
                                       ? "|"
                                       : std::to_string(m_Selected->scaleY));
    y = DrawEditableRow(window, "Scale Y", syDisplay, "edit_scaley", panelX, y);


    y += 8.f;
    y = DrawSectionHeader(window, "RENDER", C_TEXT_SECONDARY, panelX, y);

    std::string wDisplay = (m_ActiveField == EditField::SizeW && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::SizeW
                                      ? "|"
                                      : std::to_string((int) m_Selected->shape.getSize().x));
    y = DrawEditableRow(window, "W", wDisplay, "edit_w", panelX, y);
    std::string hDisplay = (m_ActiveField == EditField::SizeH && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::SizeH
                                      ? "|"
                                      : std::to_string((int) m_Selected->shape.getSize().y));
    y = DrawEditableRow(window, "H", hDisplay, "edit_h", panelX, y);

    y += 4.f;

    if (!m_Selected->previewTexture)
    {
        sf::RectangleShape colorSwatch({InspectorWidth - InspectorPad * 2, 22.f});
        colorSwatch.setFillColor(m_Selected->color);
        colorSwatch.setOutlineColor(C_BORDER_LIGHT);
        colorSwatch.setOutlineThickness(1.f);
        colorSwatch.setPosition(panelX + InspectorPad, y);
        window.draw(colorSwatch);
        y += 28.f;
    }

    std::string rDisplay = (m_ActiveField == EditField::ColorR && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::ColorR ? "|" : std::to_string(m_Selected->color.r));
    y = DrawEditableRow(window, "R", rDisplay, "edit_r", panelX, y);
    std::string gDisplay = (m_ActiveField == EditField::ColorG && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::ColorG ? "|" : std::to_string(m_Selected->color.g));
    y = DrawEditableRow(window, "G", gDisplay, "edit_g", panelX, y);
    std::string bDisplay = (m_ActiveField == EditField::ColorB && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::ColorB ? "|" : std::to_string(m_Selected->color.b));
    y = DrawEditableRow(window, "B", bDisplay, "edit_b", panelX, y);

    y += 8.f;


    if (m_Selected->previewTexture)
    {
        y = DrawSectionHeader(window, "SPRITE COMPONENT", C_TEXT_SECONDARY, panelX, y);

        std::string spriteName = m_Selected->spritePath;
        const size_t sl = spriteName.find_last_of("/\\");
        if (sl != std::string::npos) spriteName = spriteName.substr(sl + 1);
        y = DrawRow(window, "File", spriteName.empty() ? "none" : spriteName, panelX, y);

        if (m_Selected->previewTexture)
        {
            const float thumbH = 48.f;
            const float thumbW = InspectorWidth - InspectorPad * 2;
            sf::RectangleShape thumb({thumbW, thumbH});
            thumb.setPosition(panelX + InspectorPad, y);
            thumb.setFillColor(C_BG_INPUT);
            thumb.setOutlineColor(C_BORDER);
            thumb.setOutlineThickness(1.f);
            window.draw(thumb);

            sf::Sprite preview;
            preview.setTexture(*m_Selected->previewTexture);
            const sf::Vector2u ts = m_Selected->previewTexture->getSize();
            if (ts.x > 0 && ts.y > 0)
            {
                const float scaleX = thumbW / static_cast<float>(ts.x);
                const float scaleY = thumbH / static_cast<float>(ts.y);
                const float scale = std::min(scaleX, scaleY);
                preview.setScale(scale, scale);
                preview.setPosition(
                    panelX + InspectorPad + (thumbW - ts.x * scale) / 2.f,
                    y + (thumbH - ts.y * scale) / 2.f);
            }
            window.draw(preview);
            y += thumbH + 4.f;
        }

        y = DrawActionButton(window, "Remove Sprite", "remove_sprite", panelX, y, C_DANGER_DIM, C_DANGER);
    } else
    {
        y = DrawActionButton(window, "+ Sprite Component", "change_sprite", panelX, y, C_BG_ELEVATED, C_BORDER_LIGHT);
        sf::Text hint;
        hint.setFont(*m_Font);
        hint.setCharacterSize(10);
        hint.setFillColor(C_TEXT_MUTED);
        hint.setString("(drag image from browser)");
        hint.setPosition(panelX + InspectorPad + 4.f, y);
        window.draw(hint);
        y += 16.f;
    }

    y += 8.f;

    if (m_Selected->entity != 0 && m_Registry.HasComponent<VelocityComponent>(m_Selected->entity))
    {
        auto &vel = m_Registry.GetComponent<VelocityComponent>(m_Selected->entity);
        y = DrawSectionHeader(window, "VELOCITY", C_TEXT_SECONDARY, panelX, y);
        y = DrawRow(window, "dX", std::to_string(vel.dx), panelX, y);
        y = DrawRow(window, "dY", std::to_string(vel.dy), panelX, y);
        y += 4.f;
        y = DrawActionButton(window, "Remove Velocity", "remove_velocity", panelX, y, C_DANGER_DIM, C_DANGER);
        y += 8.f;
    } else if (m_Selected->entity != 0)
    {
        y = DrawActionButton(window, "+ Velocity", "add_velocity", panelX, y, C_BG_ELEVATED, C_BORDER_LIGHT);
        y += 8.f;
    }

    if (m_Selected->entity != 0 && m_Registry.HasComponent<CameraComponent>(m_Selected->entity))
    {
        y = DrawSectionHeader(window, "CAMERA", C_TEXT_SECONDARY, panelX, y);
        y = DrawRow(window, "Status", "Following Entity", panelX, y);
        y += 4.f;
        y = DrawActionButton(window, "Remove Camera", "remove_camera", panelX, y, C_DANGER_DIM, C_DANGER);
        y += 8.f;
    } else if (m_Selected->entity != 0)
    {
        y = DrawActionButton(window, "+ Camera Follow", "add_camera", panelX, y, C_BG_ELEVATED, C_BORDER_LIGHT);
        y += 8.f;
    }

    if (m_Selected->entity != 0 && m_Registry.HasComponent<CollisionComponent>(m_Selected->entity))
    {
        auto &col = m_Registry.GetComponent<CollisionComponent>(m_Selected->entity);
        y = DrawSectionHeader(window, "COLLISION", C_TEXT_SECONDARY, panelX, y);
        std::string chanDisplay = (m_ActiveField == EditField::CollisionChannel && !m_ActiveInputText.empty())
                                      ? m_ActiveInputText + "|"
                                      : (m_ActiveField == EditField::CollisionChannel
                                             ? "|"
                                             : std::to_string(col.channel));
        y = DrawEditableRow(window, "Channel", chanDisplay, "edit_collision_channel", panelX, y);
        y += 4.f;
        std::string typeLabel = "Type: " + std::string(col.type == CollisionType::Solid ? "Solid" : "Static");
        y = DrawActionButton(window, typeLabel, "toggle_collision_type", panelX, y, C_BG_ELEVATED, C_BORDER_LIGHT);
        y += 4.f;
        y = DrawActionButton(window, "Remove Collision", "remove_collision", panelX, y, C_DANGER_DIM, C_DANGER);
        y += 8.f;
    } else if (m_Selected->entity != 0)
    {
        y = DrawActionButton(window, "+ Collision", "add_collision", panelX, y, C_BG_ELEVATED, C_BORDER_LIGHT);
        y += 8.f;
    }

    if (m_Selected->entity != 0 && m_Registry.HasComponent<ScriptComponent>(m_Selected->entity))
    {
        y = DrawSectionHeader(window, "SCRIPT", C_TEXT_SECONDARY, panelX, y);
        std::string scriptName = m_Selected->scriptPath;
        const size_t slash = scriptName.find_last_of("/\\");
        if (slash != std::string::npos) scriptName = scriptName.substr(slash + 1);
        y = DrawRow(window, "File", scriptName, panelX, y);
        y = DrawRow(window, "OnCreate", "bound", panelX, y);
        y = DrawRow(window, "OnUpdate", "bound", panelX, y);
        y += 4.f;
        y = DrawActionButton(window, "Open Script", "open_script", panelX, y, C_BG_ELEVATED, C_BORDER_LIGHT);
        y = DrawActionButton(window, "Remove Script", "remove_script", panelX, y, C_DANGER_DIM, C_DANGER);
    } else if (m_Selected->entity != 0)
    {
        y = DrawActionButton(window, "+ Script", "add_script", panelX, y, C_BG_ELEVATED, C_BORDER_LIGHT);
        if (m_ActiveField == EditField::Script)
            DrawScriptInput(window, panelX, y);
    }
}

void EditorScene::DrawHierarchy(sf::RenderWindow &window)
{
    m_HierarchyHitboxes.clear();

    const float panelX = m_HierarchyBounds.left;
    const float panelY = m_HierarchyBounds.top;
    const float panelH = m_HierarchyBounds.height;

    m_HierarchyPanel.setPosition(panelX, panelY);
    m_HierarchyPanel.setSize({HierarchyWidth, panelH});
    window.draw(m_HierarchyPanel);

    sf::RectangleShape rightBorder({1.f, panelH});
    rightBorder.setFillColor(C_BORDER);
    rightBorder.setPosition(panelX + HierarchyWidth - 1.f, panelY);
    window.draw(rightBorder); {
        sf::RectangleShape header({HierarchyWidth, 36.f});
        header.setPosition(panelX, panelY);
        header.setFillColor(C_BG_ELEVATED);
        window.draw(header);

        sf::RectangleShape headerLine({HierarchyWidth, 1.f});
        headerLine.setFillColor(C_BORDER);
        headerLine.setPosition(panelX, panelY + 35.f);
        window.draw(headerLine);

        sf::Text title;
        title.setFont(*m_Font);
        title.setCharacterSize(11);
        title.setFillColor(C_TEXT_SECONDARY);
        title.setStyle(sf::Text::Bold);
        title.setString("SCENE HIERARCHY");
        title.setPosition(panelX + 12.f, panelY + 12.f);
        window.draw(title);
    }

    sf::View prevView = window.getView();

    const float winW = static_cast<float>(window.getSize().x);
    const float winH = static_cast<float>(window.getSize().y);

    sf::View clipView;
    clipView.setSize(HierarchyWidth, panelH - 36.f);
    clipView.setCenter(panelX + HierarchyWidth / 2.f, panelY + 36.f + (panelH - 36.f) / 2.f);
    clipView.setViewport({
        panelX / winW,
        (panelY + 36.f) / winH,
        HierarchyWidth / winW,
        (panelH - 36.f) / winH
    });
    window.setView(clipView);

    float y = panelY + 36.f - m_HierarchyScrollY;
    const float rowHeight = 26.f;
    int index = 0;

    auto drawRowBg = [&](bool isSelected, sf::FloatRect rowRect) {
        const bool isHovered = rowRect.contains(m_MouseScreenPos) && m_MouseScreenPos.y > panelY + 36.f;
        if (isSelected || isHovered)
        {
            sf::RectangleShape rowBg({HierarchyWidth, rowHeight});
            rowBg.setPosition(panelX, y);
            rowBg.setFillColor(isSelected ? C_ACCENT_DIM : C_BG_ELEVATED);
            window.draw(rowBg);

            if (isSelected)
            {
                sf::RectangleShape indicator({2.f, rowHeight});
                indicator.setPosition(panelX, y);
                indicator.setFillColor(C_ACCENT);
                window.draw(indicator);
            }
        }
        return isSelected;
    };


    for (auto it = m_Objects.rbegin(); it != m_Objects.rend(); ++it)
    {
        EditorObject &obj = *it;

        if (y + rowHeight > panelY + 36.f && y < panelY + panelH)
        {
            sf::FloatRect rowRect(panelX, y, HierarchyWidth, rowHeight);
            bool isSelected = drawRowBg(&obj == m_Selected, rowRect);

            sf::CircleShape icon;
            icon.setRadius(5.f);
            icon.setPosition(panelX + 16.f, y + 8.f);

            if (IsPolygonType(obj.objectType))
            {
                icon.setPointCount(GetPolygonPointCount(obj.objectType));
                icon.setFillColor(obj.color);
            } else if (obj.objectType == ObjectType::Sprite)
            {
                icon.setPointCount(4);
                icon.setFillColor(sf::Color(150, 150, 255));
            } else
            {
                icon.setPointCount(4);
                icon.setFillColor(obj.color);
            }
            window.draw(icon);

            sf::Text nameText;
            nameText.setFont(*m_Font);
            nameText.setCharacterSize(12);
            nameText.setFillColor(isSelected ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
            nameText.setString(obj.id);
            nameText.setPosition(panelX + 34.f, y + 4.f);
            window.draw(nameText);
        }

        m_HierarchyHitboxes.push_back({sf::FloatRect(panelX, y, HierarchyWidth, rowHeight), &obj});

        y += rowHeight;
        index++;
    }


    window.setView(prevView);

    if (m_HierarchyContextMenuOpen)
    {
        m_ContextHitboxes.clear();
        const float itemH = 28.f;
        const float menuW = 120.f;

        std::vector<std::pair<std::string, std::string> > actions = {
            {"Rename", "rename"},
            {"Duplicate", "duplicate"},
            {"Delete", "delete"}
        };

        const float menuH = actions.size() * itemH;
        sf::RectangleShape bg({menuW, menuH});
        bg.setPosition(m_ContextMenuPos);
        bg.setFillColor(C_BG_ELEVATED);
        bg.setOutlineColor(C_BORDER_LIGHT);
        bg.setOutlineThickness(1.f);
        window.draw(bg);

        float cy = m_ContextMenuPos.y;
        for (const auto &action: actions)
        {
            sf::FloatRect row(m_ContextMenuPos.x, cy, menuW, itemH);
            bool hov = row.contains(m_MouseScreenPos);
            if (hov)
            {
                sf::RectangleShape hovBg({menuW, itemH});
                hovBg.setPosition(m_ContextMenuPos.x, cy);
                hovBg.setFillColor(sf::Color(C_BG_ELEVATED.r + 10, C_BG_ELEVATED.g + 10, C_BG_ELEVATED.b + 10));
                window.draw(hovBg);
            }

            sf::Text t;
            t.setFont(*m_Font);
            t.setCharacterSize(12);
            t.setString(action.first);
            t.setFillColor(action.second == "delete" ? C_DANGER : (hov ? C_TEXT_PRIMARY : C_TEXT_SECONDARY));
            t.setPosition(m_ContextMenuPos.x + 12.f, cy + 6.f);
            window.draw(t);

            m_ContextHitboxes.push_back({row, action.second});
            cy += itemH;
        }
    }
}

float EditorScene::DrawSectionHeader(sf::RenderWindow &window, const std::string &title,
                                     sf::Color accent, float x, float y)
{
    sf::RectangleShape hairline({InspectorWidth, 1.f});
    hairline.setFillColor(C_BORDER);
    hairline.setPosition(x, y + 2.f);
    window.draw(hairline);
    y += 4.f;

    sf::Text text;
    text.setFont(*m_Font);
    text.setCharacterSize(10);
    text.setFillColor(accent);
    text.setStyle(sf::Text::Bold);
    text.setString(title);
    text.setPosition(x + InspectorPad + 2.f, y + 4.f);
    window.draw(text);

    return y + 20.f;
}

float EditorScene::DrawRow(sf::RenderWindow &window, const std::string &key,
                           const std::string &val, float x, float y)
{
    sf::Text keyText;
    keyText.setFont(*m_Font);
    keyText.setCharacterSize(12);
    keyText.setFillColor(C_TEXT_SECONDARY);
    keyText.setString(key);
    keyText.setPosition(x + InspectorPad + 4.f, y + 2.f);
    window.draw(keyText);

    sf::Text valText;
    valText.setFont(*m_Font);
    valText.setCharacterSize(12);
    valText.setFillColor(C_TEXT_PRIMARY);
    valText.setString(val);
    valText.setPosition(x + InspectorWidth * 0.48f, y + 2.f);
    window.draw(valText);

    sf::RectangleShape line({InspectorWidth - InspectorPad * 2, 1.f});
    line.setFillColor(sf::Color(C_BORDER.r, C_BORDER.g, C_BORDER.b, 80));
    line.setPosition(x + InspectorPad, y + 18.f);
    window.draw(line);

    return y + 20.f;
}

float EditorScene::DrawEditableRow(sf::RenderWindow &window, const std::string &key, const std::string &val,
                                   const std::string &action, float x, float y)
{
    sf::Text keyText;
    keyText.setFont(*m_Font);
    keyText.setCharacterSize(12);
    keyText.setFillColor(C_TEXT_SECONDARY);
    keyText.setString(key);
    keyText.setPosition(x + InspectorPad + 4.f, y + 3.f);
    window.draw(keyText);

    const float valX = x + InspectorWidth * 0.44f;
    const float valW = InspectorWidth - InspectorWidth * 0.44f - InspectorPad;
    const sf::FloatRect fieldRect(valX, y, valW, 20.f);
    const bool hovered = fieldRect.contains(m_MouseScreenPos);
    const bool active = (m_InspectorButtons.size() > 0) && false;

    sf::Color fieldFill = hovered ? C_BG_ELEVATED : C_BG_INPUT;
    sf::Color fieldBorder = hovered ? C_ACCENT : C_BORDER;

    sf::RectangleShape field({fieldRect.width, fieldRect.height});
    field.setPosition(fieldRect.left, fieldRect.top);
    field.setFillColor(fieldFill);
    field.setOutlineColor(fieldBorder);
    field.setOutlineThickness(1.f);
    window.draw(field);

    sf::Text valText;
    valText.setFont(*m_Font);
    valText.setCharacterSize(12);
    valText.setFillColor(hovered ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
    valText.setString(val);
    valText.setPosition(valX + 5.f, y + 3.f);
    window.draw(valText);

    m_InspectorButtons.push_back({fieldRect, action});

    sf::RectangleShape line({InspectorWidth - InspectorPad * 2, 1.f});
    line.setFillColor(sf::Color(C_BORDER.r, C_BORDER.g, C_BORDER.b, 60));
    line.setPosition(x + InspectorPad, y + 21.f);
    window.draw(line);

    return y + 22.f;
}

float EditorScene::DrawAddButton(sf::RenderWindow &window, const std::string &label,
                                 const std::string &action, float x, float y)
{
    return DrawActionButton(window, label, action, x, y, C_BG_ELEVATED, C_BORDER_LIGHT);
}

float EditorScene::DrawRemoveButton(sf::RenderWindow &window, const std::string &label,
                                    const std::string &action, float x, float y)
{
    return DrawActionButton(window, label, action, x, y, C_DANGER_DIM, C_DANGER);
}

float EditorScene::DrawActionButton(sf::RenderWindow &window, const std::string &label,
                                    const std::string &action, float x, float y,
                                    sf::Color fillColor, sf::Color borderColor)
{
    const sf::FloatRect btnRect(x + InspectorPad, y + 2.f, InspectorWidth - InspectorPad * 2, 24.f);
    const bool hovered = btnRect.contains(m_MouseScreenPos);

    sf::Color fill = hovered
                         ? sf::Color(std::min(255, fillColor.r + 16),
                                     std::min(255, fillColor.g + 16),
                                     std::min(255, fillColor.b + 16), 255)
                         : fillColor;
    sf::Color border = hovered
                           ? sf::Color(std::min(255, borderColor.r + 30),
                                       std::min(255, borderColor.g + 30),
                                       std::min(255, borderColor.b + 30), 255)
                           : borderColor;

    DrawPill(window, btnRect, fill, border);

    sf::Text text;
    text.setFont(*m_Font);
    text.setCharacterSize(11);
    text.setFillColor(hovered ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
    text.setString(label);
    text.setPosition(btnRect.left + (btnRect.width - text.getLocalBounds().width) / 2.f,
                     btnRect.top + 6.f);
    window.draw(text);

    m_InspectorButtons.push_back({btnRect, action});

    return y + 30.f;
}

float EditorScene::DrawScriptInput(sf::RenderWindow &window, float x, float y)
{
    const sf::FloatRect inputRect(x + InspectorPad, y + 2.f, InspectorWidth - InspectorPad * 2, 24.f);

    sf::RectangleShape bg({inputRect.width, inputRect.height});
    bg.setFillColor(C_BG_INPUT);
    bg.setOutlineColor(C_ACCENT);
    bg.setOutlineThickness(1.f);
    bg.setPosition(inputRect.left, inputRect.top);
    window.draw(bg);

    std::string display = (m_ActiveField == EditField::Script && !m_ActiveInputText.empty())
                              ? m_ActiveInputText
                              : (m_ActiveField != EditField::Script)
                                    ? "name"
                                    : "";
    sf::Color textColor = display == "name" ? C_TEXT_MUTED : C_TEXT_PRIMARY;

    sf::Text inputText;
    inputText.setFont(*m_Font);
    inputText.setCharacterSize(11);
    inputText.setFillColor(textColor);
    inputText.setString(display + (m_ActiveField == EditField::Script ? "|" : ""));
    inputText.setPosition(inputRect.left + 6.f, inputRect.top + 5.f);
    window.draw(inputText);

    m_ActiveInputBounds = inputRect;
    m_InspectorButtons.push_back({inputRect, "edit_script"});

    sf::Text hint;
    hint.setFont(*m_Font);
    hint.setCharacterSize(10);
    hint.setFillColor(C_TEXT_MUTED);
    hint.setString("Enter to confirm");
    hint.setPosition(x + InspectorPad, y + 30.f);
    window.draw(hint);

    return y + 44.f;
}

void EditorScene::HandleInspectorClick(sf::Vector2f pos)
{
    m_ActiveField = EditField::None;

    for (auto &btn: m_InspectorButtons)
    {
        if (!btn.bounds.contains(pos)) continue;

        if (btn.action == "add_velocity")
        {
            m_Registry.AddComponent(m_Selected->entity, VelocityComponent{0.f, 0.f});
            std::cout << "[INFO] [Inspector] VelocityComponent added to " << m_Selected->id << "\n";
        } else if (btn.action == "remove_velocity")
        {
            m_Registry.RemoveComponent<VelocityComponent>(m_Selected->entity);
            std::cout << "[INFO] [Inspector] VelocityComponent removed from " << m_Selected->id << "\n";
        } else if (btn.action == "add_camera")
        {
            m_Registry.ForEach<CameraComponent>([this](Entity e, CameraComponent &) {
                m_Registry.RemoveComponent<CameraComponent>(e);
            });
            m_Registry.AddComponent(m_Selected->entity, CameraComponent{true});
            std::cout << "[INFO] [Inspector] CameraComponent added to " << m_Selected->id << "\n";
        } else if (btn.action == "remove_camera")
        {
            m_Registry.RemoveComponent<CameraComponent>(m_Selected->entity);
            std::cout << "[INFO] [Inspector] CameraComponent removed from " << m_Selected->id << "\n";
        } else if (btn.action == "add_collision")
        {
            m_Registry.AddComponent(m_Selected->entity, CollisionComponent{0});
            std::cout << "[INFO] [Inspector] CollisionComponent added to " << m_Selected->id << "\n";
        } else if (btn.action == "remove_collision")
        {
            m_Registry.RemoveComponent<CollisionComponent>(m_Selected->entity);
            std::cout << "[INFO] [Inspector] CollisionComponent removed from " << m_Selected->id << "\n";
        } else if (btn.action == "toggle_collision_type")
        {
            if (m_Registry.HasComponent<CollisionComponent>(m_Selected->entity))
            {
                auto& col = m_Registry.GetComponent<CollisionComponent>(m_Selected->entity);
                col.type = (col.type == CollisionType::Static) ? CollisionType::Solid : CollisionType::Static;
            }
        } else if (btn.action == "edit_collision_channel" && m_Selected)
        {
            m_ActiveField = EditField::CollisionChannel;
            if (m_Registry.HasComponent<CollisionComponent>(m_Selected->entity))
            {
                m_ActiveInputText = std::to_string(
                    m_Registry.GetComponent<CollisionComponent>(m_Selected->entity).channel);
            }
        } else if (btn.action == "add_script")
        {
            m_ActiveField = EditField::Script;
            m_ActiveInputText = "";
        } else if (btn.action == "edit_script")
        {
            m_ActiveField = EditField::Script;
            m_ActiveInputText = "";
        } else if (btn.action == "remove_script")
        {
            m_Registry.RemoveComponent<ScriptComponent>(m_Selected->entity);
            m_Selected->scriptPath = "";
            std::cout << "[INFO] [Inspector] ScriptComponent removed from " << m_Selected->id << "\n";
        } else if (btn.action == "edit_name" && m_Selected)
        {
            m_ActiveField = EditField::Name;
            m_ActiveInputText = m_Selected->id;
        } else if (btn.action == "edit_tag" && m_Selected)
        {
            m_ActiveField = EditField::Tag;
            m_ActiveInputText = m_Selected->tag;
        } else if (btn.action == "edit_x" && m_Selected)
        {
            m_ActiveField = EditField::TransformX;
            m_ActiveInputText = std::to_string((int) m_Selected->shape.getPosition().x);
        } else if (btn.action == "edit_y" && m_Selected)
        {
            m_ActiveField = EditField::TransformY;
            m_ActiveInputText = std::to_string((int) m_Selected->shape.getPosition().y);
        } else if (btn.action == "edit_rot" && m_Selected)
        {
            m_ActiveField = EditField::Rotation;
            m_ActiveInputText = std::to_string(m_Selected->rotation);
        } else if (btn.action == "edit_scalex" && m_Selected)
        {
            m_ActiveField = EditField::ScaleX;
            m_ActiveInputText = std::to_string(m_Selected->scaleX);
        } else if (btn.action == "edit_scaley" && m_Selected)
        {
            m_ActiveField = EditField::ScaleY;
            m_ActiveInputText = std::to_string(m_Selected->scaleY);
        } else if (btn.action == "edit_w" && m_Selected)
        {
            m_ActiveField = EditField::SizeW;
            m_ActiveInputText = std::to_string((int) m_Selected->shape.getSize().x);
        } else if (btn.action == "edit_h" && m_Selected)
        {
            m_ActiveField = EditField::SizeH;
            m_ActiveInputText = std::to_string((int) m_Selected->shape.getSize().y);
        } else if (btn.action == "edit_r" && m_Selected)
        {
            m_ActiveField = EditField::ColorR;
            m_ActiveInputText = std::to_string(m_Selected->color.r);
        } else if (btn.action == "edit_g" && m_Selected)
        {
            m_ActiveField = EditField::ColorG;
            m_ActiveInputText = std::to_string(m_Selected->color.g);
        } else if (btn.action == "edit_b" && m_Selected)
        {
            m_ActiveField = EditField::ColorB;
            m_ActiveInputText = std::to_string(m_Selected->color.b);
        } else if (btn.action == "open_script" && m_Selected && !m_Selected->scriptPath.empty())
        {
#ifdef _WIN32
            ShellExecuteA(nullptr, "open", m_Selected->scriptPath.c_str(), nullptr, nullptr, SW_SHOW);
#elif __APPLE__
            system(("open \"" + m_Selected->scriptPath + "\"").c_str());
#else
            system(("xdg-open \"" + m_Selected->scriptPath + "\"").c_str());
#endif
        } else if (btn.action == "remove_sprite" && m_Selected)
        {
            m_Selected->spritePath.clear();
            m_Selected->previewTexture.reset();
            m_Selected->previewSprite = sf::Sprite{};
            m_Selected->shape.setFillColor(m_Selected->color);
            if (m_Selected->entity != 0 && m_Registry.HasComponent<SpriteComponent>(m_Selected->entity))
                m_Registry.RemoveComponent<SpriteComponent>(m_Selected->entity);
            std::cout << "[INFO] [Inspector] SpriteComponent removed from " << m_Selected->id << "\n";
        } else if (btn.action == "change_sprite" && m_Selected)
        {
            std::cout << "[INFO] [Inspector] Drag an image from the Content Browser to change sprite.\n";
        }
        break;
    }
}

void EditorScene::AddObject(sf::Vector2f pos, ObjectType type)
{
    std::string id = NextId();
    sf::Color c = sf::Color(100, 149, 237);
    std::string ts = "rectangle";
    if (type == ObjectType::Circle) { c = sf::Color(237, 149, 100); ts = "circle"; }
    else if (type == ObjectType::Triangle) { c = sf::Color(149, 237, 100); ts = "triangle"; }
    else if (type == ObjectType::Pentagon) { c = sf::Color(237, 100, 237); ts = "pentagon"; }
    else if (type == ObjectType::Hexagon) { c = sf::Color(237, 237, 100); ts = "hexagon"; }

    sf::Vector2f p = SnapToGrid(pos);
    json j;
    j["id"] = id;
    j["type"] = ts;
    j["x"] = p.x; j["y"] = p.y;
    j["width"] = m_GridSize; j["height"] = m_GridSize;
    j["color"] = {c.r, c.g, c.b};
    j["rotation"] = 0.f;
    j["scaleX"] = 1.f;
    j["scaleY"] = 1.f;

    auto cmd = std::make_shared<ObjectStateCommand>(id, json(), j);
    ExecuteCommand(cmd);
}

void EditorScene::AddObjectWithSprite(sf::Vector2f pos, const std::string &spritePath)
{
    std::string id = NextId();
    sf::Vector2f p = SnapToGrid(pos);
    json j;
    j["id"] = id;
    j["type"] = "sprite";
    j["x"] = p.x; j["y"] = p.y;
    j["width"] = m_GridSize * 2.f; j["height"] = m_GridSize * 2.f;
    j["color"] = {255, 255, 255};
    j["rotation"] = 0.f;
    j["scaleX"] = 1.f;
    j["scaleY"] = 1.f;

    std::error_code ec;
    std::filesystem::path pt(spritePath);
    std::filesystem::path root = std::filesystem::absolute(ASSET_PATH, ec);
    std::string rel = std::filesystem::proximate(pt, root, ec).generic_string();
    j["sprite"] = ec ? spritePath : rel;

    auto cmd = std::make_shared<ObjectStateCommand>(id, json(), j);
    ExecuteCommand(cmd);
}

void EditorScene::ApplySpriteToObject(EditorObject &obj, const std::string &spritePath)
{
    obj.spritePath = spritePath;
    obj.previewTexture = ResourceManager::Get().GetTexture(spritePath);

    if (obj.previewTexture)
    {
        obj.previewSprite.setTexture(*obj.previewTexture, true);
        const sf::Vector2u ts = obj.previewTexture->getSize();
        const sf::Vector2f sz = obj.shape.getSize();
        if (ts.x > 0 && ts.y > 0)
            obj.previewSprite.setScale(sz.x / ts.x, sz.y / ts.y);
        obj.shape.setFillColor(sf::Color(255, 255, 255, 40));
    }

    if (obj.entity != 0)
    {
        if (m_Registry.HasComponent<SpriteComponent>(obj.entity))
            m_Registry.GetComponent<SpriteComponent>(obj.entity) = SpriteComponent(spritePath, obj.shape.getSize());
        else
            m_Registry.AddComponent(obj.entity, SpriteComponent(spritePath, obj.shape.getSize()));
    }

    std::cout << "[INFO] [EditorScene] Sprite texture applied: " << spritePath << "\n";
}

void EditorScene::DeleteSelected()
{
    if (m_SelectedObjects.empty()) return;

    auto macroCmd = std::make_shared<MacroCommand>();
    for (auto* obj : m_SelectedObjects) {
        json before = SerializeObject(*obj);
        macroCmd->commands.push_back(std::make_shared<ObjectStateCommand>(obj->id, before, json()));
    }
    ExecuteCommand(macroCmd);
    ClearSelection();
}

void EditorScene::SaveToJson(const std::string &path)
{
    std::string uiPath = path.substr(0, path.find_last_of('.')) + "_ui.json";
    UIManager::Get().SetCurrentUIPath(uiPath);

    json data;
    data["name"] = "game";
    data["objects"] = json::array();

    for (auto &obj: m_Objects)
    {
        json j;
        j["id"] = obj.id;
        j["tag"] = obj.tag;
        std::string typeStr = "rectangle";
        if (obj.objectType == ObjectType::Circle) typeStr = "circle";
        else if (obj.objectType == ObjectType::Triangle) typeStr = "triangle";
        else if (obj.objectType == ObjectType::Pentagon) typeStr = "pentagon";
        else if (obj.objectType == ObjectType::Hexagon) typeStr = "hexagon";
        else if (obj.objectType == ObjectType::Sprite) typeStr = "sprite";
        j["type"] = typeStr;
        j["x"] = obj.shape.getPosition().x;
        j["y"] = obj.shape.getPosition().y;
        j["rotation"] = obj.rotation;
        j["scaleX"] = obj.scaleX;
        j["scaleY"] = obj.scaleY;
        j["width"] = obj.shape.getSize().x;
        j["height"] = obj.shape.getSize().y;

        j["color"] = {obj.color.r, obj.color.g, obj.color.b};

        if (!obj.spritePath.empty())
        {
            std::error_code ec;
            std::filesystem::path p(obj.spritePath);
            std::filesystem::path root = std::filesystem::absolute(ASSET_PATH, ec);
            std::string rel = std::filesystem::proximate(p, root, ec).generic_string();
            j["sprite"] = ec ? obj.spritePath : rel;
        }

        if (obj.entity != 0 && m_Registry.HasComponent<VelocityComponent>(obj.entity))
        {
            auto &vel = m_Registry.GetComponent<VelocityComponent>(obj.entity);
            j["velocity"] = {{"dx", vel.dx}, {"dy", vel.dy}};
        }

        if (!obj.scriptPath.empty())
        {
            std::error_code ec;
            std::filesystem::path p(obj.scriptPath);
            std::filesystem::path root = std::filesystem::absolute(ASSET_PATH, ec);
            std::string rel = std::filesystem::proximate(p, root, ec).generic_string();
            j["script"] = ec ? obj.scriptPath : rel;
        }

        if (obj.entity != 0 && m_Registry.HasComponent<CameraComponent>(obj.entity)) { j["camera"] = true; }

        if (obj.entity != 0 && m_Registry.HasComponent<CollisionComponent>(obj.entity))
        {
            auto &col = m_Registry.GetComponent<CollisionComponent>(obj.entity);
            j["collision"] = {
                {"channel", col.channel},
                {"type", col.type == CollisionType::Solid ? "solid" : "static"}
            };
        }

        data["objects"].push_back(j);
    }

    std::ofstream file(path);
    if (!file.is_open())
    {
        std::cerr << "[ERROR] [EditorScene] Failed to open file for saving: " << path << "\n";
        return;
    }
    file << data.dump(4);
    UpdateStatusText();
}

void EditorScene::LoadFromJson(const std::string &path)
{
    std::string uiPath = path.substr(0, path.find_last_of('.')) + "_ui.json";
    UIManager::Get().SetCurrentUIPath(uiPath);
    UIManager::Get().Load(uiPath);

    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "[ERROR] [EditorScene] Scene file not found or unreadable: " << path << "\n";
        return;
    }

    for (auto &obj: m_Objects)
        if (obj.entity != 0)
            m_Registry.DestroyEntity(obj.entity);

    m_Objects.clear();
    ClearSelection();
    m_UndoStack.clear();
    m_RedoStack.clear();

    json data = json::parse(file);
    for (auto &j: data["objects"])
    {
        EditorObject obj;
        obj.id = j["id"];
        if (j.contains("tag")) obj.tag = j["tag"];
        obj.color = sf::Color(j["color"][0], j["color"][1], j["color"][2]);
        obj.shape.setSize({j["width"], j["height"]});
        obj.shape.setPosition(j["x"], j["y"]);
        if (j.contains("rotation")) obj.rotation = j["rotation"];
        if (j.contains("scaleX")) obj.scaleX = j["scaleX"];
        if (j.contains("scaleY")) obj.scaleY = j["scaleY"];
        obj.shape.setRotation(obj.rotation);
        obj.shape.setScale(obj.scaleX, obj.scaleY);
        obj.shape.setFillColor(obj.color);

        const std::string typeStr = j.value("type", "rectangle");
        if (typeStr == "circle") obj.objectType = ObjectType::Circle;
        else if (typeStr == "triangle") obj.objectType = ObjectType::Triangle;
        else if (typeStr == "pentagon") obj.objectType = ObjectType::Pentagon;
        else if (typeStr == "hexagon") obj.objectType = ObjectType::Hexagon;
        else if (typeStr == "sprite") obj.objectType = ObjectType::Sprite;
        else obj.objectType = ObjectType::Rectangle;

        if (IsPolygonType(obj.objectType))
        {
            obj.circleShape.setPointCount(GetPolygonPointCount(obj.objectType));
            obj.circleShape.setRadius(obj.shape.getSize().x / 2.f);
            obj.circleShape.setPosition(obj.shape.getPosition());
            obj.circleShape.setRotation(obj.rotation);
            obj.circleShape.setScale(obj.scaleX, obj.scaleY);
            obj.circleShape.setFillColor(obj.color);
        }

        obj.entity = m_Registry.CreateEntity();
        
        TransformComponent t;
        t.x = j["x"];
        t.y = j["y"];
        t.rotation = obj.rotation;
        t.scaleX = obj.scaleX;
        t.scaleY = obj.scaleY;
        m_Registry.AddComponent(obj.entity, t);
        m_Registry.AddComponent(obj.entity, RenderComponent{obj.color, obj.shape.getSize(), MapToShapeType(obj.objectType)});
        if (!obj.tag.empty()) {
            m_Registry.AddComponent(obj.entity, TagComponent{obj.tag});
        }

        if (j.contains("sprite"))
        {
            std::string sp = j["sprite"].get<std::string>();
            std::filesystem::path p(sp);
            if (!p.is_absolute()) {
                sp = (std::filesystem::path(ASSET_PATH) / p).string();
            }
            ApplySpriteToObject(obj, sp);
        }

        if (j.contains("velocity"))
            m_Registry.AddComponent(obj.entity, VelocityComponent{
                                        j["velocity"]["dx"], j["velocity"]["dy"]
                                    });

        if (j.contains("script"))
        {
            std::string sp = j["script"].get<std::string>();
            std::filesystem::path p(sp);
            if (!p.is_absolute()) {
                sp = (std::filesystem::path(ASSET_PATH) / p).string();
            }
            auto &sc = m_Registry.AddComponent(obj.entity, ScriptComponent(LuaState::GetLua(), sp));
            sc.SetEntity(obj.entity);
            obj.scriptPath = sp;
        }

        if (j.contains("camera")) { m_Registry.AddComponent(obj.entity, CameraComponent{true}); }

        if (j.contains("collision"))
        {
            CollisionType cType = CollisionType::Static;
            if (j["collision"].contains("type") && j["collision"]["type"] == "solid")
                cType = CollisionType::Solid;
            m_Registry.AddComponent(obj.entity, CollisionComponent{j["collision"]["channel"].get<int>(), cType});
        }

        m_Objects.push_back(std::move(obj));
    }

    UpdateStatusText();
}

void EditorScene::SyncToRegistry()
{
    for (auto &obj: m_Objects)
    {
        if (obj.entity == 0) continue;

        if (m_Registry.HasComponent<TransformComponent>(obj.entity))
        {
            auto &t = m_Registry.GetComponent<TransformComponent>(obj.entity);
            t.x = obj.shape.getPosition().x;
            t.y = obj.shape.getPosition().y;
            t.rotation = obj.rotation;
            t.scaleX = obj.scaleX;
            t.scaleY = obj.scaleY;
        }

        if (m_Registry.HasComponent<RenderComponent>(obj.entity))
        {
            auto &rc = m_Registry.GetComponent<RenderComponent>(obj.entity);
            rc.size = obj.shape.getSize();
            rc.color = obj.color;
            rc.shapeType = MapToShapeType(obj.objectType);
        }

        if (obj.previewTexture && !obj.spritePath.empty())
        {
            if (m_Registry.HasComponent<SpriteComponent>(obj.entity))
            {
                m_Registry.GetComponent<SpriteComponent>(obj.entity) =
                        SpriteComponent(obj.spritePath, obj.shape.getSize());
            } else { m_Registry.AddComponent(obj.entity, SpriteComponent(obj.spritePath, obj.shape.getSize())); }
        }
    }
}

void EditorScene::SnapshotState()
{
    m_PlayModeSnapshot = json{};
    m_PlayModeSnapshot["name"] = "snapshot";
    m_PlayModeSnapshot["uiPath"] = UIManager::Get().GetCurrentUIPath();
    m_PlayModeSnapshot["objects"] = json::array();

    for (auto &obj : m_Objects)
    {
        json j;
        j["id"] = obj.id;
        j["tag"] = obj.tag;
        std::string typeStr = "rectangle";
        if (obj.objectType == ObjectType::Circle) typeStr = "circle";
        else if (obj.objectType == ObjectType::Triangle) typeStr = "triangle";
        else if (obj.objectType == ObjectType::Pentagon) typeStr = "pentagon";
        else if (obj.objectType == ObjectType::Hexagon) typeStr = "hexagon";
        else if (obj.objectType == ObjectType::Sprite) typeStr = "sprite";
        j["type"] = typeStr;
        j["x"] = obj.shape.getPosition().x;
        j["y"] = obj.shape.getPosition().y;
        j["rotation"] = obj.rotation;
        j["scaleX"] = obj.scaleX;
        j["scaleY"] = obj.scaleY;
        j["width"] = obj.shape.getSize().x;
        j["height"] = obj.shape.getSize().y;
        j["color"] = {obj.color.r, obj.color.g, obj.color.b};

        if (!obj.spritePath.empty())
        {
            std::error_code ec;
            std::filesystem::path p(obj.spritePath);
            std::filesystem::path root = std::filesystem::absolute(ASSET_PATH, ec);
            std::string rel = std::filesystem::proximate(p, root, ec).generic_string();
            j["sprite"] = ec ? obj.spritePath : rel;
        }

        if (obj.entity != 0 && m_Registry.HasComponent<VelocityComponent>(obj.entity))
        {
            auto &vel = m_Registry.GetComponent<VelocityComponent>(obj.entity);
            j["velocity"] = {{"dx", vel.dx}, {"dy", vel.dy}};
        }

        if (!obj.scriptPath.empty())
        {
            std::error_code ec;
            std::filesystem::path p(obj.scriptPath);
            std::filesystem::path root = std::filesystem::absolute(ASSET_PATH, ec);
            std::string rel = std::filesystem::proximate(p, root, ec).generic_string();
            j["script"] = ec ? obj.scriptPath : rel;
        }

        if (obj.entity != 0 && m_Registry.HasComponent<CameraComponent>(obj.entity))
            j["camera"] = true;

        if (obj.entity != 0 && m_Registry.HasComponent<CollisionComponent>(obj.entity))
        {
            auto &col = m_Registry.GetComponent<CollisionComponent>(obj.entity);
            j["collision"] = {
                {"channel", col.channel},
                {"type", col.type == CollisionType::Solid ? "solid" : "static"}
            };
        }

        m_PlayModeSnapshot["objects"].push_back(j);
    }

    m_SnapshotEntityCounter = m_Registry.GetEntityCounter();
    std::cout << "[INFO] [EditorScene] Play-mode snapshot created (" << m_Objects.size() << " objects).\n";
}

void EditorScene::RestoreSnapshot()
{
    std::cout << "[INFO] [EditorScene] Restoring play-mode snapshot...\n";

    for (auto &obj : m_Objects)
        if (obj.entity != 0)
            m_Registry.DestroyEntity(obj.entity);
    m_Registry.Clear();

    m_Objects.clear();
    ClearSelection();

    if (m_PlayModeSnapshot.contains("uiPath")) {
        std::string originalUI = m_PlayModeSnapshot["uiPath"];
        UIManager::Get().SetCurrentUIPath(originalUI);
        UIManager::Get().Load(originalUI);
    } else {
        std::string currentUI = UIManager::Get().GetCurrentUIPath();
        if (!currentUI.empty()) {
            UIManager::Get().Load(currentUI);
        }
    }

    m_Registry.SetEntityCounter(m_SnapshotEntityCounter);

    for (auto &j : m_PlayModeSnapshot["objects"])
    {
        EditorObject obj;
        obj.id = j["id"];
        if (j.contains("tag")) obj.tag = j["tag"];
        obj.color = sf::Color(j["color"][0], j["color"][1], j["color"][2]);
        obj.shape.setSize({j["width"], j["height"]});
        obj.shape.setPosition(j["x"], j["y"]);
        if (j.contains("rotation")) obj.rotation = j["rotation"];
        if (j.contains("scaleX")) obj.scaleX = j["scaleX"];
        if (j.contains("scaleY")) obj.scaleY = j["scaleY"];
        obj.shape.setRotation(obj.rotation);
        obj.shape.setScale(obj.scaleX, obj.scaleY);
        obj.shape.setFillColor(obj.color);

        const std::string typeStr = j.value("type", "rectangle");
        if (typeStr == "circle") obj.objectType = ObjectType::Circle;
        else if (typeStr == "triangle") obj.objectType = ObjectType::Triangle;
        else if (typeStr == "pentagon") obj.objectType = ObjectType::Pentagon;
        else if (typeStr == "hexagon") obj.objectType = ObjectType::Hexagon;
        else if (typeStr == "sprite") obj.objectType = ObjectType::Sprite;
        else obj.objectType = ObjectType::Rectangle;

        if (IsPolygonType(obj.objectType))
        {
            obj.circleShape.setPointCount(GetPolygonPointCount(obj.objectType));
            obj.circleShape.setRadius(obj.shape.getSize().x / 2.f);
            obj.circleShape.setPosition(obj.shape.getPosition());
            obj.circleShape.setRotation(obj.rotation);
            obj.circleShape.setScale(obj.scaleX, obj.scaleY);
            obj.circleShape.setFillColor(obj.color);
        }

        obj.entity = m_Registry.CreateEntity();

        TransformComponent t;
        t.x = j["x"];
        t.y = j["y"];
        t.rotation = obj.rotation;
        t.scaleX = obj.scaleX;
        t.scaleY = obj.scaleY;
        m_Registry.AddComponent(obj.entity, t);
        m_Registry.AddComponent(obj.entity, RenderComponent{obj.color, obj.shape.getSize(), MapToShapeType(obj.objectType)});
        if (!obj.tag.empty())
            m_Registry.AddComponent(obj.entity, TagComponent{obj.tag});

        if (j.contains("sprite"))
        {
            std::string sp = j["sprite"].get<std::string>();
            std::filesystem::path p(sp);
            if (!p.is_absolute())
                sp = (std::filesystem::path(ASSET_PATH) / p).string();
            ApplySpriteToObject(obj, sp);
        }

        if (j.contains("velocity"))
            m_Registry.AddComponent(obj.entity, VelocityComponent{
                j["velocity"]["dx"], j["velocity"]["dy"]
            });

        if (j.contains("script"))
        {
            std::string sp = j["script"].get<std::string>();
            std::filesystem::path p(sp);
            if (!p.is_absolute())
                sp = (std::filesystem::path(ASSET_PATH) / p).string();
            auto &sc = m_Registry.AddComponent(obj.entity, ScriptComponent(LuaState::GetLua(), sp));
            sc.SetEntity(obj.entity);
            obj.scriptPath = sp;
        }

        if (j.contains("camera"))
            m_Registry.AddComponent(obj.entity, CameraComponent{true});

        if (j.contains("collision"))
        {
            CollisionType cType = CollisionType::Static;
            if (j["collision"].contains("type") && j["collision"]["type"] == "solid")
                cType = CollisionType::Solid;
            m_Registry.AddComponent(obj.entity, CollisionComponent{j["collision"]["channel"].get<int>(), cType});
        }

        m_Objects.push_back(std::move(obj));
    }

    std::cout << "[INFO] [EditorScene] Play-mode snapshot restored (" << m_Objects.size() << " objects).\n";
}

sf::Vector2f EditorScene::SnapToGrid(sf::Vector2f pos) const
{
    if (!m_SnapToGrid) return pos;
    return {
        std::floor(pos.x / m_GridSize) * m_GridSize,
        std::floor(pos.y / m_GridSize) * m_GridSize
    };
}

sf::Vector2f EditorScene::MouseWorldPos() const
{
    return m_Window.mapPixelToCoords(sf::Mouse::getPosition(m_Window), m_camera);
}

EditorObject *EditorScene::ObjectAt(sf::Vector2f pos)
{
    for (auto it = m_Objects.rbegin(); it != m_Objects.rend(); ++it)
        if (it->shape.getGlobalBounds().contains(pos))
            return &(*it);
    return nullptr;
}

void EditorScene::DrawGrid()
{
    if (!m_SnapToGrid) return;

    sf::VertexArray lines(sf::Lines);

    const sf::Vector2f center = m_camera.getCenter();
    const sf::Vector2f camSize = m_camera.getSize();

    const float left = std::floor((center.x - camSize.x / 2) / m_GridSize) * m_GridSize;
    const float top = std::floor((center.y - camSize.y / 2) / m_GridSize) * m_GridSize;
    const float right = center.x + camSize.x / 2 + m_GridSize;
    const float bottom = center.y + camSize.y / 2 + m_GridSize;

    const sf::Uint8 alpha = static_cast<sf::Uint8>(m_GridOpacity);
    const sf::Color gcMinor(C_GRID_MINOR.r, C_GRID_MINOR.g, C_GRID_MINOR.b, alpha);
    const sf::Color gcMajor(C_GRID_MAJOR.r, C_GRID_MAJOR.g, C_GRID_MAJOR.b, alpha);
    const float majorStep = m_GridSize * 4.f;

    for (float x = left; x < right; x += m_GridSize)
    {
        const bool major = (std::fmod(std::abs(x), majorStep) < 0.5f);
        const sf::Color &gc = major ? gcMajor : gcMinor;
        lines.append({{x, top}, gc});
        lines.append({{x, bottom}, gc});
    }
    for (float y = top; y < bottom; y += m_GridSize)
    {
        const bool major = (std::fmod(std::abs(y), majorStep) < 0.5f);
        const sf::Color &gc = major ? gcMajor : gcMinor;
        lines.append({{left, y}, gc});
        lines.append({{right, y}, gc});
    }

    m_Window.draw(lines);
}

static sf::Vector2f RotatePoint(sf::Vector2f point, sf::Vector2f center, float angleDegrees)
{
    float angleRad = angleDegrees * 3.14159265f / 180.f;
    float cosA = std::cos(angleRad);
    float sinA = std::sin(angleRad);
    
    sf::Vector2f translated = point - center;
    sf::Vector2f rotated;
    rotated.x = translated.x * cosA - translated.y * sinA;
    rotated.y = translated.x * sinA + translated.y * cosA;
    
    return rotated + center;
}

static sf::Vector2f HandlePos(const EditorObject* obj, int idx)
{
    sf::Vector2f p = obj->shape.getPosition();
    sf::Vector2f s = obj->shape.getSize();
    sf::Vector2f c = p + s * 0.5f;
    
    sf::Vector2f u;
    switch (idx)
    {
        case 0: u = {p.x, p.y}; break;
        case 1: u = {c.x, p.y}; break;
        case 2: u = {p.x + s.x, p.y}; break;
        case 3: u = {p.x, c.y}; break;
        case 4: u = {p.x + s.x, c.y}; break;
        case 5: u = {p.x, p.y + s.y}; break;
        case 6: u = {c.x, p.y + s.y}; break;
        case 7: u = {p.x + s.x, p.y + s.y}; break;
        default: u = {0.f, 0.f}; break;
    }
    
    return RotatePoint(u, p, obj->rotation);
}

int EditorScene::GetResizeHandle(sf::Vector2f worldPos) const
{
    if (!m_Selected) return -1;

    sf::Vector2i zeroScreen{0, 0};
    sf::Vector2i eightScreen{8, 0};
    const sf::Vector2f wZero = m_Window.mapPixelToCoords(zeroScreen, m_camera);
    const sf::Vector2f wEight = m_Window.mapPixelToCoords(eightScreen, m_camera);
    const float hitRadius = std::abs(wEight.x - wZero.x);

    for (int i = 0; i < 8; ++i)
    {
        const sf::Vector2f hp = HandlePos(m_Selected, i);
        const float dx = worldPos.x - hp.x;
        const float dy = worldPos.y - hp.y;
        if (std::sqrt(dx * dx + dy * dy) <= hitRadius)
            return i;
    }
    return -1;
}

void EditorScene::DrawResizeHandles(sf::RenderWindow &window)
{
    DrawGizmos(window);
}

void EditorScene::DrawGizmos(sf::RenderWindow &window)
{
    if (!m_Selected) return;

    sf::Vector2i zeroScreen{0, 0};
    sf::Vector2i tenScreen{10, 0};
    const sf::Vector2f wZero = m_Window.mapPixelToCoords(zeroScreen, m_camera);
    const sf::Vector2f wTen  = m_Window.mapPixelToCoords(tenScreen,  m_camera);
    const float hw = std::abs(wTen.x - wZero.x);

    sf::Vector2i fortyScreen{0, 40};
    const sf::Vector2f wForty = m_Window.mapPixelToCoords(fortyScreen, m_camera);
    const float rotOffset = std::abs(wForty.y - wZero.y);

    sf::Vector2f p = m_Selected->shape.getPosition();
    sf::Vector2f s = m_Selected->shape.getSize();
    sf::Vector2f center = p + s * 0.5f;
    sf::Vector2f unrotatedTopMid = {center.x, p.y};

    sf::Vector2f unrotatedRotateHandle = {unrotatedTopMid.x, unrotatedTopMid.y - rotOffset};
    m_RotateHandlePos = RotatePoint(unrotatedRotateHandle, p, m_Selected->rotation);
    
    sf::Vector2f topMid = RotatePoint(unrotatedTopMid, p, m_Selected->rotation);

    static const sf::Color C_SCALE_FILL  = sf::Color(255, 255, 255, 230);
    static const sf::Color C_SCALE_EDGE  = sf::Color(200, 200, 220, 180);
    static const sf::Color C_SCALE_OUTL  = sf::Color(100, 90, 200, 255);

    for (int i = 0; i < 8; ++i)
    {
        const sf::Vector2f hp = HandlePos(m_Selected, i);
        const bool isCorner = (i == 0 || i == 2 || i == 5 || i == 7);

        sf::RectangleShape handle({hw * 2.f, hw * 2.f});
        handle.setOrigin(hw, hw);
        handle.setPosition(hp);
        handle.setRotation(m_Selected->rotation);
        handle.setFillColor(isCorner ? C_SCALE_FILL : C_SCALE_EDGE);
        handle.setOutlineColor(C_SCALE_OUTL);
        handle.setOutlineThickness(hw * 0.3f);
        window.draw(handle);
    }

    static const sf::Color C_ROT_LINE = sf::Color(80, 220, 140, 180);
    sf::VertexArray line(sf::Lines, 2);
    line[0] = {topMid,          C_ROT_LINE};
    line[1] = {m_RotateHandlePos, C_ROT_LINE};
    window.draw(line);

    static const sf::Color C_ROT_FILL  = sf::Color(60, 210, 120, 230);
    static const sf::Color C_ROT_OUTL  = sf::Color(30, 160, 80, 255);
    const float rotR = hw * 1.4f;

    sf::CircleShape rotHandle(rotR);
    rotHandle.setOrigin(rotR, rotR);
    rotHandle.setPosition(m_RotateHandlePos);
    rotHandle.setFillColor(m_Rotating ? sf::Color(100, 255, 160, 245) : C_ROT_FILL);
    rotHandle.setOutlineColor(C_ROT_OUTL);
    rotHandle.setOutlineThickness(hw * 0.3f);
    window.draw(rotHandle);

    const float a = rotR * 0.55f;
    sf::VertexArray arc(sf::LinesStrip, 5);
    arc[0] = {{m_RotateHandlePos.x - a,       m_RotateHandlePos.y},        sf::Color(255,255,255,200)};
    arc[1] = {{m_RotateHandlePos.x - a * 0.5f, m_RotateHandlePos.y - a},   sf::Color(255,255,255,200)};
    arc[2] = {{m_RotateHandlePos.x,             m_RotateHandlePos.y - a},   sf::Color(255,255,255,200)};
    arc[3] = {{m_RotateHandlePos.x + a * 0.5f, m_RotateHandlePos.y - a},   sf::Color(255,255,255,200)};
    arc[4] = {{m_RotateHandlePos.x + a,        m_RotateHandlePos.y},        sf::Color(255,255,255,200)};
    window.draw(arc);
}

bool EditorScene::GetRotateHandle(sf::Vector2f worldPos) const
{
    if (!m_Selected) return false;

    sf::Vector2i zeroScreen{0, 0};
    sf::Vector2i tenScreen{14, 0};
    const sf::Vector2f wZero = m_Window.mapPixelToCoords(zeroScreen, m_camera);
    const sf::Vector2f wTen  = m_Window.mapPixelToCoords(tenScreen,  m_camera);
    const float hitRadius = std::abs(wTen.x - wZero.x);

    const float dx = worldPos.x - m_RotateHandlePos.x;
    const float dy = worldPos.y - m_RotateHandlePos.y;
    return std::sqrt(dx * dx + dy * dy) <= hitRadius;
}


void EditorScene::UpdateStatusText()
{
    std::string s = "Objects: " + std::to_string(m_Objects.size());
    s += "  Grid: " + std::string(m_SnapToGrid ? "ON" : "OFF");
    if (m_Selected)
        s += "  |  " + m_Selected->id
                + "  (" + std::to_string((int) m_Selected->shape.getPosition().x)
                + ", " + std::to_string((int) m_Selected->shape.getPosition().y) + ")";
    m_StatusText.setString(s);
}

std::string EditorScene::NextId() { return "obj_" + std::to_string(m_IdCounter++); }

void EditorScene::DrawSettingsWindow(sf::RenderWindow &window)
{
    m_SettingsButtons.clear();

    const float winW = std::min(740.f, static_cast<float>(m_Window.getSize().x) - 80.f);
    const float winH = std::min(560.f, static_cast<float>(m_Window.getSize().y) - 80.f);
    const float winX = (m_Window.getSize().x - winW) / 2.f;
    const float winY = (m_Window.getSize().y - winH) / 2.f;

    sf::RectangleShape dim({(float) m_Window.getSize().x, (float) m_Window.getSize().y});
    dim.setFillColor(sf::Color(0, 0, 0, 160));
    window.draw(dim);

    sf::RectangleShape bg({winW, winH});
    bg.setPosition(winX, winY);
    bg.setFillColor(C_BG_ELEVATED);
    bg.setOutlineColor(C_BORDER_LIGHT);
    bg.setOutlineThickness(1.f);
    window.draw(bg);

    const float titleH = 38.f;
    sf::RectangleShape titleBar({winW, titleH});
    titleBar.setPosition(winX, winY);
    titleBar.setFillColor(C_BG_ELEVATED);
    window.draw(titleBar);

    sf::RectangleShape titleBorder({winW, 1.f});
    titleBorder.setPosition(winX, winY + titleH - 1.f);
    titleBorder.setFillColor(C_BORDER);
    window.draw(titleBorder);

    sf::Text titleText;
    titleText.setFont(*m_Font);
    titleText.setCharacterSize(13);
    titleText.setFillColor(C_TEXT_PRIMARY);
    titleText.setStyle(sf::Text::Bold);
    titleText.setString("Settings");
    titleText.setPosition(winX + 16.f, winY + 11.f);
    window.draw(titleText);

    const sf::FloatRect closeRect(winX + winW - 34.f, winY + 6.f, 26.f, 26.f);
    const bool closeHov = closeRect.contains(m_MouseScreenPos);
    sf::RectangleShape closeBg({26.f, 26.f});
    closeBg.setPosition(closeRect.left, closeRect.top);
    closeBg.setFillColor(closeHov ? C_DANGER_DIM : sf::Color::Transparent);
    closeBg.setOutlineColor(closeHov ? C_DANGER : sf::Color::Transparent);
    closeBg.setOutlineThickness(1.f);
    window.draw(closeBg);
    sf::Text closeText;
    closeText.setFont(*m_Font);
    closeText.setCharacterSize(14);
    closeText.setFillColor(closeHov ? C_DANGER : C_TEXT_MUTED);
    closeText.setString("x");
    closeText.setPosition(closeRect.left + 8.f, closeRect.top + 4.f);
    window.draw(closeText);
    m_SettingsButtons.push_back({closeRect, "close_settings"});

    const float tabY = winY + titleH;
    const float tabH = 32.f;
    const float tabW = winW / 5.f;
    const std::vector<std::string> tabNames = {"General", "Editor", "Rendering", "Input", "Debug"};
    const std::vector<sf::Color> tabAccents = {
        C_ACCENT, C_ACCENT, C_ACCENT, C_ACCENT, C_ACCENT
    };

    sf::RectangleShape tabBar({winW, tabH});
    tabBar.setPosition(winX, tabY);
    tabBar.setFillColor(C_BG_PANEL);
    window.draw(tabBar);
    sf::RectangleShape tabBarBorder({winW, 1.f});
    tabBarBorder.setPosition(winX, tabY + tabH - 1.f);
    tabBarBorder.setFillColor(C_BORDER);
    window.draw(tabBarBorder);

    for (int i = 0; i < (int) tabNames.size(); i++)
    {
        const sf::FloatRect tabRect(winX + i * tabW, tabY, tabW, tabH);
        const bool active = (m_SettingsTab == i);
        const bool hovered = tabRect.contains(m_MouseScreenPos);

        if (active)
        {
            sf::RectangleShape tabBg({tabW, tabH});
            tabBg.setPosition(tabRect.left, tabRect.top);
            tabBg.setFillColor(C_ACCENT_DIM);
            window.draw(tabBg);

            sf::RectangleShape accentLine({tabW, 2.f});
            accentLine.setPosition(tabRect.left, tabRect.top + tabH - 2.f);
            accentLine.setFillColor(tabAccents[i]);
            window.draw(accentLine);
        } else if (hovered)
        {
            sf::RectangleShape tabBg({tabW, tabH});
            tabBg.setPosition(tabRect.left, tabRect.top);
            tabBg.setFillColor(C_BG_ELEVATED);
            window.draw(tabBg);
        }

        sf::Text tabText;
        tabText.setFont(*m_Font);
        tabText.setCharacterSize(12);
        tabText.setFillColor(active ? tabAccents[i] : hovered ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
        tabText.setString(tabNames[i]);
        tabText.setPosition(
            tabRect.left + (tabW - tabText.getLocalBounds().width) / 2.f,
            tabRect.top + (tabH - tabText.getLocalBounds().height) / 2.f - 2.f);
        window.draw(tabText);

        m_SettingsButtons.push_back({tabRect, "tab_" + std::to_string(i)});
    }

    const float contentX = winX + 16.f;
    const float contentY = tabY + tabH + 12.f;
    const float contentW = winW - 32.f;
    float y = contentY;

    const sf::Color accent = tabAccents[m_SettingsTab];

    if (m_SettingsTab == 0)
    {
        y = DrawSettingsSectionHeader(window, "AUTOSAVE", accent, contentX, y, contentW);
        y = DrawSettingsToggle(window, "Enable AutoSave", m_AutoSaveEnabled, "toggle_autosave", contentX, y, contentW);
        y = DrawSettingsInputField(window, "Interval (Seconds)",
                                   std::to_string(static_cast<int>(m_AutoSaveIntervalSeconds)),
                                   SettingsField::AutoSaveInterval, contentX, y, contentW);
        y = DrawSettingsToggle(window, "Show Popup", m_AutoSavePopupEnabled, "toggle_autopopup", contentX, y, contentW);
        y = DrawSettingsInputField(window, "Popup Duration (Seconds)",
                                   std::to_string(static_cast<int>(m_AutoSavePopupDuration)),
                                   SettingsField::AutoSavePopupDuration, contentX, y, contentW);

        y += 16.f;
        y = DrawSettingsSectionHeader(window, "SCENE", sf::Color(100, 180, 255), contentX, y, contentW);
        y = DrawSettingsInputField(window, "Save Path", m_SceneSavePath,
                                   SettingsField::SceneSavePath, contentX, y, contentW);

        sf::Text note;
        note.setFont(*m_Font);
        note.setCharacterSize(10);
        note.setFillColor(C_TEXT_MUTED);
        note.setString("Tip: ASSET_PATH is automatically prepended.");
        note.setPosition(contentX + 2.f, y + 4.f);
        window.draw(note);
        y += 22.f;

        y += 16.f;
        y = DrawSettingsSectionHeader(window, "DISPLAY", sf::Color(200, 200, 255), contentX, y, contentW);
        y = DrawSettingsToggle(window, "AutoSave Countdown in Title Bar",
                               m_ShowAutoSaveInTitle, "toggle_title_countdown", contentX, y, contentW);
    } else if (m_SettingsTab == 1)
    {
        y = DrawSettingsSectionHeader(window, "GRID", accent, contentX, y, contentW);
        y = DrawSettingsInputField(window, "Grid Size (px)",
                                   std::to_string(static_cast<int>(m_GridSize)),
                                   SettingsField::GridSize, contentX, y, contentW);
        y = DrawSettingsInputField(window, "Grid Opacity (0-255)",
                                   std::to_string(m_GridOpacity),
                                   SettingsField::GridOpacityVal, contentX, y, contentW); {
            sf::RectangleShape preview({60.f, 20.f});
            preview.setFillColor(m_GridColor);
            preview.setOutlineColor(C_BORDER_LIGHT);
            preview.setOutlineThickness(1.f);
            preview.setPosition(contentX + 2.f, y + 2.f);
            window.draw(preview);
            sf::Text colorLabel;
            colorLabel.setFont(*m_Font);
            colorLabel.setCharacterSize(11);
            colorLabel.setFillColor(C_TEXT_MUTED);
            colorLabel.setString("Grid Color (RGB " +
                                 std::to_string(m_GridColor.r) + ", " +
                                 std::to_string(m_GridColor.g) + ", " +
                                 std::to_string(m_GridColor.b) + ")");
            colorLabel.setPosition(contentX + 68.f, y + 5.f);
            window.draw(colorLabel);
            y += 30.f;
        }

        y += 12.f;
        y = DrawSettingsSectionHeader(window, "OBJECTS", sf::Color(80, 200, 120), contentX, y, contentW);
        y = DrawSettingsInputField(window, "Default Object Size (px)",
                                   std::to_string(static_cast<int>(m_DefaultObjectSize)),
                                   SettingsField::DefaultObjectSize, contentX, y, contentW);

        y += 12.f;
        y = DrawSettingsSectionHeader(window, "SELECTION", sf::Color(255, 220, 60), contentX, y, contentW);
        y = DrawSettingsInputField(window, "Outline Thickness",
                                   std::to_string(static_cast<int>(m_SelectionOutlineThickness)),
                                   SettingsField::SelectionThickness, contentX, y, contentW); {
            sf::RectangleShape preview({60.f, 20.f});
            preview.setFillColor(m_SelectionOutlineColor);
            preview.setOutlineColor(C_BORDER_LIGHT);
            preview.setOutlineThickness(1.f);
            preview.setPosition(contentX + 2.f, y + 2.f);
            window.draw(preview);
            sf::Text colorLabel;
            colorLabel.setFont(*m_Font);
            colorLabel.setCharacterSize(11);
            colorLabel.setFillColor(C_TEXT_MUTED);
            colorLabel.setString("Selection Color");
            colorLabel.setPosition(contentX + 68.f, y + 5.f);
            window.draw(colorLabel);
            y += 30.f;
        }

        y += 8.f; {
            const sf::FloatRect btnRect(contentX, y + 2.f, contentW, 26.f);
            const bool hov = btnRect.contains(m_MouseScreenPos);
            sf::RectangleShape btn({contentW, 26.f});
            btn.setPosition(contentX, y + 2.f);
            btn.setFillColor(hov ? C_BG_ELEVATED : C_BG_INPUT);
            btn.setOutlineColor(hov ? C_BORDER_LIGHT : C_BORDER);
            btn.setOutlineThickness(1.f);
            window.draw(btn);
            sf::Text btnText;
            btnText.setFont(*m_Font);
            btnText.setCharacterSize(11);
            btnText.setFillColor(hov ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
            btnText.setString("Reset Editor Defaults");
            btnText.setPosition(btnRect.left + (btnRect.width - btnText.getLocalBounds().width) / 2.f, y + 7.f);
            window.draw(btnText);
            m_SettingsButtons.push_back({btnRect, "reset_editor_defaults"});
            y += 34.f;
        }
    } else if (m_SettingsTab == 2)
    {
        y = DrawSettingsSectionHeader(window, "DISPLAY", accent, contentX, y, contentW);
        y = DrawSettingsToggle(window, "Show FPS", m_ShowFPS, "toggle_fps", contentX, y, contentW);

        const std::vector<std::string> fpsCaps = {"Unlimited", "60", "120", "144", "240"};
        y = DrawSettingsDropdown(window, "FPS Cap", fpsCaps, m_FPSCapIndex, "fps_cap", contentX, y, contentW); {
            static const int capValues[] = {0, 60, 120, 144, 240};
            m_Window.setFramerateLimit(static_cast<unsigned>(capValues[m_FPSCapIndex]));
        }

        y += 12.f;
        y = DrawSettingsSectionHeader(window, "CAMERA & ZOOM", sf::Color(100, 200, 255), contentX, y, contentW);
        y = DrawSettingsSlider(window, "Zoom Sensitivity", m_ZoomSensitivity,
                               0.01f, 0.5f, "zoom_sens", contentX, y, contentW);
        y = DrawSettingsInputField(window, "Zoom Min",
                                   [this] {
                                       char buf[32];
                                       snprintf(buf, 32, "%.2f", m_ZoomMin);
                                       return std::string(buf);
                                   }(),
                                   SettingsField::ZoomMin, contentX, y, contentW);
        y = DrawSettingsInputField(window, "Zoom Max",
                                   [this] {
                                       char buf[32];
                                       snprintf(buf, 32, "%.2f", m_ZoomMax);
                                       return std::string(buf);
                                   }(),
                                   SettingsField::ZoomMax, contentX, y, contentW);

        y += 8.f; {
            const sf::FloatRect btnRect(contentX, y + 2.f, contentW, 26.f);
            const bool hov = btnRect.contains(m_MouseScreenPos);
            sf::RectangleShape btn({contentW, 26.f});
            btn.setPosition(contentX, y + 2.f);
            btn.setFillColor(hov ? C_BG_ELEVATED : C_BG_INPUT);
            btn.setOutlineColor(hov ? C_BORDER_LIGHT : C_BORDER);
            btn.setOutlineThickness(1.f);
            window.draw(btn);
            sf::Text btnText;
            btnText.setFont(*m_Font);
            btnText.setCharacterSize(11);
            btnText.setFillColor(hov ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
            btnText.setString("Reset Camera (Center 0,0 / Zoom 1x)");
            btnText.setPosition(btnRect.left + (btnRect.width - btnText.getLocalBounds().width) / 2.f, y + 7.f);
            window.draw(btnText);
            m_SettingsButtons.push_back({btnRect, "reset_camera"});
            y += 34.f;
        }
    } else if (m_SettingsTab == 3)
    {
        y = DrawSettingsSectionHeader(window, "CAMERA NAVIGATION", accent, contentX, y, contentW);

        const std::vector<std::string> panOptions = {"Middle Mouse", "Right Mouse"};
        int panIdx = m_PanOnMiddleButton ? 0 : 1;
        y = DrawSettingsDropdown(window, "Pan Button", panOptions, panIdx, "pan_button", contentX, y, contentW);
        m_PanOnMiddleButton = (panIdx == 0);

        y = DrawSettingsToggle(window, "Invert Pan", m_InvertPan, "toggle_invert_pan", contentX, y, contentW);

        y += 12.f;
        y = DrawSettingsSectionHeader(window, "SCROLL", sf::Color(80, 200, 120), contentX, y, contentW);
        y = DrawSettingsSlider(window, "Scroll Sensitivity (Hierarchy)", m_ScrollSensitivity,
                               1.f, 100.f, "scroll_sens", contentX, y, contentW);
        y = DrawSettingsSlider(window, "Zoom Sensitivity (Mouse Wheel)", m_ZoomSensitivity,
                               0.01f, 0.5f, "zoom_sens2", contentX, y, contentW);

        y += 12.f;
        y = DrawSettingsSectionHeader(window, "KEYBOARD SHORTCUTS", sf::Color(200, 110, 230), contentX, y, contentW);

        struct Shortcut
        {
            std::string key, action;
        };
        const std::vector<Shortcut> shortcuts = {
            {"Ctrl+S", "Save scene"},
            {"Ctrl+L", "Load scene"},
            {"Ctrl+D", "Duplicate"},
            {"Ctrl+,", "Open settings"},
            {"G", "Toggle grid"},
            {"F5", "Run scene"},
            {"Esc", "Deselect / Close settings"},
            {"Del", "Delete object"},
            {"Scroll", "Zoom"},
            {"Mid/Right Drag", "Pan camera"},
        };
        for (const auto &sc: shortcuts)
        {
            sf::RectangleShape row({contentW, 20.f});
            row.setPosition(contentX, y);
            row.setFillColor(sf::Color::Transparent);
            window.draw(row);

            sf::Text keyT;
            keyT.setFont(*m_Font);
            keyT.setCharacterSize(11);
            keyT.setFillColor(C_ACCENT_BRIGHT);
            keyT.setString(sc.key);
            keyT.setPosition(contentX + 4.f, y + 2.f);
            window.draw(keyT);

            sf::Text actT;
            actT.setFont(*m_Font);
            actT.setCharacterSize(11);
            actT.setFillColor(C_TEXT_SECONDARY);
            actT.setString(sc.action);
            actT.setPosition(contentX + contentW * 0.36f, y + 2.f);
            window.draw(actT);

            y += 20.f;
            if (y > winY + winH - 20.f) break;
        }
    } else if (m_SettingsTab == 4)
    {
        y = DrawSettingsSectionHeader(window, "VIEWPORT OVERLAYS", accent, contentX, y, contentW);
        y = DrawSettingsToggle(window, "Show Entity IDs", m_ShowEntityIDs, "toggle_entity_ids", contentX, y, contentW);
        y = DrawSettingsToggle(window, "Show Collider Outlines", m_ShowColliderOutlines, "toggle_colliders", contentX,
                               y, contentW);
        y = DrawSettingsToggle(window, "Show FPS", m_ShowFPS, "toggle_fps2", contentX, y, contentW);

        y += 12.f;
        y = DrawSettingsSectionHeader(window, "CONSOLE LOG", sf::Color(100, 180, 255), contentX, y, contentW);
        const std::vector<std::string> logLevels = {"Off", "Errors Only", "Info", "Verbose"};
        y = DrawSettingsDropdown(window, "Log Level", logLevels, m_LogLevel, "log_level", contentX, y, contentW);

        y += 12.f;
        y = DrawSettingsSectionHeader(window, "EDITOR INFO", sf::Color(200, 200, 100), contentX, y, contentW);

        struct InfoRow
        {
            std::string key, value;
        };
        const std::vector<InfoRow> infos = {
            {"Engine", "RayneEngine"},
            {"Build", "Debug"},
            {"C++ Standard", "C++20"},
            {"SFML", "2.6.x"},
            {"Lua", "5.4.6"},
            {"Objects", std::to_string(m_Objects.size())},
            {"Grid Size", std::to_string(static_cast<int>(m_GridSize)) + " px"},
            {"AutoSave", m_AutoSaveEnabled ? "Enabled" : "Disabled"},
        };
        for (const auto &info: infos)
        {
            sf::Text kText;
            kText.setFont(*m_Font);
            kText.setCharacterSize(11);
            kText.setFillColor(C_TEXT_MUTED);
            kText.setString(info.key);
            kText.setPosition(contentX + 4.f, y + 2.f);
            window.draw(kText);

            sf::Text vText;
            vText.setFont(*m_Font);
            vText.setCharacterSize(11);
            vText.setFillColor(C_TEXT_PRIMARY);
            vText.setString(info.value);
            vText.setPosition(contentX + contentW * 0.45f, y + 2.f);
            window.draw(vText);

            y += 20.f;
        }
    }
}

void EditorScene::HandleSettingsClick(sf::Vector2f pos)
{
    m_MouseScreenPos = pos;
    for (auto &btn: m_SettingsButtons)
    {
        if (!btn.bounds.contains(pos)) continue;

        if (btn.action == "close_settings")
        {
            m_ShowSettings = false;
            m_ActiveSettingsField = SettingsField::None;
            m_SettingsInputText.clear();
        } else if (btn.action.rfind("tab_", 0) == 0)
        {
            m_SettingsTab = std::stoi(btn.action.substr(4));
            m_ActiveSettingsField = SettingsField::None;
            m_SettingsInputText.clear();
        } else if (btn.action == "toggle_autosave") { m_AutoSaveEnabled = !m_AutoSaveEnabled; } else if (
            btn.action == "toggle_autopopup") { m_AutoSavePopupEnabled = !m_AutoSavePopupEnabled; } else if (
            btn.action == "toggle_title_countdown") { m_ShowAutoSaveInTitle = !m_ShowAutoSaveInTitle; } else if (
            btn.action == "toggle_fps") { m_ShowFPS = !m_ShowFPS; } else if (
            btn.action == "toggle_fps2") { m_ShowFPS = !m_ShowFPS; } else if (
            btn.action == "toggle_entity_ids") { m_ShowEntityIDs = !m_ShowEntityIDs; } else if (
            btn.action == "toggle_colliders") { m_ShowColliderOutlines = !m_ShowColliderOutlines; } else if (
            btn.action == "toggle_invert_pan") { m_InvertPan = !m_InvertPan; } else if (btn.action == "reset_camera")
        {
            m_camera.setCenter(0.f, 0.f);
            m_camera.setSize(
                (1.f - (InspectorWidth + HierarchyWidth) / m_Window.getSize().x) * m_Window.getSize().x,
                (1.f - BrowserHeight / m_Window.getSize().y - TopBarHeight / m_Window.getSize().y) * m_Window.getSize().
                y
            );
        } else if (btn.action == "reset_editor_defaults")
        {
            m_GridSize = 40.f;
            m_DefaultObjectSize = 40.f;
            m_GridColor = sf::Color(38, 38, 52);
            m_GridOpacity = 255;
            m_SelectionOutlineColor = sf::Color(255, 220, 60);
            m_SelectionOutlineThickness = 2.f;
        } else if (btn.action.rfind("fps_cap", 0) == 0) {} else if (btn.action.rfind("pan_button", 0) == 0) {} else if (
            btn.action.rfind("log_level", 0) == 0) {} else if (btn.action.rfind("input_", 0) == 0)
        {
            std::string fieldStr = btn.action.substr(6);
            if (fieldStr == "AutoSaveInterval")
            {
                m_ActiveSettingsField = SettingsField::AutoSaveInterval;
                m_SettingsInputText = std::to_string(static_cast<int>(m_AutoSaveIntervalSeconds));
            } else if (fieldStr == "AutoSavePopupDuration")
            {
                m_ActiveSettingsField = SettingsField::AutoSavePopupDuration;
                m_SettingsInputText = std::to_string(static_cast<int>(m_AutoSavePopupDuration));
            } else if (fieldStr == "SceneSavePath")
            {
                m_ActiveSettingsField = SettingsField::SceneSavePath;
                m_SettingsInputText = m_SceneSavePath;
            } else if (fieldStr == "GridSize")
            {
                m_ActiveSettingsField = SettingsField::GridSize;
                m_SettingsInputText = std::to_string(static_cast<int>(m_GridSize));
            } else if (fieldStr == "DefaultObjectSize")
            {
                m_ActiveSettingsField = SettingsField::DefaultObjectSize;
                m_SettingsInputText = std::to_string(static_cast<int>(m_DefaultObjectSize));
            } else if (fieldStr == "SelectionThickness")
            {
                m_ActiveSettingsField = SettingsField::SelectionThickness;
                m_SettingsInputText = std::to_string(static_cast<int>(m_SelectionOutlineThickness));
            } else if (fieldStr == "GridOpacityVal")
            {
                m_ActiveSettingsField = SettingsField::GridOpacityVal;
                m_SettingsInputText = std::to_string(m_GridOpacity);
            } else if (fieldStr == "ZoomMin")
            {
                m_ActiveSettingsField = SettingsField::ZoomMin;
                char b[32];
                snprintf(b, 32, "%.2f", m_ZoomMin);
                m_SettingsInputText = b;
            } else if (fieldStr == "ZoomMax")
            {
                m_ActiveSettingsField = SettingsField::ZoomMax;
                char b[32];
                snprintf(b, 32, "%.2f", m_ZoomMax);
                m_SettingsInputText = b;
            } else if (fieldStr == "ScrollSensitivity")
            {
                m_ActiveSettingsField = SettingsField::ScrollSensitivity;
                m_SettingsInputText = std::to_string(static_cast<int>(m_ScrollSensitivity));
            }
        } else if (btn.action.rfind("slider_", 0) == 0)
        {
            std::string tag = btn.action.substr(7);
            const size_t sep = tag.rfind('_');
            if (sep != std::string::npos)
            {
                float norm = std::stof(tag.substr(sep + 1));
                std::string id = tag.substr(0, sep);
                if (id == "zoom_sens" || id == "zoom_sens2") m_ZoomSensitivity = 0.01f + norm * (0.5f - 0.01f);
                else if (id == "scroll_sens") m_ScrollSensitivity = 1.f + norm * (100.f - 1.f);
            }
        } else if (btn.action.rfind("dropdown_", 0) == 0)
        {
            std::string rest = btn.action.substr(9);
            const size_t sep = rest.rfind('_');
            if (sep != std::string::npos)
            {
                int idx = std::stoi(rest.substr(sep + 1));
                std::string id = rest.substr(0, sep);
                if (id == "fps_cap") m_FPSCapIndex = idx;
                else if (id == "pan_button") m_PanOnMiddleButton = (idx == 0);
                else if (id == "log_level") m_LogLevel = idx;
            }
        }
        SaveSettings();
        break;
    }
}

float EditorScene::DrawSettingsSectionHeader(sf::RenderWindow &window,
                                             const std::string &title, sf::Color accent, float x, float y, float winW)
{
    sf::RectangleShape bar({winW, 22.f});
    bar.setFillColor(sf::Color(accent.r / 10, accent.g / 10, accent.b / 10, 200));
    bar.setPosition(x, y);
    window.draw(bar);

    sf::RectangleShape accentLine({3.f, 22.f});
    accentLine.setFillColor(accent);
    accentLine.setPosition(x, y);
    window.draw(accentLine);

    sf::Text text;
    text.setFont(*m_Font);
    text.setCharacterSize(10);
    text.setFillColor(accent);
    text.setStyle(sf::Text::Bold);
    text.setString(title);
    text.setPosition(x + 10.f, y + 6.f);
    window.draw(text);

    return y + 28.f;
}

float EditorScene::DrawSettingsToggle(sf::RenderWindow &window,
                                      const std::string &label, bool &value, const std::string &action,
                                      float x, float y, float winW)
{
    const float rowH = 28.f;
    const bool hov = sf::FloatRect(x, y, winW, rowH).contains(m_MouseScreenPos);

    if (hov)
    {
        sf::RectangleShape rowBg({winW, rowH});
        rowBg.setPosition(x, y);
        rowBg.setFillColor(C_BG_ELEVATED);
        window.draw(rowBg);
    }

    sf::Text labelText;
    labelText.setFont(*m_Font);
    labelText.setCharacterSize(12);
    labelText.setFillColor(hov ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
    labelText.setString(label);
    labelText.setPosition(x + 8.f, y + 7.f);
    window.draw(labelText);

    const float pillW = 36.f, pillH = 18.f;
    const float pillX = x + winW - pillW - 8.f;
    const float pillY = y + (rowH - pillH) / 2.f;

    sf::Color pillColor = value ? C_ACCENT : C_BG_INPUT;
    sf::Color pillBorder = value ? sf::Color(60, 200, 100) : C_BORDER;
    sf::RectangleShape pill({pillW, pillH});
    pill.setPosition(pillX, pillY);
    pill.setFillColor(pillColor);
    pill.setOutlineColor(pillBorder);
    pill.setOutlineThickness(1.f);
    window.draw(pill);

    const float knobX = value ? pillX + pillW - pillH + 2.f : pillX + 2.f;
    sf::CircleShape knob(pillH / 2.f - 2.f);
    knob.setFillColor(sf::Color::White);
    knob.setPosition(knobX, pillY + 2.f);
    window.draw(knob);

    sf::RectangleShape line({winW, 1.f});
    line.setFillColor(sf::Color(C_BORDER.r, C_BORDER.g, C_BORDER.b, 60));
    line.setPosition(x, y + rowH - 1.f);
    window.draw(line);

    m_SettingsButtons.push_back({sf::FloatRect(x, y, winW, rowH), action});
    return y + rowH;
}

float EditorScene::DrawSettingsSlider(sf::RenderWindow &window,
                                      const std::string &label, float &value, float minVal, float maxVal,
                                      const std::string &action, float x, float y, float winW)
{
    const float rowH = 36.f;

    sf::Text labelText;
    labelText.setFont(*m_Font);
    labelText.setCharacterSize(12);
    labelText.setFillColor(C_TEXT_SECONDARY);
    labelText.setString(label);
    labelText.setPosition(x + 8.f, y + 4.f);
    window.draw(labelText);

    char buf[32];
    snprintf(buf, 32, "%.2f", value);
    sf::Text valText;
    valText.setFont(*m_Font);
    valText.setCharacterSize(11);
    valText.setFillColor(C_ACCENT_BRIGHT);
    valText.setString(buf);
    valText.setPosition(x + winW - 44.f, y + 4.f);
    window.draw(valText);

    const float trackX = x + 8.f;
    const float trackW = winW - 60.f;
    const float trackY = y + 22.f;
    const float trackH = 4.f;

    sf::RectangleShape track({trackW, trackH});
    track.setPosition(trackX, trackY);
    track.setFillColor(C_BG_INPUT);
    track.setOutlineColor(C_BORDER);
    track.setOutlineThickness(1.f);
    window.draw(track);

    float norm = (value - minVal) / (maxVal - minVal);
    norm = std::clamp(norm, 0.f, 1.f);

    sf::RectangleShape fill({norm * trackW, trackH});
    fill.setPosition(trackX, trackY);
    fill.setFillColor(C_ACCENT);
    window.draw(fill);

    const float knobX = trackX + norm * trackW - 6.f;
    sf::CircleShape knob(6.f);
    knob.setFillColor(C_ACCENT_BRIGHT);
    knob.setPosition(knobX, trackY - 4.f);
    window.draw(knob);

    const sf::FloatRect trackRect(trackX, trackY - 6.f, trackW, trackH + 12.f);
    if (trackRect.contains(m_MouseScreenPos) && sf::Mouse::isButtonPressed(sf::Mouse::Left))
    {
        float clickNorm = std::clamp((m_MouseScreenPos.x - trackX) / trackW, 0.f, 1.f);
        value = minVal + clickNorm * (maxVal - minVal);
    }

    sf::RectangleShape line({winW, 1.f});
    line.setFillColor(sf::Color(C_BORDER.r, C_BORDER.g, C_BORDER.b, 60));
    line.setPosition(x, y + rowH - 1.f);
    window.draw(line);

    return y + rowH;
}

float EditorScene::DrawSettingsInputField(sf::RenderWindow &window,
                                          const std::string &label, const std::string &currentVal,
                                          SettingsField field, float x, float y, float winW)
{
    const float rowH = 30.f;
    const bool active = (m_ActiveSettingsField == field);

    sf::Text labelText;
    labelText.setFont(*m_Font);
    labelText.setCharacterSize(12);
    labelText.setFillColor(C_TEXT_MUTED);
    labelText.setString(label);
    labelText.setPosition(x + 8.f, y + 7.f);
    window.draw(labelText);

    const float fieldW = winW * 0.38f;
    const float fieldX = x + winW - fieldW - 8.f;
    const sf::FloatRect fieldRect(fieldX, y + 4.f, fieldW, 22.f);
    const bool hov = fieldRect.contains(m_MouseScreenPos);

    sf::RectangleShape fieldBg({fieldW, 22.f});
    fieldBg.setPosition(fieldX, y + 4.f);
    fieldBg.setFillColor(active ? sf::Color(30, 30, 50) : hov ? C_BG_ELEVATED : C_BG_INPUT);
    fieldBg.setOutlineColor(active ? C_ACCENT : hov ? C_BORDER_LIGHT : C_BORDER);
    fieldBg.setOutlineThickness(1.f);
    window.draw(fieldBg);

    std::string display = active ? (m_SettingsInputText + "|") : currentVal;
    sf::Text valText;
    valText.setFont(*m_Font);
    valText.setCharacterSize(11);
    valText.setFillColor(active ? C_TEXT_PRIMARY : hov ? C_TEXT_PRIMARY : sf::Color(200, 200, 220));
    valText.setString(display);
    valText.setPosition(fieldX + 6.f, y + 7.f);
    window.draw(valText);

    static const std::map<SettingsField, std::string> fieldNames = {
        {SettingsField::AutoSaveInterval, "AutoSaveInterval"},
        {SettingsField::AutoSavePopupDuration, "AutoSavePopupDuration"},
        {SettingsField::SceneSavePath, "SceneSavePath"},
        {SettingsField::GridSize, "GridSize"},
        {SettingsField::DefaultObjectSize, "DefaultObjectSize"},
        {SettingsField::SelectionThickness, "SelectionThickness"},
        {SettingsField::GridOpacityVal, "GridOpacityVal"},
        {SettingsField::ZoomSensitivity, "ZoomSensitivity"},
        {SettingsField::ZoomMin, "ZoomMin"},
        {SettingsField::ZoomMax, "ZoomMax"},
        {SettingsField::ScrollSensitivity, "ScrollSensitivity"},
    };
    std::string actionStr = "input_";
    auto it = fieldNames.find(field);
    if (it != fieldNames.end()) actionStr += it->second;
    m_SettingsButtons.push_back({fieldRect, actionStr});

    sf::RectangleShape line({winW, 1.f});
    line.setFillColor(sf::Color(C_BORDER.r, C_BORDER.g, C_BORDER.b, 60));
    line.setPosition(x, y + rowH - 1.f);
    window.draw(line);

    return y + rowH;
}

float EditorScene::DrawSettingsDropdown(sf::RenderWindow &window,
                                        const std::string &label, const std::vector<std::string> &options,
                                        int &currentIdx, const std::string &action,
                                        float x, float y, float winW)
{
    const float rowH = 28.f;

    sf::Text labelText;
    labelText.setFont(*m_Font);
    labelText.setCharacterSize(12);
    labelText.setFillColor(C_TEXT_SECONDARY);
    labelText.setString(label);
    labelText.setPosition(x + 8.f, y + 6.f);
    window.draw(labelText);

    const float optW = winW * 0.44f;
    const float optH = 22.f;
    const float optItemH = 22.f;

    float optX = x + winW - optW - 8.f;

    for (int i = 0; i < (int) options.size(); i++)
    {
        const sf::FloatRect optRect(optX + i * (optW / options.size()), y + 3.f,
                                    optW / options.size() - 2.f, optH);
        const bool active = (currentIdx == i);
        const bool hov = optRect.contains(m_MouseScreenPos);

        sf::RectangleShape opt({optRect.width, optRect.height});
        opt.setPosition(optRect.left, optRect.top);
        opt.setFillColor(active ? C_ACCENT_DIM : hov ? C_BG_ELEVATED : C_BG_INPUT);
        opt.setOutlineColor(active ? C_ACCENT : hov ? C_BORDER_LIGHT : C_BORDER);
        opt.setOutlineThickness(1.f);
        window.draw(opt);

        sf::Text optText;
        optText.setFont(*m_Font);
        optText.setCharacterSize(10);
        optText.setFillColor(active ? C_ACCENT_BRIGHT : hov ? C_TEXT_PRIMARY : C_TEXT_MUTED);
        optText.setString(options[i]);
        optText.setPosition(
            optRect.left + (optRect.width - optText.getLocalBounds().width) / 2.f,
            optRect.top + 5.f);
        window.draw(optText);

        m_SettingsButtons.push_back({optRect, "dropdown_" + action + "_" + std::to_string(i)});
    }

    sf::RectangleShape line({winW, 1.f});
    line.setFillColor(sf::Color(C_BORDER.r, C_BORDER.g, C_BORDER.b, 60));
    line.setPosition(x, y + rowH - 1.f);
    window.draw(line);

    return y + rowH;
}

void EditorScene::SaveSettings()
{
    json data;
    data["general"]["autoSaveEnabled"] = m_AutoSaveEnabled;
    data["general"]["autoSaveIntervalSeconds"] = m_AutoSaveIntervalSeconds;
    data["general"]["autoSavePopupEnabled"] = m_AutoSavePopupEnabled;
    data["general"]["autoSavePopupDuration"] = m_AutoSavePopupDuration;

    data["editor"]["gridColor"] = {m_GridColor.r, m_GridColor.g, m_GridColor.b};
    data["editor"]["gridOpacity"] = m_GridOpacity;
    data["editor"]["editorBgColor"] = {m_EditorBgColor.r, m_EditorBgColor.g, m_EditorBgColor.b};
    data["editor"]["defaultObjectSize"] = m_DefaultObjectSize;
    data["editor"]["selectionOutlineColor"] = {
        m_SelectionOutlineColor.r, m_SelectionOutlineColor.g, m_SelectionOutlineColor.b
    };
    data["editor"]["selectionOutlineThickness"] = m_SelectionOutlineThickness;

    data["rendering"]["showFPS"] = m_ShowFPS;
    data["rendering"]["fpsCapIndex"] = m_FPSCapIndex;
    data["rendering"]["zoomSensitivity"] = m_ZoomSensitivity;
    data["rendering"]["zoomMin"] = m_ZoomMin;
    data["rendering"]["zoomMax"] = m_ZoomMax;

    data["input"]["panOnMiddleButton"] = m_PanOnMiddleButton;
    data["input"]["invertPan"] = m_InvertPan;
    data["input"]["scrollSensitivity"] = m_ScrollSensitivity;

    data["debug"]["showEntityIDs"] = m_ShowEntityIDs;
    data["debug"]["showColliderOutlines"] = m_ShowColliderOutlines;
    data["debug"]["logLevel"] = m_LogLevel;
    data["debug"]["showAutoSaveInTitle"] = m_ShowAutoSaveInTitle;

    std::string editorDir = ENGINE_ASSET_PATH "/editor";
    if (!std::filesystem::exists(editorDir)) { std::filesystem::create_directories(editorDir); }

    std::ofstream file(editorDir + "/editor_settings.json");
    if (file.is_open())
        file << data.dump(4);
}

void EditorScene::LoadSettings()
{
    std::string editorDir = ENGINE_ASSET_PATH "/editor";
    std::ifstream file(editorDir + "/editor_settings.json");
    if (!file.is_open()) return;

    json data;
    try
    {
        file >> data;

        if (data.contains("general"))
        {
            m_AutoSaveEnabled = data["general"].value("autoSaveEnabled", m_AutoSaveEnabled);
            m_AutoSaveIntervalSeconds = data["general"].value("autoSaveIntervalSeconds", m_AutoSaveIntervalSeconds);
            m_AutoSavePopupEnabled = data["general"].value("autoSavePopupEnabled", m_AutoSavePopupEnabled);
            m_AutoSavePopupDuration = data["general"].value("autoSavePopupDuration", m_AutoSavePopupDuration);
        }

        if (data.contains("editor"))
        {
            if (data["editor"].contains("gridColor"))
            {
                m_GridColor.r = data["editor"]["gridColor"][0];
                m_GridColor.g = data["editor"]["gridColor"][1];
                m_GridColor.b = data["editor"]["gridColor"][2];
            }
            m_GridOpacity = data["editor"].value("gridOpacity", m_GridOpacity);
            if (data["editor"].contains("editorBgColor"))
            {
                m_EditorBgColor.r = data["editor"]["editorBgColor"][0];
                m_EditorBgColor.g = data["editor"]["editorBgColor"][1];
                m_EditorBgColor.b = data["editor"]["editorBgColor"][2];
            }
            m_DefaultObjectSize = data["editor"].value("defaultObjectSize", m_DefaultObjectSize);
            if (data["editor"].contains("selectionOutlineColor"))
            {
                m_SelectionOutlineColor.r = data["editor"]["selectionOutlineColor"][0];
                m_SelectionOutlineColor.g = data["editor"]["selectionOutlineColor"][1];
                m_SelectionOutlineColor.b = data["editor"]["selectionOutlineColor"][2];
            }
            m_SelectionOutlineThickness = data["editor"].
                    value("selectionOutlineThickness", m_SelectionOutlineThickness);
        }

        if (data.contains("rendering"))
        {
            m_ShowFPS = data["rendering"].value("showFPS", m_ShowFPS);
            m_FPSCapIndex = data["rendering"].value("fpsCapIndex", m_FPSCapIndex);
            m_ZoomSensitivity = data["rendering"].value("zoomSensitivity", m_ZoomSensitivity);
            m_ZoomMin = data["rendering"].value("zoomMin", m_ZoomMin);
            m_ZoomMax = data["rendering"].value("zoomMax", m_ZoomMax);
        }

        if (data.contains("input"))
        {
            m_PanOnMiddleButton = data["input"].value("panOnMiddleButton", m_PanOnMiddleButton);
            m_InvertPan = data["input"].value("invertPan", m_InvertPan);
            m_ScrollSensitivity = data["input"].value("scrollSensitivity", m_ScrollSensitivity);
        }

        if (data.contains("debug"))
        {
            m_ShowEntityIDs = data["debug"].value("showEntityIDs", m_ShowEntityIDs);
            m_ShowColliderOutlines = data["debug"].value("showColliderOutlines", m_ShowColliderOutlines);
            m_LogLevel = data["debug"].value("logLevel", m_LogLevel);
            m_ShowAutoSaveInTitle = data["debug"].value("showAutoSaveInTitle", m_ShowAutoSaveInTitle);
        }
    } catch (...) {}
}

EditorObject* EditorScene::ObjectById(const std::string& id) {
    for (auto& obj : m_Objects) {
        if (obj.id == id) return &obj;
    }
    return nullptr;
}

void EditorScene::RemoveObject(const std::string& id) {
    for (auto it = m_Objects.begin(); it != m_Objects.end(); ++it) {
        if (it->id == id) {
            if (m_Selected == &(*it)) m_Selected = nullptr;
            if (it->entity != 0) m_Registry.DestroyEntity(it->entity);
            m_Objects.erase(it);
            return;
        }
    }
}

json EditorScene::SerializeObject(const EditorObject& obj) const {
    json j;
    j["id"] = obj.id;
    j["tag"] = obj.tag;
    std::string typeStr = "rectangle";
    if (obj.objectType == ObjectType::Circle) typeStr = "circle";
    else if (obj.objectType == ObjectType::Triangle) typeStr = "triangle";
    else if (obj.objectType == ObjectType::Pentagon) typeStr = "pentagon";
    else if (obj.objectType == ObjectType::Hexagon) typeStr = "hexagon";
    else if (obj.objectType == ObjectType::Sprite) typeStr = "sprite";
    j["type"] = typeStr;
    j["x"] = obj.shape.getPosition().x;
    j["y"] = obj.shape.getPosition().y;
    j["rotation"] = obj.rotation;
    j["scaleX"] = obj.scaleX;
    j["scaleY"] = obj.scaleY;
    j["width"] = obj.shape.getSize().x;
    j["height"] = obj.shape.getSize().y;

    j["color"] = {obj.color.r, obj.color.g, obj.color.b};

    if (!obj.spritePath.empty()) {
        std::error_code ec;
        std::filesystem::path p(obj.spritePath);
        std::filesystem::path root = std::filesystem::absolute(ASSET_PATH, ec);
        std::string rel = std::filesystem::proximate(p, root, ec).generic_string();
        j["sprite"] = ec ? obj.spritePath : rel;
    }

    if (obj.entity != 0 && m_Registry.HasComponent<VelocityComponent>(obj.entity)) {
        auto &vel = m_Registry.GetComponent<VelocityComponent>(obj.entity);
        j["velocity"] = {{"dx", vel.dx}, {"dy", vel.dy}};
    }

    if (!obj.scriptPath.empty()) {
        std::error_code ec;
        std::filesystem::path p(obj.scriptPath);
        std::filesystem::path root = std::filesystem::absolute(ASSET_PATH, ec);
        std::string rel = std::filesystem::proximate(p, root, ec).generic_string();
        j["script"] = ec ? obj.scriptPath : rel;
    }

    if (obj.entity != 0 && m_Registry.HasComponent<CameraComponent>(obj.entity)) { j["camera"] = true; }

    if (obj.entity != 0 && m_Registry.HasComponent<CollisionComponent>(obj.entity)) {
        auto &col = m_Registry.GetComponent<CollisionComponent>(obj.entity);
        j["collision"] = {
            {"channel", col.channel},
            {"type", col.type == CollisionType::Solid ? "solid" : "static"}
        };
    }
    return j;
}

void EditorScene::DeserializeObject(const json& j) {
    EditorObject obj;
    obj.id = j["id"];
    if (j.contains("tag")) obj.tag = j["tag"];
    obj.color = sf::Color(j["color"][0], j["color"][1], j["color"][2]);
    obj.shape.setSize({j["width"], j["height"]});
    obj.shape.setPosition(j["x"], j["y"]);
    if (j.contains("rotation")) obj.rotation = j["rotation"];
    if (j.contains("scaleX")) obj.scaleX = j["scaleX"];
    if (j.contains("scaleY")) obj.scaleY = j["scaleY"];
    obj.shape.setRotation(obj.rotation);
    obj.shape.setScale(obj.scaleX, obj.scaleY);
    obj.shape.setFillColor(obj.color);

    const std::string typeStr = j.value("type", "rectangle");
    if (typeStr == "circle") obj.objectType = ObjectType::Circle;
    else if (typeStr == "triangle") obj.objectType = ObjectType::Triangle;
    else if (typeStr == "pentagon") obj.objectType = ObjectType::Pentagon;
    else if (typeStr == "hexagon") obj.objectType = ObjectType::Hexagon;
    else if (typeStr == "sprite") obj.objectType = ObjectType::Sprite;
    else obj.objectType = ObjectType::Rectangle;

    if (IsPolygonType(obj.objectType)) {
        obj.circleShape.setPointCount(GetPolygonPointCount(obj.objectType));
        obj.circleShape.setRadius(obj.shape.getSize().x / 2.f);
        obj.circleShape.setPosition(obj.shape.getPosition());
        obj.circleShape.setRotation(obj.rotation);
        obj.circleShape.setScale(obj.scaleX, obj.scaleY);
        obj.circleShape.setFillColor(obj.color);
    }

    obj.entity = m_Registry.CreateEntity();
    
    TransformComponent t;
    t.x = j["x"];
    t.y = j["y"];
    t.rotation = obj.rotation;
    t.scaleX = obj.scaleX;
    t.scaleY = obj.scaleY;
    m_Registry.AddComponent(obj.entity, t);
    m_Registry.AddComponent(obj.entity, RenderComponent{obj.color, obj.shape.getSize(), MapToShapeType(obj.objectType)});
    if (!obj.tag.empty()) {
        m_Registry.AddComponent(obj.entity, TagComponent{obj.tag});
    }

    if (j.contains("sprite")) {
        std::string sp = j["sprite"].get<std::string>();
        std::filesystem::path p(sp);
        if (!p.is_absolute()) {
            sp = (std::filesystem::path(ASSET_PATH) / p).string();
        }
        ApplySpriteToObject(obj, sp);
    }

    if (j.contains("velocity"))
        m_Registry.AddComponent(obj.entity, VelocityComponent{
                                    j["velocity"]["dx"], j["velocity"]["dy"]
                                });

    if (j.contains("script")) {
        std::string sp = j["script"].get<std::string>();
        std::filesystem::path p(sp);
        if (!p.is_absolute()) {
            sp = (std::filesystem::path(ASSET_PATH) / p).string();
        }
        auto &sc = m_Registry.AddComponent(obj.entity, ScriptComponent(LuaState::GetLua(), sp));
        sc.SetEntity(obj.entity);
        obj.scriptPath = sp;
    }

    if (j.contains("camera")) { m_Registry.AddComponent(obj.entity, CameraComponent{true}); }

    if (j.contains("collision")) {
        CollisionType cType = CollisionType::Static;
        if (j["collision"].contains("type") && j["collision"]["type"] == "solid")
            cType = CollisionType::Solid;
        m_Registry.AddComponent(obj.entity, CollisionComponent{j["collision"]["channel"].get<int>(), cType});
    }

    m_Objects.push_back(std::move(obj));
    UpdateStatusText();
}

void EditorScene::ApplyState(EditorObject& obj, const json& j) {
    RemoveObject(obj.id);
    DeserializeObject(j);
}

void EditorScene::ExecuteCommand(std::shared_ptr<EditorCommand> command) {
    command->Execute(this);
    m_UndoStack.push_back(command);
    m_RedoStack.clear();
}

void EditorScene::UndoCommand() {
    if (!m_UndoStack.empty()) {
        auto cmd = m_UndoStack.back();
        m_UndoStack.pop_back();
        cmd->Undo(this);
        m_RedoStack.push_back(cmd);
        m_Selected = nullptr;
        UpdateStatusText();
    }
}

void EditorScene::RedoCommand() {
    if (!m_RedoStack.empty()) {
        auto cmd = m_RedoStack.back();
        m_RedoStack.pop_back();
        cmd->Execute(this);
        m_UndoStack.push_back(cmd);
        m_Selected = nullptr;
        UpdateStatusText();
    }
}

void ObjectStateCommand::Execute(EditorScene* scene) {
    if (afterState.is_null()) {
        scene->RemoveObject(objectId);
    } else {
        auto* obj = scene->ObjectById(objectId);
        if (obj) {
            scene->ApplyState(*obj, afterState);
        } else {
            scene->DeserializeObject(afterState);
        }
    }
}

void ObjectStateCommand::Undo(EditorScene* scene) {
    if (beforeState.is_null()) {
        scene->RemoveObject(objectId);
    } else {
        auto* obj = scene->ObjectById(objectId);
        if (obj) {
            scene->ApplyState(*obj, beforeState);
        } else {
            scene->DeserializeObject(beforeState);
        }
    }
}

void EditorScene::ClearSelection() {
    for (auto* o : m_SelectedObjects) o->selected = false;
    m_SelectedObjects.clear();
    m_Selected = nullptr;
}
void EditorScene::SelectObject(EditorObject* obj, bool multi) {
    if (!multi) ClearSelection();
    if (obj) {
        if (std::find(m_SelectedObjects.begin(), m_SelectedObjects.end(), obj) == m_SelectedObjects.end()) {
            m_SelectedObjects.push_back(obj);
            obj->selected = true;
        }
        m_Selected = m_SelectedObjects.size() == 1 ? m_SelectedObjects.back() : nullptr; 
    }
}
bool EditorScene::IsSelected(const EditorObject* obj) const {
    return std::find(m_SelectedObjects.begin(), m_SelectedObjects.end(), obj) != m_SelectedObjects.end();
}

static std::filesystem::path GetAppDir()
{
#ifdef _WIN32
    char pathBuf[MAX_PATH];
    if (GetModuleFileNameA(NULL, pathBuf, MAX_PATH)) {
        return std::filesystem::path(pathBuf).parent_path();
    }
#endif
    return std::filesystem::current_path();
}

static std::filesystem::path FindProjectRoot()
{
    std::error_code ec;
    std::filesystem::path cur = std::filesystem::current_path(ec);
    if (!ec && std::filesystem::exists(cur / "assets")) {
        return cur;
    }

    std::filesystem::path appDir = GetAppDir();
    if (std::filesystem::exists(appDir / "assets")) {
        return appDir;
    }

    return cur;
}

static std::string ResolveCMakeExecutable()
{
    if (system("cmake --version >nul 2>&1") == 0) {
        return "cmake";
    }
#ifdef _WIN32
    std::vector<std::string> candidates = {
        "\"C:\\Program Files\\JetBrains\\CLion 2025.1\\bin\\cmake\\win\\x64\\bin\\cmake.exe\"",
        "\"C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe\"",
        "\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe\""
    };
    for (const auto& c : candidates) {
        std::string test = c + " --version >nul 2>&1";
        if (system(test.c_str()) == 0) {
            return c;
        }
    }
#else
    if (system("cmake --version >/dev/null 2>&1") == 0) {
        return "cmake";
    }
#endif
    return "";
}

void EditorScene::ExportStandaloneGame()
{
    SyncToRegistry();
    SaveToJson(std::string(ASSET_PATH) + "/" + m_SceneSavePath);
    SaveProjectSettings();
    ConsolePanel::AddLogGlobal("[INFO] Auto-saved scene and project settings before build.", false);
    ConsolePanel::AddLogGlobal("Starting standalone game build and export...", false);
    
    m_ShowBuildPopup = true;
    m_BuildFinished = false;
    m_BuildProgress = 0.0f;
    m_CancelBuildRequested = false;
    m_BuildStatusText = "Preparing build...";
    
    std::thread([this]() {
        auto updateStatus = [this](const std::string& text, float progress) {
            std::lock_guard<std::mutex> lock(m_BuildMutex);
            m_BuildStatusText = text;
            if (progress >= 0.0f) m_BuildProgress = progress;
        };
        
        auto checkCancel = [this]() -> bool {
            std::lock_guard<std::mutex> lock(m_BuildMutex);
            return m_CancelBuildRequested;
        };

        std::filesystem::path rootDir = FindProjectRoot();
        std::filesystem::path appDir = GetAppDir();
        std::string cmakeBin = ResolveCMakeExecutable();
        
        std::filesystem::path buildDir;
        if (std::filesystem::exists(rootDir / "CMakeLists.txt")) {
            if (std::filesystem::exists(rootDir / "cmake-build-debug" / "CMakeCache.txt")) {
                buildDir = rootDir / "cmake-build-debug";
            } else if (std::filesystem::exists(rootDir / "cmake-build-release" / "CMakeCache.txt")) {
                buildDir = rootDir / "cmake-build-release";
            } else if (std::filesystem::exists(rootDir / "build" / "CMakeCache.txt")) {
                buildDir = rootDir / "build";
            }
        }

        if (!cmakeBin.empty() && !buildDir.empty()) {
            updateStatus("Building RayneGame target with CMake...", 20.0f);
            ConsolePanel::AddLogGlobal("Building RayneGame in: " + buildDir.string(), false);
            
            std::string cmdBuild = cmakeBin + " --build \"" + buildDir.string() + "\" --target RayneGame 2>&1";
            FILE* pipe =
#ifdef _WIN32
                _popen(cmdBuild.c_str(), "r");
#else
                popen(cmdBuild.c_str(), "r");
#endif
            if (pipe) {
                char buffer[256];
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    if (checkCancel()) break;
                    std::string line = buffer;
                    if (!line.empty() && line.back() == '\n') line.pop_back();
                    ConsolePanel::AddLogGlobal(line, false);
                    updateStatus(line, -1.f);
                }
#ifdef _WIN32
                _pclose(pipe);
#else
                pclose(pipe);
#endif
            }
        }

        if (checkCancel()) {
            updateStatus("Export cancelled.", 0.0f);
            std::lock_guard<std::mutex> lock(m_BuildMutex);
            m_BuildFinished = true;
            return;
        }

        std::filesystem::path exePath;
        std::vector<std::filesystem::path> possibleExePaths = {
            appDir / "templates" / "RayneGame.exe",
            rootDir / "templates" / "RayneGame.exe",
            appDir / "RayneGame.exe",
            rootDir / "RayneGame.exe"
        };
        if (!buildDir.empty()) {
            possibleExePaths.push_back(buildDir / "RayneGame.exe");
            possibleExePaths.push_back(buildDir / "Release" / "RayneGame.exe");
            possibleExePaths.push_back(buildDir / "Debug" / "RayneGame.exe");
        }
        
        for (const auto& p : possibleExePaths) {
            if (!p.empty() && std::filesystem::exists(p)) {
                exePath = p;
                break;
            }
        }

        if (!exePath.empty()) {
            updateStatus("Packaging game into Export/...", 70.0f);
            try {
                std::filesystem::path exportDir = rootDir / "Export";
                std::filesystem::create_directories(exportDir);

                std::string outExeName = m_ProjectName.empty() ? "RayneGame.exe" : m_ProjectName + ".exe";
                std::filesystem::copy_file(exePath, exportDir / outExeName, std::filesystem::copy_options::overwrite_existing);

                updateStatus("Copying assets...", 80.0f);
                std::filesystem::path srcAssets = std::filesystem::exists(rootDir / "assets") ? (rootDir / "assets") : (appDir / "assets");
                if (std::filesystem::exists(srcAssets)) {
                    std::filesystem::copy(srcAssets, exportDir / "assets", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
                }
                
                updateStatus("Copying engine_content...", 90.0f);
                std::filesystem::path srcEngine = std::filesystem::exists(rootDir / "engine_content") ? (rootDir / "engine_content") : (appDir / "engine_content");
                if (std::filesystem::exists(srcEngine)) {
                    std::filesystem::copy(srcEngine, exportDir / "engine_content", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
                }

                std::vector<std::filesystem::path> dllSearchDirs = {
                    appDir,
                    exePath.parent_path(),
                    rootDir
                };
                if (!buildDir.empty()) {
                    dllSearchDirs.push_back(buildDir);
                }
                for (const auto& sDir : dllSearchDirs) {
                    if (std::filesystem::exists(sDir)) {
                        for (const auto& entry : std::filesystem::directory_iterator(sDir)) {
                            if (entry.path().extension() == ".dll") {
                                std::filesystem::copy_file(entry.path(), exportDir / entry.path().filename(), std::filesystem::copy_options::overwrite_existing);
                            }
                        }
                    }
                }

                std::string zipName = (m_ProjectName.empty() ? "RayneGame" : m_ProjectName) + "_Standalone.zip";
                std::filesystem::path zipTarget = rootDir / zipName;
                bool zipCreated = false;
#ifdef _WIN32
                std::string tarCmd = "cd /d \"" + exportDir.string() + "\" && tar.exe -a -cf \"" + zipTarget.string() + "\" *";
                FILE* pZip = _popen(tarCmd.c_str(), "r");
                if (pZip) {
                    int ret = _pclose(pZip);
                    if (ret == 0 && std::filesystem::exists(zipTarget)) zipCreated = true;
                }
#endif
                if (!zipCreated && !cmakeBin.empty()) {
                    std::string zipCmd = cmakeBin + " -E tar cf \"" + zipTarget.string() + "\" --format=zip .";
#ifdef _WIN32
                    FILE* pZip = _popen(("cd /d \"" + exportDir.string() + "\" && " + zipCmd).c_str(), "r");
                    if (pZip) _pclose(pZip);
#else
                    FILE* pZip = popen(("cd \"" + exportDir.string() + "\" && " + zipCmd).c_str(), "r");
                    if (pZip) pclose(pZip);
#endif
                    if (std::filesystem::exists(zipTarget)) zipCreated = true;
                }
                if (zipCreated) {
                    ConsolePanel::AddLogGlobal("Standalone game zip created: " + zipTarget.string(), false);
                }

                updateStatus("Export finished! Exported to Export/ folder.", 100.0f);
                ConsolePanel::AddLogGlobal("Standalone game exported successfully to: " + exportDir.string(), false);

#ifdef _WIN32
                ShellExecuteA(NULL, "explore", exportDir.string().c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif
            } catch (const std::exception& e) {
                updateStatus(std::string("Packaging error: ") + e.what(), 100.0f);
                ConsolePanel::AddLogGlobal(std::string("Export error: ") + e.what(), true);
            }
        } else {
            updateStatus("Export failed: templates/RayneGame.exe not found.", 0.0f);
            ConsolePanel::AddLogGlobal("Export failed: templates/RayneGame.exe not found. Please ensure templates/RayneGame.exe is present.", true);
        }

        {
            std::lock_guard<std::mutex> lock(m_BuildMutex);
            m_BuildFinished = true;
        }
    }).detach();
}

void EditorScene::PackageEngineZip()
{
    {
        std::lock_guard<std::mutex> lock(m_BuildMutex);
        if (m_ShowBuildPopup && !m_BuildFinished) {
            ConsolePanel::AddLogGlobal("Another packaging/build task is already in progress.", true);
            return;
        }
        m_ShowBuildPopup = true;
        m_BuildFinished = false;
        m_CancelBuildRequested = false;
        m_BuildProgress = 0.0f;
        m_BuildStatusText = "Starting Engine packaging...";
    }

    std::thread([this]() {
        auto updateStatus = [this](const std::string& status, float progress) {
            std::lock_guard<std::mutex> lock(m_BuildMutex);
            m_BuildStatusText = status;
            if (progress >= 0.0f) m_BuildProgress = progress;
        };

        auto checkCancel = [this]() {
            std::lock_guard<std::mutex> lock(m_BuildMutex);
            return m_CancelBuildRequested;
        };

        try {
            updateStatus("Locating project and engine files...", 10.0f);
            std::filesystem::path rootDir = FindProjectRoot();
            std::filesystem::path appDir = GetAppDir();
            std::string cmakeBin = ResolveCMakeExecutable();

            std::filesystem::path engineExeDir = appDir;
            if (!std::filesystem::exists(engineExeDir / "RayneEngine.exe")) {
                if (std::filesystem::exists(rootDir / "RayneEngine.exe")) {
                    engineExeDir = rootDir;
                } else if (std::filesystem::exists(rootDir / "cmake-build-release" / "RayneEngine.exe")) {
                    engineExeDir = rootDir / "cmake-build-release";
                } else if (std::filesystem::exists(rootDir / "cmake-build-debug" / "RayneEngine.exe")) {
                    engineExeDir = rootDir / "cmake-build-debug";
                }
            }

            updateStatus("Preparing package contents...", 30.0f);
            std::filesystem::path zipTarget = rootDir / "RayneEngine-Editor.zip";

            updateStatus("Packaging RayneEngine into " + zipTarget.filename().string() + "...", 50.0f);
            ConsolePanel::AddLogGlobal("Packaging RayneEngine Editor to: " + zipTarget.string(), false);

            bool packaged = false;
#ifdef _WIN32
            std::string tarCmd = "cd /d \"" + engineExeDir.string() + "\" && tar.exe -a -cf \"" + zipTarget.string() + "\" RayneEngine.exe assets engine_content templates *.dll";
            FILE* pPipe = _popen(tarCmd.c_str(), "r");
            if (pPipe) {
                int ret = _pclose(pPipe);
                if (ret == 0 && std::filesystem::exists(zipTarget)) packaged = true;
            }
#endif
            if (!packaged && !cmakeBin.empty()) {
                std::string cmd = cmakeBin + " -E tar cf \"" + zipTarget.string() + "\" --format=zip \"RayneEngine.exe\" \"assets\" \"engine_content\" \"templates\"";
#ifdef _WIN32
                for (const auto& entry : std::filesystem::directory_iterator(engineExeDir)) {
                    if (entry.path().extension() == ".dll") {
                        cmd += " \"" + entry.path().filename().string() + "\"";
                    }
                }
                std::string fullCmd = "cd /d \"" + engineExeDir.string() + "\" && " + cmd;
                FILE* pipe = _popen(fullCmd.c_str(), "r");
                if (pipe) {
                    char buf[256];
                    while (fgets(buf, sizeof(buf), pipe) != nullptr) {}
                    _pclose(pipe);
                }
#else
                std::string fullCmd = "cd \"" + engineExeDir.string() + "\" && " + cmd;
                FILE* pipe = popen(fullCmd.c_str(), "r");
                if (pipe) {
                    char buf[256];
                    while (fgets(buf, sizeof(buf), pipe) != nullptr) {}
                    pclose(pipe);
                }
#endif
                if (std::filesystem::exists(zipTarget)) packaged = true;
            }

            updateStatus("Engine packaged successfully!", 100.0f);
            ConsolePanel::AddLogGlobal("RayneEngine-Editor.zip created successfully in: " + rootDir.string(), false);

#ifdef _WIN32
            ShellExecuteA(NULL, "explore", rootDir.string().c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif
        } catch (const std::exception& e) {
            updateStatus(std::string("Packaging error: ") + e.what(), 100.0f);
            ConsolePanel::AddLogGlobal(std::string("Package error: ") + e.what(), true);
        }

        {
            std::lock_guard<std::mutex> lock(m_BuildMutex);
            m_BuildFinished = true;
        }
    }).detach();
}



void EditorScene::DrawBuildPopup(sf::RenderWindow& window) {
    if (!m_ShowBuildPopup) return;
    
    sf::RectangleShape bg(sf::Vector2f(window.getSize().x, window.getSize().y));
    bg.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(bg);
    
    float pW = 500.f;
    float pH = 180.f;
    float pX = (window.getSize().x - pW) / 2.f;
    float pY = (window.getSize().y - pH) / 2.f;
    
    sf::RectangleShape panel(sf::Vector2f(pW, pH));
    panel.setPosition(pX, pY);
    panel.setFillColor(C_BG_PANEL);
    panel.setOutlineColor(C_BORDER);
    panel.setOutlineThickness(1.f);
    window.draw(panel);
    
    std::string statusText;
    float progress = 0.0f;
    bool finished = false;
    {
        std::lock_guard<std::mutex> lock(m_BuildMutex);
        statusText = m_BuildStatusText;
        progress = m_BuildProgress;
        finished = m_BuildFinished;
    }
    
    std::string headerStr = (statusText.find("Engine") != std::string::npos || statusText.find("RayneEngine") != std::string::npos)
        ? "Packaging RayneEngine (.zip)"
        : "Building Standalone Game";
    sf::Text header(headerStr, *m_Font, 16);
    header.setPosition(pX + 20.f, pY + 20.f);
    header.setFillColor(C_TEXT_PRIMARY);
    window.draw(header);
    
    sf::Text status(statusText, *m_Font, 12);
    status.setPosition(pX + 20.f, pY + 60.f);
    status.setFillColor(C_TEXT_SECONDARY);
    window.draw(status);
    
    float barW = pW - 40.f;
    float barH = 20.f;
    sf::RectangleShape barBg(sf::Vector2f(barW, barH));
    barBg.setPosition(pX + 20.f, pY + 90.f);
    barBg.setFillColor(C_BG_INPUT);
    barBg.setOutlineColor(C_BORDER);
    barBg.setOutlineThickness(1.f);
    window.draw(barBg);
    
    if (progress > 0.0f) {
        sf::RectangleShape barFill(sf::Vector2f(barW * (progress / 100.f), barH));
        barFill.setPosition(pX + 20.f, pY + 90.f);
        barFill.setFillColor(C_ACCENT);
        window.draw(barFill);
    }
    
    float btnW = 100.f;
    float btnH = 30.f;
    float btnX = pX + pW / 2.f - btnW / 2.f;
    float btnY = pY + pH - 20.f - btnH;
    
    sf::RectangleShape btn(sf::Vector2f(btnW, btnH));
    btn.setPosition(btnX, btnY);
    
    sf::Vector2f mPos = MouseWorldPos();
    mPos = {(float)sf::Mouse::getPosition(window).x, (float)sf::Mouse::getPosition(window).y};
    bool hoverBtn = btn.getGlobalBounds().contains(mPos);
    
    btn.setFillColor(hoverBtn ? C_ACCENT_HOV : C_ACCENT);
    window.draw(btn);
    
    sf::Text btnText(finished ? "Close" : "Cancel", *m_Font, 12);
    btnText.setFillColor(sf::Color::White);
    btnText.setPosition(
        btnX + btnW / 2.f - btnText.getGlobalBounds().width / 2.f,
        btnY + btnH / 2.f - btnText.getGlobalBounds().height / 2.f - 4.f
    );
    window.draw(btnText);
}

static std::string ColorToHex(sf::Color c) {
    char buf[12];
    snprintf(buf, sizeof(buf), "#%02X%02X%02X", c.r, c.g, c.b);
    return std::string(buf);
}

static sf::Color HexToColor(const std::string& hex, sf::Color def = sf::Color(18, 20, 23)) {
    if (hex.empty()) return def;
    std::string s = hex;
    if (s[0] == '#') s = s.substr(1);
    if (s.size() != 6) return def;
    try {
        unsigned int val = std::stoul(s, nullptr, 16);
        return sf::Color((val >> 16) & 0xFF, (val >> 8) & 0xFF, val & 0xFF);
    } catch (...) {
        return def;
    }
}

void EditorScene::LoadProjectSettings() {
    std::string path = std::string(ENGINE_ASSET_PATH) + "/project_settings.json";
    if (std::filesystem::exists(path)) {
        try {
            std::ifstream f(path);
            json j;
            f >> j;
            m_ProjectName = j.value("ProjectName", "RayneGame");
            m_ProjectVersion = j.value("Version", "1.0.0");
            m_ProjectAuthor = j.value("Author", "");
            m_ProjectStartScene = j.value("StartScene", "scenes/game.json");
            m_ProjectWindowWidth = j.value("WindowWidth", 1280);
            m_ProjectWindowHeight = j.value("WindowHeight", 720);
            m_ProjectVSync = j.value("VSync", true);
            m_ProjectTargetFPS = j.value("TargetFPS", 60);
            m_ProjectFullscreen = j.value("Fullscreen", false);
            if (j.contains("ClearColor")) {
                m_ProjectClearColor = HexToColor(j["ClearColor"].get<std::string>(), sf::Color(18, 20, 23));
            }
            m_ProjectMasterVolume = j.value("MasterVolume", 100.f);
            m_ProjectMusicVolume = j.value("MusicVolume", 100.f);
        } catch (...) {
            std::cout << "[ERROR] Failed to load project settings\n";
        }
    }

    if (g_App) {
        g_App->SetProjectName(m_ProjectName);
        g_App->SetProjectVersion(m_ProjectVersion);
        g_App->SetProjectAuthor(m_ProjectAuthor);
        g_App->SetVSync(m_ProjectVSync);
        g_App->SetTargetFPS(m_ProjectTargetFPS);
        g_App->SetClearColor(m_ProjectClearColor);
        g_App->SetMasterVolume(m_ProjectMasterVolume);
        g_App->SetMusicVolume(m_ProjectMusicVolume);
    }
}

void EditorScene::SaveProjectSettings() {
    std::string path = std::string(ENGINE_ASSET_PATH) + "/project_settings.json";
    try {
        json j;
        j["ProjectName"] = m_ProjectName;
        j["Version"] = m_ProjectVersion;
        j["Author"] = m_ProjectAuthor;
        j["StartScene"] = m_ProjectStartScene;
        j["WindowWidth"] = m_ProjectWindowWidth;
        j["WindowHeight"] = m_ProjectWindowHeight;
        j["VSync"] = m_ProjectVSync;
        j["TargetFPS"] = m_ProjectTargetFPS;
        j["Fullscreen"] = m_ProjectFullscreen;
        j["ClearColor"] = ColorToHex(m_ProjectClearColor);
        j["MasterVolume"] = m_ProjectMasterVolume;
        j["MusicVolume"] = m_ProjectMusicVolume;

        std::ofstream f(path);
        f << j.dump(4);
    } catch (...) {
        std::cout << "[ERROR] Failed to save project settings\n";
    }

    if (g_App) {
        g_App->SetProjectName(m_ProjectName);
        g_App->SetProjectVersion(m_ProjectVersion);
        g_App->SetProjectAuthor(m_ProjectAuthor);
        g_App->SetVSync(m_ProjectVSync);
        g_App->SetTargetFPS(m_ProjectTargetFPS);
        g_App->SetClearColor(m_ProjectClearColor);
        g_App->SetMasterVolume(m_ProjectMasterVolume);
        g_App->SetMusicVolume(m_ProjectMusicVolume);
    }
}

void EditorScene::CommitActiveProjectSettingsField() {
    if (m_ActiveProjectSettingsField == ProjectSettingsField::None) return;

    if (m_ActiveProjectSettingsField == ProjectSettingsField::ProjectName) {
        if (!m_ProjectSettingsInputText.empty()) {
            m_ProjectName = m_ProjectSettingsInputText;
        }
    } else if (m_ActiveProjectSettingsField == ProjectSettingsField::Version) {
        if (!m_ProjectSettingsInputText.empty()) {
            m_ProjectVersion = m_ProjectSettingsInputText;
        }
    } else if (m_ActiveProjectSettingsField == ProjectSettingsField::Author) {
        m_ProjectAuthor = m_ProjectSettingsInputText;
    } else if (m_ActiveProjectSettingsField == ProjectSettingsField::StartScene) {
        if (!m_ProjectSettingsInputText.empty()) {
            m_ProjectStartScene = m_ProjectSettingsInputText;
        }
    } else if (m_ActiveProjectSettingsField == ProjectSettingsField::WindowWidth) {
        try { int v = std::stoi(m_ProjectSettingsInputText); if (v > 100) m_ProjectWindowWidth = v; } catch(...) {}
    } else if (m_ActiveProjectSettingsField == ProjectSettingsField::WindowHeight) {
        try { int v = std::stoi(m_ProjectSettingsInputText); if (v > 100) m_ProjectWindowHeight = v; } catch(...) {}
    } else if (m_ActiveProjectSettingsField == ProjectSettingsField::TargetFPS) {
        try { int v = std::stoi(m_ProjectSettingsInputText); if (v >= 0) m_ProjectTargetFPS = v; } catch(...) {}
    } else if (m_ActiveProjectSettingsField == ProjectSettingsField::ClearColorHex) {
        m_ProjectClearColor = HexToColor(m_ProjectSettingsInputText, m_ProjectClearColor);
    } else if (m_ActiveProjectSettingsField == ProjectSettingsField::MasterVolume) {
        try { float v = std::stof(m_ProjectSettingsInputText); m_ProjectMasterVolume = std::clamp(v, 0.f, 100.f); } catch(...) {}
    } else if (m_ActiveProjectSettingsField == ProjectSettingsField::MusicVolume) {
        try { float v = std::stof(m_ProjectSettingsInputText); m_ProjectMusicVolume = std::clamp(v, 0.f, 100.f); } catch(...) {}
    }

    m_ActiveProjectSettingsField = ProjectSettingsField::None;
    m_ProjectSettingsInputText.clear();
    SaveProjectSettings();
}

float EditorScene::DrawProjectSettingsToggle(sf::RenderWindow &window, const std::string &label,
                                             bool value, const std::string &action,
                                             float x, float y, float winW)
{
    sf::Text tLabel(label, *m_Font, 11);
    tLabel.setPosition(x, y + 4.f);
    tLabel.setFillColor(C_TEXT_SECONDARY);
    window.draw(tLabel);

    float inX = x + 160.f;
    sf::RectangleShape togBtn(sf::Vector2f(44.f, 22.f));
    togBtn.setPosition(inX, y);
    togBtn.setFillColor(value ? C_ACCENT : C_BG_INPUT);
    togBtn.setOutlineColor(C_BORDER);
    togBtn.setOutlineThickness(1.f);
    window.draw(togBtn);

    sf::CircleShape knob(8.f);
    knob.setFillColor(C_TEXT_PRIMARY);
    knob.setPosition(value ? (inX + 24.f) : (inX + 4.f), y + 3.f);
    window.draw(knob);

    sf::Text statusText(value ? "ON" : "OFF", *m_Font, 10);
    statusText.setPosition(inX + 52.f, y + 4.f);
    statusText.setFillColor(value ? C_SUCCESS : C_TEXT_MUTED);
    window.draw(statusText);

    m_ProjectSettingsButtons.push_back({togBtn.getGlobalBounds(), action});
    return 32.f;
}

float EditorScene::DrawProjectSettingsInputField(sf::RenderWindow &window, const std::string &label,
                                                 const std::string &currentVal, ProjectSettingsField field,
                                                 float x, float y, float winW)
{
    sf::Text tLabel(label, *m_Font, 11);
    tLabel.setPosition(x, y + 4.f);
    tLabel.setFillColor(C_TEXT_SECONDARY);
    window.draw(tLabel);

    float inX = x + 160.f;
    float inW = winW - inX - 20.f;

    sf::RectangleShape box(sf::Vector2f(inW, 22.f));
    box.setPosition(inX, y);
    box.setFillColor(C_BG_INPUT);
    box.setOutlineThickness(1.f);

    bool active = (m_ActiveProjectSettingsField == field);
    box.setOutlineColor(active ? C_ACCENT : C_BORDER);

    window.draw(box);

    std::string display = active ? m_ProjectSettingsInputText : currentVal;
    if (active && (int)(m_FPSClock.getElapsedTime().asSeconds() * 2) % 2 == 0) display += "|";

    sf::Text tVal(display, *m_Font, 11);
    tVal.setPosition(inX + 6.f, y + 4.f);
    tVal.setFillColor(C_TEXT_PRIMARY);
    window.draw(tVal);

    if (field == ProjectSettingsField::ClearColorHex) {
        sf::Color previewCol = HexToColor(active ? m_ProjectSettingsInputText : currentVal, m_ProjectClearColor);
        sf::RectangleShape colorPreview(sf::Vector2f(16.f, 16.f));
        colorPreview.setPosition(inX + inW - 20.f, y + 3.f);
        colorPreview.setFillColor(previewCol);
        colorPreview.setOutlineThickness(1.f);
        colorPreview.setOutlineColor(C_BORDER_LIGHT);
        window.draw(colorPreview);
    }

    m_ProjectSettingsButtons.push_back({{inX, y, inW, 22.f}, "edit_proj_" + std::to_string((int)field)});
    return 32.f;
}

void EditorScene::DrawProjectSettingsWindow(sf::RenderWindow &window) {
    if (!m_ShowProjectSettings) return;

    const float w = 540.f;
    const float h = 420.f;
    const float x = (window.getSize().x - w) / 2.f;
    const float y = (window.getSize().y - h) / 2.f;

    sf::RectangleShape backdrop(sf::Vector2f(window.getSize().x, window.getSize().y));
    backdrop.setFillColor(sf::Color(0, 0, 0, 140));
    window.draw(backdrop);

    sf::RectangleShape panel(sf::Vector2f(w, h));
    panel.setPosition(x, y);
    panel.setFillColor(C_BG_PANEL);
    panel.setOutlineColor(C_BORDER_LIGHT);
    panel.setOutlineThickness(1.f);
    window.draw(panel);

    m_ProjectSettingsButtons.clear();

    sf::Text title("Project Settings", *m_Font, 16);
    title.setPosition(x + 20.f, y + 16.f);
    title.setFillColor(C_TEXT_PRIMARY);
    window.draw(title);

    sf::RectangleShape closeBtn(sf::Vector2f(22.f, 22.f));
    closeBtn.setPosition(x + w - 32.f, y + 16.f);
    closeBtn.setFillColor(C_BG_INPUT);
    closeBtn.setOutlineColor(C_BORDER);
    closeBtn.setOutlineThickness(1.f);
    window.draw(closeBtn);
    
    sf::Text closeTxt("X", *m_Font, 11);
    closeTxt.setPosition(x + w - 26.f, y + 19.f);
    closeTxt.setFillColor(C_TEXT_MUTED);
    window.draw(closeTxt);
    m_ProjectSettingsButtons.push_back({closeBtn.getGlobalBounds(), "close_proj_settings"});

    float tabY = y + 50.f;
    std::vector<std::string> tabs = {"General", "Display & Graphics", "Audio"};
    float tabX = x + 20.f;
    for (int i = 0; i < (int)tabs.size(); ++i) {
        sf::Text tabText(tabs[i], *m_Font, 11);
        float tw = tabText.getLocalBounds().width + 24.f;
        sf::RectangleShape tabBox(sf::Vector2f(tw, 26.f));
        tabBox.setPosition(tabX, tabY);
        bool isCurrentTab = (m_ProjectSettingsTab == i);
        tabBox.setFillColor(isCurrentTab ? C_ACCENT : C_BG_ELEVATED);
        tabBox.setOutlineColor(isCurrentTab ? C_ACCENT_BRIGHT : C_BORDER);
        tabBox.setOutlineThickness(1.f);
        window.draw(tabBox);

        tabText.setPosition(tabX + 12.f, tabY + 6.f);
        tabText.setFillColor(isCurrentTab ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
        window.draw(tabText);

        m_ProjectSettingsButtons.push_back({tabBox.getGlobalBounds(), "tab_proj_" + std::to_string(i)});
        tabX += tw + 6.f;
    }

    sf::RectangleShape divLine(sf::Vector2f(w - 40.f, 1.f));
    divLine.setPosition(x + 20.f, tabY + 34.f);
    divLine.setFillColor(C_BORDER);
    window.draw(divLine);

    float currY = tabY + 44.f;

    if (m_ProjectSettingsTab == 0) {
        currY += DrawProjectSettingsInputField(window, "Project Name", m_ProjectName, ProjectSettingsField::ProjectName, x + 20.f, currY, x + w);
        currY += DrawProjectSettingsInputField(window, "Version", m_ProjectVersion, ProjectSettingsField::Version, x + 20.f, currY, x + w);
        currY += DrawProjectSettingsInputField(window, "Author / Studio", m_ProjectAuthor, ProjectSettingsField::Author, x + 20.f, currY, x + w);
        currY += DrawProjectSettingsInputField(window, "Start Scene", m_ProjectStartScene, ProjectSettingsField::StartScene, x + 20.f, currY, x + w);
    } else if (m_ProjectSettingsTab == 1) {
        currY += DrawProjectSettingsInputField(window, "Window Width", std::to_string(m_ProjectWindowWidth), ProjectSettingsField::WindowWidth, x + 20.f, currY, x + w);
        currY += DrawProjectSettingsInputField(window, "Window Height", std::to_string(m_ProjectWindowHeight), ProjectSettingsField::WindowHeight, x + 20.f, currY, x + w);
        currY += DrawProjectSettingsToggle(window, "Vertical Sync (VSync)", m_ProjectVSync, "toggle_vsync", x + 20.f, currY, x + w);
        currY += DrawProjectSettingsInputField(window, "Target FPS (0 = Uncapped)", std::to_string(m_ProjectTargetFPS), ProjectSettingsField::TargetFPS, x + 20.f, currY, x + w);
        currY += DrawProjectSettingsToggle(window, "Fullscreen (Standalone)", m_ProjectFullscreen, "toggle_fullscreen", x + 20.f, currY, x + w);
        currY += DrawProjectSettingsInputField(window, "Clear Color (Hex)", ColorToHex(m_ProjectClearColor), ProjectSettingsField::ClearColorHex, x + 20.f, currY, x + w);
    } else if (m_ProjectSettingsTab == 2) {
        currY += DrawProjectSettingsInputField(window, "Master Volume (0-100)", std::to_string((int)m_ProjectMasterVolume), ProjectSettingsField::MasterVolume, x + 20.f, currY, x + w);
        currY += DrawProjectSettingsInputField(window, "Music Volume (0-100)", std::to_string((int)m_ProjectMusicVolume), ProjectSettingsField::MusicVolume, x + 20.f, currY, x + w);
    }

    float btnW = 110.f;
    float btnH = 28.f;
    float btnX = x + w - btnW - 20.f;
    float btnY = y + h - btnH - 16.f;
    sf::RectangleShape saveBtn(sf::Vector2f(btnW, btnH));
    saveBtn.setPosition(btnX, btnY);
    saveBtn.setFillColor(C_ACCENT);
    window.draw(saveBtn);

    sf::Text saveTxt("Save & Close", *m_Font, 11);
    saveTxt.setPosition(btnX + (btnW - saveTxt.getLocalBounds().width) / 2.f, btnY + 7.f);
    saveTxt.setFillColor(C_TEXT_PRIMARY);
    window.draw(saveTxt);
    m_ProjectSettingsButtons.push_back({{btnX, btnY, btnW, btnH}, "close_proj_settings"});
}

void EditorScene::HandleProjectSettingsClick(sf::Vector2f pos) {
    for (const auto& btn : m_ProjectSettingsButtons) {
        if (btn.bounds.contains(pos)) {
            if (btn.action == "close_proj_settings") {
                CommitActiveProjectSettingsField();
                m_ShowProjectSettings = false;
                SaveProjectSettings();
            } else if (btn.action == "toggle_vsync") {
                CommitActiveProjectSettingsField();
                m_ProjectVSync = !m_ProjectVSync;
                SaveProjectSettings();
            } else if (btn.action == "toggle_fullscreen") {
                CommitActiveProjectSettingsField();
                m_ProjectFullscreen = !m_ProjectFullscreen;
                SaveProjectSettings();
            } else if (btn.action.find("tab_proj_") == 0) {
                CommitActiveProjectSettingsField();
                m_ProjectSettingsTab = std::stoi(btn.action.substr(9));
            } else if (btn.action.find("edit_proj_") == 0) {
                CommitActiveProjectSettingsField();
                int fieldIdx = std::stoi(btn.action.substr(10));
                m_ActiveProjectSettingsField = static_cast<ProjectSettingsField>(fieldIdx);
                
                if (m_ActiveProjectSettingsField == ProjectSettingsField::ProjectName)
                    m_ProjectSettingsInputText = m_ProjectName;
                else if (m_ActiveProjectSettingsField == ProjectSettingsField::Version)
                    m_ProjectSettingsInputText = m_ProjectVersion;
                else if (m_ActiveProjectSettingsField == ProjectSettingsField::Author)
                    m_ProjectSettingsInputText = m_ProjectAuthor;
                else if (m_ActiveProjectSettingsField == ProjectSettingsField::StartScene)
                    m_ProjectSettingsInputText = m_ProjectStartScene;
                else if (m_ActiveProjectSettingsField == ProjectSettingsField::WindowWidth)
                    m_ProjectSettingsInputText = std::to_string(m_ProjectWindowWidth);
                else if (m_ActiveProjectSettingsField == ProjectSettingsField::WindowHeight)
                    m_ProjectSettingsInputText = std::to_string(m_ProjectWindowHeight);
                else if (m_ActiveProjectSettingsField == ProjectSettingsField::TargetFPS)
                    m_ProjectSettingsInputText = std::to_string(m_ProjectTargetFPS);
                else if (m_ActiveProjectSettingsField == ProjectSettingsField::ClearColorHex)
                    m_ProjectSettingsInputText = ColorToHex(m_ProjectClearColor);
                else if (m_ActiveProjectSettingsField == ProjectSettingsField::MasterVolume)
                    m_ProjectSettingsInputText = std::to_string((int)m_ProjectMasterVolume);
                else if (m_ActiveProjectSettingsField == ProjectSettingsField::MusicVolume)
                    m_ProjectSettingsInputText = std::to_string((int)m_ProjectMusicVolume);
            }
            return;
        }
    }
    CommitActiveProjectSettingsField();
}
