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

    sf::FloatRect inspVp(
        m_InspectorBounds.left / winW,
        m_InspectorBounds.top / winH,
        m_InspectorBounds.width / winW,
        m_InspectorBounds.height / winH
    );
    m_InspectorView.setViewport(inspVp);
    m_InspectorView.setSize(m_InspectorBounds.width, m_InspectorBounds.height);
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
        else if (m_InspectorBounds.contains(m_MouseScreenPos))
        {
            m_InspectorTargetScroll -= event.mouseWheelScroll.delta * 40.f;
            m_InspectorTargetScroll = std::max(0.f, std::min(m_InspectorTargetScroll, m_InspectorMaxScroll));
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

            if (m_CanvasBounds.contains(m_MouseScreenPos))
            {
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
                case 0:
                    newPos.x = std::min(m_ResizeObjOrigin.x + delta.x,
                                        m_ResizeObjOrigin.x + m_ResizeObjSize.x - minSize);
                    newPos.y = std::min(m_ResizeObjOrigin.y + delta.y,
                                        m_ResizeObjOrigin.y + m_ResizeObjSize.y - minSize);
                    newSize.x = std::max(m_ResizeObjSize.x - delta.x, minSize);
                    newSize.y = std::max(m_ResizeObjSize.y - delta.y, minSize);
                    break;
                case 1:
                    newPos.y = std::min(m_ResizeObjOrigin.y + delta.y,
                                        m_ResizeObjOrigin.y + m_ResizeObjSize.y - minSize);
                    newSize.y = std::max(m_ResizeObjSize.y - delta.y, minSize);
                    break;
                case 2:
                    newPos.y = std::min(m_ResizeObjOrigin.y + delta.y,
                                        m_ResizeObjOrigin.y + m_ResizeObjSize.y - minSize);
                    newSize.x = std::max(m_ResizeObjSize.x + delta.x, minSize);
                    newSize.y = std::max(m_ResizeObjSize.y - delta.y, minSize);
                    break;
                case 3:
                    newPos.x = std::min(m_ResizeObjOrigin.x + delta.x,
                                        m_ResizeObjOrigin.x + m_ResizeObjSize.x - minSize);
                    newSize.x = std::max(m_ResizeObjSize.x - delta.x, minSize);
                    break;
                case 4:
                    newSize.x = std::max(m_ResizeObjSize.x + delta.x, minSize);
                    break;
                case 5:
                    newPos.x = std::min(m_ResizeObjOrigin.x + delta.x,
                                        m_ResizeObjOrigin.x + m_ResizeObjSize.x - minSize);
                    newSize.x = std::max(m_ResizeObjSize.x - delta.x, minSize);
                    newSize.y = std::max(m_ResizeObjSize.y + delta.y, minSize);
                    break;
                case 6:
                    newSize.y = std::max(m_ResizeObjSize.y + delta.y, minSize);
                    break;
                case 7:
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

        char c = static_cast<char>(event.text.unicode);
        if (m_ActiveField == EditField::Id || m_ActiveField == EditField::UIText)
            m_ActiveInputText += c;
        else if (std::isdigit(c) || c == '-' || c == '.')
            m_ActiveInputText += c;

        if (m_ActiveField == EditField::Id) { m_SelectedElement->id = m_ActiveInputText; }
        else if (m_ActiveField == EditField::UIText) { m_SelectedElement->text = m_ActiveInputText; }
        else if (m_ActiveField == EditField::TransformX)
        {
            try { m_SelectedElement->position.x = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::TransformY)
        {
            try { m_SelectedElement->position.y = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::ZIndex)
        {
            try { m_SelectedElement->zIndex = std::stoi(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::SizeW)
        {
            try { m_SelectedElement->size.x = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::SizeH)
        {
            try { m_SelectedElement->size.y = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::ColorR)
        {
            try { m_SelectedElement->color.r = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::ColorG)
        {
            try { m_SelectedElement->color.g = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::ColorB)
        {
            try { m_SelectedElement->color.b = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::ColorA)
        {
            try { m_SelectedElement->color.a = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::Opacity)
        {
            try { m_SelectedElement->opacity = std::clamp(std::stof(m_ActiveInputText), 0.f, 255.f); } catch (...) {}
        }
        else if (m_ActiveField == EditField::CharacterSize)
        {
            try { m_SelectedElement->characterSize = static_cast<unsigned int>(std::max(1, std::stoi(m_ActiveInputText))); } catch (...) {}
        }
        else if (m_ActiveField == EditField::LetterSpacing)
        {
            try { m_SelectedElement->letterSpacing = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::LineSpacing)
        {
            try { m_SelectedElement->lineSpacing = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::TextOutlineR)
        {
            try { m_SelectedElement->textOutlineColor.r = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::TextOutlineG)
        {
            try { m_SelectedElement->textOutlineColor.g = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::TextOutlineB)
        {
            try { m_SelectedElement->textOutlineColor.b = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::TextOutlineThickness)
        {
            try { m_SelectedElement->textOutlineThickness = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::TextOffsetX)
        {
            try { m_SelectedElement->textOffset.x = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::TextOffsetY)
        {
            try { m_SelectedElement->textOffset.y = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::OutlineR)
        {
            try { m_SelectedElement->outlineColor.r = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::OutlineG)
        {
            try { m_SelectedElement->outlineColor.g = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::OutlineB)
        {
            try { m_SelectedElement->outlineColor.b = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::OutlineThickness)
        {
            try { m_SelectedElement->outlineThickness = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::BorderR)
        {
            try { m_SelectedElement->borderColor.r = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::BorderG)
        {
            try { m_SelectedElement->borderColor.g = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::BorderB)
        {
            try { m_SelectedElement->borderColor.b = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::BorderThickness)
        {
            try { m_SelectedElement->borderThickness = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::HoverR)
        {
            try { m_SelectedElement->hoverColor.r = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::HoverG)
        {
            try { m_SelectedElement->hoverColor.g = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::HoverB)
        {
            try { m_SelectedElement->hoverColor.b = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::PressedR)
        {
            try { m_SelectedElement->pressedColor.r = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::PressedG)
        {
            try { m_SelectedElement->pressedColor.g = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::PressedB)
        {
            try { m_SelectedElement->pressedColor.b = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::NormalR)
        {
            try { m_SelectedElement->normalColor.r = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::NormalG)
        {
            try { m_SelectedElement->normalColor.g = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::NormalB)
        {
            try { m_SelectedElement->normalColor.b = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::TextColorR)
        {
            try { m_SelectedElement->textColor.r = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::TextColorG)
        {
            try { m_SelectedElement->textColor.g = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::TextColorB)
        {
            try { m_SelectedElement->textColor.b = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }

        m_SelectedElement->UpdateDrawables();
    }
}

void UIEditorScene::Update(float deltaTime)
{
    m_InspectorScrollOffset += (m_InspectorTargetScroll - m_InspectorScrollOffset) * 15.f * deltaTime;
}

void UIEditorScene::Render(sf::RenderWindow &window)
{
    window.setView(window.getDefaultView());
    window.clear(C_BG_DEEP);

    DrawCanvas(window);

    window.setView(window.getDefaultView());

    DrawToolbar(window);
    DrawPalette(window);
    DrawInspector(window);
    DrawHierarchy(window);
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

    sf::FloatRect backRect(10.f, 4.f, 120.f, ToolbarHeight - 8.f);
    DrawActionButton(window, "<- Back to Editor", "back", backRect.left, backRect.top, C_SURFACE, C_BORDER_LIGHT);
    m_ToolbarHitboxes.push_back({backRect, "back"});

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

    // Background and border drawn at real screen coords
    sf::RectangleShape panel({InspectorWidth, m_InspectorBounds.height});
    panel.setFillColor(C_BG_INSPECTOR);
    panel.setPosition(m_InspectorBounds.left, m_InspectorBounds.top);
    window.draw(panel);

    sf::RectangleShape border({1.f, m_InspectorBounds.height});
    border.setFillColor(C_BORDER);
    border.setPosition(m_InspectorBounds.left, m_InspectorBounds.top);
    window.draw(border);

    if (!m_SelectedElement)
    {
        // Still reset clip in case it was left active from a previous frame
        m_InspectorClipTop    = 0.f;
        m_InspectorClipBottom = 99999.f;
        return;
    }

    // Clip bounds
    const float clipTop    = m_InspectorBounds.top;
    const float clipBottom = m_InspectorBounds.top + m_InspectorBounds.height;

    // Expose clip bounds to draw helpers so they can skip out-of-bounds rows
    m_InspectorClipTop    = clipTop;
    m_InspectorClipBottom = clipBottom;

    // y starts shifted by -scrollOffset so helpers draw at real screen positions naturally.
    // All hitboxes stored by helpers are thus already in real screen space.
    float scrollOff = m_InspectorScrollOffset;
    float y = m_InspectorBounds.top + 10.f - scrollOff;
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
    std::string aDisplay = (m_ActiveField == EditField::ColorA && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::ColorA
                                      ? "|"
                                      : std::to_string(m_SelectedElement->color.a));
    y = DrawEditableRow(window, "A", aDisplay, "edit_a", px, y);

    // Visible toggle
    y += 4.f;
    bool isVisible = m_SelectedElement->visible;
    DrawActionButton(window, isVisible ? "[Visible]" : "[Hidden]", "visible_toggle",
                     px + 10.f, y,
                     isVisible ? C_GREEN_DIM : C_SURFACE,
                     isVisible ? C_GREEN : C_BORDER_LIGHT);
    y += 30.f;

    // Opacity
    std::string opacityDisplay = (m_ActiveField == EditField::Opacity && !m_ActiveInputText.empty())
                                     ? m_ActiveInputText + "|"
                                     : (m_ActiveField == EditField::Opacity
                                            ? "|"
                                            : std::to_string((int)m_SelectedElement->opacity));
    y = DrawEditableRow(window, "Opacity", opacityDisplay, "edit_opacity", px, y);

    // Outline color/thickness (panel/button border)
    std::string outRDisplay = (m_ActiveField == EditField::OutlineR && !m_ActiveInputText.empty())
                                  ? m_ActiveInputText + "|"
                                  : (m_ActiveField == EditField::OutlineR ? "|" : std::to_string(m_SelectedElement->outlineColor.r));
    y = DrawEditableRow(window, "Outline R", outRDisplay, "edit_outline_r", px, y);
    std::string outGDisplay = (m_ActiveField == EditField::OutlineG && !m_ActiveInputText.empty())
                                  ? m_ActiveInputText + "|"
                                  : (m_ActiveField == EditField::OutlineG ? "|" : std::to_string(m_SelectedElement->outlineColor.g));
    y = DrawEditableRow(window, "Outline G", outGDisplay, "edit_outline_g", px, y);
    std::string outBDisplay = (m_ActiveField == EditField::OutlineB && !m_ActiveInputText.empty())
                                  ? m_ActiveInputText + "|"
                                  : (m_ActiveField == EditField::OutlineB ? "|" : std::to_string(m_SelectedElement->outlineColor.b));
    y = DrawEditableRow(window, "Outline B", outBDisplay, "edit_outline_b", px, y);
    std::string outThkDisplay = (m_ActiveField == EditField::OutlineThickness && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::OutlineThickness ? "|" : std::to_string((int)m_SelectedElement->outlineThickness));
    y = DrawEditableRow(window, "Outline Thk", outThkDisplay, "edit_outline_thickness", px, y);

    if (m_SelectedElement->type == UIElementType::Text || m_SelectedElement->type == UIElementType::Button)
    {
        y += 10.f;
        y = DrawSectionHeader(window, "TEXT", sf::Color(255, 100, 150), px, y);

        std::string txtDisplay = (m_ActiveField == EditField::UIText && !m_ActiveInputText.empty())
                                     ? m_ActiveInputText + "|"
                                     : (m_ActiveField == EditField::UIText ? "|" : m_SelectedElement->text);
        y = DrawEditableRow(window, "Text", txtDisplay, "edit_text", px, y);

        // Font Size
        std::string fsDisplay = (m_ActiveField == EditField::CharacterSize && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::CharacterSize ? "|" : std::to_string(m_SelectedElement->characterSize));
        y = DrawEditableRow(window, "Font Size", fsDisplay, "edit_fontsize", px, y);

        // Text Align toggles
        y += 4.f;
        bool isLeft   = m_SelectedElement->textAlign == TextAlign::Left;
        bool isCenter = m_SelectedElement->textAlign == TextAlign::Center;
        bool isRight  = m_SelectedElement->textAlign == TextAlign::Right;
        DrawActionButton(window, "Left",   "align_left",   px + 10.f,  y, isLeft   ? C_ACCENT_DIM : C_SURFACE, isLeft   ? C_ACCENT : C_BORDER_LIGHT);
        DrawActionButton(window, "Center", "align_center", px + 70.f,  y, isCenter ? C_ACCENT_DIM : C_SURFACE, isCenter ? C_ACCENT : C_BORDER_LIGHT);
        DrawActionButton(window, "Right",  "align_right",  px + 150.f, y, isRight  ? C_ACCENT_DIM : C_SURFACE, isRight  ? C_ACCENT : C_BORDER_LIGHT);
        y += 30.f;

        // Style toggles
        bool isBold      = (m_SelectedElement->textStyle & sf::Text::Bold) != 0;
        bool isItalic    = (m_SelectedElement->textStyle & sf::Text::Italic) != 0;
        bool isUnderline = (m_SelectedElement->textStyle & sf::Text::Underlined) != 0;
        bool isUpperCase = m_SelectedElement->textUpperCase;
        DrawActionButton(window, "Bold",      "style_bold_toggle",      px + 10.f,  y, isBold      ? C_ACCENT_DIM : C_SURFACE, isBold      ? C_ACCENT : C_BORDER_LIGHT);
        DrawActionButton(window, "Italic",    "style_italic_toggle",    px + 70.f,  y, isItalic    ? C_ACCENT_DIM : C_SURFACE, isItalic    ? C_ACCENT : C_BORDER_LIGHT);
        DrawActionButton(window, "Underline", "style_underline_toggle", px + 130.f, y, isUnderline ? C_ACCENT_DIM : C_SURFACE, isUnderline ? C_ACCENT : C_BORDER_LIGHT);
        y += 30.f;
        DrawActionButton(window, "UpperCase", "uppercase_toggle", px + 10.f, y, isUpperCase ? C_ACCENT_DIM : C_SURFACE, isUpperCase ? C_ACCENT : C_BORDER_LIGHT);
        y += 30.f;

        // Letter/Line spacing
        std::string lsDisplay = (m_ActiveField == EditField::LetterSpacing && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::LetterSpacing ? "|" : std::to_string(m_SelectedElement->letterSpacing).substr(0,4));
        y = DrawEditableRow(window, "Ltr Spacing", lsDisplay, "edit_letterspacing", px, y);
        std::string lineDisplay = (m_ActiveField == EditField::LineSpacing && !m_ActiveInputText.empty())
                                      ? m_ActiveInputText + "|"
                                      : (m_ActiveField == EditField::LineSpacing ? "|" : std::to_string(m_SelectedElement->lineSpacing).substr(0,4));
        y = DrawEditableRow(window, "Line Spacing", lineDisplay, "edit_linespacing", px, y);

        // Text Outline
        std::string toRDisplay = (m_ActiveField == EditField::TextOutlineR && !m_ActiveInputText.empty())
                                     ? m_ActiveInputText + "|"
                                     : (m_ActiveField == EditField::TextOutlineR ? "|" : std::to_string(m_SelectedElement->textOutlineColor.r));
        y = DrawEditableRow(window, "TxtOut R", toRDisplay, "edit_textoutline_r", px, y);
        std::string toGDisplay = (m_ActiveField == EditField::TextOutlineG && !m_ActiveInputText.empty())
                                     ? m_ActiveInputText + "|"
                                     : (m_ActiveField == EditField::TextOutlineG ? "|" : std::to_string(m_SelectedElement->textOutlineColor.g));
        y = DrawEditableRow(window, "TxtOut G", toGDisplay, "edit_textoutline_g", px, y);
        std::string toBDisplay = (m_ActiveField == EditField::TextOutlineB && !m_ActiveInputText.empty())
                                     ? m_ActiveInputText + "|"
                                     : (m_ActiveField == EditField::TextOutlineB ? "|" : std::to_string(m_SelectedElement->textOutlineColor.b));
        y = DrawEditableRow(window, "TxtOut B", toBDisplay, "edit_textoutline_b", px, y);
        std::string toThkDisplay = (m_ActiveField == EditField::TextOutlineThickness && !m_ActiveInputText.empty())
                                       ? m_ActiveInputText + "|"
                                       : (m_ActiveField == EditField::TextOutlineThickness ? "|" : std::to_string((int)m_SelectedElement->textOutlineThickness));
        y = DrawEditableRow(window, "TxtOut Thk", toThkDisplay, "edit_textoutline_thickness", px, y);

        // Text Offset
        std::string txDisplay = (m_ActiveField == EditField::TextOffsetX && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::TextOffsetX ? "|" : std::to_string((int)m_SelectedElement->textOffset.x));
        y = DrawEditableRow(window, "Offset X", txDisplay, "edit_textoffset_x", px, y);
        std::string tyDisplay = (m_ActiveField == EditField::TextOffsetY && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::TextOffsetY ? "|" : std::to_string((int)m_SelectedElement->textOffset.y));
        y = DrawEditableRow(window, "Offset Y", tyDisplay, "edit_textoffset_y", px, y);

        // Text Color
        std::string tcRDisplay = (m_ActiveField == EditField::TextColorR && !m_ActiveInputText.empty())
                                     ? m_ActiveInputText + "|"
                                     : (m_ActiveField == EditField::TextColorR ? "|" : std::to_string(m_SelectedElement->textColor.r));
        y = DrawEditableRow(window, "Text R", tcRDisplay, "edit_textcolor_r", px, y);
        std::string tcGDisplay = (m_ActiveField == EditField::TextColorG && !m_ActiveInputText.empty())
                                     ? m_ActiveInputText + "|"
                                     : (m_ActiveField == EditField::TextColorG ? "|" : std::to_string(m_SelectedElement->textColor.g));
        y = DrawEditableRow(window, "Text G", tcGDisplay, "edit_textcolor_g", px, y);
        std::string tcBDisplay = (m_ActiveField == EditField::TextColorB && !m_ActiveInputText.empty())
                                     ? m_ActiveInputText + "|"
                                     : (m_ActiveField == EditField::TextColorB ? "|" : std::to_string(m_SelectedElement->textColor.b));
        y = DrawEditableRow(window, "Text B", tcBDisplay, "edit_textcolor_b", px, y);
    }

    if (m_SelectedElement->type == UIElementType::Button)
    {
        y += 10.f;
        y = DrawSectionHeader(window, "BUTTON", sf::Color(100, 220, 255), px, y);

        // Normal Color
        std::string nrDisplay = (m_ActiveField == EditField::NormalR && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::NormalR ? "|" : std::to_string(m_SelectedElement->normalColor.r));
        y = DrawEditableRow(window, "Normal R", nrDisplay, "edit_normalr", px, y);
        std::string ngDisplay = (m_ActiveField == EditField::NormalG && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::NormalG ? "|" : std::to_string(m_SelectedElement->normalColor.g));
        y = DrawEditableRow(window, "Normal G", ngDisplay, "edit_normalg", px, y);
        std::string nbDisplay = (m_ActiveField == EditField::NormalB && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::NormalB ? "|" : std::to_string(m_SelectedElement->normalColor.b));
        y = DrawEditableRow(window, "Normal B", nbDisplay, "edit_normalb", px, y);

        // Hover Color
        std::string hrDisplay = (m_ActiveField == EditField::HoverR && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::HoverR ? "|" : std::to_string(m_SelectedElement->hoverColor.r));
        y = DrawEditableRow(window, "Hover R", hrDisplay, "edit_hoverr", px, y);
        std::string hgDisplay = (m_ActiveField == EditField::HoverG && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::HoverG ? "|" : std::to_string(m_SelectedElement->hoverColor.g));
        y = DrawEditableRow(window, "Hover G", hgDisplay, "edit_hoverg", px, y);
        std::string hbDisplay = (m_ActiveField == EditField::HoverB && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::HoverB ? "|" : std::to_string(m_SelectedElement->hoverColor.b));
        y = DrawEditableRow(window, "Hover B", hbDisplay, "edit_hoverb", px, y);

        // Pressed Color
        std::string prDisplay = (m_ActiveField == EditField::PressedR && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::PressedR ? "|" : std::to_string(m_SelectedElement->pressedColor.r));
        y = DrawEditableRow(window, "Pressed R", prDisplay, "edit_pressedr", px, y);
        std::string pgDisplay = (m_ActiveField == EditField::PressedG && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::PressedG ? "|" : std::to_string(m_SelectedElement->pressedColor.g));
        y = DrawEditableRow(window, "Pressed G", pgDisplay, "edit_pressedg", px, y);
        std::string pbDisplay = (m_ActiveField == EditField::PressedB && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::PressedB ? "|" : std::to_string(m_SelectedElement->pressedColor.b));
        y = DrawEditableRow(window, "Pressed B", pbDisplay, "edit_pressedb", px, y);

        // Border Color/Thickness
        std::string brDisplay = (m_ActiveField == EditField::BorderR && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::BorderR ? "|" : std::to_string(m_SelectedElement->borderColor.r));
        y = DrawEditableRow(window, "Border R", brDisplay, "edit_borderr", px, y);
        std::string bgDisplay = (m_ActiveField == EditField::BorderG && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::BorderG ? "|" : std::to_string(m_SelectedElement->borderColor.g));
        y = DrawEditableRow(window, "Border G", bgDisplay, "edit_borderg", px, y);
        std::string bbDisplay = (m_ActiveField == EditField::BorderB && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::BorderB ? "|" : std::to_string(m_SelectedElement->borderColor.b));
        y = DrawEditableRow(window, "Border B", bbDisplay, "edit_borderb", px, y);
        std::string bThkDisplay = (m_ActiveField == EditField::BorderThickness && !m_ActiveInputText.empty())
                                      ? m_ActiveInputText + "|"
                                      : (m_ActiveField == EditField::BorderThickness ? "|" : std::to_string((int)m_SelectedElement->borderThickness));
        y = DrawEditableRow(window, "Border Thk", bThkDisplay, "edit_borderthickness", px, y);

        // Disabled toggle
        y += 4.f;
        bool isDisabled = m_SelectedElement->disabled;
        DrawActionButton(window, isDisabled ? "[Disabled]" : "[Enabled]", "disabled_toggle",
                         px + 10.f, y,
                         isDisabled ? sf::Color(80, 20, 20) : C_GREEN_DIM,
                         isDisabled ? C_RED : C_GREEN);
        y += 30.f;
    }

    // Update max scroll based on total content height
    // y is screen-space, so add scrollOff to recover the logical bottom
    float totalContentBottom = y + scrollOff;
    m_InspectorMaxScroll = std::max(0.f, totalContentBottom - m_InspectorBounds.top - m_InspectorBounds.height + 20.f);

    // Redraw left border so it's always crisp on top of content
    sf::RectangleShape borderFront({1.f, m_InspectorBounds.height});
    borderFront.setFillColor(C_BORDER);
    borderFront.setPosition(m_InspectorBounds.left, m_InspectorBounds.top);
    window.draw(borderFront);

    // Scrollbar (thin accent bar on the right edge of the inspector)
    if (m_InspectorMaxScroll > 0.f)
    {
        float ratio     = m_InspectorBounds.height / (m_InspectorBounds.height + m_InspectorMaxScroll);
        float barH      = std::max(20.f, m_InspectorBounds.height * ratio);
        float barOffset = (m_InspectorScrollOffset / m_InspectorMaxScroll) * (m_InspectorBounds.height - barH);
        sf::RectangleShape scrollBar({4.f, barH});
        scrollBar.setFillColor(sf::Color(120, 120, 180, 180));
        scrollBar.setPosition(m_InspectorBounds.left + InspectorWidth - 5.f, m_InspectorBounds.top + barOffset);
        window.draw(scrollBar);
    }

    // Reset clip so helpers draw normally for other panels (Hierarchy, Palette, Toolbar)
    m_InspectorClipTop    = 0.f;
    m_InspectorClipBottom = 99999.f;
}

void UIEditorScene::DrawCanvas(sf::RenderWindow &window)
{
    window.setView(m_CanvasView);

    sf::RectangleShape shadow(m_CanvasSize);
    shadow.setPosition(8.f, 8.f);
    shadow.setFillColor(sf::Color(0, 0, 0, 120));
    window.draw(shadow);

    sf::RectangleShape canvasBg(m_CanvasSize);
    canvasBg.setPosition(0, 0);
    canvasBg.setFillColor(sf::Color(18, 18, 28));
    canvasBg.setOutlineColor(sf::Color(99, 102, 241, 200));
    canvasBg.setOutlineThickness(2.f);
    window.draw(canvasBg);

    sf::Text badge;
    badge.setFont(*m_Font);
    badge.setCharacterSize(16);
    badge.setFillColor(sf::Color(160, 165, 205));
    badge.setString("Screen Canvas (1920 x 1080) - 16:9  [Stretched in Game]");
    badge.setPosition(0.f, -28.f);
    window.draw(badge);

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

    UIManager::Get().Render(window);

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
        {bounds.left - hw, bounds.top - hw},
        {bounds.left + bounds.width / 2.f - hw, bounds.top - hw},
        {bounds.left + bounds.width - hw, bounds.top - hw},
        {bounds.left - hw, bounds.top + bounds.height / 2.f - hw},
        {bounds.left + bounds.width - hw, bounds.top + bounds.height / 2.f - hw},
        {bounds.left - hw, bounds.top + bounds.height - hw},
        {bounds.left + bounds.width / 2.f - hw, bounds.top + bounds.height - hw},
        {bounds.left + bounds.width - hw, bounds.top + bounds.height - hw}
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
    }
    else if (action == "layer_forward") { if (m_SelectedElement) { m_SelectedElement->zIndex += 1; } }
    else if (action == "layer_backward") { if (m_SelectedElement) { m_SelectedElement->zIndex -= 1; } }
    else if (action == "edit_w")
    {
        m_ActiveField = EditField::SizeW;
        m_ActiveInputText = std::to_string((int) m_SelectedElement->size.x);
    }
    else if (action == "edit_h")
    {
        m_ActiveField = EditField::SizeH;
        m_ActiveInputText = std::to_string((int) m_SelectedElement->size.y);
    }
    else if (action == "edit_r")
    {
        m_ActiveField = EditField::ColorR;
        m_ActiveInputText = std::to_string(m_SelectedElement->color.r);
    }
    else if (action == "edit_g")
    {
        m_ActiveField = EditField::ColorG;
        m_ActiveInputText = std::to_string(m_SelectedElement->color.g);
    }
    else if (action == "edit_b")
    {
        m_ActiveField = EditField::ColorB;
        m_ActiveInputText = std::to_string(m_SelectedElement->color.b);
    }
    else if (action == "edit_a")
    {
        m_ActiveField = EditField::ColorA;
        m_ActiveInputText = std::to_string(m_SelectedElement->color.a);
    }
    else if (action == "edit_opacity")
    {
        m_ActiveField = EditField::Opacity;
        m_ActiveInputText = std::to_string((int)m_SelectedElement->opacity);
    }
    else if (action == "edit_text")
    {
        m_ActiveField = EditField::UIText;
        m_ActiveInputText = m_SelectedElement->text;
    }
    else if (action == "edit_fontsize")
    {
        m_ActiveField = EditField::CharacterSize;
        m_ActiveInputText = std::to_string(m_SelectedElement->characterSize);
    }
    else if (action == "edit_letterspacing")
    {
        m_ActiveField = EditField::LetterSpacing;
        m_ActiveInputText = std::to_string(m_SelectedElement->letterSpacing);
    }
    else if (action == "edit_linespacing")
    {
        m_ActiveField = EditField::LineSpacing;
        m_ActiveInputText = std::to_string(m_SelectedElement->lineSpacing);
    }
    // Align toggles
    else if (action == "align_left")
    {
        if (m_SelectedElement) { m_SelectedElement->textAlign = TextAlign::Left; m_SelectedElement->UpdateDrawables(); }
    }
    else if (action == "align_center")
    {
        if (m_SelectedElement) { m_SelectedElement->textAlign = TextAlign::Center; m_SelectedElement->UpdateDrawables(); }
    }
    else if (action == "align_right")
    {
        if (m_SelectedElement) { m_SelectedElement->textAlign = TextAlign::Right; m_SelectedElement->UpdateDrawables(); }
    }
    // Style toggles
    else if (action == "style_bold_toggle")
    {
        if (m_SelectedElement)
        {
            m_SelectedElement->textStyle = static_cast<sf::Text::Style>(m_SelectedElement->textStyle ^ sf::Text::Bold);
            m_SelectedElement->UpdateDrawables();
        }
    }
    else if (action == "style_italic_toggle")
    {
        if (m_SelectedElement)
        {
            m_SelectedElement->textStyle = static_cast<sf::Text::Style>(m_SelectedElement->textStyle ^ sf::Text::Italic);
            m_SelectedElement->UpdateDrawables();
        }
    }
    else if (action == "style_underline_toggle")
    {
        if (m_SelectedElement)
        {
            m_SelectedElement->textStyle = static_cast<sf::Text::Style>(m_SelectedElement->textStyle ^ sf::Text::Underlined);
            m_SelectedElement->UpdateDrawables();
        }
    }
    else if (action == "uppercase_toggle")
    {
        if (m_SelectedElement) { m_SelectedElement->textUpperCase = !m_SelectedElement->textUpperCase; m_SelectedElement->UpdateDrawables(); }
    }
    // Visible / disabled toggles
    else if (action == "visible_toggle")
    {
        if (m_SelectedElement) { m_SelectedElement->visible = !m_SelectedElement->visible; m_SelectedElement->UpdateDrawables(); }
    }
    else if (action == "disabled_toggle")
    {
        if (m_SelectedElement) { m_SelectedElement->disabled = !m_SelectedElement->disabled; m_SelectedElement->UpdateDrawables(); }
    }
    // Text outline edits
    else if (action == "edit_textoutline_r")
    {
        m_ActiveField = EditField::TextOutlineR;
        m_ActiveInputText = std::to_string(m_SelectedElement->textOutlineColor.r);
    }
    else if (action == "edit_textoutline_g")
    {
        m_ActiveField = EditField::TextOutlineG;
        m_ActiveInputText = std::to_string(m_SelectedElement->textOutlineColor.g);
    }
    else if (action == "edit_textoutline_b")
    {
        m_ActiveField = EditField::TextOutlineB;
        m_ActiveInputText = std::to_string(m_SelectedElement->textOutlineColor.b);
    }
    else if (action == "edit_textoutline_thickness")
    {
        m_ActiveField = EditField::TextOutlineThickness;
        m_ActiveInputText = std::to_string(m_SelectedElement->textOutlineThickness);
    }
    // Text offset edits
    else if (action == "edit_textoffset_x")
    {
        m_ActiveField = EditField::TextOffsetX;
        m_ActiveInputText = std::to_string((int)m_SelectedElement->textOffset.x);
    }
    else if (action == "edit_textoffset_y")
    {
        m_ActiveField = EditField::TextOffsetY;
        m_ActiveInputText = std::to_string((int)m_SelectedElement->textOffset.y);
    }
    // Panel outline edits
    else if (action == "edit_outline_r")
    {
        m_ActiveField = EditField::OutlineR;
        m_ActiveInputText = std::to_string(m_SelectedElement->outlineColor.r);
    }
    else if (action == "edit_outline_g")
    {
        m_ActiveField = EditField::OutlineG;
        m_ActiveInputText = std::to_string(m_SelectedElement->outlineColor.g);
    }
    else if (action == "edit_outline_b")
    {
        m_ActiveField = EditField::OutlineB;
        m_ActiveInputText = std::to_string(m_SelectedElement->outlineColor.b);
    }
    else if (action == "edit_outline_thickness")
    {
        m_ActiveField = EditField::OutlineThickness;
        m_ActiveInputText = std::to_string(m_SelectedElement->outlineThickness);
    }
    // Button border edits
    else if (action == "edit_borderr")
    {
        m_ActiveField = EditField::BorderR;
        m_ActiveInputText = std::to_string(m_SelectedElement->borderColor.r);
    }
    else if (action == "edit_borderg")
    {
        m_ActiveField = EditField::BorderG;
        m_ActiveInputText = std::to_string(m_SelectedElement->borderColor.g);
    }
    else if (action == "edit_borderb")
    {
        m_ActiveField = EditField::BorderB;
        m_ActiveInputText = std::to_string(m_SelectedElement->borderColor.b);
    }
    else if (action == "edit_borderthickness")
    {
        m_ActiveField = EditField::BorderThickness;
        m_ActiveInputText = std::to_string(m_SelectedElement->borderThickness);
    }
    // Button hover edits
    else if (action == "edit_hoverr")
    {
        m_ActiveField = EditField::HoverR;
        m_ActiveInputText = std::to_string(m_SelectedElement->hoverColor.r);
    }
    else if (action == "edit_hoverg")
    {
        m_ActiveField = EditField::HoverG;
        m_ActiveInputText = std::to_string(m_SelectedElement->hoverColor.g);
    }
    else if (action == "edit_hoverb")
    {
        m_ActiveField = EditField::HoverB;
        m_ActiveInputText = std::to_string(m_SelectedElement->hoverColor.b);
    }
    // Button pressed edits
    else if (action == "edit_pressedr")
    {
        m_ActiveField = EditField::PressedR;
        m_ActiveInputText = std::to_string(m_SelectedElement->pressedColor.r);
    }
    else if (action == "edit_pressedg")
    {
        m_ActiveField = EditField::PressedG;
        m_ActiveInputText = std::to_string(m_SelectedElement->pressedColor.g);
    }
    else if (action == "edit_pressedb")
    {
        m_ActiveField = EditField::PressedB;
        m_ActiveInputText = std::to_string(m_SelectedElement->pressedColor.b);
    }
    // Button normal color edits
    else if (action == "edit_normalr")
    {
        m_ActiveField = EditField::NormalR;
        m_ActiveInputText = std::to_string(m_SelectedElement->normalColor.r);
    }
    else if (action == "edit_normalg")
    {
        m_ActiveField = EditField::NormalG;
        m_ActiveInputText = std::to_string(m_SelectedElement->normalColor.g);
    }
    else if (action == "edit_normalb")
    {
        m_ActiveField = EditField::NormalB;
        m_ActiveInputText = std::to_string(m_SelectedElement->normalColor.b);
    }
    // Text color edits
    else if (action == "edit_textcolor_r")
    {
        m_ActiveField = EditField::TextColorR;
        m_ActiveInputText = std::to_string(m_SelectedElement->textColor.r);
    }
    else if (action == "edit_textcolor_g")
    {
        m_ActiveField = EditField::TextColorG;
        m_ActiveInputText = std::to_string(m_SelectedElement->textColor.g);
    }
    else if (action == "edit_textcolor_b")
    {
        m_ActiveField = EditField::TextColorB;
        m_ActiveInputText = std::to_string(m_SelectedElement->textColor.b);
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
    if (y + 28.f <= m_InspectorClipTop || y >= m_InspectorClipBottom)
        return y + 28.f;

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
    // Skip drawing and hitbox if completely outside the visible inspector area
    if (y + rowH <= m_InspectorClipTop || y >= m_InspectorClipBottom)
        return y + rowH;

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
    // Skip if completely outside the visible inspector area
    if (y + 30.f <= m_InspectorClipTop || y >= m_InspectorClipBottom)
        return y + 30.f;

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
