#include "InputManager.h"

#include "sol/state.hpp"

void InputManager::HandleEvent(const sf::Event &event)
{
    if (event.type == sf::Event::TextEntered)
    {
        m_TextEntered.push_back(event.text.unicode);
        m_HasInputThisFrame = true;
    }

    if (event.type == sf::Event::KeyPressed)
    {
        const int key = static_cast<int>(event.key.code);
        if (!m_KeysDown.count(key))
            m_KeysPressed.insert(key);
        m_KeysDown.insert(key);
        m_HasInputThisFrame = true;
    }

    if (event.type == sf::Event::KeyReleased)
    {
        const int key = static_cast<int>(event.key.code);
        m_KeysDown.erase(key);
        m_KeysReleased.insert(key);
        m_HasInputThisFrame = true;
    }

    if (event.type == sf::Event::MouseButtonPressed)
    {
        const int btn = static_cast<int>(event.mouseButton.button);
        if (!m_MouseDown.count(btn))
            m_MousePressed.insert(btn);
        m_MouseDown.insert(btn);
        m_HasInputThisFrame = true;
    }

    if (event.type == sf::Event::MouseButtonReleased)
    {
        const int btn = static_cast<int>(event.mouseButton.button);
        m_MouseDown.erase(btn);
        m_MouseReleased.insert(btn);
        m_HasInputThisFrame = true;
    }

    if (event.type == sf::Event::MouseMoved)
    {
        m_MousePosition = {event.mouseMove.x, event.mouseMove.y};
        m_HasInputThisFrame = true;
    }

    if (event.type == sf::Event::MouseWheelScrolled)
    {
        m_ScrollDelta = event.mouseWheelScroll.delta;
        m_HasInputThisFrame = true;
    }

    if (event.type == sf::Event::JoystickButtonPressed ||
        event.type == sf::Event::JoystickButtonReleased ||
        event.type == sf::Event::JoystickMoved) { m_HasInputThisFrame = true; }
}

void InputManager::EndFrame()
{
    m_TextEntered.clear();
    m_KeysPressed.clear();
    m_KeysReleased.clear();
    m_MousePressed.clear();
    m_MouseReleased.clear();
    m_ScrollDelta = 0.f;
    m_HasInputThisFrame = false;
}

bool InputManager::IsKeyDown(sf::Keyboard::Key key) { return Get().m_KeysDown.count(static_cast<int>(key)) > 0; }

bool InputManager::IsKeyPressed(sf::Keyboard::Key key) { return Get().m_KeysPressed.count(static_cast<int>(key)) > 0; }

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

sf::Vector2i InputManager::MousePosition() { return Get().m_MousePosition; }

float InputManager::MouseScrollDelta() { return Get().m_ScrollDelta; }

sf::Keyboard::Key InputManager::StringToKey(std::string keyName)
{
    for (char &c: keyName) c = static_cast<char>(std::tolower(c));

    if (keyName.length() == 1)
    {
        char c = keyName[0];
        if (c >= 'a' && c <= 'z')
            return static_cast<sf::Keyboard::Key>(sf::Keyboard::A + (c - 'a'));
        if (c >= '0' && c <= '9')
            return static_cast<sf::Keyboard::Key>(sf::Keyboard::Num0 + (c - '0'));
    }

    if (keyName == "space") return sf::Keyboard::Space;
    if (keyName == "enter" || keyName == "return") return sf::Keyboard::Enter;
    if (keyName == "escape" || keyName == "esc") return sf::Keyboard::Escape;
    if (keyName == "shift" || keyName == "lshift") return sf::Keyboard::LShift;
    if (keyName == "rshift") return sf::Keyboard::RShift;
    if (keyName == "ctrl" || keyName == "lctrl") return sf::Keyboard::LControl;
    if (keyName == "rctrl") return sf::Keyboard::RControl;
    if (keyName == "alt" || keyName == "lalt") return sf::Keyboard::LAlt;
    if (keyName == "ralt") return sf::Keyboard::RAlt;
    if (keyName == "left") return sf::Keyboard::Left;
    if (keyName == "right") return sf::Keyboard::Right;
    if (keyName == "up") return sf::Keyboard::Up;
    if (keyName == "down") return sf::Keyboard::Down;
    if (keyName == "tab") return sf::Keyboard::Tab;
    if (keyName == "delete" || keyName == "del") return sf::Keyboard::Delete;
    if (keyName == "backspace") return sf::Keyboard::BackSpace;

    return sf::Keyboard::Unknown;
}

