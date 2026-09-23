#include "UIManager.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "../Scripting/LuaState.h"
#include "../Resources/ResourceManager.h"
#include "../Input/InputManager.h"

using json = nlohmann::json;

void UIElement::UpdateDrawables()
{
    shape.setPosition(position);
    shape.setSize(size);
    extraShape.setFillColor(sf::Color::Transparent);

    if (type == UIElementType::Button || type == UIElementType::Checkbox || type == UIElementType::TextInput)
    {
        if (disabled)
        {
            sf::Color dc = disabledColor;
            dc.a = static_cast<sf::Uint8>(std::clamp(opacity, 0.f, 255.f));
            shape.setFillColor(dc);
        }
        else
        {
            sf::Color base = isPressed ? pressedColor : (isHovered ? hoverColor : normalColor);
            if (type == UIElementType::TextInput && isFocused) base = pressedColor;
            base.a = static_cast<sf::Uint8>(std::clamp(opacity, 0.f, 255.f));
            shape.setFillColor(base);
        }
        shape.setOutlineColor(borderColor);
        shape.setOutlineThickness(borderThickness);

        if (type == UIElementType::Checkbox)
        {
            shape.setTexture(isChecked ? checkedTexture.get() : texture.get());
            if (isChecked && !checkedTexture)
            {
                extraShape.setSize({size.x * 0.6f, size.y * 0.6f});
                extraShape.setPosition(position.x + size.x * 0.2f, position.y + size.y * 0.2f);
                extraShape.setFillColor(textColor);
            }
        }
        else if (type == UIElementType::Button)
        {
            if (isPressed && pressedTexture)
                shape.setTexture(pressedTexture.get());
            else if (isHovered && hoverTexture)
                shape.setTexture(hoverTexture.get());
            else
                shape.setTexture(texture.get());
        }
        else
        {
            shape.setTexture(texture.get());
        }
    }
    else if (type == UIElementType::Panel || type == UIElementType::Image)
    {
        sf::Color c = color;
        c.a = static_cast<sf::Uint8>(std::clamp(static_cast<float>(color.a) * (opacity / 255.f), 0.f, 255.f));
        shape.setFillColor(c);
        shape.setOutlineColor(outlineColor);
        shape.setOutlineThickness(outlineThickness);
        shape.setTexture(texture.get());
    }
    else if (type == UIElementType::Slider)
    {
        sf::Color c = color;
        c.a = static_cast<sf::Uint8>(std::clamp(opacity, 0.f, 255.f));
        shape.setFillColor(c);
        shape.setTexture(texture.get());

        float range = sliderMax - sliderMin;
        float percent = (range > 0) ? (sliderValue - sliderMin) / range : 0.f;
        float knobWidth = size.y * 0.8f;
        extraShape.setSize({knobWidth, size.y * 1.2f});
        extraShape.setPosition(position.x + percent * size.x - knobWidth / 2.f, position.y - size.y * 0.1f);
        extraShape.setFillColor(normalColor);
        extraShape.setTexture(knobTexture.get());
    }
    else if (type == UIElementType::ProgressBar)
    {
        sf::Color c = color;
        c.a = static_cast<sf::Uint8>(std::clamp(opacity, 0.f, 255.f));
        shape.setFillColor(c);
        shape.setTexture(texture.get());

        float percent = (progressMax > 0) ? std::clamp(progressValue / progressMax, 0.f, 1.f) : 0.f;
        extraShape.setSize({size.x * percent, size.y});
        extraShape.setPosition(position);
        extraShape.setFillColor(normalColor);
        extraShape.setTexture(fillTexture.get());
    }
    else
    {
        shape.setFillColor(sf::Color::Transparent);
        shape.setOutlineColor(sf::Color::Transparent);
        shape.setOutlineThickness(0.f);
    }

    if (type == UIElementType::Text || type == UIElementType::Button || type == UIElementType::Checkbox || type == UIElementType::TextInput)
    {
        if (font) drawableText.setFont(*font);

        std::string displayText = text;
        if (type == UIElementType::TextInput)
        {
            displayText = text + (isFocused ? "_" : "");
        }
        if (textUpperCase)
            std::transform(displayText.begin(), displayText.end(), displayText.begin(), ::toupper);

        drawableText.setString(displayText);
        drawableText.setCharacterSize(characterSize);
        drawableText.setFillColor(textColor);
        drawableText.setStyle(textStyle);
        drawableText.setLetterSpacing(letterSpacing);
        drawableText.setLineSpacing(lineSpacing);
        drawableText.setOutlineColor(textOutlineColor);
        drawableText.setOutlineThickness(textOutlineThickness);

        sf::FloatRect bounds = drawableText.getLocalBounds();
        float textY = position.y + (size.y - bounds.height) / 2.f - bounds.top + textOffset.y;
        float textX = 0.f;

        if (type == UIElementType::Button || type == UIElementType::Checkbox)
        {
            textX = position.x + (size.x - bounds.width) / 2.f - bounds.left + textOffset.x;
        }
        else if (type == UIElementType::TextInput)
        {
            textX = position.x + 5.f + textOffset.x;
        }
        else
        {
            switch (textAlign)
            {
                case TextAlign::Left:
                    textX = position.x + textOffset.x;
                    break;
                case TextAlign::Center:
                    textX = position.x + (size.x - bounds.width) / 2.f - bounds.left + textOffset.x;
                    break;
                case TextAlign::Right:
                    textX = position.x + size.x - bounds.width - bounds.left + textOffset.x;
                    break;
            }
        }

        drawableText.setPosition(textX, textY);
    }
}


