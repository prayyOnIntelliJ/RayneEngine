#include "EditorScene.h"
#include "../Scenes/SceneManager.h"
#include <iostream>
#include <algorithm>
#include <fstream>

#include "../../../cmake-build-debug/_deps/sfml-src/src/SFML/Window/Win32/CursorImpl.hpp"
#include "../Scripting/ScriptComponent.h"
#include "SFML/Window/Event.hpp"

EditorScene::EditorScene(SceneManager& manager, sf::RenderWindow& window, Registry& registry)
    : Scene(manager), m_Window(window), m_Registry(registry)
{
    m_Font.loadFromFile(ASSET_PATH "fonts/Merriweather.ttf");

    m_ContentBrowser = std::make_unique<ContentBrowser>(m_Font, ASSET_PATH);

    m_camera = window.getDefaultView();
    sf::View adjustedView = m_camera;
    adjustedView.setViewport(
        {
        0.f, 0.f,
        1.f - (InspectorWidth / window.getSize().x),
        1.f - (BrowserHeight / window.getSize().y)
        });
    m_camera = adjustedView;

    m_ToolboxSize.x = window.getSize().x;

    m_Preview.setSize({ m_GridSize, m_GridSize });
    m_Preview.setFillColor(sf::Color(100, 200, 100, 80));
    m_Preview.setOutlineColor(sf::Color(100, 255, 100));
    m_Preview.setOutlineThickness(1.f);

    m_Toolbar.setFillColor(sf::Color(30, 30, 30, 220));

    m_HelpText.setFont(m_Font);
    m_HelpText.setCharacterSize(24);
    m_HelpText.setFillColor(sf::Color(180, 180, 180));
    m_HelpText.setString(
        "LMB: Place / Select   RMB: Delete   Drag: Move   "
        "G: Grid   Entf: Delete   Strg+S: Save   Strg+L: Load   F5: Play"
    );

    m_StatusText.setFont(m_Font);
    m_StatusText.setCharacterSize(13);
    m_StatusText.setFillColor(sf::Color::Yellow);

    m_InspectorPanel.setFillColor(sf::Color(22, 22, 32, 240));
    m_InspectorPanel.setOutlineColor(sf::Color(60, 60, 80));
    m_InspectorPanel.setOutlineThickness(1.f);

    UpdateStatusText();
}

void EditorScene::OnEnter()
{
    std::cout << "[EditorScene] Active\n";
    UpdateStatusText();
}

void EditorScene::OnExit()
{
    SyncToRegistry();
}

