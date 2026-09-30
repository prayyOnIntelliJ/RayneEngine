#include "EventManager.h"

void EventManager::SubscribeCollision(std::function<void(CollisionEvent)> callback)
{
    m_CollisionCallbacks.push_back(std::move(callback));
}

void EventManager::FireCollision(Entity a, Entity b)
{
    const size_t count = m_CollisionCallbacks.size();
    for (size_t i = 0; i < count && i < m_CollisionCallbacks.size(); ++i)
        m_CollisionCallbacks[i]({a, b});
}

void EventManager::SubscribeButtonClick(std::function<void(const std::string &)> callback)
{
    m_ButtonClickCallbacks.push_back(std::move(callback));
}

void EventManager::FireButtonClick(const std::string &buttonId)
{
    const size_t count = m_ButtonClickCallbacks.size();
    for (size_t i = 0; i < count && i < m_ButtonClickCallbacks.size(); ++i)
        m_ButtonClickCallbacks[i](buttonId);
}

void EventManager::SubscribeButtonHover(std::function<void(const std::string &)> callback)
{
    m_ButtonHoverCallbacks.push_back(std::move(callback));
}

void EventManager::FireButtonHover(const std::string &buttonId)
{
    const size_t count = m_ButtonHoverCallbacks.size();
    for (size_t i = 0; i < count && i < m_ButtonHoverCallbacks.size(); ++i)
        m_ButtonHoverCallbacks[i](buttonId);
}

void EventManager::SubscribeSliderChange(std::function<void(const std::string &, float)> callback)
{
    m_SliderChangeCallbacks.push_back(std::move(callback));
}

void EventManager::FireSliderChange(const std::string &sliderId, float value)
{
    const size_t count = m_SliderChangeCallbacks.size();
    for (size_t i = 0; i < count && i < m_SliderChangeCallbacks.size(); ++i)
        m_SliderChangeCallbacks[i](sliderId, value);
}

void EventManager::SubscribeCheckboxChange(std::function<void(const std::string &, bool)> callback)
{
    m_CheckboxChangeCallbacks.push_back(std::move(callback));
}

void EventManager::FireCheckboxChange(const std::string &checkboxId, bool checked)
{
    const size_t count = m_CheckboxChangeCallbacks.size();
    for (size_t i = 0; i < count && i < m_CheckboxChangeCallbacks.size(); ++i)
        m_CheckboxChangeCallbacks[i](checkboxId, checked);
}

void EventManager::SubscribeTextInputChange(std::function<void(const std::string &, const std::string &)> callback)
{
    m_TextInputChangeCallbacks.push_back(std::move(callback));
}

void EventManager::FireTextInputChange(const std::string &inputId, const std::string &text)
{
    const size_t count = m_TextInputChangeCallbacks.size();
    for (size_t i = 0; i < count && i < m_TextInputChangeCallbacks.size(); ++i)
        m_TextInputChangeCallbacks[i](inputId, text);
}

void EventManager::SubscribeTextInputSubmit(std::function<void(const std::string &, const std::string &)> callback)
{
    m_TextInputSubmitCallbacks.push_back(std::move(callback));
}

void EventManager::FireTextInputSubmit(const std::string &inputId, const std::string &text)
{
    const size_t count = m_TextInputSubmitCallbacks.size();
    for (size_t i = 0; i < count && i < m_TextInputSubmitCallbacks.size(); ++i)
        m_TextInputSubmitCallbacks[i](inputId, text);
}

void EventManager::SubscribeUIHover(std::function<void(const std::string &, bool)> callback)
{
    m_UIHoverCallbacks.push_back(std::move(callback));
}

void EventManager::FireUIHover(const std::string &elementId, bool hovered)
{
    const size_t count = m_UIHoverCallbacks.size();
    for (size_t i = 0; i < count && i < m_UIHoverCallbacks.size(); ++i)
        m_UIHoverCallbacks[i](elementId, hovered);
}

void EventManager::SubscribeUIFocus(std::function<void(const std::string &, bool)> callback)
{
    m_UIFocusCallbacks.push_back(std::move(callback));
}

void EventManager::FireUIFocus(const std::string &elementId, bool focused)
{
    const size_t count = m_UIFocusCallbacks.size();
    for (size_t i = 0; i < count && i < m_UIFocusCallbacks.size(); ++i)
        m_UIFocusCallbacks[i](elementId, focused);
}

void EventManager::SubscribeInput(std::function<void(const sf::Event &)> callback)
{
    m_InputCallbacks.push_back(std::move(callback));
}

void EventManager::FireInput(const sf::Event &event)
{
    const size_t count = m_InputCallbacks.size();
    for (size_t i = 0; i < count && i < m_InputCallbacks.size(); ++i)
        m_InputCallbacks[i](event);
}

void EventManager::Clear()
{
    m_CollisionCallbacks.clear();
    m_ButtonClickCallbacks.clear();
    m_ButtonHoverCallbacks.clear();
    m_SliderChangeCallbacks.clear();
    m_CheckboxChangeCallbacks.clear();
    m_TextInputChangeCallbacks.clear();
    m_TextInputSubmitCallbacks.clear();
    m_UIHoverCallbacks.clear();
    m_UIFocusCallbacks.clear();
    m_InputCallbacks.clear();
}