void UIManager::Init(std::shared_ptr<sf::Font> defaultFont) { m_DefaultFont = defaultFont; }

void UIManager::Update(float dt, sf::Vector2f mousePos, bool mouseClicked, bool mouseReleased)
{
    std::vector<UIElement *> sortedElements;
    sortedElements.reserve(m_Elements.size());
    for (auto &el: m_Elements) sortedElements.push_back(&el);

    std::stable_sort(sortedElements.begin(), sortedElements.end(), [](const UIElement *a, const UIElement *b) {
        return a->zIndex > b->zIndex;
    });

    bool buttonHit = false;
    
    const auto& textEntered = InputManager::Get().GetTextEntered();

    for (auto *el: sortedElements)
    {
        if (!el->visible) continue;
        auto executeAction = [&](const std::string& action, const std::string& param, const std::string& id, const std::string& eventType) {
            if (action.empty() || action == "None") return;
            std::string luaCode;
            if (action == "Quit") luaCode = "Engine.Quit()";
            else if (action == "Restart") luaCode = "Engine.RestartCurrentScene()";
            else if (action == "TogglePause") luaCode = "Engine.SetPaused(not Engine.GetPaused())";
            else if (action == "SetFullscreen") luaCode = "Engine.SetFullscreen(" + param + ")";
            else if (action == "SetTimeScale") luaCode = "Engine.SetTimeScale(" + param + ")";
            else if (action == "TakeScreenshot") luaCode = "Engine.TakeScreenshot()";
            else if (action == "OpenURL") luaCode = "Engine.OpenURL(\"" + param + "\")";
            else if (action == "Log") luaCode = "Engine.Log(\"" + param + "\")";
            
            if (!luaCode.empty()) {
                try {
                    LuaState::GetLua().safe_script(luaCode);
                } catch (const std::exception& e) {
                    std::cerr << "[ERROR] [UI] " << eventType << " action failed for " << id << ": " << e.what() << "\n";
                }
            }
        };

        bool canInteract = (el->type == UIElementType::Button || el->type == UIElementType::Checkbox || el->type == UIElementType::Slider || el->type == UIElementType::TextInput);
        
        if (canInteract && !el->disabled)
        {
            sf::FloatRect bounds(el->position.x, el->position.y, el->size.x, el->size.y);
            bool hovered = !buttonHit && bounds.contains(mousePos);

            if (hovered && !el->isHovered)
            {
                executeAction(el->onHoverAction, el->onHoverParam, el->id, "Hover");
            }
            
            el->isHovered = hovered;

            if (hovered)
            {
                buttonHit = true;
                if (mouseClicked) { 
                    el->isPressed = true; 
                    if (el->type == UIElementType::TextInput) el->isFocused = true;
                }
            }
            else
            {
                if (mouseClicked && el->type == UIElementType::TextInput) el->isFocused = false;
            }

            if (el->type == UIElementType::Slider && el->isPressed)
            {
                float relativeX = mousePos.x - el->position.x;
                float percent = std::clamp(relativeX / el->size.x, 0.f, 1.f);
                el->sliderValue = el->sliderMin + percent * (el->sliderMax - el->sliderMin);
            }

            if (mouseReleased)
            {
                if (el->isPressed && el->isHovered)
                {
                    m_LastClickedButton = el->id;
                    if (el->type == UIElementType::Checkbox) el->isChecked = !el->isChecked;
                    executeAction(el->onClickAction, el->onClickParam, el->id, "Click");
                }
                el->isPressed = false;
            }

            if (el->type == UIElementType::TextInput && el->isFocused)
            {
                for (sf::Uint32 unicode : textEntered)
                {
                    if (unicode == '\b')
                    {
                        if (!el->text.empty()) el->text.pop_back();
                    }
                    else if (unicode >= 32 && unicode < 128)
                    {
                        el->text += static_cast<char>(unicode);
                    }
                }
            }
        }
        else if (canInteract && el->disabled)
        {
            el->isHovered = false;
            el->isPressed = false;
            el->isFocused = false;
        }
        el->UpdateDrawables();
    }
}

