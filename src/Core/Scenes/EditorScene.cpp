#include "EditorScene.h"
#include "../Scenes/SceneManager.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <filesystem>

#include "../ECS/Components.h"
#include "../Scripting/ScriptComponent.h"
#include "../Scripting/LuaState.h"
#include "../Resources/ResourceManager.h"
#include "SFML/Window/Event.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#undef CreateWindow
#endif

namespace fs = std::filesystem;

static const sf::Color C_BG_DEEP = sf::Color(10, 10, 18);
static const sf::Color C_BG_PANEL = sf::Color(16, 16, 28);
static const sf::Color C_BG_INSPECTOR = sf::Color(13, 13, 22);
static const sf::Color C_BG_TOOLBAR = sf::Color(9, 9, 16);
static const sf::Color C_SURFACE = sf::Color(24, 24, 40);
static const sf::Color C_SURFACE_HOV = sf::Color(32, 32, 54);
static const sf::Color C_BORDER = sf::Color(36, 36, 58);
static const sf::Color C_BORDER_LIGHT = sf::Color(48, 48, 78);
static const sf::Color C_ACCENT = sf::Color(99, 102, 241);
static const sf::Color C_ACCENT_DIM = sf::Color(60, 62, 160);
static const sf::Color C_ACCENT_BRIGHT = sf::Color(148, 150, 255);
static const sf::Color C_TEXT_PRIMARY = sf::Color(220, 220, 238);
static const sf::Color C_TEXT_SECONDARY = sf::Color(140, 140, 168);
static const sf::Color C_TEXT_MUTED = sf::Color(72, 72, 100);
static const sf::Color C_GREEN = sf::Color(52, 211, 100);
static const sf::Color C_GREEN_DIM = sf::Color(22, 78, 42);
static const sf::Color C_RED = sf::Color(248, 80, 80);
static const sf::Color C_RED_DIM = sf::Color(80, 20, 20);

EditorScene::EditorScene(SceneManager &manager, sf::RenderWindow &window, Registry &registry)
    : Scene(manager), m_Window(window), m_Registry(registry)
{
    m_Font = ResourceManager::Get().GetFont(ASSET_PATH "fonts/Merriweather.ttf");
    m_ContentBrowser = std::make_unique<ContentBrowser>(*m_Font, ASSET_PATH);

    m_camera = window.getDefaultView();

    const float winW = static_cast<float>(window.getSize().x);
    const float winH = static_cast<float>(window.getSize().y);

    sf::View adjustedView = m_camera;
    adjustedView.setViewport({
        0.f,
        TopBarHeight / winH,
        1.f - (InspectorWidth / winW),
        1.f - (BrowserHeight / winH) - (TopBarHeight / winH)
    });
    m_camera = adjustedView;

    UpdateBounds();

    m_Preview.setSize({m_GridSize, m_GridSize});
    m_Preview.setFillColor(sf::Color(100, 200, 100, 60));
    m_Preview.setOutlineColor(sf::Color(100, 255, 100, 180));
    m_Preview.setOutlineThickness(1.f);

    m_CirclePreview.setRadius(m_GridSize / 2.f);
    m_CirclePreview.setFillColor(sf::Color(100, 200, 100, 60));
    m_CirclePreview.setOutlineColor(sf::Color(100, 255, 100, 180));
    m_CirclePreview.setOutlineThickness(1.f);

    m_StatusText.setFont(*m_Font);
    m_StatusText.setCharacterSize(11);
    m_StatusText.setFillColor(C_TEXT_MUTED);

    m_InspectorPanel.setFillColor(C_BG_INSPECTOR);

    InitMenus();
    UpdateStatusText();
}

