#include "InputManager.h"

#include "sol/state.hpp"

void InputManager::HandleEvent(const sf::Event& event)
{
    if (event.type == sf::Event::KeyPressed)
    {
        const int key = static_cast<int>(event.key.code);
        if (!m_KeysDown.count(key))
            m_KeysPressed.insert(key);
        m_KeysDown.insert(key);
    }

    if (event.type == sf::Event::KeyReleased)
    {
        const int key = static_cast<int>(event.key.code);
        m_KeysDown.erase(key);
        m_KeysReleased.insert(key);
    }

    if (event.type == sf::Event::MouseButtonPressed)
    {
        const int btn = static_cast<int>(event.mouseButton.button);
        if (!m_MouseDown.count(btn))
            m_MousePressed.insert(btn);
        m_MouseDown.insert(btn);
    }

    if (event.type == sf::Event::MouseButtonReleased)
    {
        const int btn = static_cast<int>(event.mouseButton.button);
        m_MouseDown.erase(btn);
        m_MouseReleased.insert(btn);
    }

    if (event.type == sf::Event::MouseMoved)
        m_MousePosition = { event.mouseMove.x, event.mouseMove.y };

    if (event.type == sf::Event::MouseWheelScrolled)
        m_ScrollDelta = event.mouseWheelScroll.delta;
}

void InputManager::EndFrame()
{
    m_KeysPressed.clear();
    m_KeysReleased.clear();
    m_MousePressed.clear();
    m_MouseReleased.clear();
    m_ScrollDelta = 0.f;
}

bool InputManager::IsKeyDown(sf::Keyboard::Key key)
{
    return Get().m_KeysDown.count(static_cast<int>(key)) > 0;
}

bool InputManager::IsKeyPressed(sf::Keyboard::Key key)
{
    return Get().m_KeysPressed.count(static_cast<int>(key)) > 0;
}

bool InputManager::IsKeyReleased(sf::Keyboard::Key key)
{
    return Get().m_KeysReleased.count(static_cast<int>(key)) > 0;
}

bool InputManager::IsMouseDown(sf::Mouse::Button button)
{
    return Get().m_MouseDown.count(static_cast<int>(button)) > 0;
}

bool InputManager::IsMousePressed(sf::Mouse::Button button)
{
    return Get().m_MousePressed.count(static_cast<int>(button)) > 0;
}

bool InputManager::IsMouseReleased(sf::Mouse::Button button)
{
    return Get().m_MouseReleased.count(static_cast<int>(button)) > 0;
}

sf::Vector2i InputManager::MousePosition()
{
    return Get().m_MousePosition;
}

float InputManager::MouseScrollDelta()
{
    return Get().m_ScrollDelta;
}

void InputManager::RegisterLua(sol::state& lua)
{
    auto input = lua.create_named_table("Input");
    input.set_function("IsKeyDown", [](int k) { return IsKeyDown(static_cast<sf::Keyboard::Key>(k)); });
    input.set_function("IsKeyPressed",   [](int k) { return IsKeyPressed(static_cast<sf::Keyboard::Key>(k)); });
    input.set_function("IsKeyReleased",  [](int k) { return IsKeyReleased(static_cast<sf::Keyboard::Key>(k)); });
    input.set_function("IsMouseDown",    [](int b) { return IsMouseDown(static_cast<sf::Mouse::Button>(b)); });
    input.set_function("IsMousePressed", [](int b) { return IsMousePressed(static_cast<sf::Mouse::Button>(b)); });
    input.set_function("IsMouseReleased",[](int b) { return IsMouseReleased(static_cast<sf::Mouse::Button>(b)); });
    input.set_function("MouseX",         []() { return MousePosition().x; });
    input.set_function("MouseY",         []() { return MousePosition().y; });
    input.set_function("MouseScroll",    []() { return MouseScrollDelta(); });

    auto keys = lua.create_named_table("Key");
    keys["A"] = (int)sf::Keyboard::A; keys["B"] = (int)sf::Keyboard::B;
    keys["C"] = (int)sf::Keyboard::C; keys["D"] = (int)sf::Keyboard::D;
    keys["E"] = (int)sf::Keyboard::E; keys["F"] = (int)sf::Keyboard::F;
    keys["G"] = (int)sf::Keyboard::G; keys["H"] = (int)sf::Keyboard::H;
    keys["I"] = (int)sf::Keyboard::I; keys["J"] = (int)sf::Keyboard::J;
    keys["K"] = (int)sf::Keyboard::K; keys["L"] = (int)sf::Keyboard::L;
    keys["M"] = (int)sf::Keyboard::M; keys["N"] = (int)sf::Keyboard::N;
    keys["O"] = (int)sf::Keyboard::O; keys["P"] = (int)sf::Keyboard::P;
    keys["Q"] = (int)sf::Keyboard::Q; keys["R"] = (int)sf::Keyboard::R;
    keys["S"] = (int)sf::Keyboard::S; keys["T"] = (int)sf::Keyboard::T;
    keys["U"] = (int)sf::Keyboard::U; keys["V"] = (int)sf::Keyboard::V;
    keys["W"] = (int)sf::Keyboard::W; keys["X"] = (int)sf::Keyboard::X;
    keys["Y"] = (int)sf::Keyboard::Y; keys["Z"] = (int)sf::Keyboard::Z;
    keys["Space"]  = (int)sf::Keyboard::Space;
    keys["Enter"]  = (int)sf::Keyboard::Enter;
    keys["Escape"] = (int)sf::Keyboard::Escape;
    keys["LShift"] = (int)sf::Keyboard::LShift;
    keys["RShift"] = (int)sf::Keyboard::RShift;
    keys["LCtrl"]  = (int)sf::Keyboard::LControl;
    keys["RCtrl"]  = (int)sf::Keyboard::RControl;
    keys["Left"]   = (int)sf::Keyboard::Left;
    keys["Right"]  = (int)sf::Keyboard::Right;
    keys["Up"]     = (int)sf::Keyboard::Up;
    keys["Down"]   = (int)sf::Keyboard::Down;
    keys["Tab"]    = (int)sf::Keyboard::Tab;
    keys["Delete"] = (int)sf::Keyboard::Delete;

    auto mouse = lua.create_named_table("Mouse");
    mouse["Left"]   = (int)sf::Mouse::Left;
    mouse["Right"]  = (int)sf::Mouse::Right;
    mouse["Middle"] = (int)sf::Mouse::Middle;
}