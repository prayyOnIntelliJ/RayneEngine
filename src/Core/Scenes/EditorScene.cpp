#include "EditorScene.h"
#include "../Scenes/SceneManager.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <filesystem>

#include "../Scripting/ScriptComponent.h"
#include "../Resources/ResourceManager.h"
#include "SFML/Window/Event.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#undef CreateWindow
#endif

namespace fs = std::filesystem;

EditorScene::EditorScene(SceneManager &manager, sf::RenderWindow &window, Registry &registry)
    : Scene(manager), m_Window(window), m_Registry(registry)
{
    m_Font = ResourceManager::Get().GetFont(ASSET_PATH "fonts/Merriweather.ttf");

    m_ContentBrowser = std::make_unique<ContentBrowser>(*m_Font, ASSET_PATH);

    m_camera = window.getDefaultView();
    sf::View adjustedView = m_camera;
    adjustedView.setViewport({
        0.f, 0.f,
        1.f - (InspectorWidth / window.getSize().x),
        1.f - (BrowserHeight / window.getSize().y)
    });
    m_camera = adjustedView;

    UpdateBounds();

    m_Preview.setSize({m_GridSize, m_GridSize});
    m_Preview.setFillColor(sf::Color(100, 200, 100, 80));
    m_Preview.setOutlineColor(sf::Color(100, 255, 100));
    m_Preview.setOutlineThickness(1.f);

    m_StatusText.setFont(*m_Font);
    m_StatusText.setCharacterSize(13);
    m_StatusText.setFillColor(sf::Color::Yellow);

    m_InspectorPanel.setFillColor(sf::Color(22, 22, 32, 240));
    m_InspectorPanel.setOutlineColor(sf::Color(60, 60, 80));
    m_InspectorPanel.setOutlineThickness(1.f);

    UpdateStatusText();
}

void EditorScene::UpdateBounds()
{
    const float w = static_cast<float>(m_Window.getSize().x);
    const float h = static_cast<float>(m_Window.getSize().y);

    m_ChooserBounds = {0.f, h - BrowserHeight, ChooserWidth, BrowserHeight};
    m_BrowserBounds = {ChooserWidth, h - BrowserHeight, w - InspectorWidth - ChooserWidth, BrowserHeight};
    m_InspectorBounds = {w - InspectorWidth, 0.f, InspectorWidth, h};
}

void EditorScene::OnEnter()
{
    std::cout << "[EditorScene] Active\n";
    UpdateBounds();
    UpdateStatusText();
}

void EditorScene::OnExit() { SyncToRegistry(); }