void EditorScene::InitMenus()
{
    MenuEntry datei;
    datei.label = "File";
    datei.items = {
        {"Save", "save", false, "Ctrl+S"},
        {"Load", "load", false, "Ctrl+L"},
        {"", "", true, ""},
        {"Quit", "quit", false, ""}
    };

    MenuEntry edit;
    edit.label = "Edit";
    edit.items = {
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

    m_Menus = {datei, edit, ansicht, tools};
}

void EditorScene::OnEnter()
{
    std::cout << "[EditorScene] Active\n";
    UpdateBounds();
    UpdateStatusText();
}

void EditorScene::OnExit() { SyncToRegistry(); }

void EditorScene::UpdateBounds()
{
    const float w = static_cast<float>(m_Window.getSize().x);
    const float h = static_cast<float>(m_Window.getSize().y);

    m_BrowserBounds = {0.f, h - BrowserHeight, w - InspectorWidth, BrowserHeight};
    m_InspectorBounds = {w - InspectorWidth, TopBarHeight, InspectorWidth, h - TopBarHeight};
}

void EditorScene::HandleMenuAction(const std::string &action)
{
    if (action == "save")
    {
        SaveToJson(ASSET_PATH "scenes/game.json");
        std::cout << "[EditorScene] Saved\n";
    } else if (action == "load")
    {
        LoadFromJson(ASSET_PATH "scenes/game.json");
        std::cout << "[EditorScene] Loaded\n";
    } else if (action == "quit") { m_Window.close(); } else if (action == "delete")
    {
        DeleteSelected();
        UpdateStatusText();
    } else if (action == "deselect")
    {
        if (m_Selected) m_Selected->selected = false;
        m_Selected = nullptr;
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
        m_manager.SwitchSceneTo("game");
    } else if (action == "reset_scene")
    {
        for (auto &obj: m_Objects)
            if (obj.entity != 0) m_Registry.DestroyEntity(obj.entity);
        m_Objects.clear();
        m_Selected = nullptr;
        UpdateStatusText();
    } else if (action == "duplicate" && m_Selected)
    {
        const ObjectType t = (m_Selected->objectType == ObjectType::Sprite)
                                 ? ObjectType::Rectangle
                                 : m_Selected->objectType;
        AddObject(m_Selected->shape.getPosition() + sf::Vector2f(m_GridSize, 0.f), t);
    }
}

void EditorScene::HandleEvent(const sf::Event &event)
{
    UpdateBounds();

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
            if (m_PlacementType == ObjectType::Circle)
                m_CirclePreview.setPosition(SnapToGrid(pos));
            else
                m_Preview.setPosition(SnapToGrid(pos));
        }

        if (m_Dragging && m_Selected)
            m_Selected->shape.setPosition(SnapToGrid(MouseWorldPos() - m_DragOffset));

        return;
    }

    const bool inTopBars = m_MouseScreenPos.y < TopBarHeight;
    const bool inMenuBar = m_MouseScreenPos.y < MenuBarHeight;
    const bool inBrowser = m_BrowserBounds.contains(m_MouseScreenPos);
    const bool inInspector = m_InspectorBounds.contains(m_MouseScreenPos);

    if (event.type == sf::Event::MouseButtonReleased &&
        event.mouseButton.button == sf::Mouse::Left &&
        m_ContentBrowser->HasDraggedAsset())
    {
        DraggedAsset drag = m_ContentBrowser->GetDraggedAsset();
        bool handled = false;

        if (drag.type == AssetType::Script)
        {
            EditorObject *hit = ObjectAt(MouseWorldPos());
            if (hit && hit->entity != 0 && !m_Registry.HasComponent<ScriptComponent>(hit->entity))
            {
                auto &sc = m_Registry.AddComponent(hit->entity, ScriptComponent(LuaState::GetLua(), drag.path));
                sc.SetEntity(hit->entity);
                hit->scriptPath = drag.path;
                std::cout << "[ContentBrowser] Script dropped onto " << hit->id << "\n";
            }
            handled = true;
        } else if (drag.type == AssetType::Image)
        {
            if (!inBrowser && !inTopBars)
            {
                EditorObject *hit = ObjectAt(MouseWorldPos());
                if (hit)
                    ApplySpriteToObject(*hit, drag.path);
                else
                    AddObjectWithSprite(MouseWorldPos(), drag.path);
            }
            handled = true;
        }

        if (handled)
        {
            m_ContentBrowser->ClearDrag();
            return;
        }
    }

    if (inBrowser || m_ContentBrowser->HasDraggedAsset() ||
        m_ContentBrowser->IsInputActive() || m_ContentBrowser->IsContextMenuOpen())
    {
        m_ContentBrowser->HandleEvent(event, m_MouseScreenPos);
        if (m_ContentBrowser->IsInputActive() &&
            (event.type == sf::Event::TextEntered || event.type == sf::Event::KeyPressed))
            return;
    }

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
                } else if (m_ActiveField == EditField::Script)
                {
                    std::string fullPath = std::string(ASSET_PATH) + m_ActiveInputText + ".lua";
                    std::ifstream check(fullPath);
                    if (!check.is_open())
                    {
                        std::ofstream newFile(fullPath);
                        newFile << "function OnCreate()\n\nend\n\n";
                        newFile << "function OnUpdate(dt)\n\nend\n";
                        newFile.close();
                        std::cout << "[Inspector] Created script: " << fullPath << "\n";
                    }
                    check.close();
                    auto &sc = m_Registry.AddComponent(m_Selected->entity,
                                                       ScriptComponent(LuaState::GetLua(), fullPath));
                    sc.SetEntity(m_Selected->entity);
                    m_Selected->scriptPath = fullPath;
                    std::cout << "[Inspector] Script assigned: " << fullPath << "\n";
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
                } else if (m_ActiveField == EditField::SizeW || m_ActiveField == EditField::SizeH)
                {
                    try
                    {
                        float val = std::stof(m_ActiveInputText);
                        sf::Vector2f size = m_Selected->shape.getSize();
                        if (m_ActiveField == EditField::SizeW) size.x = val;
                        else size.y = val;
                        m_Selected->shape.setSize(size);
                        if (m_Selected->objectType == ObjectType::Sprite && m_Selected->previewTexture)
                        {
                            auto texSize = m_Selected->previewTexture->getSize();
                            m_Selected->previewSprite.setScale(size.x / texSize.x, size.y / texSize.y);
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
            if (m_ActiveField == EditField::Name || m_ActiveField == EditField::Script)
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
        vp.top = TopBarHeight / nH;
        vp.width = 1.f - (InspectorWidth / nW);
        vp.height = 1.f - (BrowserHeight / nH) - (TopBarHeight / nH);
        m_camera.setViewport(vp);
        UpdateBounds();
    }

    if (event.type == sf::Event::MouseWheelScrolled && !inTopBars && !inInspector && !inBrowser)
    {
        const float factor = event.mouseWheelScroll.delta > 0 ? 0.9f : 1.1f;
        m_camera.zoom(factor);
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Middle)
    {
        m_panning = true;
        m_panStart = m_Window.mapPixelToCoords(sf::Mouse::getPosition(m_Window), m_camera);
    }

    if (event.type == sf::Event::MouseButtonReleased &&
        event.mouseButton.button == sf::Mouse::Middle)
        m_panning = false;

    if (event.type == sf::Event::MouseButtonReleased &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        if (m_Dragging && m_Selected && m_Selected->entity != 0 &&
            m_Registry.HasComponent<TransformComponent>(m_Selected->entity))
        {
            auto &t = m_Registry.GetComponent<TransformComponent>(m_Selected->entity);
            t.x = m_Selected->shape.getPosition().x;
            t.y = m_Selected->shape.getPosition().y;
        }
        m_Dragging = false;
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left)
    {
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

        if (inBrowser) return;

        m_ActiveField = EditField::None;

        sf::Vector2f pos = MouseWorldPos();
        EditorObject *hit = ObjectAt(pos);

        if (m_Selected) m_Selected->selected = false;

        if (hit)
        {
            m_Selected = hit;
            m_Selected->selected = true;
            m_Dragging = true;
            m_DragOffset = pos - m_Selected->shape.getPosition();
        } else
        {
            m_Selected = nullptr;
            AddObject(pos, m_PlacementType);
        }

        UpdateStatusText();
    }

    if (event.type == sf::Event::KeyPressed)
    {
        const bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl);

        if (ctrl && event.key.code == sf::Keyboard::S) HandleMenuAction("save");
        if (ctrl && event.key.code == sf::Keyboard::L) HandleMenuAction("load");
        if (ctrl && event.key.code == sf::Keyboard::D) HandleMenuAction("duplicate");
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
            m_manager.SwitchSceneTo("game");
        }
    }
}