std::string InputManager::KeyToString(sf::Keyboard::Key key)
{
    if (key >= sf::Keyboard::A && key <= sf::Keyboard::Z)
        return std::string(1, static_cast<char>('A' + (key - sf::Keyboard::A)));
    if (key >= sf::Keyboard::Num0 && key <= sf::Keyboard::Num9)
        return std::string(1, static_cast<char>('0' + (key - sf::Keyboard::Num0)));
    if (key >= sf::Keyboard::Numpad0 && key <= sf::Keyboard::Numpad9)
        return "Numpad" + std::to_string(key - sf::Keyboard::Numpad0);
    if (key >= sf::Keyboard::F1 && key <= sf::Keyboard::F15)
        return "F" + std::to_string(1 + (key - sf::Keyboard::F1));

    switch (key)
    {
        case sf::Keyboard::Space: return "Space";
        case sf::Keyboard::Enter: return "Enter";
        case sf::Keyboard::Escape: return "Escape";
        case sf::Keyboard::LShift: return "LShift";
        case sf::Keyboard::RShift: return "RShift";
        case sf::Keyboard::LControl: return "LCtrl";
        case sf::Keyboard::RControl: return "RCtrl";
        case sf::Keyboard::LAlt: return "LAlt";
        case sf::Keyboard::RAlt: return "RAlt";
        case sf::Keyboard::LSystem: return "LSystem";
        case sf::Keyboard::RSystem: return "RSystem";
        case sf::Keyboard::Menu: return "Menu";
        case sf::Keyboard::Left: return "Left";
        case sf::Keyboard::Right: return "Right";
        case sf::Keyboard::Up: return "Up";
        case sf::Keyboard::Down: return "Down";
        case sf::Keyboard::Tab: return "Tab";
        case sf::Keyboard::Delete: return "Delete";
        case sf::Keyboard::BackSpace: return "Backspace";
        case sf::Keyboard::PageUp: return "PageUp";
        case sf::Keyboard::PageDown: return "PageDown";
        case sf::Keyboard::End: return "End";
        case sf::Keyboard::Home: return "Home";
        case sf::Keyboard::Insert: return "Insert";
        case sf::Keyboard::Add: return "+";
        case sf::Keyboard::Subtract: return "-";
        case sf::Keyboard::Multiply: return "*";
        case sf::Keyboard::Divide: return "/";
        case sf::Keyboard::Pause: return "Pause";
        default: return "Unknown";
    }
}

sf::Mouse::Button InputManager::StringToMouseButton(std::string btnName)
{
    for (char &c: btnName) c = static_cast<char>(std::tolower(c));
    if (btnName == "left" || btnName == "0") return sf::Mouse::Left;
    if (btnName == "right" || btnName == "1") return sf::Mouse::Right;
    if (btnName == "middle" || btnName == "2") return sf::Mouse::Middle;
    if (btnName == "xbutton1" || btnName == "3") return sf::Mouse::XButton1;
    if (btnName == "xbutton2" || btnName == "4") return sf::Mouse::XButton2;
    return sf::Mouse::Left;
}

std::string InputManager::MouseButtonToString(sf::Mouse::Button button)
{
    switch (button)
    {
        case sf::Mouse::Left: return "Left";
        case sf::Mouse::Right: return "Right";
        case sf::Mouse::Middle: return "Middle";
        case sf::Mouse::XButton1: return "XButton1";
        case sf::Mouse::XButton2: return "XButton2";
        default: return "Unknown";
    }
}

