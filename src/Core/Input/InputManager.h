#ifndef RAYNEENGINE_INPUTMANAGER_H
#define RAYNEENGINE_INPUTMANAGER_H

#include <unordered_set>
#include "SFML/Window/Event.hpp"
#include "SFML/Window/Keyboard.hpp"
#include "SFML/Window/Mouse.hpp"
#include "SFML/System/Vector2.hpp"
#include "sol/state.hpp"

class InputManager
{
public:
    static InputManager& Get()
    {
        static InputManager instance;
        return instance;
    }

    void HandleEvent(const sf::Event& event);
    void EndFrame();

    static bool IsKeyDown(sf::Keyboard::Key key);
    static bool IsKeyPressed(sf::Keyboard::Key key);
    static bool IsKeyReleased(sf::Keyboard::Key key);

    static bool IsMouseDown(sf::Mouse::Button button);
    static bool IsMousePressed(sf::Mouse::Button button);
    static bool IsMouseReleased(sf::Mouse::Button button);

    static sf::Vector2i MousePosition();
    static float MouseScrollDelta();

    static void RegisterLua(sol::state& lua);

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
};

#endif