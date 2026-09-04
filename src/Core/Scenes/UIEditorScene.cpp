#include "UIEditorScene.h"
#include <iostream>
#include <algorithm>
#include "../Resources/ResourceManager.h"
#include <SFML/Window/Event.hpp>

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

UIEditorScene::UIEditorScene(SceneManager &manager, sf::RenderWindow &window)
    : Scene(manager), m_Window(window)
{
    m_Font = ResourceManager::Get().GetFont(ASSET_PATH "fonts/Merriweather.ttf");
    m_CanvasView = window.getDefaultView();
    UpdateBounds();
}

void UIEditorScene::OnEnter()
{
    std::cout << "[INFO] [UIEditorScene] Entered UI Editor\n";
    UpdateBounds();
    m_SelectedElement = nullptr;
}

void UIEditorScene::OnExit() { std::cout << "[INFO] [UIEditorScene] Exited UI Editor\n"; }

void UIEditorScene::UpdateBounds()
{
    const float winW = static_cast<float>(m_Window.getSize().x);
    const float winH = static_cast<float>(m_Window.getSize().y);

    m_PaletteBounds = {0.f, ToolbarHeight, PaletteWidth, winH - ToolbarHeight};
    m_InspectorBounds = {winW - InspectorWidth, ToolbarHeight, InspectorWidth, winH - ToolbarHeight - HierarchyHeight};
    m_HierarchyBounds = {winW - InspectorWidth, winH - HierarchyHeight, InspectorWidth, HierarchyHeight};
    m_CanvasBounds = {PaletteWidth, ToolbarHeight, winW - PaletteWidth - InspectorWidth, winH - ToolbarHeight};

    sf::FloatRect vp(
        PaletteWidth / winW,
        ToolbarHeight / winH,
        m_CanvasBounds.width / winW,
        m_CanvasBounds.height / winH
    );
    m_CanvasView.setViewport(vp);

    if (!m_ViewInitialized)
    {
        const float margin = 50.f;
        const float availW = std::max(100.f, m_CanvasBounds.width - margin * 2.f);
        const float availH = std::max(100.f, m_CanvasBounds.height - margin * 2.f);
        float scale = std::min(availW / m_CanvasSize.x, availH / m_CanvasSize.y);
        if (scale <= 0.001f) scale = 0.5f;

        m_CanvasView.setSize(m_CanvasBounds.width / scale, m_CanvasBounds.height / scale);
        m_CanvasView.setCenter(m_CanvasSize.x / 2.f, m_CanvasSize.y / 2.f);
        m_ViewInitialized = true;
    }
}