void UIManager::Render(sf::RenderWindow &window)
{
    std::vector<const UIElement *> sortedElements;
    sortedElements.reserve(m_Elements.size());
    for (const auto &el: m_Elements) sortedElements.push_back(&el);

    std::stable_sort(sortedElements.begin(), sortedElements.end(), [](const UIElement *a, const UIElement *b) {
        return a->zIndex < b->zIndex;
    });

    for (const auto *el: sortedElements)
    {
        if (!el->visible) continue;

        if (el->shape.getFillColor() != sf::Color::Transparent ||
            el->shape.getOutlineThickness() != 0.f || el->shape.getTexture() != nullptr)
            window.draw(el->shape);
            
        if (el->extraShape.getFillColor() != sf::Color::Transparent || el->extraShape.getTexture() != nullptr)
            window.draw(el->extraShape);

        if (el->type == UIElementType::Text || el->type == UIElementType::Button || el->type == UIElementType::Checkbox || el->type == UIElementType::TextInput)
        {
            if (el->font)
                window.draw(el->drawableText);
        }
    }
}

void UIManager::SetTexture(const std::string &id, const std::string &path)
{
    if (auto *el = GetElement(id))
    {
        el->texturePath = path;
        el->texture = ResourceManager::Get().GetTexture(path);
        el->UpdateDrawables();
    }
}

void UIManager::SetHoverTexture(const std::string &id, const std::string &path)
{
    if (auto *el = GetElement(id))
    {
        el->hoverTexturePath = path;
        el->hoverTexture = ResourceManager::Get().GetTexture(path);
        el->UpdateDrawables();
    }
}

void UIManager::SetPressedTexture(const std::string &id, const std::string &path)
{
    if (auto *el = GetElement(id))
    {
        el->pressedTexturePath = path;
        el->pressedTexture = ResourceManager::Get().GetTexture(path);
        el->UpdateDrawables();
    }
}

void UIManager::SetCheckedTexture(const std::string &id, const std::string &path)
{
    if (auto *el = GetElement(id))
    {
        el->checkedTexturePath = path;
        el->checkedTexture = ResourceManager::Get().GetTexture(path);
        el->UpdateDrawables();
    }
}

void UIManager::SetKnobTexture(const std::string &id, const std::string &path)
{
    if (auto *el = GetElement(id))
    {
        el->knobTexturePath = path;
        el->knobTexture = ResourceManager::Get().GetTexture(path);
        el->UpdateDrawables();
    }
}