void InputManager::RegisterLua(sol::state &lua)
{
    auto input = lua.create_named_table("Input");

    input.set_function("HasInput", [] { return InputManager::Get().HasInputThisFrame(); });
    input.set_function("KeyToString", [](int k) { return KeyToString(static_cast<sf::Keyboard::Key>(k)); });
    input.set_function("MouseButtonToString", [](int b) {
        return MouseButtonToString(static_cast<sf::Mouse::Button>(b));
    });
    input.set_function("GetLastEvent", [&lua]() -> sol::object {
        sol::object last = lua["__last_input_event"];
        if (last.valid()) return last;
        return sol::nil;
    });

    input.set_function("IsKeyDown", sol::overload(
                           [](int k) { return IsKeyDown(static_cast<sf::Keyboard::Key>(k)); },
                           [](const std::string &name) { return IsKeyDown(StringToKey(name)); }
                       ));

    input.set_function("IsKeyPressed", sol::overload(
                           [](int k) { return IsKeyPressed(static_cast<sf::Keyboard::Key>(k)); },
                           [](const std::string &name) { return IsKeyPressed(StringToKey(name)); }
                       ));

    input.set_function("IsKeyJustPressed", sol::overload(
                           [](int k) { return IsKeyPressed(static_cast<sf::Keyboard::Key>(k)); },
                           [](const std::string &name) { return IsKeyPressed(StringToKey(name)); }
                       ));

    input.set_function("IsKeyReleased", sol::overload(
                           [](int k) { return IsKeyReleased(static_cast<sf::Keyboard::Key>(k)); },
                           [](const std::string &name) { return IsKeyReleased(StringToKey(name)); }
                       ));

    input.set_function("IsMouseDown", sol::overload(
                           [](int b) { return IsMouseDown(static_cast<sf::Mouse::Button>(b)); },
                           [](const std::string &name) { return IsMouseDown(StringToMouseButton(name)); }
                       ));

    input.set_function("IsMousePressed", sol::overload(
                           [](int b) { return IsMousePressed(static_cast<sf::Mouse::Button>(b)); },
                           [](const std::string &name) { return IsMousePressed(StringToMouseButton(name)); }
                       ));

    input.set_function("IsMouseReleased", sol::overload(
                           [](int b) { return IsMouseReleased(static_cast<sf::Mouse::Button>(b)); },
                           [](const std::string &name) { return IsMouseReleased(StringToMouseButton(name)); }
                       ));

    input.set_function("MouseX", [] { return MousePosition().x; });
    input.set_function("MouseY", [] { return MousePosition().y; });
    input.set_function("MouseScroll", [] { return MouseScrollDelta(); });

    auto keys = lua.create_named_table("Key");
    keys["A"] = static_cast<int>(sf::Keyboard::A);
    keys["B"] = static_cast<int>(sf::Keyboard::B);
    keys["C"] = static_cast<int>(sf::Keyboard::C);
    keys["D"] = static_cast<int>(sf::Keyboard::D);
    keys["E"] = static_cast<int>(sf::Keyboard::E);
    keys["F"] = static_cast<int>(sf::Keyboard::F);
    keys["G"] = static_cast<int>(sf::Keyboard::G);
    keys["H"] = static_cast<int>(sf::Keyboard::H);
    keys["I"] = static_cast<int>(sf::Keyboard::I);
    keys["J"] = static_cast<int>(sf::Keyboard::J);
    keys["K"] = static_cast<int>(sf::Keyboard::K);
    keys["L"] = static_cast<int>(sf::Keyboard::L);
    keys["M"] = static_cast<int>(sf::Keyboard::M);
    keys["N"] = static_cast<int>(sf::Keyboard::N);
    keys["O"] = static_cast<int>(sf::Keyboard::O);
    keys["P"] = static_cast<int>(sf::Keyboard::P);
    keys["Q"] = static_cast<int>(sf::Keyboard::Q);
    keys["R"] = static_cast<int>(sf::Keyboard::R);
    keys["S"] = static_cast<int>(sf::Keyboard::S);
    keys["T"] = static_cast<int>(sf::Keyboard::T);
    keys["U"] = static_cast<int>(sf::Keyboard::U);
    keys["V"] = static_cast<int>(sf::Keyboard::V);
    keys["W"] = static_cast<int>(sf::Keyboard::W);
    keys["X"] = static_cast<int>(sf::Keyboard::X);
    keys["Y"] = static_cast<int>(sf::Keyboard::Y);
    keys["Z"] = static_cast<int>(sf::Keyboard::Z);
    keys["Space"] = static_cast<int>(sf::Keyboard::Space);
    keys["Enter"] = static_cast<int>(sf::Keyboard::Enter);
    keys["Escape"] = static_cast<int>(sf::Keyboard::Escape);
    keys["LShift"] = static_cast<int>(sf::Keyboard::LShift);
    keys["RShift"] = static_cast<int>(sf::Keyboard::RShift);
    keys["LCtrl"] = static_cast<int>(sf::Keyboard::LControl);
    keys["RCtrl"] = static_cast<int>(sf::Keyboard::RControl);
    keys["Left"] = static_cast<int>(sf::Keyboard::Left);
    keys["Right"] = static_cast<int>(sf::Keyboard::Right);
    keys["Up"] = static_cast<int>(sf::Keyboard::Up);
    keys["Down"] = static_cast<int>(sf::Keyboard::Down);
    keys["Tab"] = static_cast<int>(sf::Keyboard::Tab);
    keys["Delete"] = static_cast<int>(sf::Keyboard::Delete);

    lua["KeyCode"] = keys;

    auto mouse = lua.create_named_table("Mouse");
    mouse["Left"] = static_cast<int>(sf::Mouse::Left);
    mouse["Right"] = static_cast<int>(sf::Mouse::Right);
    mouse["Middle"] = static_cast<int>(sf::Mouse::Middle);

    auto inputEvent = lua.create_named_table("InputEvent");
    inputEvent["Unknown"] = static_cast<int>(InputEventType::Unknown);
    inputEvent["KeyDown"] = static_cast<int>(InputEventType::KeyDown);
    inputEvent["Keydown"] = static_cast<int>(InputEventType::KeyDown);
    inputEvent["KeyPressed"] = static_cast<int>(InputEventType::KeyDown);

    inputEvent["KeyUp"] = static_cast<int>(InputEventType::KeyUp);
    inputEvent["Keyup"] = static_cast<int>(InputEventType::KeyUp);
    inputEvent["KeyReleased"] = static_cast<int>(InputEventType::KeyUp);

    inputEvent["MouseDown"] = static_cast<int>(InputEventType::MouseDown);
    inputEvent["Mousedown"] = static_cast<int>(InputEventType::MouseDown);
    inputEvent["MousePressed"] = static_cast<int>(InputEventType::MouseDown);

    inputEvent["MouseUp"] = static_cast<int>(InputEventType::MouseUp);
    inputEvent["Mouseup"] = static_cast<int>(InputEventType::MouseUp);
    inputEvent["MouseReleased"] = static_cast<int>(InputEventType::MouseUp);

    inputEvent["MouseMove"] = static_cast<int>(InputEventType::MouseMove);
    inputEvent["Mousemove"] = static_cast<int>(InputEventType::MouseMove);
    inputEvent["MouseMoved"] = static_cast<int>(InputEventType::MouseMove);

    inputEvent["MouseWheel"] = static_cast<int>(InputEventType::MouseWheel);
    inputEvent["Mousewheel"] = static_cast<int>(InputEventType::MouseWheel);
    inputEvent["MouseScroll"] = static_cast<int>(InputEventType::MouseWheel);

    inputEvent["TextEntered"] = static_cast<int>(InputEventType::TextEntered);
    inputEvent["Text"] = static_cast<int>(InputEventType::TextEntered);

    inputEvent["JoystickPressed"] = static_cast<int>(InputEventType::JoystickPressed);
    inputEvent["JoystickReleased"] = static_cast<int>(InputEventType::JoystickReleased);
    inputEvent["JoystickMoved"] = static_cast<int>(InputEventType::JoystickMoved);
    inputEvent[static_cast<int>(InputEventType::KeyDown)] = "KeyDown";
    inputEvent[static_cast<int>(InputEventType::KeyUp)] = "KeyUp";
    inputEvent[static_cast<int>(InputEventType::MouseDown)] = "MouseDown";
    inputEvent[static_cast<int>(InputEventType::MouseUp)] = "MouseUp";
    inputEvent[static_cast<int>(InputEventType::MouseMove)] = "MouseMove";
    inputEvent[static_cast<int>(InputEventType::MouseWheel)] = "MouseWheel";
    inputEvent[static_cast<int>(InputEventType::TextEntered)] = "TextEntered";
    inputEvent[static_cast<int>(InputEventType::JoystickPressed)] = "JoystickPressed";
    inputEvent[static_cast<int>(InputEventType::JoystickReleased)] = "JoystickReleased";
    inputEvent[static_cast<int>(InputEventType::JoystickMoved)] = "JoystickMoved";

    lua["InputEventType"] = inputEvent;
    lua["EventType"] = inputEvent;
}