void UIEditorScene::HandleEvent(const sf::Event &event)
{
    if (event.type == sf::Event::Resized)
    {
        UpdateBounds();
        return;
    }

    const sf::Vector2i pixelPos = sf::Mouse::getPosition(m_Window);
    m_MouseScreenPos = m_Window.mapPixelToCoords(pixelPos, m_Window.getDefaultView());
    m_MouseCanvasPos = m_Window.mapPixelToCoords(pixelPos, m_CanvasView);

    if (event.type == sf::Event::MouseWheelScrolled)
    {
        if (m_CanvasBounds.contains(m_MouseScreenPos))
        {
            float delta = event.mouseWheelScroll.delta;
            float factor = delta > 0 ? 0.9f : 1.1f;
            m_CanvasView.zoom(factor);
        }
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed)
    {
        if (event.mouseButton.button == sf::Mouse::Left)
        {
            if (m_PaletteBounds.contains(m_MouseScreenPos))
            {
                for (auto &btn: m_PaletteHitboxes)
                {
                    if (btn.bounds.contains(m_MouseScreenPos))
                    {
                        HandleAction(btn.action);
                        return;
                    }
                }
            }
            if (m_InspectorBounds.contains(m_MouseScreenPos))
            {
                for (auto &btn: m_InspectorHitboxes)
                {
                    if (btn.bounds.contains(m_MouseScreenPos))
                    {
                        HandleAction(btn.action);
                        return;
                    }
                }
            }
            if (m_ToolbarHitboxes.size() > 0 && m_MouseScreenPos.y < ToolbarHeight)
            {
                for (auto &btn: m_ToolbarHitboxes)
                {
                    if (btn.bounds.contains(m_MouseScreenPos))
                    {
                        HandleAction(btn.action);
                        return;
                    }
                }
            }
            if (m_HierarchyBounds.contains(m_MouseScreenPos))
            {
                for (auto &[rect, el]: m_HierarchyHitboxes)
                {
                    if (rect.contains(m_MouseScreenPos))
                    {
                        m_SelectedElement = el;
                        m_ActiveField = EditField::None;
                        return;
                    }
                }
            }

            // Canvas interaction
            if (m_CanvasBounds.contains(m_MouseScreenPos))
            {
                // Check resize handles first
                if (m_SelectedElement)
                {
                    m_ResizeHandle = GetResizeHandle(m_MouseCanvasPos);
                    if (m_ResizeHandle != -1)
                    {
                        m_Resizing = true;
                        m_ResizeMouseStart = m_MouseCanvasPos;
                        m_ResizeObjOrigin = m_SelectedElement->position;
                        m_ResizeObjSize = m_SelectedElement->size;
                        return;
                    }
                }

                // Check selection (top-most / highest zIndex first)
                UIElement *clicked = nullptr;
                std::vector<UIElement *> candidates;
                for (auto &el: UIManager::Get().GetElements()) candidates.push_back(&el);
                std::stable_sort(candidates.begin(), candidates.end(), [](const UIElement *a, const UIElement *b) {
                    return a->zIndex > b->zIndex;
                });
                for (auto *el: candidates)
                {
                    sf::FloatRect bounds(el->position.x, el->position.y, el->size.x, el->size.y);
                    if (bounds.contains(m_MouseCanvasPos))
                    {
                        clicked = el;
                        break;
                    }
                }

                if (clicked)
                {
                    m_SelectedElement = clicked;
                    m_Dragging = true;
                    m_DragOffset = m_MouseCanvasPos - clicked->position;
                } else { m_SelectedElement = nullptr; }
                m_ActiveField = EditField::None;
            }
        } else if (event.mouseButton.button == sf::Mouse::Middle || event.mouseButton.button == sf::Mouse::Right)
        {
            if (m_CanvasBounds.contains(m_MouseScreenPos))
            {
                m_Panning = true;
                m_PanStart = m_MouseCanvasPos;
            }
        }
    }

    if (event.type == sf::Event::MouseMoved)
    {
        if (m_Panning)
        {
            sf::Vector2f newPos = m_Window.mapPixelToCoords(pixelPos, m_CanvasView);
            m_CanvasView.move(m_PanStart - newPos);
        }
        if (m_Dragging && m_SelectedElement)
        {
            m_SelectedElement->position = m_MouseCanvasPos - m_DragOffset;
            m_SelectedElement->UpdateDrawables();
        }
        if (m_Resizing && m_SelectedElement)
        {
            sf::Vector2f delta = m_MouseCanvasPos - m_ResizeMouseStart;
            sf::Vector2f newPos = m_ResizeObjOrigin;
            sf::Vector2f newSize = m_ResizeObjSize;
            const float minSize = 4.f;

            switch (m_ResizeHandle)
            {
                case 0: // Top-Left
                    newPos.x = std::min(m_ResizeObjOrigin.x + delta.x,
                                        m_ResizeObjOrigin.x + m_ResizeObjSize.x - minSize);
                    newPos.y = std::min(m_ResizeObjOrigin.y + delta.y,
                                        m_ResizeObjOrigin.y + m_ResizeObjSize.y - minSize);
                    newSize.x = std::max(m_ResizeObjSize.x - delta.x, minSize);
                    newSize.y = std::max(m_ResizeObjSize.y - delta.y, minSize);
                    break;
                case 1: // Top
                    newPos.y = std::min(m_ResizeObjOrigin.y + delta.y,
                                        m_ResizeObjOrigin.y + m_ResizeObjSize.y - minSize);
                    newSize.y = std::max(m_ResizeObjSize.y - delta.y, minSize);
                    break;
                case 2: // Top-Right
                    newPos.y = std::min(m_ResizeObjOrigin.y + delta.y,
                                        m_ResizeObjOrigin.y + m_ResizeObjSize.y - minSize);
                    newSize.x = std::max(m_ResizeObjSize.x + delta.x, minSize);
                    newSize.y = std::max(m_ResizeObjSize.y - delta.y, minSize);
                    break;
                case 3: // Left
                    newPos.x = std::min(m_ResizeObjOrigin.x + delta.x,
                                        m_ResizeObjOrigin.x + m_ResizeObjSize.x - minSize);
                    newSize.x = std::max(m_ResizeObjSize.x - delta.x, minSize);
                    break;
                case 4: // Right
                    newSize.x = std::max(m_ResizeObjSize.x + delta.x, minSize);
                    break;
                case 5: // Bottom-Left
                    newPos.x = std::min(m_ResizeObjOrigin.x + delta.x,
                                        m_ResizeObjOrigin.x + m_ResizeObjSize.x - minSize);
                    newSize.x = std::max(m_ResizeObjSize.x - delta.x, minSize);
                    newSize.y = std::max(m_ResizeObjSize.y + delta.y, minSize);
                    break;
                case 6: // Bottom
                    newSize.y = std::max(m_ResizeObjSize.y + delta.y, minSize);
                    break;
                case 7: // Bottom-Right
                    newSize.x = std::max(m_ResizeObjSize.x + delta.x, minSize);
                    newSize.y = std::max(m_ResizeObjSize.y + delta.y, minSize);
                    break;
            }

            m_SelectedElement->position = newPos;
            m_SelectedElement->size = newSize;
            m_SelectedElement->UpdateDrawables();
        }
    }

    if (event.type == sf::Event::MouseButtonReleased)
    {
        m_Dragging = false;
        m_Panning = false;
        m_Resizing = false;
        m_ResizeHandle = -1;
    }

    if (event.type == sf::Event::KeyPressed)
    {
        if (event.key.code == sf::Keyboard::Escape)
        {
            m_SelectedElement = nullptr;
            m_ActiveField = EditField::None;
        } else if (event.key.code == sf::Keyboard::Delete && m_SelectedElement && m_ActiveField ==
                   EditField::None) { DeleteSelected(); } else if (
            event.key.code == sf::Keyboard::Enter && m_ActiveField != EditField::None)
        {
            m_ActiveField = EditField::None;
            m_ActiveInputText.clear();
        } else if (event.key.code == sf::Keyboard::Backspace && m_ActiveField != EditField::None && !m_ActiveInputText.
                   empty()) { m_ActiveInputText.pop_back(); }
    }

    if (event.type == sf::Event::TextEntered && m_ActiveField != EditField::None && m_SelectedElement)
    {
        if (event.text.unicode == 8 || event.text.unicode == 13 || event.text.unicode == 27) return;
        // Backspace/Enter/Esc

        char c = static_cast<char>(event.text.unicode);
        if (m_ActiveField == EditField::Id || m_ActiveField == EditField::UIText)
            m_ActiveInputText += c;
        else if (std::isdigit(c) || c == '-')
            m_ActiveInputText += c;

        if (m_ActiveField == EditField::Id) { m_SelectedElement->id = m_ActiveInputText; } else if (
            m_ActiveField == EditField::UIText) { m_SelectedElement->text = m_ActiveInputText; } else if (
            m_ActiveField ==
            EditField::TransformX)
        {
            try { m_SelectedElement->position.x = std::stof(m_ActiveInputText); } catch (...) {}
        } else if (m_ActiveField ==
                   EditField::TransformY)
        {
            try { m_SelectedElement->position.y = std::stof(m_ActiveInputText); } catch (...) {}
        } else if (m_ActiveField ==
                   EditField::ZIndex)
        {
            try { m_SelectedElement->zIndex = std::stoi(m_ActiveInputText); } catch (...) {}
        } else if (m_ActiveField ==
                   EditField::SizeW)
        {
            try { m_SelectedElement->size.x = std::stof(m_ActiveInputText); } catch (...) {}
        } else if (m_ActiveField ==
                   EditField::SizeH)
        {
            try { m_SelectedElement->size.y = std::stof(m_ActiveInputText); } catch (...) {}
        } else if (m_ActiveField == EditField::ColorR)
        {
            try { m_SelectedElement->color.r = std::clamp(std::stoi(m_ActiveInputText), 0, 255); } catch (...) {}
        } else if (m_ActiveField == EditField::ColorG)
        {
            try { m_SelectedElement->color.g = std::clamp(std::stoi(m_ActiveInputText), 0, 255); } catch (...) {}
        } else if (m_ActiveField == EditField::ColorB)
        {
            try { m_SelectedElement->color.b = std::clamp(std::stoi(m_ActiveInputText), 0, 255); } catch (...) {}
        }

        m_SelectedElement->UpdateDrawables();
    }
}