void UIManager::SetFillTexture(const std::string &id, const std::string &path)
{
    if (auto *el = GetElement(id))
    {
        el->fillTexturePath = path;
        el->fillTexture = ResourceManager::Get().GetTexture(path);
        el->UpdateDrawables();
    }
}

void UIManager::SetChecked(const std::string &id, bool checked)
{
    if (auto *el = GetElement(id))
    {
        el->isChecked = checked;
        el->UpdateDrawables();
    }
}

bool UIManager::GetChecked(const std::string &id)
{
    if (auto *el = GetElement(id)) return el->isChecked;
    return false;
}

void UIManager::SetSliderValue(const std::string &id, float value)
{
    if (auto *el = GetElement(id))
    {
        el->sliderValue = std::clamp(value, el->sliderMin, el->sliderMax);
        el->UpdateDrawables();
    }
}

float UIManager::GetSliderValue(const std::string &id)
{
    if (auto *el = GetElement(id)) return el->sliderValue;
    return 0.f;
}

void UIManager::SetSliderMinMax(const std::string &id, float min, float max)
{
    if (auto *el = GetElement(id))
    {
        el->sliderMin = min;
        el->sliderMax = max;
        el->sliderValue = std::clamp(el->sliderValue, min, max);
        el->UpdateDrawables();
    }
}

void UIManager::SetProgressValue(const std::string &id, float value)
{
    if (auto *el = GetElement(id))
    {
        el->progressValue = value;
        el->UpdateDrawables();
    }
}

float UIManager::GetProgressValue(const std::string &id)
{
    if (auto *el = GetElement(id)) return el->progressValue;
    return 0.f;
}

void UIManager::SetProgressMax(const std::string &id, float max)
{
    if (auto *el = GetElement(id))
    {
        el->progressMax = max;
        el->UpdateDrawables();
    }
}

void UIManager::SetFocused(const std::string &id, bool focused)
{
    if (auto *el = GetElement(id))
    {
        el->isFocused = focused;
        el->UpdateDrawables();
    }
}

bool UIManager::GetFocused(const std::string &id)
{
    if (auto *el = GetElement(id)) return el->isFocused;
    return false;
}