void EditorScene::Update(float) {}

void EditorScene::Render(sf::RenderWindow &window)
{
    window.setView(m_camera);
    DrawGrid();

    for (auto &obj: m_Objects)
    {
        if (obj.objectType == ObjectType::Circle)
        {
            obj.circleShape.setPosition(obj.shape.getPosition());
            obj.circleShape.setRadius(obj.shape.getSize().x / 2.f);
            obj.circleShape.setFillColor(obj.color);
            obj.circleShape.setOutlineColor(obj.selected ? sf::Color(255, 220, 60) : sf::Color::Transparent);
            obj.circleShape.setOutlineThickness(obj.selected ? 2.f : 0.f);
            window.draw(obj.circleShape);
        } else
        {
            obj.shape.setOutlineColor(obj.selected ? sf::Color(255, 220, 60) : sf::Color::Transparent);
            obj.shape.setOutlineThickness(obj.selected ? 2.f : 0.f);
            window.draw(obj.shape);
        }
    }

    for (auto &obj: m_Objects)
    {
        if (obj.objectType == ObjectType::Sprite && obj.previewTexture)
        {
            obj.previewSprite.setTexture(*obj.previewTexture);
            obj.previewSprite.setPosition(obj.shape.getPosition());
            window.draw(obj.previewSprite);
        }
    }

    if (!m_Dragging)
    {
        if (m_PlacementType == ObjectType::Circle)
            window.draw(m_CirclePreview);
        else
            window.draw(m_Preview);
    }

    const sf::View uiView(sf::FloatRect(
        0.f, 0.f,
        static_cast<float>(m_Window.getSize().x),
        static_cast<float>(m_Window.getSize().y)));
    window.setView(uiView);
    UpdateBounds();

    m_ContentBrowser->Render(window,
                             m_BrowserBounds.left, m_BrowserBounds.top,
                             m_BrowserBounds.width, m_BrowserBounds.height);

    DrawInspector(window); {
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

    if (m_AddDropdownOpen) DrawAddDropdown(window);
}

static void DrawPill(sf::RenderWindow &window, sf::FloatRect r, sf::Color fill, sf::Color outline)
{
    sf::RectangleShape bg({r.width, r.height});
    bg.setPosition(r.left, r.top);
    bg.setFillColor(fill);
    bg.setOutlineColor(outline);
    bg.setOutlineThickness(1.f);
    window.draw(bg);
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
            hbg.setFillColor(open ? C_ACCENT_DIM : C_SURFACE_HOV);
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
            dbg.setFillColor(C_SURFACE);
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
    watermark.setFillColor(sf::Color(42, 42, 68));
    watermark.setString("RayneEngine");
    watermark.setPosition(w - watermark.getLocalBounds().width - 12.f, 9.f);
    window.draw(watermark);
}

void EditorScene::DrawToolbar(sf::RenderWindow &window)
{
    const float w = static_cast<float>(window.getSize().x);
    const float ty = MenuBarHeight;

    sf::RectangleShape bg({w, ToolbarHeight});
    bg.setFillColor(C_BG_TOOLBAR);
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
                             ? sf::Color(accentFill.r / 5, accentFill.g / 5, accentFill.b / 3, 230)
                             : hov
                                   ? C_SURFACE_HOV
                                   : sf::Color(0, 0, 0, 0);
        sf::Color border = active
                               ? sf::Color(accentBorder.r, accentBorder.g, accentBorder.b, 200)
                               : hov
                                     ? C_BORDER_LIGHT
                                     : sf::Color(0, 0, 0, 0);
        DrawPill(window, r, fill, border);

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

        sf::Color fill = act ? C_ACCENT_DIM : hov ? C_SURFACE_HOV : C_SURFACE;
        sf::Color border = act ? C_ACCENT : hov ? C_BORDER_LIGHT : C_BORDER;
        DrawPill(window, ar, fill, border);

        sf::Text at;
        at.setFont(*m_Font);
        at.setCharacterSize(12);
        at.setFillColor(act ? C_ACCENT_BRIGHT : hov ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
        at.setString("+ Add  v");
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
        drawBtn(dr, "Delete", false, C_RED, C_RED);
        m_ToolbarHitboxes.push_back({dr, "delete"});
        cx += dr.width + 8.f;
    }

    drawSep(cx);
    cx += 12.f; {
        const sf::FloatRect gr(cx, ty + 4.f, 70.f, ToolbarHeight - 8.f);
        drawBtn(gr, m_SnapToGrid ? "Grid ON" : "Grid", m_SnapToGrid);
        m_ToolbarHitboxes.push_back({gr, "toggle_grid"});
    } {
        const float runW = 88.f;
        const float runX = w - InspectorWidth - runW - 10.f;
        const sf::FloatRect rr(runX, ty + 4.f, runW, ToolbarHeight - 8.f);
        const bool hov = rr.contains(m_MouseScreenPos);

        sf::Color fill = hov ? sf::Color(22, 70, 38, 240) : C_GREEN_DIM;
        sf::Color border = hov ? C_GREEN : sf::Color(34, 140, 64);
        DrawPill(window, rr, fill, border);

        sf::Text rt;
        rt.setFont(*m_Font);
        rt.setCharacterSize(12);
        rt.setFillColor(hov ? C_GREEN : sf::Color(80, 180, 110));
        rt.setString("Run   F5");
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

    struct DropItem
    {
        std::string label;
        std::string action;
        std::string desc;
    };
    const std::vector<DropItem> items = {
        {"Rectangle", "add_rect", "Rechteck-Primitiv"},
        {"Circle", "add_circle", "Kreis-Primitiv"}
    };

    const float dropH = static_cast<float>(items.size()) * itemH + 12.f;

    sf::RectangleShape bg({dropW, dropH});
    bg.setFillColor(C_SURFACE);
    bg.setOutlineColor(C_BORDER_LIGHT);
    bg.setOutlineThickness(1.f);
    bg.setPosition(dropX, dropY);
    window.draw(bg);

    float iy = dropY + 6.f;
    for (auto &item: items)
    {
        const sf::FloatRect ir(dropX, iy, dropW, itemH);
        const bool hov = ir.contains(m_MouseScreenPos);
        const bool cur = (item.action == "add_rect" && m_PlacementType == ObjectType::Rectangle) ||
                         (item.action == "add_circle" && m_PlacementType == ObjectType::Circle);

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
        header.setFillColor(C_SURFACE);
        window.draw(header);

        sf::RectangleShape headerLine({InspectorWidth, 1.f});
        headerLine.setFillColor(C_BORDER);
        headerLine.setPosition(panelX, panelY + 35.f);
        window.draw(headerLine);

        sf::Text title;
        title.setFont(*m_Font);
        title.setCharacterSize(11);
        title.setFillColor(C_TEXT_MUTED);
        title.setStyle(sf::Text::Bold);
        title.setString("INSPECTOR");
        title.setPosition(panelX + InspectorPad + 2.f, panelY + 12.f);
        window.draw(title);
    }

    if (!m_Selected)
    {
        sf::Text empty;
        empty.setFont(*m_Font);
        empty.setCharacterSize(12);
        empty.setFillColor(C_TEXT_MUTED);
        empty.setString("No selection");
        empty.setPosition(panelX + InspectorPad + 2.f, panelY + 52.f);
        window.draw(empty);
        return;
    }

    float y = panelY + 42.f;

    y = DrawSectionHeader(window, "OBJECT", sf::Color(100, 160, 255), panelX, y);

    std::string nameDisplay = (m_ActiveField == EditField::Name && !m_ActiveInputText.empty())
                                  ? m_ActiveInputText + "|"
                                  : (m_ActiveField == EditField::Name ? "|" : m_Selected->id);
    y = DrawEditableRow(window, "Name", nameDisplay, "edit_name", panelX, y);
    y = DrawRow(window, "Entity", std::to_string(m_Selected->entity), panelX, y); {
        const std::string typeLabel = m_Selected->objectType == ObjectType::Circle
                                          ? "circle"
                                          : m_Selected->objectType == ObjectType::Sprite
                                                ? "sprite"
                                                : "rectangle";
        y = DrawRow(window, "Type", typeLabel, panelX, y);
    }

    y += 8.f;
    y = DrawSectionHeader(window, "TRANSFORM", sf::Color(80, 200, 120), panelX, y);

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

    y += 8.f;
    y = DrawSectionHeader(window, "RENDER", sf::Color(220, 170, 60), panelX, y);

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

    if (m_Selected->objectType != ObjectType::Sprite)
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

    if (m_Selected->objectType == ObjectType::Sprite)
    {
        std::string spriteName = m_Selected->spritePath;
        const size_t sl = spriteName.find_last_of("/\\");
        if (sl != std::string::npos) spriteName = spriteName.substr(sl + 1);
        y += 4.f;
        y = DrawRow(window, "Sprite", spriteName.empty() ? "none" : spriteName, panelX, y);
        y = DrawActionButton(window, "Change Sprite", "change_sprite", panelX, y, C_ACCENT_DIM, C_ACCENT);
    }

    y += 8.f;

    if (m_Selected->entity != 0 && m_Registry.HasComponent<VelocityComponent>(m_Selected->entity))
    {
        auto &vel = m_Registry.GetComponent<VelocityComponent>(m_Selected->entity);
        y = DrawSectionHeader(window, "VELOCITY", sf::Color(200, 110, 230), panelX, y);
        y = DrawRow(window, "dX", std::to_string(vel.dx), panelX, y);
        y = DrawRow(window, "dY", std::to_string(vel.dy), panelX, y);
        y += 4.f;
        y = DrawActionButton(window, "Remove Velocity", "remove_velocity", panelX, y, C_RED_DIM, C_RED);
        y += 8.f;
    } else if (m_Selected->entity != 0)
    {
        y = DrawActionButton(window, "+ Velocity", "add_velocity", panelX, y, C_SURFACE, C_BORDER_LIGHT);
        y += 8.f;
    }

    if (m_Selected->entity != 0 && m_Registry.HasComponent<ScriptComponent>(m_Selected->entity))
    {
        y = DrawSectionHeader(window, "SCRIPT", sf::Color(230, 90, 90), panelX, y);
        std::string scriptName = m_Selected->scriptPath;
        const size_t slash = scriptName.find_last_of("/\\");
        if (slash != std::string::npos) scriptName = scriptName.substr(slash + 1);
        y = DrawRow(window, "File", scriptName, panelX, y);
        y = DrawRow(window, "OnCreate", "bound", panelX, y);
        y = DrawRow(window, "OnUpdate", "bound", panelX, y);
        y += 4.f;
        y = DrawActionButton(window, "Open Script", "open_script", panelX, y, C_SURFACE, C_BORDER_LIGHT);
        y = DrawActionButton(window, "Remove Script", "remove_script", panelX, y, C_RED_DIM, C_RED);
    } else if (m_Selected->entity != 0)
    {
        y = DrawActionButton(window, "+ Script", "add_script", panelX, y, C_SURFACE, C_BORDER_LIGHT);
        if (m_ActiveField == EditField::Script)
            DrawScriptInput(window, panelX, y);
    }
}

float EditorScene::DrawSectionHeader(sf::RenderWindow &window, const std::string &title,
                                     sf::Color accent, float x, float y)
{
    sf::RectangleShape bar({InspectorWidth, 22.f});
    bar.setFillColor(sf::Color(accent.r / 10, accent.g / 10, accent.b / 10, 180));
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
    text.setPosition(x + InspectorPad + 4.f, y + 6.f);
    window.draw(text);

    return y + 24.f;
}

float EditorScene::DrawRow(sf::RenderWindow &window, const std::string &key,
                           const std::string &val, float x, float y)
{
    sf::Text keyText;
    keyText.setFont(*m_Font);
    keyText.setCharacterSize(12);
    keyText.setFillColor(C_TEXT_MUTED);
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
    keyText.setFillColor(C_TEXT_MUTED);
    keyText.setString(key);
    keyText.setPosition(x + InspectorPad + 4.f, y + 3.f);
    window.draw(keyText);

    const float valX = x + InspectorWidth * 0.44f;
    const float valW = InspectorWidth - InspectorWidth * 0.44f - InspectorPad;
    const sf::FloatRect fieldRect(valX, y, valW, 20.f);
    const bool hovered = fieldRect.contains(m_MouseScreenPos);
    const bool active = (m_InspectorButtons.size() > 0) && false;

    sf::Color fieldFill = hovered ? C_SURFACE_HOV : C_SURFACE;
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
    valText.setFillColor(hovered ? C_TEXT_PRIMARY : sf::Color(200, 200, 220));
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
    return DrawActionButton(window, label, action, x, y, C_SURFACE, C_BORDER_LIGHT);
}

float EditorScene::DrawRemoveButton(sf::RenderWindow &window, const std::string &label,
                                    const std::string &action, float x, float y)
{
    return DrawActionButton(window, label, action, x, y, C_RED_DIM, C_RED);
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
    bg.setFillColor(C_SURFACE);
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
            std::cout << "[Inspector] VelocityComponent added to " << m_Selected->id << "\n";
        } else if (btn.action == "remove_velocity")
        {
            m_Registry.RemoveComponent<VelocityComponent>(m_Selected->entity);
            std::cout << "[Inspector] VelocityComponent removed from " << m_Selected->id << "\n";
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
            std::cout << "[Inspector] ScriptComponent removed from " << m_Selected->id << "\n";
        } else if (btn.action == "edit_name" && m_Selected)
        {
            m_ActiveField = EditField::Name;
            m_ActiveInputText = m_Selected->id;
        } else if (btn.action == "edit_x" && m_Selected)
        {
            m_ActiveField = EditField::TransformX;
            m_ActiveInputText = std::to_string((int) m_Selected->shape.getPosition().x);
        } else if (btn.action == "edit_y" && m_Selected)
        {
            m_ActiveField = EditField::TransformY;
            m_ActiveInputText = std::to_string((int) m_Selected->shape.getPosition().y);
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
        }
        break;
    }
}