void UIEditorScene::Update(float deltaTime)
{
    // Maybe blink caret or animations in future
}

void UIEditorScene::Render(sf::RenderWindow &window)
{
    window.setView(window.getDefaultView());
    window.clear(C_BG_DEEP);

    // Draw Canvas
    DrawCanvas(window);

    // Reset view for UI panels
    window.setView(window.getDefaultView());

    DrawToolbar(window);
    DrawPalette(window);
    DrawHierarchy(window);
    DrawInspector(window);
}

void UIEditorScene::DrawToolbar(sf::RenderWindow &window)
{
    m_ToolbarHitboxes.clear();
    const float winW = static_cast<float>(window.getSize().x);

    sf::RectangleShape bar({winW, ToolbarHeight});
    bar.setFillColor(C_BG_TOOLBAR);
    bar.setPosition(0, 0);
    window.draw(bar);

    sf::RectangleShape border({winW, 1.f});
    border.setFillColor(C_BORDER);
    border.setPosition(0, ToolbarHeight - 1.f);
    window.draw(border);

    // Back to editor
    sf::FloatRect backRect(10.f, 4.f, 120.f, ToolbarHeight - 8.f);
    DrawActionButton(window, "<- Back to Editor", "back", backRect.left, backRect.top, C_SURFACE, C_BORDER_LIGHT);
    m_ToolbarHitboxes.push_back({backRect, "back"});

    // Save UI
    sf::FloatRect saveRect(140.f, 4.f, 80.f, ToolbarHeight - 8.f);
    DrawActionButton(window, "Save UI", "save", saveRect.left, saveRect.top, C_GREEN_DIM, C_GREEN);
    m_ToolbarHitboxes.push_back({saveRect, "save"});
}

