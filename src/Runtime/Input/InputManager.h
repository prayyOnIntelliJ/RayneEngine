#ifndef RAYNEENGINE_INPUTMANAGER_H
#define RAYNEENGINE_INPUTMANAGER_H

#include <unordered_set>
#include <vector>
#include "SFML/Window/Event.hpp"
#include "SFML/Window/Keyboard.hpp"
#include "SFML/Window/Mouse.hpp"
#include "SFML/System/Vector2.hpp"
#include "sol/state.hpp"

enum class InputEventType
{
    Unknown = 0,
    KeyDown = 1,
    KeyUp = 2,
    MouseDown = 3,
    MouseUp = 4,
    MouseMove = 5,
    MouseWheel = 6,
    TextEntered = 7,
    JoystickPressed = 8,
    JoystickReleased = 9,
    JoystickMoved = 10
};

class InputManager
{
public:
    static InputManager &Get()
    {
        static InputManager instance;
        return instance;
    }

    void HandleEvent(const sf::Event &event);

    void EndFrame();

    static bool IsKeyDown(sf::Keyboard::Key key);

    static bool IsKeyPressed(sf::Keyboard::Key key);

    static bool IsKeyReleased(sf::Keyboard::Key key);

    static bool IsMouseDown(sf::Mouse::Button button);

    static bool IsMousePressed(sf::Mouse::Button button);

    static bool IsMouseReleased(sf::Mouse::Button button);

    static sf::Vector2i MousePosition();

    static float MouseScrollDelta();

    const std::vector<sf::Uint32> &GetTextEntered() const { return m_TextEntered; }

    static sf::Keyboard::Key StringToKey(std::string keyName);

    static std::string KeyToString(sf::Keyboard::Key key);

    static sf::Mouse::Button StringToMouseButton(std::string btnName);

    static std::string MouseButtonToString(sf::Mouse::Button button);

    bool HasInputThisFrame() const { return m_HasInputThisFrame; }

    static void RegisterLua(sol::state &lua);

private:
    InputManager() = default;

    std::unordered_set<int> m_KeysDown;
    std::unordered_set<int> m_KeysPressed;
    std::unordered_set<int> m_KeysReleased;

    std::unordered_set<int> m_MouseDown;
    std::unordered_set<int> m_MousePressed;
    std::unordered_set<int> m_MouseReleased;

    sf::Vector2i m_MousePosition;
    float m_ScrollDelta = 0.f;
    std::vector<sf::Uint32> m_TextEntered;
    bool m_HasInputThisFrame = false;
};

#endif
