#include "UIManager.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void UIElement::UpdateDrawables()
{
    shape.setPosition(position);
    shape.setSize(size);
    
    if (type == UIElementType::Button)
    {
        if (isPressed) shape.setFillColor(pressedColor);
        else if (isHovered) shape.setFillColor(hoverColor);
        else shape.setFillColor(normalColor);
    }
    else if (type == UIElementType::Panel)
    {
        shape.setFillColor(color);
    }
    else
    {
        shape.setFillColor(sf::Color::Transparent);
    }
    
    if (type == UIElementType::Text || type == UIElementType::Button)
    {
        if (font) drawableText.setFont(*font);
        drawableText.setString(text);
        drawableText.setCharacterSize(characterSize);
        drawableText.setFillColor(textColor);
        
        if (type == UIElementType::Button) {
            sf::FloatRect bounds = drawableText.getLocalBounds();
            drawableText.setPosition(
                position.x + (size.x - bounds.width) / 2.f - bounds.left,
                position.y + (size.y - bounds.height) / 2.f - bounds.top
            );
        } else {
            drawableText.setPosition(position);
        }
    }
}

void UIManager::Init(std::shared_ptr<sf::Font> defaultFont)
{
    m_DefaultFont = defaultFont;
}

void UIManager::Update(float dt, sf::Vector2f mousePos, bool mouseClicked, bool mouseReleased)
{
    // Process top-most (highest zIndex) elements first
    std::vector<UIElement*> sortedElements;
    sortedElements.reserve(m_Elements.size());
    for (auto& el : m_Elements) sortedElements.push_back(&el);
    
    std::stable_sort(sortedElements.begin(), sortedElements.end(), [](const UIElement* a, const UIElement* b) {
        return a->zIndex > b->zIndex;
    });

    bool buttonHit = false;
    for (auto* el : sortedElements)
    {
        if (el->type == UIElementType::Button)
        {
            sf::FloatRect bounds(el->position.x, el->position.y, el->size.x, el->size.y);
            bool hovered = !buttonHit && bounds.contains(mousePos);
            el->isHovered = hovered;
            
            if (hovered) {
                buttonHit = true;
                if (mouseClicked) {
                    el->isPressed = true;
                }
            }
            if (mouseReleased) {
                if (el->isPressed && el->isHovered) {
                    m_LastClickedButton = el->id;
                }
                el->isPressed = false;
            }
        }
        el->UpdateDrawables();
    }
}

void UIManager::Render(sf::RenderWindow& window)
{
    // Draw sorted by zIndex ascending (lowest zIndex first, highest on top)
    std::vector<const UIElement*> sortedElements;
    sortedElements.reserve(m_Elements.size());
    for (const auto& el : m_Elements) sortedElements.push_back(&el);

    std::stable_sort(sortedElements.begin(), sortedElements.end(), [](const UIElement* a, const UIElement* b) {
        return a->zIndex < b->zIndex;
    });

    for (const auto* el : sortedElements)
    {
        if (el->shape.getFillColor() != sf::Color::Transparent)
            window.draw(el->shape);
            
        if (el->type == UIElementType::Text || el->type == UIElementType::Button)
        {
            if (el->font)
                window.draw(el->drawableText);
        }
    }
}

void UIManager::Save(const std::string& path)
{
    json data;
    data["ui_elements"] = json::array();
    
    for (auto& el : m_Elements)
    {
        json j;
        j["id"] = el.id;
        j["type"] = (el.type == UIElementType::Text) ? "text" : (el.type == UIElementType::Panel) ? "panel" : "button";
        j["x"] = el.position.x;
        j["y"] = el.position.y;
        j["width"] = el.size.x;
        j["height"] = el.size.y;
        j["color"] = {el.color.r, el.color.g, el.color.b, el.color.a};
        j["zIndex"] = el.zIndex;
        
        if (el.type == UIElementType::Text || el.type == UIElementType::Button)
        {
            j["text"] = el.text;
            j["characterSize"] = el.characterSize;
            j["textColor"] = {el.textColor.r, el.textColor.g, el.textColor.b, el.textColor.a};
        }
        
        if (el.type == UIElementType::Button)
        {
            j["normalColor"] = {el.normalColor.r, el.normalColor.g, el.normalColor.b, el.normalColor.a};
            j["hoverColor"] = {el.hoverColor.r, el.hoverColor.g, el.hoverColor.b, el.hoverColor.a};
            j["pressedColor"] = {el.pressedColor.r, el.pressedColor.g, el.pressedColor.b, el.pressedColor.a};
        }
        
        data["ui_elements"].push_back(j);
    }
    
    std::ofstream file(path);
    if (file.is_open())
        file << data.dump(4);
}