void UIEditorScene::DrawPalette(sf::RenderWindow &window)
{
    m_PaletteHitboxes.clear();

    sf::RectangleShape panel({PaletteWidth, m_PaletteBounds.height});
    panel.setFillColor(C_BG_PANEL);
    panel.setPosition(m_PaletteBounds.left, m_PaletteBounds.top);
    window.draw(panel);

    sf::RectangleShape border({1.f, m_PaletteBounds.height});
    border.setFillColor(C_BORDER);
    border.setPosition(m_PaletteBounds.left + PaletteWidth - 1.f, m_PaletteBounds.top);
    window.draw(border);

    float y = m_PaletteBounds.top + 10.f;
    DrawSectionHeader(window, "PALETTE", C_ACCENT, m_PaletteBounds.left, y);
    y += 30.f;

    auto drawAddBtn = [&](const std::string &label, const std::string &action) {
        sf::FloatRect r(m_PaletteBounds.left + 10.f, y, PaletteWidth - 20.f, 28.f);
        DrawActionButton(window, "+ " + label, action, r.left, r.top, C_SURFACE, C_BORDER);
        m_PaletteHitboxes.push_back({r, action});
        y += 34.f;
    };

    drawAddBtn("UI Panel", "add_panel");
    drawAddBtn("UI Text", "add_text");
    drawAddBtn("UI Button", "add_button");
}

void UIEditorScene::DrawHierarchy(sf::RenderWindow &window)
{
    m_HierarchyHitboxes.clear();

    sf::RectangleShape panel({InspectorWidth, HierarchyHeight});
    panel.setFillColor(C_BG_PANEL);
    panel.setPosition(m_HierarchyBounds.left, m_HierarchyBounds.top);
    window.draw(panel);

    sf::RectangleShape borderTop({InspectorWidth, 1.f});
    borderTop.setFillColor(C_BORDER);
    borderTop.setPosition(m_HierarchyBounds.left, m_HierarchyBounds.top);
    window.draw(borderTop);

    float y = m_HierarchyBounds.top + 10.f;
    DrawSectionHeader(window, "UI HIERARCHY", C_ACCENT, m_HierarchyBounds.left, y);
    y += 30.f;

    std::vector<UIElement *> sortedElements;
    for (auto &el: UIManager::Get().GetElements()) sortedElements.push_back(&el);
    std::stable_sort(sortedElements.begin(), sortedElements.end(), [](const UIElement *a, const UIElement *b) {
        return a->zIndex > b->zIndex;
    });

    for (auto *el: sortedElements)
    {
        sf::FloatRect r(m_HierarchyBounds.left + 4.f, y, InspectorWidth - 8.f, 24.f);
        bool hov = r.contains(m_MouseScreenPos);
        bool sel = (m_SelectedElement == el);

        if (hov || sel)
        {
            sf::RectangleShape bg(r.getSize());
            bg.setPosition(r.getPosition());
            bg.setFillColor(sel ? C_ACCENT_DIM : C_SURFACE_HOV);
            window.draw(bg);
        }

        std::string display = "[Z: " + std::to_string(el->zIndex) + "] " + (el->id.empty() ? "Unnamed" : el->id);
        sf::Text t;
        t.setFont(*m_Font);
        t.setCharacterSize(12);
        t.setFillColor(sel ? sf::Color::White : C_TEXT_PRIMARY);
        t.setString(display);
        t.setPosition(r.left + 8.f, r.top + 4.f);
        window.draw(t);

        m_HierarchyHitboxes.push_back({r, el});
        y += 26.f;
    }
}