void EditorScene::AddObject(sf::Vector2f pos, ObjectType type)
{
    EditorObject obj;
    obj.id = NextId();
    obj.objectType = type;
    obj.color = (type == ObjectType::Circle) ? sf::Color(237, 149, 100) : sf::Color(100, 149, 237);

    obj.shape.setSize({m_GridSize, m_GridSize});
    obj.shape.setPosition(SnapToGrid(pos));
    obj.shape.setFillColor(obj.color);

    if (type == ObjectType::Circle)
    {
        obj.circleShape.setRadius(m_GridSize / 2.f);
        obj.circleShape.setPosition(SnapToGrid(pos));
        obj.circleShape.setFillColor(obj.color);
    }

    obj.entity = m_Registry.CreateEntity();
    m_Registry.AddComponent(obj.entity, TransformComponent{
                                obj.shape.getPosition().x,
                                obj.shape.getPosition().y
                            });
    m_Registry.AddComponent(obj.entity, RenderComponent{
                                obj.color,
                                obj.shape.getSize()
                            });

    m_Objects.push_back(std::move(obj));

    auto &placed = m_Objects.back();
    if (placed.previewTexture)
        placed.previewSprite.setTexture(*placed.previewTexture, true);

    UpdateStatusText();
}

void EditorScene::AddObjectWithSprite(sf::Vector2f pos, const std::string &spritePath)
{
    EditorObject obj;
    obj.id = NextId();
    obj.color = sf::Color::White;
    obj.objectType = ObjectType::Sprite;

    obj.shape.setSize({m_GridSize * 2.f, m_GridSize * 2.f});
    obj.shape.setPosition(SnapToGrid(pos));
    obj.shape.setFillColor(sf::Color::White);

    obj.entity = m_Registry.CreateEntity();
    m_Registry.AddComponent(obj.entity, TransformComponent{
                                obj.shape.getPosition().x,
                                obj.shape.getPosition().y
                            });
    m_Registry.AddComponent(obj.entity, RenderComponent{
                                obj.color,
                                obj.shape.getSize()
                            });

    ApplySpriteToObject(obj, spritePath);
    m_Objects.push_back(std::move(obj));

    auto &placed = m_Objects.back();
    if (placed.previewTexture)
        placed.previewSprite.setTexture(*placed.previewTexture, true);

    UpdateStatusText();
}