void UIManager::Load(const std::string& path)
{
    m_Elements.clear();
    
    std::ifstream file(path);
    if (!file.is_open()) return;
    
    json data;
    try {
        data = json::parse(file);
    } catch(...) { return; }
    
    if (!data.contains("ui_elements")) return;
    
    for (auto& j : data["ui_elements"])
    {
        UIElement el;
        el.id = j.value("id", "unnamed");
        
        std::string typeStr = j.value("type", "panel");
        if (typeStr == "text") el.type = UIElementType::Text;
        else if (typeStr == "button") el.type = UIElementType::Button;
        else el.type = UIElementType::Panel;
        
        el.position = {j.value("x", 0.f), j.value("y", 0.f)};
        el.size = {j.value("width", 100.f), j.value("height", 50.f)};
        el.zIndex = j.value("zIndex", 0);
        
        if (j.contains("color")) el.color = sf::Color(j["color"][0], j["color"][1], j["color"][2], j["color"].size() > 3 ? j["color"][3].get<int>() : 255);
        else el.color = sf::Color::White;
        
        if (el.type == UIElementType::Text || el.type == UIElementType::Button)
        {
            el.text = j.value("text", "Text");
            el.characterSize = j.value("characterSize", 16);
            if (j.contains("textColor")) el.textColor = sf::Color(j["textColor"][0], j["textColor"][1], j["textColor"][2], j["textColor"].size() > 3 ? j["textColor"][3].get<int>() : 255);
        }
        
        if (el.type == UIElementType::Button)
        {
            if (j.contains("normalColor")) el.normalColor = sf::Color(j["normalColor"][0], j["normalColor"][1], j["normalColor"][2], j["normalColor"].size() > 3 ? j["normalColor"][3].get<int>() : 255);
            if (j.contains("hoverColor")) el.hoverColor = sf::Color(j["hoverColor"][0], j["hoverColor"][1], j["hoverColor"][2], j["hoverColor"].size() > 3 ? j["hoverColor"][3].get<int>() : 255);
            if (j.contains("pressedColor")) el.pressedColor = sf::Color(j["pressedColor"][0], j["pressedColor"][1], j["pressedColor"][2], j["pressedColor"].size() > 3 ? j["pressedColor"][3].get<int>() : 255);
        }
        
        el.font = m_DefaultFont;
        el.UpdateDrawables();
        m_Elements.push_back(el);
    }
}

UIElement* UIManager::CreateElement(const std::string& id, UIElementType type)
{
    UIElement el;
    el.id = id;
    el.type = type;
    el.position = {0, 0};
    el.size = {100, 50};
    el.color = sf::Color::White;
    el.font = m_DefaultFont;
    if (type == UIElementType::Text || type == UIElementType::Button) {
        el.text = (type == UIElementType::Button) ? "Button" : "Text";
        el.textColor = (type == UIElementType::Button) ? sf::Color::Black : sf::Color::White;
    }
    el.UpdateDrawables();
    m_Elements.push_back(el);
    return &m_Elements.back();
}

void UIManager::RemoveElement(const std::string& id)
{
    std::erase_if(m_Elements, [&](const UIElement& e) { return e.id == id; });
}

UIElement* UIManager::GetElement(const std::string& id)
{
    for (auto& el : m_Elements) {
        if (el.id == id) return &el;
    }
    return nullptr;
}

void UIManager::SetText(const std::string& id, const std::string& text)
{
    if (auto* el = GetElement(id)) {
        el->text = text;
        el->UpdateDrawables();
    }
}

std::string UIManager::GetText(const std::string& id)
{
    if (auto* el = GetElement(id)) return el->text;
    return "";
}

void UIManager::SetPosition(const std::string& id, float x, float y)
{
    if (auto* el = GetElement(id)) {
        el->position = {x, y};
        el->UpdateDrawables();
    }
}

void UIManager::SetSize(const std::string& id, float w, float h)
{
    if (auto* el = GetElement(id)) {
        el->size = {w, h};
        el->UpdateDrawables();
    }
}

void UIManager::SetColor(const std::string& id, int r, int g, int b, int a)
{
    if (auto* el = GetElement(id)) {
        el->color = sf::Color(r, g, b, a);
        el->UpdateDrawables();
    }
}

void UIManager::SetZIndex(const std::string& id, int z)
{
    if (auto* el = GetElement(id)) {
        el->zIndex = z;
    }
}

int UIManager::GetZIndex(const std::string& id)
{
    if (auto* el = GetElement(id)) return el->zIndex;
    return 0;
}

bool UIManager::IsButtonClicked(const std::string& id)
{
    return (m_LastClickedButton == id);
}