void UIEditorScene::DrawInspector(sf::RenderWindow &window)
{
    m_InspectorHitboxes.clear();

    sf::RectangleShape panel({InspectorWidth, m_InspectorBounds.height});
    panel.setFillColor(C_BG_INSPECTOR);
    panel.setPosition(m_InspectorBounds.left, m_InspectorBounds.top);
    window.draw(panel);

    sf::RectangleShape border({1.f, m_InspectorBounds.height});
    border.setFillColor(C_BORDER);
    border.setPosition(m_InspectorBounds.left, m_InspectorBounds.top);
    window.draw(border);

    if (!m_SelectedElement) return;

    float y = m_InspectorBounds.top + 10.f;
    float px = m_InspectorBounds.left;

    y = DrawSectionHeader(window, "PROPERTIES", C_ACCENT, px, y);

    std::string idDisplay = (m_ActiveField == EditField::Id && !m_ActiveInputText.empty())
                                ? m_ActiveInputText + "|"
                                : (m_ActiveField == EditField::Id ? "|" : m_SelectedElement->id);
    y = DrawEditableRow(window, "ID", idDisplay, "edit_id", px, y);

    y += 10.f;
    y = DrawSectionHeader(window, "TRANSFORM", sf::Color(100, 200, 255), px, y);

    std::string xDisplay = (m_ActiveField == EditField::TransformX && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::TransformX
                                      ? "|"
                                      : std::to_string((int) m_SelectedElement->position.x));
    y = DrawEditableRow(window, "X", xDisplay, "edit_x", px, y);
    std::string yDisplay = (m_ActiveField == EditField::TransformY && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::TransformY
                                      ? "|"
                                      : std::to_string((int) m_SelectedElement->position.y));
    y = DrawEditableRow(window, "Y", yDisplay, "edit_y", px, y);

    std::string zDisplay = (m_ActiveField == EditField::ZIndex && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::ZIndex ? "|" : std::to_string(m_SelectedElement->zIndex));
    y = DrawEditableRow(window, "Z (Depth)", zDisplay, "edit_z", px, y);

    y += 4.f;
    DrawActionButton(window, "+ Forward", "layer_forward", px + 10.f, y, C_SURFACE, C_BORDER_LIGHT);
    DrawActionButton(window, "- Backward", "layer_backward", px + 110.f, y, C_SURFACE, C_BORDER_LIGHT);
    y += 30.f;

    std::string wDisplay = (m_ActiveField == EditField::SizeW && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::SizeW
                                      ? "|"
                                      : std::to_string((int) m_SelectedElement->size.x));
    y = DrawEditableRow(window, "W", wDisplay, "edit_w", px, y);
    std::string hDisplay = (m_ActiveField == EditField::SizeH && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::SizeH
                                      ? "|"
                                      : std::to_string((int) m_SelectedElement->size.y));
    y = DrawEditableRow(window, "H", hDisplay, "edit_h", px, y);

    y += 10.f;
    y = DrawSectionHeader(window, "STYLE", sf::Color(255, 200, 100), px, y);

    std::string rDisplay = (m_ActiveField == EditField::ColorR && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::ColorR
                                      ? "|"
                                      : std::to_string(m_SelectedElement->color.r));
    y = DrawEditableRow(window, "R", rDisplay, "edit_r", px, y);
    std::string gDisplay = (m_ActiveField == EditField::ColorG && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::ColorG
                                      ? "|"
                                      : std::to_string(m_SelectedElement->color.g));
    y = DrawEditableRow(window, "G", gDisplay, "edit_g", px, y);
    std::string bDisplay = (m_ActiveField == EditField::ColorB && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::ColorB
                                      ? "|"
                                      : std::to_string(m_SelectedElement->color.b));
    y = DrawEditableRow(window, "B", bDisplay, "edit_b", px, y);

    if (m_SelectedElement->type == UIElementType::Text || m_SelectedElement->type == UIElementType::Button)
    {
        y += 10.f;
        y = DrawSectionHeader(window, "TEXT", sf::Color(255, 100, 150), px, y);
        std::string txtDisplay = (m_ActiveField == EditField::UIText && !m_ActiveInputText.empty())
                                     ? m_ActiveInputText + "|"
                                     : (m_ActiveField == EditField::UIText ? "|" : m_SelectedElement->text);
        y = DrawEditableRow(window, "Text", txtDisplay, "edit_text", px, y);
    }
}