void UIManager::Save(const std::string &path)
{
    json data;
    data["ui_elements"] = json::array();

    for (auto &el: m_Elements)
    {
        json j;
        j["id"] = el.id;
        if (el.type == UIElementType::Text) j["type"] = "text";
        else if (el.type == UIElementType::Button) j["type"] = "button";
        else if (el.type == UIElementType::Image) j["type"] = "image";
        else if (el.type == UIElementType::Checkbox) j["type"] = "checkbox";
        else if (el.type == UIElementType::Slider) j["type"] = "slider";
        else if (el.type == UIElementType::TextInput) j["type"] = "textinput";
        else if (el.type == UIElementType::ProgressBar) j["type"] = "progressbar";
        else j["type"] = "panel";

        j["x"] = el.position.x;
        j["y"] = el.position.y;
        j["width"] = el.size.x;
        j["height"] = el.size.y;
        j["color"] = {el.color.r, el.color.g, el.color.b, el.color.a};
        j["zIndex"] = el.zIndex;
        j["opacity"] = el.opacity;
        j["visible"] = el.visible;
        j["outlineColor"] = {el.outlineColor.r, el.outlineColor.g, el.outlineColor.b, el.outlineColor.a};
        j["outlineThickness"] = el.outlineThickness;
        j["cornerRadius"] = el.cornerRadius;

        j["texturePath"] = el.texturePath;

        if (el.type == UIElementType::Text || el.type == UIElementType::Button || el.type == UIElementType::Checkbox || el.type == UIElementType::TextInput)
        {
            j["text"] = el.text;
            j["characterSize"] = el.characterSize;
            j["textColor"] = {el.textColor.r, el.textColor.g, el.textColor.b, el.textColor.a};
            j["textStyle"] = static_cast<int>(el.textStyle);
            j["textAlign"] = static_cast<int>(el.textAlign);
            j["textUpperCase"] = el.textUpperCase;
            j["letterSpacing"] = el.letterSpacing;
            j["lineSpacing"] = el.lineSpacing;
            j["textOutlineColor"] = {el.textOutlineColor.r, el.textOutlineColor.g, el.textOutlineColor.b, el.textOutlineColor.a};
            j["textOutlineThickness"] = el.textOutlineThickness;
            j["textOffsetX"] = el.textOffset.x;
            j["textOffsetY"] = el.textOffset.y;
        }

        if (el.type == UIElementType::Button || el.type == UIElementType::Checkbox || el.type == UIElementType::Slider || el.type == UIElementType::TextInput)
        {
            j["normalColor"] = {el.normalColor.r, el.normalColor.g, el.normalColor.b, el.normalColor.a};
            j["hoverColor"] = {el.hoverColor.r, el.hoverColor.g, el.hoverColor.b, el.hoverColor.a};
            j["pressedColor"] = {el.pressedColor.r, el.pressedColor.g, el.pressedColor.b, el.pressedColor.a};
            j["borderColor"] = {el.borderColor.r, el.borderColor.g, el.borderColor.b, el.borderColor.a};
            j["borderThickness"] = el.borderThickness;
            j["disabled"] = el.disabled;
            j["disabledColor"] = {el.disabledColor.r, el.disabledColor.g, el.disabledColor.b, el.disabledColor.a};
            
            if (el.type == UIElementType::Button || el.type == UIElementType::Checkbox)
            {
                j["onClickAction"] = el.onClickAction;
                j["onClickParam"] = el.onClickParam;
                j["onHoverAction"] = el.onHoverAction;
                j["onHoverParam"] = el.onHoverParam;
            }

            if (el.type == UIElementType::Button)
            {
                j["hoverTexturePath"] = el.hoverTexturePath;
                j["pressedTexturePath"] = el.pressedTexturePath;
            }
        }

        if (el.type == UIElementType::Checkbox)
        {
            j["isChecked"] = el.isChecked;
            j["checkedTexturePath"] = el.checkedTexturePath;
        }
        if (el.type == UIElementType::Slider)
        {
            j["sliderValue"] = el.sliderValue;
            j["sliderMin"] = el.sliderMin;
            j["sliderMax"] = el.sliderMax;
            j["knobTexturePath"] = el.knobTexturePath;
        }
        if (el.type == UIElementType::ProgressBar)
        {
            j["progressValue"] = el.progressValue;
            j["progressMax"] = el.progressMax;
            j["fillTexturePath"] = el.fillTexturePath;
        }

        data["ui_elements"].push_back(j);
    }

    std::ofstream file(path);
    if (file.is_open())
        file << data.dump(4, ' ', false, json::error_handler_t::replace);
}