void EditorScene::HandleEvent(const sf::Event &event)
{
    UpdateBounds();

    const bool inBrowser = m_BrowserBounds.contains(m_MouseScreenPos);
    const bool inChooser = m_ChooserBounds.contains(m_MouseScreenPos);
    const bool inInspector = m_InspectorBounds.contains(m_MouseScreenPos);

    if (inBrowser || m_ContentBrowser->HasDraggedAsset() || m_ContentBrowser->IsInputActive() || m_ContentBrowser->IsContextMenuOpen())
    {
        m_ContentBrowser->HandleEvent(event, m_MouseScreenPos);
        if (m_ContentBrowser->IsInputActive() && (
                event.type == sf::Event::TextEntered || event.type == sf::Event::KeyPressed))
            return;
    }

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left && m_ContentBrowser
        ->HasDraggedAsset())
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
                sc.OnCreate();
                hit->scriptPath = drag.path;
                std::cout << "[ContentBrowser] Script dropped onto " << hit->id << "\n";
            }
            handled = true;
        } else if (drag.type == AssetType::Image)
        {
            bool onChooser = false;
            for (auto &[rect, type]: m_ChooserButtons)
                if (rect.contains(m_MouseScreenPos))
                {
                    onChooser = true;
                    break;
                }

            if (onChooser)
            {
                m_PlacementType = ObjectType::Sprite;
                m_PlacementSpritePath = drag.path;
                m_PlacementTexture = ResourceManager::Get().GetTexture(drag.path);
                std::cout << "[Chooser] Sprite set: " << drag.path << "\n";
            } else if (!inBrowser)
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

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left && inChooser)
    {
        for (auto &[rect, type]: m_ChooserButtons)
        {
            if (rect.contains(m_MouseScreenPos))
            {
                m_PlacementType = type;
                break;
            }
        }
        return;
    }

    if ((event.type == sf::Event::MouseWheelScrolled || event.type == sf::Event::MouseButtonPressed)
        && (inBrowser || inChooser))
        return;

    if (event.type == sf::Event::TextEntered && m_ActiveField != EditField::None)
    {
        if (event.text.unicode == '\b')
        {
            if (!m_ActiveInputText.empty())
                m_ActiveInputText.pop_back();
        } else if (event.text.unicode == '\r' || event.text.unicode == '\n')
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
                        newFile << "-- " << m_ActiveInputText << "\n\n";
                        newFile << "function OnCreate()\n\n";
                        newFile << "end\n\n";
                        newFile << "function OnUpdate(dt)\n\n";
                        newFile << "end\n";
                        newFile.close();
                        std::cout << "[Inspector] Created script: " << fullPath << "\n";
                    }
                    check.close();

                    auto &sc = m_Registry.AddComponent(m_Selected->entity,
                                                       ScriptComponent(LuaState::GetLua(), fullPath));
                    sc.SetEntity(m_Selected->entity);
                    sc.OnCreate();
                    m_Selected->scriptPath = fullPath;
                    std::cout << "[Inspector] Created script: " << fullPath << "\n";
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
                } else if (m_ActiveField == EditField::ColorR || m_ActiveField == EditField::ColorG || m_ActiveField ==
                           EditField::ColorB)
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
        const float newW = static_cast<float>(event.size.width);
        const float newH = static_cast<float>(event.size.height);

        sf::FloatRect vp = m_camera.getViewport();
        vp.width = 1.f - (InspectorWidth / newW);
        vp.height = 1.f - (BrowserHeight / newH);
        m_camera.setViewport(vp);

        UpdateBounds();
    }

    if (event.type == sf::Event::MouseWheelScrolled)
    {
        const float factor = event.mouseWheelScroll.delta > 0 ? 0.9f : 1.1f;
        m_camera.zoom(factor);
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Middle)
    {
        m_panning = true;
        m_panStart = m_Window.mapPixelToCoords(
            sf::Mouse::getPosition(m_Window), m_camera);
    }

    if (event.type == sf::Event::MouseButtonReleased &&
        event.mouseButton.button == sf::Mouse::Middle) { m_panning = false; }

    if (event.type == sf::Event::MouseMoved)
    {
        m_MouseScreenPos = {(float) event.mouseMove.x, (float) event.mouseMove.y};

        if (m_panning)
        {
            sf::Vector2f current = m_Window.mapPixelToCoords(
                {event.mouseMove.x, event.mouseMove.y}, m_camera);
            m_camera.move(m_panStart - current);
        }

        sf::Vector2f pos = MouseWorldPos();
        m_Preview.setPosition(SnapToGrid(pos));

        if (m_Dragging && m_Selected) { m_Selected->shape.setPosition(SnapToGrid(pos - m_DragOffset)); }
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        sf::Vector2f screenPos = m_MouseScreenPos;

        if (inInspector)
        {
            HandleInspectorClick(screenPos);
            return;
        }

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
            if (m_PlacementType == ObjectType::Sprite && !m_PlacementSpritePath.empty())
                AddObjectWithSprite(pos, m_PlacementSpritePath);
            else
                AddObject(pos);
        }

        UpdateStatusText();
    }

    if (event.type == sf::Event::MouseButtonReleased &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        if (m_Dragging && m_Selected && m_Selected->entity != 0 && m_Registry.HasComponent<
                TransformComponent>(m_Selected->entity))
        {
            auto &t = m_Registry.GetComponent<TransformComponent>(m_Selected->entity);
            t.x = m_Selected->shape.getPosition().x;
            t.y = m_Selected->shape.getPosition().y;
        }
        m_Dragging = false;
    }



    if (event.type == sf::Event::KeyPressed)
    {
        const bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl);

        if (ctrl && event.key.code == sf::Keyboard::S)
        {
            SaveToJson(ASSET_PATH "scenes/game.json");
            std::cout << "[EditorScene] Saved!\n";
        }

        if (ctrl && event.key.code == sf::Keyboard::L)
        {
            LoadFromJson(ASSET_PATH "scenes/game.json");
            std::cout << "[EditorScene] Loaded!\n";
        }

        if (event.key.code == sf::Keyboard::G)
        {
            m_SnapToGrid = !m_SnapToGrid;
            UpdateStatusText();
        }

        if (event.key.code == sf::Keyboard::Delete)
        {
            DeleteSelected();
            UpdateStatusText();
        }

        if (event.key.code == sf::Keyboard::Escape)
        {
            if (m_ActiveField != EditField::None)
            {
                m_ActiveField = EditField::None;
                m_ActiveInputText.clear();
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

void EditorScene::Update(float deltaTime) {}

void EditorScene::Render(sf::RenderWindow &window)
{
    window.setView(m_camera);
    DrawGrid();

    for (auto &obj: m_Objects)
    {
        if (obj.selected)
        {
            obj.shape.setOutlineColor(sf::Color::Yellow);
            obj.shape.setOutlineThickness(2.f);
        } else
        {
            obj.shape.setOutlineColor(sf::Color::Transparent);
            obj.shape.setOutlineThickness(0.f);
        }
        window.draw(obj.shape);
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
        window.draw(m_Preview);

    sf::View uiView(sf::FloatRect(0.f, 0.f,
                                  static_cast<float>(m_Window.getSize().x),
                                  static_cast<float>(m_Window.getSize().y)));
    window.setView(uiView);
    UpdateBounds();
    const float winH = m_ChooserBounds.top + BrowserHeight;
    const float winW = m_InspectorBounds.left + InspectorWidth;

    m_ContentBrowser->Render(window, m_BrowserBounds.left, m_BrowserBounds.top, m_BrowserBounds.width,
                             m_BrowserBounds.height);

    m_StatusText.setPosition(8.f, 8.f);
    m_StatusText.setString(
        "Mouse(" + std::to_string((int) m_MouseScreenPos.x) + ", " + std::to_string((int) m_MouseScreenPos.y) + ")"
        "  Win(" + std::to_string(m_Window.getSize().x) + "x" + std::to_string(m_Window.getSize().y) + ")"
    );
    window.draw(m_StatusText);

    DrawObjectChooser(window);
    DrawInspector(window);
}

void EditorScene::DrawObjectChooser(sf::RenderWindow &window)
{
    m_ChooserButtons.clear();

    const float x = m_ChooserBounds.left;
    const float y = m_ChooserBounds.top;
    const float panelH = m_ChooserBounds.height;

    const bool isDraggingImage = m_ContentBrowser->HasDraggedAsset() &&
                                 m_ContentBrowser->GetDraggedAsset().type == AssetType::Image;

    sf::RectangleShape bg({ChooserWidth, panelH});
    bg.setPosition(x, y);
    bg.setFillColor(sf::Color(18, 18, 30, 250));
    bg.setOutlineColor(sf::Color(55, 55, 80));
    bg.setOutlineThickness(1.f);
    window.draw(bg);

    sf::RectangleShape headerBg({ChooserWidth, 24.f});
    headerBg.setPosition(x, y);
    headerBg.setFillColor(sf::Color(30, 30, 50, 255));
    window.draw(headerBg);

    sf::Text headerLabel;
    headerLabel.setFont(*m_Font);
    headerLabel.setCharacterSize(11);
    headerLabel.setFillColor(sf::Color(160, 160, 220));
    headerLabel.setStyle(sf::Text::Bold);
    headerLabel.setString("PLACE");
    headerLabel.setPosition(x + 8.f, y + 5.f);
    window.draw(headerLabel); {
        const bool active = m_PlacementType == ObjectType::Rectangle;
        sf::FloatRect btnRect(x + 6.f, y + 30.f, ChooserWidth - 12.f, 26.f);
        m_ChooserButtons.emplace_back(btnRect, ObjectType::Rectangle);
        bool hovered = btnRect.contains(m_MouseScreenPos);
        sf::Color accent(100, 160, 255);

        sf::RectangleShape btn({btnRect.width, btnRect.height});
        btn.setPosition(btnRect.left, btnRect.top);
        btn.setFillColor(
            active ? sf::Color(100, 160, 255, 50) : hovered ? sf::Color(40, 40, 60) : sf::Color(28, 28, 42));
        btn.setOutlineColor(active ? accent : hovered ? sf::Color(80, 80, 120) : sf::Color(45, 45, 65));
        btn.setOutlineThickness(1.f);
        window.draw(btn);

        sf::RectangleShape icon({10.f, 10.f});
        icon.setPosition(btnRect.left + 6.f, btnRect.top + 8.f);
        icon.setFillColor(active ? sf::Color(100, 160, 255, 180) : sf::Color(80, 80, 100));
        window.draw(icon);

        sf::Text t;
        t.setFont(*m_Font);
        t.setCharacterSize(11);
        t.setFillColor(active ? accent : sf::Color(180, 180, 200));
        t.setString("Rectangle");
        t.setPosition(btnRect.left + 22.f, btnRect.top + 7.f);
        window.draw(t);
    } {
        const bool active = m_PlacementType == ObjectType::Sprite;
        const float dzY = y + 64.f;
        const float dzH = panelH - 64.f - 8.f;
        sf::FloatRect dropZone(x + 6.f, dzY, ChooserWidth - 12.f, dzH);
        m_ChooserButtons.emplace_back(dropZone, ObjectType::Sprite);

        bool hovered = dropZone.contains(m_MouseScreenPos);
        bool dropHovered = isDraggingImage && hovered;

        sf::Color accent = dropHovered
                               ? sf::Color(255, 220, 80)
                               : active
                                     ? sf::Color(220, 180, 80)
                                     : sf::Color(80, 80, 100);

        sf::RectangleShape dz({dropZone.width, dropZone.height});
        dz.setPosition(dropZone.left, dropZone.top);
        dz.setFillColor(dropHovered
                            ? sf::Color(80, 60, 0, 120)
                            : active
                                  ? sf::Color(220, 180, 80, 30)
                                  : sf::Color(28, 28, 42));
        dz.setOutlineColor(accent);
        dz.setOutlineThickness(dropHovered ? 2.f : 1.f);
        window.draw(dz);

        sf::RectangleShape imgIcon({28.f, 22.f});
        imgIcon.setPosition(dropZone.left + (dropZone.width - 28.f) / 2.f, dropZone.top + 8.f);
        imgIcon.setFillColor(sf::Color(accent.r, accent.g, accent.b, 40));
        imgIcon.setOutlineColor(accent);
        imgIcon.setOutlineThickness(1.f);
        window.draw(imgIcon);

        sf::Text label2;
        label2.setFont(*m_Font);
        label2.setCharacterSize(10);
        label2.setStyle(active || dropHovered ? sf::Text::Bold : sf::Text::Regular);
        label2.setFillColor(accent);
        if (dropHovered)
            label2.setString("Drop here!");
        else if (!m_PlacementSpritePath.empty())
            label2.setString(fs::path(m_PlacementSpritePath).filename().string().substr(0, 12));
        else
            label2.setString("Sprite");
        label2.setPosition(
            dropZone.left + (dropZone.width - label2.getLocalBounds().width) / 2.f,
            dropZone.top + 34.f);
        window.draw(label2);

        sf::Text hint;
        hint.setFont(*m_Font);
        hint.setCharacterSize(9);
        hint.setFillColor(dropHovered
                              ? sf::Color(255, 220, 80, 220)
                              : active && m_PlacementSpritePath.empty()
                                    ? sf::Color(200, 140, 60)
                                    : sf::Color(80, 80, 110));
        hint.setString(dropHovered
                           ? "release to set sprite"
                           : active && !m_PlacementSpritePath.empty()
                                 ? "LMB to place"
                                 : "drag image here");
        hint.setPosition(
            dropZone.left + (dropZone.width - hint.getLocalBounds().width) / 2.f,
            dropZone.top + 50.f);
        window.draw(hint);
    }
}

void EditorScene::DrawInspector(sf::RenderWindow &window)
{
    m_InspectorButtons.clear();

    const float x = m_InspectorBounds.left;
    const float maxH = m_InspectorBounds.height;

    m_InspectorPanel.setPosition(x, 0.f);
    m_InspectorPanel.setSize({InspectorWidth, maxH});
    window.draw(m_InspectorPanel);

    sf::RectangleShape header({InspectorWidth, 30.f});
    header.setPosition(x, 0.f);
    header.setFillColor(sf::Color(35, 35, 55, 255));
    window.draw(header);

    sf::Text title;
    title.setFont(*m_Font);
    title.setCharacterSize(12);
    title.setFillColor(sf::Color(160, 160, 220));
    title.setStyle(sf::Text::Bold);
    title.setString("INSPECTOR");
    title.setPosition(x + InspectorPad, 8.f);
    window.draw(title);

    if (!m_Selected)
    {
        sf::Text empty;
        empty.setFont(*m_Font);
        empty.setCharacterSize(11);
        empty.setFillColor(sf::Color(80, 80, 100));
        empty.setString("No object selected");
        empty.setPosition(x + InspectorPad, 48.f);
        window.draw(empty);
        return;
    }

    float y = 34.f;


    y = DrawSectionHeader(window, "Object", sf::Color(100, 180, 255), x, y);
    std::string nameDisplay = (m_ActiveField == EditField::Name && !m_ActiveInputText.empty())
                                  ? m_ActiveInputText + "|"
                                  : (m_ActiveField == EditField::Name ? "|" : m_Selected->id);
    y = DrawEditableRow(window, "Name", nameDisplay, "edit_name", x, y);
    y = DrawRow(window, "Entity", std::to_string(m_Selected->entity), x, y);
    y += 6.f;


    y = DrawSectionHeader(window, "Transform", sf::Color(120, 220, 120), x, y);
    std::string txDisplay = (m_ActiveField == EditField::TransformX && !m_ActiveInputText.empty())
                                ? m_ActiveInputText + "|"
                                : (m_ActiveField == EditField::TransformX
                                       ? "|"
                                       : std::to_string((int) m_Selected->shape.getPosition().x));
    y = DrawEditableRow(window, "x", txDisplay, "edit_x", x, y);

    std::string tyDisplay = (m_ActiveField == EditField::TransformY && !m_ActiveInputText.empty())
                                ? m_ActiveInputText + "|"
                                : (m_ActiveField == EditField::TransformY
                                       ? "|"
                                       : std::to_string((int) m_Selected->shape.getPosition().y));
    y = DrawEditableRow(window, "y", tyDisplay, "edit_y", x, y);
    y += 6.f;


    y = DrawSectionHeader(window, "Render", sf::Color(220, 180, 80), x, y);
    std::string wDisplay = (m_ActiveField == EditField::SizeW && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::SizeW
                                      ? "|"
                                      : std::to_string((int) m_Selected->shape.getSize().x));
    y = DrawEditableRow(window, "width", wDisplay, "edit_w", x, y);

    std::string hDisplay = (m_ActiveField == EditField::SizeH && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::SizeH
                                      ? "|"
                                      : std::to_string((int) m_Selected->shape.getSize().y));
    y = DrawEditableRow(window, "height", hDisplay, "edit_h", x, y);

    std::string rDisplay = (m_ActiveField == EditField::ColorR && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::ColorR ? "|" : std::to_string(m_Selected->color.r));
    y = DrawEditableRow(window, "r", rDisplay, "edit_r", x, y);

    std::string gDisplay = (m_ActiveField == EditField::ColorG && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::ColorG ? "|" : std::to_string(m_Selected->color.g));
    y = DrawEditableRow(window, "g", gDisplay, "edit_g", x, y);

    std::string bDisplay = (m_ActiveField == EditField::ColorB && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::ColorB ? "|" : std::to_string(m_Selected->color.b));
    y = DrawEditableRow(window, "b", bDisplay, "edit_b", x, y);

    if (m_Selected->objectType == ObjectType::Sprite)
    {
        std::string spriteName = m_Selected->spritePath;
        const size_t sl = spriteName.find_last_of("/\\");
        if (sl != std::string::npos) spriteName = spriteName.substr(sl + 1);
        y = DrawRow(window, "sprite", spriteName.empty() ? "none" : spriteName, x, y);
        y = DrawAddButton(window, "Change Sprite", "change_sprite", x, y);
    } else
    {
        sf::RectangleShape colorPreview({18.f, 18.f});
        colorPreview.setFillColor(m_Selected->color);
        colorPreview.setOutlineColor(sf::Color(150, 150, 150));
        colorPreview.setOutlineThickness(1.f);
        colorPreview.setPosition(x + InspectorWidth - 28.f, y - 56.f);
        window.draw(colorPreview);
    }
    y += 6.f;


    if (m_Selected->entity != 0 &&
        m_Registry.HasComponent<VelocityComponent>(m_Selected->entity))
    {
        auto &vel = m_Registry.GetComponent<VelocityComponent>(m_Selected->entity);
        y = DrawSectionHeader(window, "Velocity", sf::Color(220, 120, 220), x, y);
        y = DrawRow(window, "dx", std::to_string(vel.dx), x, y);
        y = DrawRow(window, "dy", std::to_string(vel.dy), x, y);
        y = DrawRemoveButton(window, "- Remove VelocityComponent", "remove_velocity", x, y);
        y += 6.f;
    } else if (m_Selected->entity != 0) { y = DrawAddButton(window, "+ Add VelocityComponent", "add_velocity", x, y); }


    if (m_Selected->entity != 0 &&
        m_Registry.HasComponent<ScriptComponent>(m_Selected->entity))
    {
        y = DrawSectionHeader(window, "Script", sf::Color(220, 100, 100), x, y);

        std::string scriptName = m_Selected->scriptPath;
        const size_t slash = scriptName.find_last_of("/\\");

        if (slash != std::string::npos) scriptName = scriptName.substr(slash + 1);
        y = DrawRow(window, "file", scriptName, x, y);
        y = DrawRow(window, "OnCreate", "bound", x, y);
        y = DrawRow(window, "OnUpdate", "bound", x, y);
        y = DrawAddButton(window, "Open Script", "open_script", x, y);
        y = DrawRemoveButton(window, "- Remove ScriptComponent", "remove_script", x, y);
        y += 6.f;
    } else if (m_Selected->entity != 0)
    {
        y = DrawAddButton(window, "+ Add ScriptComponent", "add_script", x, y);
        if (m_ActiveField == EditField::Script)
            y = DrawScriptInput(window, x, y);
    }
}

float EditorScene::DrawSectionHeader(sf::RenderWindow &window, const std::string &title,
                                     sf::Color accent, float x, float y)
{
    sf::RectangleShape bar({3.f, 18.f});
    bar.setFillColor(accent);
    bar.setPosition(x + InspectorPad, y + 1.f);
    window.draw(bar);

    sf::Text text;
    text.setFont(*m_Font);
    text.setCharacterSize(12);
    text.setFillColor(accent);
    text.setStyle(sf::Text::Bold);
    text.setString(title);
    text.setPosition(x + InspectorPad + 8.f, y + 2.f);
    window.draw(text);

    sf::RectangleShape line({InspectorWidth - InspectorPad * 2, 1.f});
    line.setFillColor(sf::Color(60, 60, 80));
    line.setPosition(x + InspectorPad, y + 20.f);
    window.draw(line);

    return y + 26.f;
}

float EditorScene::DrawRow(sf::RenderWindow &window, const std::string &key,
                           const std::string &val, float x, float y)
{
    sf::Text keyText;
    keyText.setFont(*m_Font);
    keyText.setCharacterSize(12);
    keyText.setFillColor(sf::Color(140, 140, 160));
    keyText.setString(key);
    keyText.setPosition(x + InspectorPad + 8.f, y);
    window.draw(keyText);

    sf::Text valText;
    valText.setFont(*m_Font);
    valText.setCharacterSize(12);
    valText.setFillColor(sf::Color(220, 220, 220));
    valText.setString(val);
    valText.setPosition(x + InspectorWidth * 0.55f, y);
    window.draw(valText);

    return y + 18.f;
}

float EditorScene::DrawEditableRow(sf::RenderWindow &window, const std::string &key, const std::string &val,
                                   const std::string &action, float x, float y)
{
    sf::Text keyText;
    keyText.setFont(*m_Font);
    keyText.setCharacterSize(12);
    keyText.setFillColor(sf::Color(140, 140, 160));
    keyText.setString(key);
    keyText.setPosition(x + InspectorPad + 8.f, y);
    window.draw(keyText);

    const float valX = x + InspectorWidth * 0.45f;
    const float valW = InspectorWidth - InspectorWidth * 0.45f - InspectorPad;
    sf::FloatRect boundsOut = sf::FloatRect(valX - 2.f, y - 1.f, valW, 16.f);
    bool hovered = boundsOut.contains(m_MouseScreenPos);

    sf::RectangleShape bg({boundsOut.width, boundsOut.height});
    bg.setPosition(boundsOut.left, boundsOut.top);
    bg.setFillColor(hovered ? sf::Color(50, 50, 80, 200) : sf::Color(35, 35, 55, 150));
    bg.setOutlineColor(hovered ? sf::Color(120, 120, 120) : sf::Color(60, 60, 90));
    bg.setOutlineThickness(1.f);
    window.draw(bg);

    sf::Text valText;
    valText.setFont(*m_Font);
    valText.setCharacterSize(12);
    valText.setFillColor(sf::Color(220, 220, 255));
    valText.setString(val);
    valText.setPosition(valX, y);
    window.draw(valText);

    m_InspectorButtons.push_back({boundsOut, action});

    return y + 18.f;
}

float EditorScene::DrawAddButton(sf::RenderWindow &window, const std::string &label,
                                 const std::string &action, float x, float y)
{
    sf::FloatRect btnRect(x + InspectorPad, y, InspectorWidth - InspectorPad * 2, 22.f);
    bool hovered = btnRect.contains(m_MouseScreenPos);

    sf::RectangleShape btn({btnRect.width, btnRect.height});
    btn.setFillColor(hovered ? sf::Color(60, 90, 60, 220) : sf::Color(40, 60, 40, 200));
    btn.setOutlineColor(hovered ? sf::Color(120, 180, 120) : sf::Color(80, 120, 80));
    btn.setOutlineThickness(1.f);
    btn.setPosition(btnRect.left, btnRect.top);
    window.draw(btn);

    sf::Text text;
    text.setFont(*m_Font);
    text.setCharacterSize(11);
    text.setFillColor(hovered ? sf::Color(160, 230, 160) : sf::Color(120, 200, 120));
    text.setString(label);
    text.setPosition(x + InspectorPad + 6.f, y + 4.f);
    window.draw(text);

    m_InspectorButtons.push_back({btnRect, action});

    return y + 28.f;
}

float EditorScene::DrawRemoveButton(sf::RenderWindow &window, const std::string &label,
                                    const std::string &action, float x, float y)
{
    sf::FloatRect btnRect(x + InspectorPad, y, InspectorWidth - InspectorPad * 2, 22.f);
    bool hovered = btnRect.contains(m_MouseScreenPos);

    sf::RectangleShape btn({btnRect.width, btnRect.height});
    btn.setFillColor(hovered ? sf::Color(120, 40, 40, 220) : sf::Color(80, 30, 30, 200));
    btn.setOutlineColor(hovered ? sf::Color(200, 80, 80) : sf::Color(140, 60, 60));
    btn.setOutlineThickness(1.f);
    btn.setPosition(btnRect.left, btnRect.top);
    window.draw(btn);

    sf::Text text;
    text.setFont(*m_Font);
    text.setCharacterSize(11);
    text.setFillColor(hovered ? sf::Color(255, 180, 180) : sf::Color(220, 140, 140));
    text.setString(label);
    text.setPosition(x + InspectorPad + 6.f, y + 4.f);
    window.draw(text);

    m_InspectorButtons.push_back({btnRect, action});

    return y + 28.f;
}

float EditorScene::DrawScriptInput(sf::RenderWindow &window, float x, float y)
{
    sf::RectangleShape bg({InspectorWidth - InspectorPad * 2, 22.f});
    bg.setFillColor(sf::Color(30, 30, 50, 240));
    bg.setOutlineColor(sf::Color(100, 100, 200));
    bg.setOutlineThickness(1.f);
    bg.setPosition(x + InspectorPad, y);
    window.draw(bg);

    std::string display = (m_ActiveField == EditField::Script && !m_ActiveInputText.empty())
                              ? m_ActiveInputText
                              : (m_ActiveField != EditField::Script)
                                    ? "scripting/name.lua"
                                    : "";
    sf::Color textColor = display == "scripting/name.lua" ? sf::Color(80, 80, 120) : sf::Color(200, 200, 255);

    sf::Text inputText;
    inputText.setFont(*m_Font);
    inputText.setCharacterSize(11);
    inputText.setFillColor(textColor);
    inputText.setString(display + (m_ActiveField == EditField::Script ? "|" : ""));
    inputText.setPosition(x + InspectorPad + 4.f, y + 4.f);
    window.draw(inputText);

    m_ActiveInputBounds = bg.getGlobalBounds();
    m_InspectorButtons.push_back({m_ActiveInputBounds, "edit_script"});

    sf::Text hint;
    hint.setFont(*m_Font);
    hint.setCharacterSize(10);
    hint.setFillColor(sf::Color(80, 80, 100));
    hint.setString("Enter to confirm");
    hint.setPosition(x + InspectorPad, y + 26.f);
    window.draw(hint);

    return y + 42.f;
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

void EditorScene::AddObject(sf::Vector2f pos)
{
    EditorObject obj;
    obj.id = NextId();
    obj.color = sf::Color(100, 149, 237);
    obj.shape.setSize({m_GridSize, m_GridSize});
    obj.shape.setPosition(SnapToGrid(pos));
    obj.shape.setFillColor(obj.color);

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
    obj.shape.setSize({m_GridSize * 2.f, m_GridSize * 2.f});
    obj.shape.setPosition(SnapToGrid(pos));
    obj.shape.setFillColor(sf::Color::White);
    obj.objectType = ObjectType::Sprite;

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
        const sf::Vector2f size = obj.shape.getSize();
        if (ts.x > 0 && ts.y > 0) { obj.previewSprite.setScale(size.x / ts.x, size.y / ts.y); }
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

    std::erase_if(m_Objects,
                  [this](const EditorObject &o) { return &o == m_Selected; });
    m_Selected = nullptr;
}

void EditorScene::SaveToJson(const std::string &path)
{
    json data;
    data["name"] = "game";

    for (auto &obj: m_Objects)
    {
        json j;
        j["id"] = obj.id;
        j["type"] = "rectangle";
        j["x"] = obj.shape.getPosition().x;
        j["y"] = obj.shape.getPosition().y;
        j["width"] = obj.shape.getSize().x;
        j["height"] = obj.shape.getSize().y;
        j["color"] = {obj.color.r, obj.color.g, obj.color.b};
        data["objects"].push_back(j);

        if (!obj.spritePath.empty())
            j["sprite"] = obj.spritePath;
        data["objects"].push_back(j);

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

        obj.entity = m_Registry.CreateEntity();
        m_Registry.AddComponent(obj.entity, TransformComponent{j["x"], j["y"]});
        m_Registry.AddComponent(obj.entity, RenderComponent{obj.color, obj.shape.getSize()});

        if (j.contains("sprite"))
        {
            std::string sp = j["sprite"];
            ApplySpriteToObject(obj, sp);
        }

        if (j.contains("velocity"))
        {
            m_Registry.AddComponent(obj.entity, VelocityComponent
                                    {j["velocity"]["dx"], j["velocity"]["dy"]});
        }

        if (j.contains("script"))
        {
            std::string scriptPath = j["script"];
            auto &sc = m_Registry.AddComponent(obj.entity, ScriptComponent(LuaState::GetLua(), scriptPath));
            sc.SetEntity(obj.entity);
            sc.OnCreate();
            obj.scriptPath = scriptPath;
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
    return
    {
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

    sf::Vector2f center = m_camera.getCenter();
    sf::Vector2f camSize = m_camera.getSize();

    float left = std::floor((center.x - camSize.x / 2) / m_GridSize) * m_GridSize;
    float top = std::floor((center.y - camSize.y / 2) / m_GridSize) * m_GridSize;
    float right = center.x + camSize.x / 2 + m_GridSize;
    float bottom = center.y + camSize.y / 2 + m_GridSize;

    for (float x = left; x < right; x += m_GridSize)
    {
        lines.append({{x, top}, sf::Color(55, 55, 55)});
        lines.append({{x, bottom}, sf::Color(55, 55, 55)});
    }
    for (float y = top; y < bottom; y += m_GridSize)
    {
        lines.append({{left, y}, sf::Color(55, 55, 55)});
        lines.append({{right, y}, sf::Color(55, 55, 55)});
    }
    m_Window.draw(lines);
}

void EditorScene::UpdateStatusText()
{
    std::string s = "Objects: " + std::to_string(m_Objects.size());
    s += "   Grid: " + std::string(m_SnapToGrid ? "ON" : "OFF");
    if (m_Selected)
        s += "   Selected: " + m_Selected->id
                + "  @ (" + std::to_string((int) m_Selected->shape.getPosition().x)
                + ", " + std::to_string((int) m_Selected->shape.getPosition().y) + ")";
    m_StatusText.setString(s);
}

std::string EditorScene::NextId() { return "obj_" + std::to_string(m_IdCounter++); }