void UIEditorScene::DrawCanvas(sf::RenderWindow &window)
{
    window.setView(m_CanvasView);

    // Draw subtle drop shadow for the 1920x1080 screen canvas
    sf::RectangleShape shadow(m_CanvasSize);
    shadow.setPosition(8.f, 8.f);
    shadow.setFillColor(sf::Color(0, 0, 0, 120));
    window.draw(shadow);

    // Draw Screen Canvas area (1920x1080)
    sf::RectangleShape canvasBg(m_CanvasSize);
    canvasBg.setPosition(0, 0);
    canvasBg.setFillColor(sf::Color(18, 18, 28));
    canvasBg.setOutlineColor(sf::Color(99, 102, 241, 200)); // Indigo accent border
    canvasBg.setOutlineThickness(2.f);
    window.draw(canvasBg);

    // Draw Screen Canvas Header / Badge
    sf::Text badge;
    badge.setFont(*m_Font);
    badge.setCharacterSize(16);
    badge.setFillColor(sf::Color(160, 165, 205));
    badge.setString("Screen Canvas (1920 x 1080) - 16:9  [Stretched in Game]");
    badge.setPosition(0.f, -28.f);
    window.draw(badge);

    // Draw Grid inside screen canvas
    sf::RectangleShape line;
    line.setFillColor(sf::Color(255, 255, 255, 12));
    for (float i = 100.f; i < m_CanvasSize.x; i += 100.f)
    {
        line.setPosition(i, 0);
        line.setSize({1.f, m_CanvasSize.y});
        window.draw(line);
    }
    for (float i = 100.f; i < m_CanvasSize.y; i += 100.f)
    {
        line.setPosition(0, i);
        line.setSize({m_CanvasSize.x, 1.f});
        window.draw(line);
    }

    // Draw UIManager elements (automatically sorted by zIndex ascending)
    UIManager::Get().Render(window);

    // Draw Selection Overlay
    if (m_SelectedElement)
    {
        sf::RectangleShape outline(m_SelectedElement->size);
        outline.setPosition(m_SelectedElement->position);
        outline.setFillColor(sf::Color::Transparent);
        outline.setOutlineColor(sf::Color(255, 220, 60));
        outline.setOutlineThickness(2.f);
        window.draw(outline);

        DrawResizeHandles(window);
    }
}

void UIEditorScene::DrawResizeHandles(sf::RenderWindow &window)
{
    if (!m_SelectedElement) return;

    const sf::FloatRect bounds(m_SelectedElement->position, m_SelectedElement->size);
    const float hw = 4.f;

    sf::Vector2f positions[8] = {
        {bounds.left - hw, bounds.top - hw}, // Top-Left
        {bounds.left + bounds.width / 2.f - hw, bounds.top - hw}, // Top
        {bounds.left + bounds.width - hw, bounds.top - hw}, // Top-Right
        {bounds.left - hw, bounds.top + bounds.height / 2.f - hw}, // Left
        {bounds.left + bounds.width - hw, bounds.top + bounds.height / 2.f - hw}, // Right
        {bounds.left - hw, bounds.top + bounds.height - hw}, // Bottom-Left
        {bounds.left + bounds.width / 2.f - hw, bounds.top + bounds.height - hw}, // Bottom
        {bounds.left + bounds.width - hw, bounds.top + bounds.height - hw} // Bottom-Right
    };

    sf::RectangleShape handle({hw * 2.f, hw * 2.f});
    handle.setFillColor(sf::Color::White);
    handle.setOutlineColor(sf::Color::Black);
    handle.setOutlineThickness(1.f);

    for (const auto &pos: positions)
    {
        handle.setPosition(pos);
        window.draw(handle);
    }
}

int UIEditorScene::GetResizeHandle(sf::Vector2f worldPos) const
{
    if (!m_SelectedElement) return -1;
    const sf::FloatRect bounds(m_SelectedElement->position, m_SelectedElement->size);
    const float hw = 6.f;
    sf::Vector2f positions[8] = {
        {bounds.left, bounds.top},
        {bounds.left + bounds.width / 2.f, bounds.top},
        {bounds.left + bounds.width, bounds.top},
        {bounds.left, bounds.top + bounds.height / 2.f},
        {bounds.left + bounds.width, bounds.top + bounds.height / 2.f},
        {bounds.left, bounds.top + bounds.height},
        {bounds.left + bounds.width / 2.f, bounds.top + bounds.height},
        {bounds.left + bounds.width, bounds.top + bounds.height}
    };
    for (int i = 0; i < 8; ++i)
    {
        sf::FloatRect hr(positions[i].x - hw, positions[i].y - hw, hw * 2.f, hw * 2.f);
        if (hr.contains(worldPos)) return i;
    }
    return -1;
}

