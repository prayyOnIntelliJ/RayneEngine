#ifndef RAYNEENGINE_EVENTMANAGER_H
#define RAYNEENGINE_EVENTMANAGER_H
#include <functional>
#include <string>
#include <vector>
#include <SFML/Window/Event.hpp>

#include "../ECS/Entity.h"

struct CollisionEvent
{
    Entity a;
    Entity b;
};

class EventManager
{
public:
    static EventManager &Get()
    {
        static EventManager instance;
        return instance;
    }

    void SubscribeCollision(std::function<void(CollisionEvent)> callback);
    void FireCollision(Entity a, Entity b);

    void SubscribeButtonClick(std::function<void(const std::string &)> callback);
    void FireButtonClick(const std::string &buttonId);

    void SubscribeButtonHover(std::function<void(const std::string &)> callback);
    void FireButtonHover(const std::string &buttonId);

    void SubscribeSliderChange(std::function<void(const std::string &, float)> callback);
    void FireSliderChange(const std::string &sliderId, float value);

    void SubscribeCheckboxChange(std::function<void(const std::string &, bool)> callback);
    void FireCheckboxChange(const std::string &checkboxId, bool checked);

    void SubscribeTextInputChange(std::function<void(const std::string &, const std::string &)> callback);
    void FireTextInputChange(const std::string &inputId, const std::string &text);

    void SubscribeTextInputSubmit(std::function<void(const std::string &, const std::string &)> callback);
    void FireTextInputSubmit(const std::string &inputId, const std::string &text);

    void SubscribeUIHover(std::function<void(const std::string &, bool)> callback);
    void FireUIHover(const std::string &elementId, bool hovered);

    void SubscribeUIFocus(std::function<void(const std::string &, bool)> callback);
    void FireUIFocus(const std::string &elementId, bool focused);

    void SubscribeInput(std::function<void(const sf::Event &)> callback);
    void FireInput(const sf::Event &event);

    void Clear();

private:
    EventManager() = default;

    std::vector<std::function<void(CollisionEvent)> > m_CollisionCallbacks;
    std::vector<std::function<void(const std::string &)> > m_ButtonClickCallbacks;
    std::vector<std::function<void(const std::string &)> > m_ButtonHoverCallbacks;
    std::vector<std::function<void(const std::string &, float)> > m_SliderChangeCallbacks;
    std::vector<std::function<void(const std::string &, bool)> > m_CheckboxChangeCallbacks;
    std::vector<std::function<void(const std::string &, const std::string &)> > m_TextInputChangeCallbacks;
    std::vector<std::function<void(const std::string &, const std::string &)> > m_TextInputSubmitCallbacks;
    std::vector<std::function<void(const std::string &, bool)> > m_UIHoverCallbacks;
    std::vector<std::function<void(const std::string &, bool)> > m_UIFocusCallbacks;
    std::vector<std::function<void(const sf::Event &)> > m_InputCallbacks;
};

#endif