void UIManager::Load(const std::string &path)
{
    m_Elements.clear();

    std::ifstream file(path);
    if (!file.is_open()) return;

    json data;
    try { data = json::parse(file); } catch (...) { return; }

    if (!data.contains("ui_elements")) return;

    for (auto &j: data["ui_elements"])
    {
        UIElement el;
        el.id = j.value("id", "unnamed");

        std::string typeStr = j.value("type", "panel");
        if (typeStr == "text") el.type = UIElementType::Text;
        else if (typeStr == "button") el.type = UIElementType::Button;
        else if (typeStr == "image") el.type = UIElementType::Image;
        else if (typeStr == "checkbox") el.type = UIElementType::Checkbox;
        else if (typeStr == "slider") el.type = UIElementType::Slider;
        else if (typeStr == "textinput") el.type = UIElementType::TextInput;
        else if (typeStr == "progressbar") el.type = UIElementType::ProgressBar;
        else el.type = UIElementType::Panel;

        el.position = {j.value("x", 0.f), j.value("y", 0.f)};
        el.size = {j.value("width", 100.f), j.value("height", 50.f)};
        el.zIndex = j.value("zIndex", 0);
        el.opacity = j.value("opacity", 255.f);
        el.visible = j.value("visible", true);
        el.outlineThickness = j.value("outlineThickness", 0.f);
        el.cornerRadius = j.value("cornerRadius", 0.f);
        
        el.texturePath = j.value("texturePath", "");
        if (!el.texturePath.empty()) el.texture = ResourceManager::Get().GetTexture(el.texturePath);

        if (j.contains("color"))
            el.color = sf::Color(j["color"][0], j["color"][1], j["color"][2],
                                 j["color"].size() > 3 ? j["color"][3].get<int>() : 255);
        else el.color = sf::Color::White;

        if (j.contains("outlineColor"))
            el.outlineColor = sf::Color(j["outlineColor"][0], j["outlineColor"][1],
                                        j["outlineColor"][2],
                                        j["outlineColor"].size() > 3 ? j["outlineColor"][3].get<int>() : 255);

        if (el.type == UIElementType::Text || el.type == UIElementType::Button || el.type == UIElementType::Checkbox || el.type == UIElementType::TextInput)
        {
            el.text = j.value("text", "Text");
            if (el.type == UIElementType::TextInput) el.text = j.value("text", "");
            el.characterSize = j.value("characterSize", 16);
            el.textStyle = static_cast<sf::Text::Style>(j.value("textStyle", 0));
            el.textAlign = static_cast<TextAlign>(j.value("textAlign", 0));
            el.textUpperCase = j.value("textUpperCase", false);
            el.letterSpacing = j.value("letterSpacing", 1.0f);
            el.lineSpacing = j.value("lineSpacing", 1.0f);
            el.textOutlineThickness = j.value("textOutlineThickness", 0.f);
            el.textOffset = {j.value("textOffsetX", 0.f), j.value("textOffsetY", 0.f)};

            if (j.contains("textColor"))
                el.textColor = sf::Color(j["textColor"][0], j["textColor"][1],
                                         j["textColor"][2],
                                         j["textColor"].size() > 3
                                             ? j["textColor"][3].get<int>()
                                             : 255);
            if (j.contains("textOutlineColor"))
                el.textOutlineColor = sf::Color(j["textOutlineColor"][0], j["textOutlineColor"][1],
                                                j["textOutlineColor"][2],
                                                j["textOutlineColor"].size() > 3
                                                    ? j["textOutlineColor"][3].get<int>()
                                                    : 255);
        }

        if (el.type == UIElementType::Button || el.type == UIElementType::Checkbox || el.type == UIElementType::Slider || el.type == UIElementType::TextInput)
        {
            el.borderThickness = j.value("borderThickness", 0.f);
            el.disabled = j.value("disabled", false);
            el.onClickAction = j.value("onClickAction", "");
            el.onClickParam = j.value("onClickParam", "");
            el.onHoverAction = j.value("onHoverAction", "");
            el.onHoverParam = j.value("onHoverParam", "");

            if (j.contains("normalColor"))
                el.normalColor = sf::Color(j["normalColor"][0], j["normalColor"][1],
                                           j["normalColor"][2],
                                           j["normalColor"].size() > 3
                                               ? j["normalColor"][3].get<int>()
                                               : 255);
            if (j.contains("hoverColor"))
                el.hoverColor = sf::Color(j["hoverColor"][0], j["hoverColor"][1],
                                          j["hoverColor"][2],
                                          j["hoverColor"].size() > 3
                                              ? j["hoverColor"][3].get<int>()
                                              : 255);
            if (j.contains("pressedColor"))
                el.pressedColor = sf::Color(j["pressedColor"][0], j["pressedColor"][1],
                                            j["pressedColor"][2],
                                            j["pressedColor"].size() > 3
                                                ? j["pressedColor"][3].get<int>()
                                                : 255);
            if (j.contains("borderColor"))
                el.borderColor = sf::Color(j["borderColor"][0], j["borderColor"][1],
                                           j["borderColor"][2],
                                           j["borderColor"].size() > 3
                                               ? j["borderColor"][3].get<int>()
                                               : 255);
            if (j.contains("disabledColor"))
                el.disabledColor = sf::Color(j["disabledColor"][0], j["disabledColor"][1],
                                             j["disabledColor"][2],
                                             j["disabledColor"].size() > 3
                                                 ? j["disabledColor"][3].get<int>()
                                                 : 255);
        }

        if (el.type == UIElementType::Button)
        {
            el.hoverTexturePath = j.value("hoverTexturePath", "");
            if (!el.hoverTexturePath.empty()) el.hoverTexture = ResourceManager::Get().GetTexture(el.hoverTexturePath);
            el.pressedTexturePath = j.value("pressedTexturePath", "");
            if (!el.pressedTexturePath.empty()) el.pressedTexture = ResourceManager::Get().GetTexture(el.pressedTexturePath);
        }

        if (el.type == UIElementType::Checkbox)
        {
            el.isChecked = j.value("isChecked", false);
            el.checkedTexturePath = j.value("checkedTexturePath", "");
            if (!el.checkedTexturePath.empty()) el.checkedTexture = ResourceManager::Get().GetTexture(el.checkedTexturePath);
        }
        if (el.type == UIElementType::Slider)
        {
            el.sliderValue = j.value("sliderValue", 0.5f);
            el.sliderMin = j.value("sliderMin", 0.0f);
            el.sliderMax = j.value("sliderMax", 1.0f);
            el.knobTexturePath = j.value("knobTexturePath", "");
            if (!el.knobTexturePath.empty()) el.knobTexture = ResourceManager::Get().GetTexture(el.knobTexturePath);
        }
        if (el.type == UIElementType::ProgressBar)
        {
            el.progressValue = j.value("progressValue", 0.5f);
            el.progressMax = j.value("progressMax", 1.0f);
            el.fillTexturePath = j.value("fillTexturePath", "");
            if (!el.fillTexturePath.empty()) el.fillTexture = ResourceManager::Get().GetTexture(el.fillTexturePath);
        }

        el.font = m_DefaultFont;
        el.UpdateDrawables();
        m_Elements.push_back(el);
    }
}