void EditorScene::ApplySpriteToObject(EditorObject &obj, const std::string &spritePath)
{
    obj.spritePath = spritePath;
    obj.objectType = ObjectType::Sprite;
    obj.previewTexture = ResourceManager::Get().GetTexture(spritePath);

    if (obj.previewTexture)
    {
        obj.previewSprite.setTexture(*obj.previewTexture);
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

    std::cout << "[Editor] Sprite applied: " << spritePath << "\n";
}

void EditorScene::DeleteSelected()
{
    if (!m_Selected) return;

    if (m_Selected->entity != 0)
        m_Registry.DestroyEntity(m_Selected->entity);

    std::erase_if(m_Objects, [this](const EditorObject &o) { return &o == m_Selected; });
    m_Selected = nullptr;
}

void EditorScene::SaveToJson(const std::string &path)
{
    json data;
    data["name"] = "game";
    data["objects"] = json::array();

    for (auto &obj: m_Objects)
    {
        json j;
        j["id"] = obj.id;
        j["type"] = (obj.objectType == ObjectType::Circle)
                        ? "circle"
                        : (obj.objectType == ObjectType::Sprite)
                              ? "sprite"
                              : "rectangle";
        j["x"] = obj.shape.getPosition().x;
        j["y"] = obj.shape.getPosition().y;
        j["width"] = obj.shape.getSize().x;
        j["height"] = obj.shape.getSize().y;
        j["color"] = {obj.color.r, obj.color.g, obj.color.b};

        if (!obj.spritePath.empty())
            j["sprite"] = obj.spritePath;

        if (obj.entity != 0 && m_Registry.HasComponent<VelocityComponent>(obj.entity))
        {
            auto &vel = m_Registry.GetComponent<VelocityComponent>(obj.entity);
            j["velocity"] = {{"dx", vel.dx}, {"dy", vel.dy}};
        }

        if (!obj.scriptPath.empty())
            j["script"] = obj.scriptPath;

        data["objects"].push_back(j);
    }

    std::ofstream file(path);
    file << data.dump(4);
    UpdateStatusText();
}

void EditorScene::LoadFromJson(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "[EditorScene] File not found: " << path << "\n";
        return;
    }

    for (auto &obj: m_Objects)
        if (obj.entity != 0)
            m_Registry.DestroyEntity(obj.entity);

    m_Objects.clear();
    m_Selected = nullptr;

    json data = json::parse(file);
    for (auto &j: data["objects"])
    {
        EditorObject obj;
        obj.id = j["id"];
        obj.color = sf::Color(j["color"][0], j["color"][1], j["color"][2]);
        obj.shape.setSize({j["width"], j["height"]});
        obj.shape.setPosition(j["x"], j["y"]);
        obj.shape.setFillColor(obj.color);

        const std::string typeStr = j.value("type", "rectangle");
        if (typeStr == "circle")
        {
            obj.objectType = ObjectType::Circle;
            obj.circleShape.setRadius(obj.shape.getSize().x / 2.f);
            obj.circleShape.setPosition(obj.shape.getPosition());
            obj.circleShape.setFillColor(obj.color);
        } else if (typeStr == "sprite")
            obj.objectType = ObjectType::Sprite;
        else
            obj.objectType = ObjectType::Rectangle;

        obj.entity = m_Registry.CreateEntity();
        m_Registry.AddComponent(obj.entity, TransformComponent{j["x"], j["y"]});
        m_Registry.AddComponent(obj.entity, RenderComponent{obj.color, obj.shape.getSize()});

        if (j.contains("sprite"))
            ApplySpriteToObject(obj, j["sprite"].get<std::string>());

        if (j.contains("velocity"))
            m_Registry.AddComponent(obj.entity, VelocityComponent{
                                        j["velocity"]["dx"], j["velocity"]["dy"]
                                    });

        if (j.contains("script"))
        {
            const std::string sp = j["script"];
            auto &sc = m_Registry.AddComponent(obj.entity, ScriptComponent(LuaState::GetLua(), sp));
            sc.SetEntity(obj.entity);
            obj.scriptPath = sp;
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
        if (!m_Registry.HasComponent<TransformComponent>(obj.entity)) continue;
        auto &t = m_Registry.GetComponent<TransformComponent>(obj.entity);
        t.x = obj.shape.getPosition().x;
        t.y = obj.shape.getPosition().y;
    }
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

    for (float x = left; x < right; x += m_GridSize)
    {
        lines.append({{x, top}, sf::Color(38, 38, 52)});
        lines.append({{x, bottom}, sf::Color(38, 38, 52)});
    }
    for (float y = top; y < bottom; y += m_GridSize)
    {
        lines.append({{left, y}, sf::Color(38, 38, 52)});
        lines.append({{right, y}, sf::Color(38, 38, 52)});
    }

    m_Window.draw(lines);
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