std::string UIEditorScene::NextId(UIElementType type)
{
    static int idCounter = 1;
    std::string prefix = (type == UIElementType::Text)
                             ? "Text_"
                             : (type == UIElementType::Panel)
                                   ? "Panel_"
                                   : "Button_";
    return prefix + std::to_string(idCounter++);
}

void UIEditorScene::HandleAction(const std::string &action)
{
    if (action == "back") { m_manager.SwitchSceneTo("editor"); } else if (action == "save")
    {
        UIManager::Get().Save(std::string(ASSET_PATH) + "ui.json");
        std::cout << "[INFO] [UIEditorScene] UI Saved.\n";
    } else if (action == "add_panel")
    {
        int maxZ = 0;
        for (auto &el: UIManager::Get().GetElements()) maxZ = std::max(maxZ, el.zIndex);
        m_SelectedElement = UIManager::Get().CreateElement(NextId(UIElementType::Panel), UIElementType::Panel);
        if (m_SelectedElement)
        {
            m_SelectedElement->size = {300.f, 200.f};
            m_SelectedElement->position = {m_CanvasSize.x / 2.f - 150.f, m_CanvasSize.y / 2.f - 100.f};
            m_SelectedElement->color = sf::Color(35, 35, 55, 230);
            m_SelectedElement->zIndex = maxZ + 1;
            m_SelectedElement->UpdateDrawables();
        }
    } else if (action == "add_text")
    {
        int maxZ = 0;
        for (auto &el: UIManager::Get().GetElements()) maxZ = std::max(maxZ, el.zIndex);
        m_SelectedElement = UIManager::Get().CreateElement(NextId(UIElementType::Text), UIElementType::Text);
        if (m_SelectedElement)
        {
            m_SelectedElement->size = {200.f, 40.f};
            m_SelectedElement->position = {m_CanvasSize.x / 2.f - 100.f, m_CanvasSize.y / 2.f - 20.f};
            m_SelectedElement->characterSize = 24;
            m_SelectedElement->zIndex = maxZ + 1;
            m_SelectedElement->UpdateDrawables();
        }
    } else if (action == "add_button")
    {
        int maxZ = 0;
        for (auto &el: UIManager::Get().GetElements()) maxZ = std::max(maxZ, el.zIndex);
        m_SelectedElement = UIManager::Get().CreateElement(NextId(UIElementType::Button), UIElementType::Button);
        if (m_SelectedElement)
        {
            m_SelectedElement->size = {180.f, 50.f};
            m_SelectedElement->position = {m_CanvasSize.x / 2.f - 90.f, m_CanvasSize.y / 2.f - 25.f};
            m_SelectedElement->zIndex = maxZ + 1;
            m_SelectedElement->UpdateDrawables();
        }
    } else if (action == "edit_id")
    {
        m_ActiveField = EditField::Id;
        m_ActiveInputText = m_SelectedElement->id;
    } else if (action == "edit_x")
    {
        m_ActiveField = EditField::TransformX;
        m_ActiveInputText = std::to_string((int) m_SelectedElement->position.x);
    } else if (action == "edit_y")
    {
        m_ActiveField = EditField::TransformY;
        m_ActiveInputText = std::to_string((int) m_SelectedElement->position.y);
    } else if (action == "edit_z")
    {
        m_ActiveField = EditField::ZIndex;
        m_ActiveInputText = std::to_string(m_SelectedElement->zIndex);
    } else if (action == "layer_forward") { if (m_SelectedElement) { m_SelectedElement->zIndex += 1; } } else if (
        action == "layer_backward") { if (m_SelectedElement) { m_SelectedElement->zIndex -= 1; } } else if (
        action == "edit_w")
    {
        m_ActiveField = EditField::SizeW;
        m_ActiveInputText = std::to_string((int) m_SelectedElement->size.x);
    } else if (action == "edit_h")
    {
        m_ActiveField = EditField::SizeH;
        m_ActiveInputText = std::to_string((int) m_SelectedElement->size.y);
    } else if (action == "edit_r")
    {
        m_ActiveField = EditField::ColorR;
        m_ActiveInputText = std::to_string(m_SelectedElement->color.r);
    } else if (action == "edit_g")
    {
        m_ActiveField = EditField::ColorG;
        m_ActiveInputText = std::to_string(m_SelectedElement->color.g);
    } else if (action == "edit_b")
    {
        m_ActiveField = EditField::ColorB;
        m_ActiveInputText = std::to_string(m_SelectedElement->color.b);
    } else if (action == "edit_text")
    {
        m_ActiveField = EditField::UIText;
        m_ActiveInputText = m_SelectedElement->text;
    }
}