UIElement *UIManager::CreateElement(const std::string &id, UIElementType type)
{
    UIElement el;
    el.id = id;
    el.type = type;
    el.position = {0, 0};
    el.size = {100, 50};
    el.color = sf::Color::White;
    el.font = m_DefaultFont;
    if (type == UIElementType::Text || type == UIElementType::Button || type == UIElementType::Checkbox || type == UIElementType::TextInput)
    {
        el.text = (type == UIElementType::Button) ? "Button" : ((type == UIElementType::Checkbox) ? "Check" : ((type == UIElementType::TextInput) ? "" : "Text"));
        el.textColor = (type == UIElementType::Button) ? sf::Color::Black : sf::Color::White;
    }
    el.UpdateDrawables();
    m_Elements.push_back(el);
    return &m_Elements.back();
}

void UIManager::RemoveElement(const std::string &id)
{
    std::erase_if(m_Elements, [&](const UIElement &e) { return e.id == id; });
}

UIElement *UIManager::GetElement(const std::string &id)
{
    for (auto &el: m_Elements) { if (el.id == id) return &el; }
    return nullptr;
}

void UIManager::SetText(const std::string &id, const std::string &text)
{
    if (auto *el = GetElement(id))
    {
        el->text = text;
        el->UpdateDrawables();
    }
}

std::string UIManager::GetText(const std::string &id)
{
    if (auto *el = GetElement(id)) return el->text;
    return "";
}

void UIManager::SetPosition(const std::string &id, float x, float y)
{
    if (auto *el = GetElement(id))
    {
        el->position = {x, y};
        el->UpdateDrawables();
    }
}

void UIManager::SetSize(const std::string &id, float w, float h)
{
    if (auto *el = GetElement(id))
    {
        el->size = {w, h};
        el->UpdateDrawables();
    }
}

void UIManager::SetColor(const std::string &id, int r, int g, int b, int a)
{
    if (auto *el = GetElement(id))
    {
        el->color = sf::Color(r, g, b, a);
        el->UpdateDrawables();
    }
}