void EditorScene::HandleEvent(const sf::Event& event)
{
    const float winW = static_cast<float>(m_Window.getSize().x);
    const float winH = static_cast<float>(m_Window.getSize().y);
    const bool inBrowser = m_MouseScreenPos.y >= winH - BrowserHeight &&
                           m_MouseScreenPos.x >= ChooserWidth &&
                           m_MouseScreenPos.x < winW - InspectorWidth;

    if (inBrowser || m_ContentBrowser->HasDraggedAsset())
        m_ContentBrowser->HandleEvent(event, m_MouseScreenPos);

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left && m_ContentBrowser->HasDraggedAsset())
    {
        DraggedAsset drag = m_ContentBrowser->GetDraggedAsset();
        bool handled = false;

        if (drag.type == AssetType::Script)
        {
            EditorObject* hit = ObjectAt(MouseWorldPos());
            if (hit && hit->entity != 0 && !m_Registry.HasComponent<ScriptComponent>(hit->entity))
            {
                auto& sc = m_Registry.AddComponent(hit->entity, ScriptComponent(LuaState::GetLua(), drag.path));
                sc.SetEntity(hit->entity);
                sc.OnCreate();
                hit->scriptPath = drag.path;
                std::cout << "[ContentBrowser] Script dropped onto " << hit->id << "\n";
            }
            handled = true;
        }
        else if (drag.type == AssetType::Image)
        {
            bool onChooser = false;
            for (auto& [rect, type] : m_ChooserButtons)
                if (rect.contains(m_MouseScreenPos)) { onChooser = true; break; }

            if (onChooser)
            {
                m_PlacementType = ObjectType::Sprite;
                m_PlacementSpritePath = drag.path;
                m_PlacementTexture = std::make_shared<sf::Texture>();
                m_PlacementTexture->loadFromFile(drag.path);
                std::cout << "[Chooser] Sprite set: " << drag.path << "\n";
            }
            else if (!inBrowser)
            {
                EditorObject* hit = ObjectAt(MouseWorldPos());
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

    if (event.type == sf::Event::MouseWheelScrolled && inBrowser || event.type == sf::Event::MouseButtonPressed && inBrowser) return;

    if (event.type == sf::Event::TextEntered && m_NameInputActive)
    {
        if (event.text.unicode == '\b')
        {
            if (!m_NameInputText.empty())
                m_NameInputText.pop_back();
        }
        else if (event.text.unicode == '\r' || event.text.unicode == '\n')
        {
            if (m_Selected && !m_NameInputText.empty())
            {
                m_Selected->id = m_NameInputText;
                UpdateStatusText();
            }
            m_NameInputActive = false;
            m_NameInputText.clear();
        }
        else if (event.text.unicode >= 32 && event.text.unicode < 128)
        {
            m_NameInputText += static_cast<char>(event.text.unicode);
        }
        return;
    }
    if (event.type == sf::Event::TextEntered && m_ScriptInputActive)
    {
        if (event.text.unicode == '\b')
        {
            if (!m_ScriptInputText.empty())
                m_ScriptInputText.pop_back();
        }
        else if (event.text.unicode == '\r' || event.text.unicode == '\n')
        {
            if (m_Selected && !m_ScriptInputText.empty())
            {
                std::string fullPath = std::string(ASSET_PATH) + m_ScriptInputText + ".lua";

                std::ifstream check(fullPath);
                if (!check.is_open())
                {
                    std::ofstream newFile(fullPath);
                    newFile << "-- " << m_ScriptInputText << "\n\n";
                    newFile << "function OnCreate()\n\n";
                    newFile << "end\n\n";
                    newFile << "function OnUpdate(dt)\n\n";
                    newFile << "end\n";
                    newFile.close();
                    std::cout << "[Inspector] Created script: " << fullPath << "\n";
                }
                check.close();

                auto& sc = m_Registry.AddComponent(m_Selected->entity, ScriptComponent(LuaState::GetLua(), fullPath));
                sc.SetEntity(m_Selected->entity);
                sc.OnCreate();
                m_Selected->scriptPath = fullPath;
                std::cout << "[Inspector] Created script: " << fullPath << "\n";
            }
            m_ScriptInputActive = false;
            m_ScriptInputText.clear();
        }
        else if (event.text.unicode < 128)
        {
            m_ScriptInputText += static_cast<char>(event.text.unicode);
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

        m_ToolboxSize.x = newW - InspectorWidth;
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
        event.mouseButton.button == sf::Mouse::Middle)
    {
        m_panning = false;
    }

    if (event.type == sf::Event::MouseMoved)
    {
        m_MouseScreenPos = { (float)event.mouseMove.x, (float)event.mouseMove.y };

        if (m_panning)
        {
            sf::Vector2f current = m_Window.mapPixelToCoords(
                { event.mouseMove.x, event.mouseMove.y }, m_camera);
            m_camera.move(m_panStart - current);
        }

        sf::Vector2f pos = MouseWorldPos();
        m_Preview.setPosition(SnapToGrid(pos));

        if (m_Dragging && m_Selected)
        {
            m_Selected->shape.setPosition(SnapToGrid(pos - m_DragOffset));
        }
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        sf::Vector2f screenPos = m_MouseScreenPos;

        for (auto& [rect, type] : m_ChooserButtons)
        {
            if (rect.contains(screenPos))
            {
                m_PlacementType = type;
                return;
            }
        }

        if (screenPos.x >= m_Window.getSize().x - InspectorWidth)
        {
            if (m_ScriptInputBounds.contains(screenPos))
            {
                m_ScriptInputActive = true;
                return;
            }
            m_ScriptInputActive = false;
            HandleInspectorClick(screenPos);
            return;
        }

        m_ScriptInputActive = false;

        sf::Vector2f pos = MouseWorldPos();
        EditorObject* hit = ObjectAt(pos);

        if (m_Selected) m_Selected->selected = false;

        if (hit)
        {
            m_Selected = hit;
            m_Selected->selected = true;
            m_Dragging = true;
            m_DragOffset = pos - m_Selected->shape.getPosition();
        }
        else
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
        if (m_Dragging && m_Selected && m_Selected->entity != 0 && m_Registry.HasComponent<TransformComponent>(m_Selected->entity))
        {
            auto& t = m_Registry.GetComponent<TransformComponent>(m_Selected->entity);
            t.x = m_Selected->shape.getPosition().x;
            t.y = m_Selected->shape.getPosition().y;
        }
        m_Dragging = false;
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Right)
    {
        if (EditorObject* hit = ObjectAt(MouseWorldPos()))
        {
            if (hit->entity != 0)
                m_Registry.DestroyEntity(hit->entity);

            if (m_Selected == hit) m_Selected = nullptr;

            std::erase_if(m_Objects,
                [hit](const EditorObject& o) { return &o == hit; });
            UpdateStatusText();
        }
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
            if (m_NameInputActive) { m_NameInputActive = false; m_NameInputText.clear(); }
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

void EditorScene::Render(sf::RenderWindow& window)
{
    window.setView(m_camera);
    DrawGrid();

    for (auto& obj : m_Objects)
    {
        if (obj.selected)
        {
            obj.shape.setOutlineColor(sf::Color::Yellow);
            obj.shape.setOutlineThickness(2.f);
        }
        else
        {
            obj.shape.setOutlineColor(sf::Color::Transparent);
            obj.shape.setOutlineThickness(0.f);
        }
        window.draw(obj.shape);
    }

    for (auto& obj : m_Objects)
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

    window.setView(window.getDefaultView());
    const float winH = static_cast<float>(window.getSize().y);
    const float winW = static_cast<float>(window.getSize().x);

    m_ContentBrowser->Render(window, ChooserWidth, winH - BrowserHeight, winW - InspectorWidth - ChooserWidth, BrowserHeight);

    m_Toolbar.setPosition(0.f, winH - m_ToolboxSize.y);
    m_Toolbar.setSize({ winW - InspectorWidth, m_ToolboxSize.y });
    window.draw(m_Toolbar);

    m_HelpText.setPosition(8.f, m_Toolbar.getPosition().y);
    window.draw(m_HelpText);

    m_StatusText.setPosition(8.f, 8.f);
    window.draw(m_StatusText);

    DrawObjectChooser(window);
    DrawInspector(window);
}

void EditorScene::DrawObjectChooser(sf::RenderWindow& window)
{
    m_ChooserButtons.clear();

    const float winH = static_cast<float>(window.getSize().y);
    const float x = 0.f;
    const float y = winH - BrowserHeight;
    const float panelH = BrowserHeight;

    const bool isDraggingImage = m_ContentBrowser->HasDraggedAsset() &&
                                 m_ContentBrowser->GetDraggedAsset().type == AssetType::Image;

    sf::RectangleShape bg({ ChooserWidth, panelH });
    bg.setPosition(x, y);
    bg.setFillColor(sf::Color(18, 18, 30, 250));
    bg.setOutlineColor(sf::Color(55, 55, 80));
    bg.setOutlineThickness(1.f);
    window.draw(bg);

    sf::RectangleShape headerBg({ ChooserWidth, 24.f });
    headerBg.setPosition(x, y);
    headerBg.setFillColor(sf::Color(30, 30, 50, 255));
    window.draw(headerBg);

    sf::Text headerLabel;
    headerLabel.setFont(m_Font);
    headerLabel.setCharacterSize(11);
    headerLabel.setFillColor(sf::Color(160, 160, 220));
    headerLabel.setStyle(sf::Text::Bold);
    headerLabel.setString("PLACE");
    headerLabel.setPosition(x + 8.f, y + 5.f);
    window.draw(headerLabel);

    // Rectangle button
    {
        const bool active = m_PlacementType == ObjectType::Rectangle;
        sf::FloatRect btnRect(x + 6.f, y + 30.f, ChooserWidth - 12.f, 26.f);
        m_ChooserButtons.emplace_back(btnRect, ObjectType::Rectangle);
        bool hovered = btnRect.contains(m_MouseScreenPos);
        sf::Color accent(100, 160, 255);

        sf::RectangleShape btn({ btnRect.width, btnRect.height });
        btn.setPosition(btnRect.left, btnRect.top);
        btn.setFillColor(active ? sf::Color(100, 160, 255, 50) : hovered ? sf::Color(40, 40, 60) : sf::Color(28, 28, 42));
        btn.setOutlineColor(active ? accent : hovered ? sf::Color(80, 80, 120) : sf::Color(45, 45, 65));
        btn.setOutlineThickness(1.f);
        window.draw(btn);

        sf::RectangleShape icon({ 10.f, 10.f });
        icon.setPosition(btnRect.left + 6.f, btnRect.top + 8.f);
        icon.setFillColor(active ? sf::Color(100, 160, 255, 180) : sf::Color(80, 80, 100));
        window.draw(icon);

        sf::Text t;
        t.setFont(m_Font); t.setCharacterSize(11);
        t.setFillColor(active ? accent : sf::Color(180, 180, 200));
        t.setString("Rectangle");
        t.setPosition(btnRect.left + 22.f, btnRect.top + 7.f);
        window.draw(t);
    }

    // Sprite dropzone
    {
        const bool active = m_PlacementType == ObjectType::Sprite;
        const float dzY = y + 64.f;
        const float dzH = panelH - 64.f - 8.f;
        sf::FloatRect dropZone(x + 6.f, dzY, ChooserWidth - 12.f, dzH);
        m_ChooserButtons.emplace_back(dropZone, ObjectType::Sprite);

        bool hovered = dropZone.contains(m_MouseScreenPos);
        bool dropHovered = isDraggingImage && hovered;

        sf::Color accent = dropHovered ? sf::Color(255, 220, 80) :
                           active      ? sf::Color(220, 180, 80) :
                                         sf::Color(80, 80, 100);

        sf::RectangleShape dz({ dropZone.width, dropZone.height });
        dz.setPosition(dropZone.left, dropZone.top);
        dz.setFillColor(dropHovered ? sf::Color(80, 60, 0, 120) :
                        active      ? sf::Color(220, 180, 80, 30) :
                                      sf::Color(28, 28, 42));
        dz.setOutlineColor(accent);
        dz.setOutlineThickness(dropHovered ? 2.f : 1.f);
        window.draw(dz);

        sf::RectangleShape imgIcon({ 28.f, 22.f });
        imgIcon.setPosition(dropZone.left + (dropZone.width - 28.f) / 2.f, dropZone.top + 8.f);
        imgIcon.setFillColor(sf::Color(accent.r, accent.g, accent.b, 40));
        imgIcon.setOutlineColor(accent);
        imgIcon.setOutlineThickness(1.f);
        window.draw(imgIcon);

        sf::Text label2;
        label2.setFont(m_Font);
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
        hint.setFont(m_Font);
        hint.setCharacterSize(9);
        hint.setFillColor(dropHovered ? sf::Color(255, 220, 80, 220) :
                          active && m_PlacementSpritePath.empty() ? sf::Color(200, 140, 60) :
                          sf::Color(80, 80, 110));
        hint.setString(dropHovered            ? "release to set sprite" :
                       active && !m_PlacementSpritePath.empty() ? "LMB to place" :
                                                                   "drag image here");
        hint.setPosition(
            dropZone.left + (dropZone.width - hint.getLocalBounds().width) / 2.f,
            dropZone.top + 50.f);
        window.draw(hint);
    }
}

void EditorScene::DrawInspector(sf::RenderWindow& window)
{
    m_InspectorButtons.clear();

    const float winW = static_cast<float>(window.getSize().x);
    const float winH = static_cast<float>(window.getSize().y);
    const float x = winW - InspectorWidth;
    const float maxH = winH - m_ToolboxSize.y;

    m_InspectorPanel.setPosition(x, 0.f);
    m_InspectorPanel.setSize({ InspectorWidth, maxH });
    window.draw(m_InspectorPanel);

    sf::RectangleShape header({ InspectorWidth, 30.f });
    header.setPosition(x, 0.f);
    header.setFillColor(sf::Color(35, 35, 55, 255));
    window.draw(header);

    sf::Text title;
    title.setFont(m_Font);
    title.setCharacterSize(12);
    title.setFillColor(sf::Color(160, 160, 220));
    title.setStyle(sf::Text::Bold);
    title.setString("INSPECTOR");
    title.setPosition(x + InspectorPad, 8.f);
    window.draw(title);

    if (!m_Selected)
    {
        sf::Text empty;
        empty.setFont(m_Font);
        empty.setCharacterSize(11);
        empty.setFillColor(sf::Color(80, 80, 100));
        empty.setString("No object selected");
        empty.setPosition(x + InspectorPad, 48.f);
        window.draw(empty);
        return;
    }

    float y = 34.f;

    // Name / Entity
    y = DrawSectionHeader(window, "Object", sf::Color(100, 180, 255), x, y);
    y = DrawEditableRow(window, "Name", m_NameInputActive ? m_NameInputText + "|" : m_Selected->id, "edit_name", m_NameInputBounds, x, y);
    y = DrawRow(window, "Entity", std::to_string(m_Selected->entity), x, y);
    y += 6.f;

    // TransformComponent
    y = DrawSectionHeader(window, "Transform", sf::Color(120, 220, 120), x, y);
    y = DrawRow(window, "x", std::to_string((int)m_Selected->shape.getPosition().x), x, y);
    y = DrawRow(window, "y", std::to_string((int)m_Selected->shape.getPosition().y), x, y);
    y += 6.f;

    // RenderComponent
    y = DrawSectionHeader(window, "Render", sf::Color(220, 180, 80), x, y);
    y = DrawRow(window, "width",  std::to_string((int)m_Selected->shape.getSize().x), x, y);
    y = DrawRow(window, "height", std::to_string((int)m_Selected->shape.getSize().y), x, y);
    y = DrawRow(window, "r", std::to_string(m_Selected->color.r), x, y);
    y = DrawRow(window, "g", std::to_string(m_Selected->color.g), x, y);
    y = DrawRow(window, "b", std::to_string(m_Selected->color.b), x, y);

    if (m_Selected->objectType == ObjectType::Sprite)
    {
        std::string spriteName = m_Selected->spritePath;
        const size_t sl = spriteName.find_last_of("/\\");
        if (sl != std::string::npos) spriteName = spriteName.substr(sl + 1);
        y = DrawRow(window, "sprite", spriteName.empty() ? "none" : spriteName, x, y);
        y = DrawAddButton(window, "Change Sprite", "change_sprite", x, y);
    }
    else
    {
        y = DrawRow(window, "r", std::to_string(m_Selected->color.r), x, y);
        y = DrawRow(window, "g", std::to_string(m_Selected->color.g), x, y);
        y = DrawRow(window, "b", std::to_string(m_Selected->color.b), x, y);

        sf::RectangleShape colorPreview({ 18.f, 18.f });
        colorPreview.setFillColor(m_Selected->color);
        colorPreview.setOutlineColor(sf::Color(150, 150, 150));
        colorPreview.setOutlineThickness(1.f);
        colorPreview.setPosition(x + InspectorWidth - 28.f, y - 20.f);
        window.draw(colorPreview);
    }
    y += 6.f;

    // VelocityComponent
    if (m_Selected->entity != 0 &&
        m_Registry.HasComponent<VelocityComponent>(m_Selected->entity))
    {
        auto& vel = m_Registry.GetComponent<VelocityComponent>(m_Selected->entity);
        y = DrawSectionHeader(window, "Velocity", sf::Color(220, 120, 220), x, y);
        y = DrawRow(window, "dx", std::to_string(vel.dx), x, y);
        y = DrawRow(window, "dy", std::to_string(vel.dy), x, y);
        y += 6.f;
    }
    else if (m_Selected->entity != 0)
    {
        y = DrawAddButton(window, "+ Add VelocityComponent", "add_velocity", x, y);
    }

    // ScriptComponent
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
        y += 6.f;
    }
    else if (m_Selected->entity != 0)
    {
        y = DrawAddButton(window, "+ Add ScriptComponent", "add_script", x, y);
        if (m_ScriptInputActive)
            y = DrawScriptInput(window, x, y);
    }
}

float EditorScene::DrawSectionHeader(sf::RenderWindow& window, const std::string& title,
    sf::Color accent, float x, float y)
{
    sf::RectangleShape bar({ 3.f, 18.f });
    bar.setFillColor(accent);
    bar.setPosition(x + InspectorPad, y + 1.f);
    window.draw(bar);

    sf::Text text;
    text.setFont(m_Font);
    text.setCharacterSize(12);
    text.setFillColor(accent);
    text.setStyle(sf::Text::Bold);
    text.setString(title);
    text.setPosition(x + InspectorPad + 8.f, y + 2.f);
    window.draw(text);

    sf::RectangleShape line({ InspectorWidth - InspectorPad * 2, 1.f });
    line.setFillColor(sf::Color(60, 60, 80));
    line.setPosition(x + InspectorPad, y + 20.f);
    window.draw(line);

    return y + 26.f;
}

float EditorScene::DrawRow(sf::RenderWindow& window, const std::string& key,
    const std::string& val, float x, float y)
{
    sf::Text keyText;
    keyText.setFont(m_Font);
    keyText.setCharacterSize(12);
    keyText.setFillColor(sf::Color(140, 140, 160));
    keyText.setString(key);
    keyText.setPosition(x + InspectorPad + 8.f, y);
    window.draw(keyText);

    sf::Text valText;
    valText.setFont(m_Font);
    valText.setCharacterSize(12);
    valText.setFillColor(sf::Color(220, 220, 220));
    valText.setString(val);
    valText.setPosition(x + InspectorWidth * 0.55f, y);
    window.draw(valText);

    return y + 18.f;
}

float EditorScene::DrawEditableRow(sf::RenderWindow& window, const std::string& key, const std::string& val,
    const std::string& action, sf::FloatRect& boundsOut, float x, float y)
{
    sf::Text keyText;
    keyText.setFont(m_Font);
    keyText.setCharacterSize(12);
    keyText.setFillColor(sf::Color(140, 140, 160));
    keyText.setString(key);
    keyText.setPosition(x + InspectorPad + 8.f, y);
    window.draw(keyText);

    const float valX = x + InspectorWidth * 0.55f;
    const float valW = InspectorWidth - InspectorWidth * 0.55f - InspectorPad;
    boundsOut = sf::FloatRect(valX - 2.f, y - 1.f, valW, 16.f);
    bool hovered = boundsOut.contains(m_MouseScreenPos);

    sf::RectangleShape bg({ boundsOut.width, boundsOut.height });
    bg.setPosition(boundsOut.left, boundsOut.top);
    bg.setFillColor(hovered ? sf::Color(50, 50, 80, 200) : sf::Color(35, 35, 55, 150));
    bg.setOutlineColor(hovered ? sf::Color(120, 120, 120) : sf::Color(60, 60, 90));
    bg.setOutlineThickness(1.f);
    window.draw(bg);

    sf::Text valText;
    valText.setFont(m_Font);
    valText.setCharacterSize(12);
    valText.setFillColor(sf::Color(220, 220, 255));
    valText.setString(val);
    valText.setPosition(valX, y);
    window.draw(valText);

    m_InspectorButtons.push_back({ boundsOut, action });

    return y + 18.f;
}

float EditorScene::DrawAddButton(sf::RenderWindow& window, const std::string& label,
                                 const std::string& action, float x, float y)
{
    sf::FloatRect btnRect(x + InspectorPad, y, InspectorWidth - InspectorPad * 2, 22.f);
    bool hovered = btnRect.contains(m_MouseScreenPos);

    sf::RectangleShape btn({ btnRect.width, btnRect.height });
    btn.setFillColor(hovered ? sf::Color(60, 90, 60, 220) : sf::Color(40, 60, 40, 200));
    btn.setOutlineColor(hovered ? sf::Color(120, 180, 120) : sf::Color(80, 120, 80));
    btn.setOutlineThickness(1.f);
    btn.setPosition(btnRect.left, btnRect.top);
    window.draw(btn);

    sf::Text text;
    text.setFont(m_Font);
    text.setCharacterSize(11);
    text.setFillColor(hovered ? sf::Color(160, 230, 160) : sf::Color(120, 200, 120));
    text.setString(label);
    text.setPosition(x + InspectorPad + 6.f, y + 4.f);
    window.draw(text);

    m_InspectorButtons.push_back({ btnRect, action });

    return y + 28.f;
}

float EditorScene::DrawScriptInput(sf::RenderWindow &window, float x, float y)
{
    sf::RectangleShape bg({ InspectorWidth - InspectorPad * 2, 22.f });
    bg.setFillColor(sf::Color(30, 30, 50, 240));
    bg.setOutlineColor(sf::Color(100, 100, 200));
    bg.setOutlineThickness(1.f);
    bg.setPosition(x + InspectorPad, y);
    window.draw(bg);

    std::string display = m_ScriptInputText.empty() ? "scripting/name.lua" : m_ScriptInputText;
    sf::Color textColor = m_ScriptInputText.empty() ? sf::Color(80, 80, 120) : sf::Color(200, 200, 255);

    sf::Text inputText;
    inputText.setFont(m_Font);
    inputText.setCharacterSize(11);
    inputText.setFillColor(textColor);
    inputText.setString(display + (m_ScriptInputActive ? "|" : ""));
    inputText.setPosition(x + InspectorPad + 4.f, y + 4.f);
    window.draw(inputText);

    m_ScriptInputBounds = bg.getGlobalBounds();

    sf::Text hint;
    hint.setFont(m_Font);
    hint.setCharacterSize(10);
    hint.setFillColor(sf::Color(80, 80, 100));
    hint.setString("Enter to confirm");
    hint.setPosition(x + InspectorPad, y + 26.f);
    window.draw(hint);

    return y + 42.f;
}

void EditorScene::HandleInspectorClick(sf::Vector2f pos)
{
    for (auto& btn : m_InspectorButtons)
    {
        if (!btn.bounds.contains(pos)) continue;

        if (btn.action == "add_velocity")
        {
            m_Registry.AddComponent(m_Selected->entity, VelocityComponent{ 0.f, 0.f });
            std::cout << "[Inspector] VelocityComponent added to " << m_Selected->id << "\n";
        }
        else if (btn.action == "add_script")
        {
            m_ScriptInputActive = true;
        }
        else if (btn.action == "edit_name" && m_Selected)
        {
            m_NameInputActive = true;
            m_NameInputText = m_Selected->id;
        }
        else if (btn.action == "open_script" && m_Selected && !m_Selected->scriptPath.empty())
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
    obj.shape.setSize({ m_GridSize, m_GridSize });
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

    auto& placed = m_Objects.back();
    if (placed.previewTexture)
        placed.previewSprite.setTexture(*placed.previewTexture, true);

    UpdateStatusText();
}

void EditorScene::AddObjectWithSprite(sf::Vector2f pos, const std::string &spritePath)
{
    EditorObject obj;
    obj.id = NextId();
    obj.color = sf::Color::White;
    obj.shape.setSize({ m_GridSize * 2.f, m_GridSize * 2.f });
    obj.shape.setPosition(SnapToGrid(pos));
    obj.shape.setFillColor(sf::Color::White);
    obj.objectType = ObjectType::Sprite;

    ApplySpriteToObject(obj, spritePath);

    obj.entity = m_Registry.CreateEntity();
    m_Registry.AddComponent(obj.entity, TransformComponent{
        obj.shape.getPosition().x,
        obj.shape.getPosition().y
    });
    m_Registry.AddComponent(obj.entity, RenderComponent{
        obj.color,
        obj.shape.getSize()
    });
    m_Registry.AddComponent(obj.entity, SpriteComponent(spritePath, obj.shape.getSize()));

    m_Objects.push_back(std::move(obj));

    auto& placed = m_Objects.back();
    if (placed.previewTexture)
        placed.previewSprite.setTexture(*placed.previewTexture, true);

    UpdateStatusText();
}

void EditorScene::ApplySpriteToObject(EditorObject& obj, const std::string& spritePath)
{
    obj.spritePath = spritePath;
    obj.objectType = ObjectType::Sprite;
    obj.previewTexture = std::make_shared<sf::Texture>();

    if (obj.previewTexture->loadFromFile(spritePath))
    {
        obj.previewSprite.setTexture(*obj.previewTexture);
        const sf::Vector2u ts = obj.previewTexture->getSize();
        const sf::Vector2f size = obj.shape.getSize();
        obj.previewSprite.setScale(size.x / ts.x, size.y / ts.y);
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
        [this](const EditorObject& o) { return &o == m_Selected; });
    m_Selected = nullptr;
}

void EditorScene::SaveToJson(const std::string& path)
{
    json data;
    data["name"] = "game";

    for (auto& obj : m_Objects)
    {
        json j;
        j["id"]     = obj.id;
        j["type"]   = "rectangle";
        j["x"]      = obj.shape.getPosition().x;
        j["y"]      = obj.shape.getPosition().y;
        j["width"]  = obj.shape.getSize().x;
        j["height"] = obj.shape.getSize().y;
        j["color"]  = { obj.color.r, obj.color.g, obj.color.b };
        data["objects"].push_back(j);

        if (!obj.spritePath.empty())
            j["sprite"] = obj.spritePath;
        data["objects"].push_back(j);

        if (obj.entity != 0 && m_Registry.HasComponent<VelocityComponent>(obj.entity))
        {
            auto& vel = m_Registry.GetComponent<VelocityComponent>(obj.entity);
            j["velocity"] = { { "dx", vel.dx }, { "dy", vel.dy }};
        }

        if (!obj.scriptPath.empty())
            j["script"] = obj.scriptPath;

        data["objects"].push_back(j);
    }

    std::ofstream file(path);
    file << data.dump(4);
    UpdateStatusText();
}

void EditorScene::LoadFromJson(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "[EditorScene] File not found: " << path << "\n";
        return;
    }

    for (auto& obj : m_Objects)
        if (obj.entity != 0)
            m_Registry.DestroyEntity(obj.entity);

    m_Objects.clear();
    m_Selected = nullptr;

    json data = json::parse(file);
    for (auto& j : data["objects"])
    {
        EditorObject obj;
        obj.id    = j["id"];
        obj.color = sf::Color(j["color"][0], j["color"][1], j["color"][2]);
        obj.shape.setSize({ j["width"], j["height"] });
        obj.shape.setPosition(j["x"], j["y"]);
        obj.shape.setFillColor(obj.color);

        obj.entity = m_Registry.CreateEntity();
        m_Registry.AddComponent(obj.entity, TransformComponent{ j["x"], j["y"] });
        m_Registry.AddComponent(obj.entity, RenderComponent{ obj.color, obj.shape.getSize() });

        if (j.contains("sprite"))
        {
            std::string sp = j["sprite"];
            ApplySpriteToObject(obj, sp);
            m_Registry.AddComponent(obj.entity, SpriteComponent(sp, obj.shape.getSize()));
        }

        if (j.contains("velocity"))
        {
            m_Registry.AddComponent(obj.entity, VelocityComponent
                {j["velocity"]["dx"], j["velocity"]["dy"]});
        }

        if (j.contains("script"))
        {
            std::string scriptPath = j["script"];
            auto& sc = m_Registry.AddComponent(obj.entity, ScriptComponent(LuaState::GetLua(), scriptPath));
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
    for (auto& obj : m_Objects)
    {
        if (obj.entity == 0) continue;
        if (!m_Registry.HasComponent<TransformComponent>(obj.entity)) continue;

        auto& t = m_Registry.GetComponent<TransformComponent>(obj.entity);
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

EditorObject* EditorScene::ObjectAt(sf::Vector2f pos)
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

    sf::Vector2f center  = m_camera.getCenter();
    sf::Vector2f camSize = m_camera.getSize();

    float left   = std::floor((center.x - camSize.x / 2) / m_GridSize) * m_GridSize;
    float top    = std::floor((center.y - camSize.y / 2) / m_GridSize) * m_GridSize;
    float right  = center.x + camSize.x / 2 + m_GridSize;
    float bottom = center.y + camSize.y / 2 + m_GridSize;

    for (float x = left; x < right; x += m_GridSize)
    {
        lines.append({ { x, top },    sf::Color(55, 55, 55) });
        lines.append({ { x, bottom }, sf::Color(55, 55, 55) });
    }
    for (float y = top; y < bottom; y += m_GridSize)
    {
        lines.append({ { left,  y }, sf::Color(55, 55, 55) });
        lines.append({ { right, y }, sf::Color(55, 55, 55) });
    }
    m_Window.draw(lines);
}

void EditorScene::UpdateStatusText()
{
    std::string s = "Objects: " + std::to_string(m_Objects.size());
    s += "   Grid: " + std::string(m_SnapToGrid ? "ON" : "OFF");
    if (m_Selected)
        s += "   Selected: " + m_Selected->id
           + "  @ (" + std::to_string((int)m_Selected->shape.getPosition().x)
           + ", "    + std::to_string((int)m_Selected->shape.getPosition().y) + ")";
    m_StatusText.setString(s);
}

std::string EditorScene::NextId()
{
    return "obj_" + std::to_string(m_IdCounter++);
}