void UIEditorScene::DeleteSelected()
{
    if (m_SelectedElement)
    {
        UIManager::Get().RemoveElement(m_SelectedElement->id);
        m_SelectedElement = nullptr;
    }
}

void UIEditorScene::DrawPill(sf::RenderWindow &window, const sf::FloatRect &r, sf::Color fill, sf::Color outline)
{
    float radius = r.height / 2.f;
    sf::ConvexShape shape(40);
    int cornerPoints = 10;

    auto addArc = [&](int startIndex, float cx, float cy, float startAngle, float endAngle) {
        for (int i = 0; i < cornerPoints; ++i)
        {
            float t = static_cast<float>(i) / (cornerPoints - 1);
            float angle = startAngle + (endAngle - startAngle) * t;
            shape.setPoint(startIndex + i, sf::Vector2f(cx + std::cos(angle) * radius, cy + std::sin(angle) * radius));
        }
    };

    addArc(0, r.left + radius, r.top + radius, 3.14159f, 3.14159f * 1.5f);
    addArc(cornerPoints, r.left + r.width - radius, r.top + radius, 3.14159f * 1.5f, 3.14159f * 2.f);
    addArc(cornerPoints * 2, r.left + r.width - radius, r.top + r.height - radius, 0.f, 3.14159f * 0.5f);
    addArc(cornerPoints * 3, r.left + radius, r.top + r.height - radius, 3.14159f * 0.5f, 3.14159f);

    shape.setFillColor(fill);
    shape.setOutlineColor(outline);
    shape.setOutlineThickness(1.f);
    window.draw(shape);
}

float UIEditorScene::DrawSectionHeader(sf::RenderWindow &window, const std::string &title, sf::Color accent, float x,
                                       float y)
{
    sf::RectangleShape bg({InspectorWidth, 22.f});
    bg.setFillColor(C_SURFACE);
    bg.setPosition(x, y);
    window.draw(bg);

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

float UIEditorScene::DrawRow(sf::RenderWindow &window, const std::string &key, const std::string &val, float x, float y)
{
    sf::Text tk;
    tk.setFont(*m_Font);
    tk.setCharacterSize(12);
    tk.setFillColor(C_TEXT_SECONDARY);
    tk.setString(key);
    tk.setPosition(x + 10.f, y + 4.f);
    window.draw(tk);

    sf::Text tv;
    tv.setFont(*m_Font);
    tv.setCharacterSize(12);
    tv.setFillColor(C_TEXT_PRIMARY);
    tv.setString(val);
    tv.setPosition(x + 100.f, y + 4.f);
    window.draw(tv);

    return y + 26.f;
}

float UIEditorScene::DrawEditableRow(sf::RenderWindow &window, const std::string &key, const std::string &val,
                                     const std::string &action, float x, float y)
{
    const float rowH = 26.f;
    sf::FloatRect r(x + 96.f, y + 2.f, InspectorWidth - 106.f, 20.f);
    bool hov = r.contains(m_MouseScreenPos);

    if (hov)
    {
        sf::RectangleShape hbg(r.getSize());
        hbg.setPosition(r.getPosition());
        hbg.setFillColor(C_SURFACE_HOV);
        window.draw(hbg);
    }

    sf::Text tk;
    tk.setFont(*m_Font);
    tk.setCharacterSize(12);
    tk.setFillColor(C_TEXT_SECONDARY);
    tk.setString(key);
    tk.setPosition(x + 10.f, y + 4.f);
    window.draw(tk);

    sf::Text tv;
    tv.setFont(*m_Font);
    tv.setCharacterSize(12);
    tv.setFillColor(C_TEXT_PRIMARY);
    tv.setString(val);
    tv.setPosition(x + 100.f, y + 4.f);
    window.draw(tv);

    m_InspectorHitboxes.push_back({r, action});
    return y + rowH;
}

float UIEditorScene::DrawActionButton(sf::RenderWindow &window, const std::string &label, const std::string &action,
                                      float x, float y, sf::Color fillColor, sf::Color borderColor)
{
    sf::FloatRect r(x, y, 0.f, 24.f);
    sf::Text t;
    t.setFont(*m_Font);
    t.setCharacterSize(12);
    t.setString(label);
    r.width = t.getLocalBounds().width + 24.f;

    bool hov = r.contains(m_MouseScreenPos);
    if (hov) fillColor.a = std::min(255, fillColor.a + 40);

    DrawPill(window, r, fillColor, borderColor);

    t.setFillColor(C_TEXT_PRIMARY);
    t.setPosition(x + 12.f, y + 3.f);
    window.draw(t);

    m_InspectorHitboxes.push_back({r, action});
    return y + 30.f;
}