void UIManager::SetZIndex(const std::string &id, int z) { if (auto *el = GetElement(id)) { el->zIndex = z; } }

int UIManager::GetZIndex(const std::string &id)
{
    if (auto *el = GetElement(id)) return el->zIndex;
    return 0;
}

bool UIManager::IsButtonClicked(const std::string &id) { return (m_LastClickedButton == id); }

void UIManager::SetVisible(const std::string &id, bool visible)
{
    if (auto *el = GetElement(id))
    {
        el->visible = visible;
        el->UpdateDrawables();
    }
}

bool UIManager::GetVisible(const std::string &id)
{
    if (auto *el = GetElement(id)) return el->visible;
    return true;
}

void UIManager::SetOpacity(const std::string &id, float opacity)
{
    if (auto *el = GetElement(id))
    {
        el->opacity = std::clamp(opacity, 0.f, 255.f);
        el->UpdateDrawables();
    }
}

float UIManager::GetOpacity(const std::string &id)
{
    if (auto *el = GetElement(id)) return el->opacity;
    return 255.f;
}

void UIManager::SetTextStyle(const std::string &id, int style)
{
    if (auto *el = GetElement(id))
    {
        el->textStyle = static_cast<sf::Text::Style>(style);
        el->UpdateDrawables();
    }
}

void UIManager::SetTextAlign(const std::string &id, int align)
{
    if (auto *el = GetElement(id))
    {
        el->textAlign = static_cast<TextAlign>(std::clamp(align, 0, 2));
        el->UpdateDrawables();
    }
}

void UIManager::SetUpperCase(const std::string &id, bool upper)
{
    if (auto *el = GetElement(id))
    {
        el->textUpperCase = upper;
        el->UpdateDrawables();
    }
}

bool UIManager::GetUpperCase(const std::string &id)
{
    if (auto *el = GetElement(id)) return el->textUpperCase;
    return false;
}

void UIManager::SetFontSize(const std::string &id, int size)
{
    if (auto *el = GetElement(id))
    {
        el->characterSize = static_cast<unsigned int>(std::max(1, size));
        el->UpdateDrawables();
    }
}

void UIManager::SetLetterSpacing(const std::string &id, float spacing)
{
    if (auto *el = GetElement(id))
    {
        el->letterSpacing = spacing;
        el->UpdateDrawables();
    }
}

void UIManager::SetLineSpacing(const std::string &id, float spacing)
{
    if (auto *el = GetElement(id))
    {
        el->lineSpacing = spacing;
        el->UpdateDrawables();
    }
}

void UIManager::SetTextOutline(const std::string &id, int r, int g, int b, int a, float thickness)
{
    if (auto *el = GetElement(id))
    {
        el->textOutlineColor = sf::Color(r, g, b, a);
        el->textOutlineThickness = thickness;
        el->UpdateDrawables();
    }
}

void UIManager::SetTextOffset(const std::string &id, float ox, float oy)
{
    if (auto *el = GetElement(id))
    {
        el->textOffset = {ox, oy};
        el->UpdateDrawables();
    }
}

void UIManager::SetTextColor(const std::string &id, int r, int g, int b, int a)
{
    if (auto *el = GetElement(id))
    {
        el->textColor = sf::Color(r, g, b, a);
        el->UpdateDrawables();
    }
}

void UIManager::SetOutline(const std::string &id, int r, int g, int b, int a, float thickness)
{
    if (auto *el = GetElement(id))
    {
        el->outlineColor = sf::Color(r, g, b, a);
        el->outlineThickness = thickness;
        el->UpdateDrawables();
    }
}

void UIManager::SetDisabled(const std::string &id, bool disabled)
{
    if (auto *el = GetElement(id))
    {
        el->disabled = disabled;
        el->UpdateDrawables();
    }
}

bool UIManager::IsButtonHovered(const std::string &id)
{
    if (auto *el = GetElement(id)) return el->isHovered;
    return false;
}

