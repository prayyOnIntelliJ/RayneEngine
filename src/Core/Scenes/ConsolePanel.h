#ifndef CONSOLEPANEL_H
#define CONSOLEPANEL_H

#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <streambuf>
#include <iostream>
#include <functional>

#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/Text.hpp"
#include "SFML/Graphics/Font.hpp"
#include "SFML/Window/Event.hpp"

class ConsoleRedirector : public std::streambuf
{
public:
    ConsoleRedirector(std::ostream& stream, std::function<void(const std::string&, bool)> callback, bool isError);
    ~ConsoleRedirector();

protected:
    virtual int_type overflow(int_type v) override;
    virtual std::streamsize xsputn(const char* p, std::streamsize n) override;

private:
    std::ostream& m_Stream;
    std::streambuf* m_OldBuf;
    std::function<void(const std::string&, bool)> m_Callback;
    bool m_IsError;
    std::string m_Buffer;
};

struct ConsoleMessage
{
    std::string text;
    bool isError;
};

class ConsolePanel
{
public:
    static void InitRedirectors();
    static void AddLogGlobal(const std::string& message, bool isError);

    ConsolePanel(const sf::Font& font);
    ~ConsolePanel();

    void HandleEvent(const sf::Event& event, sf::Vector2f mouseScreenPos);
    void Render(sf::RenderWindow& window, float x, float y, float width, float height);

    bool IsInputActive() const { return m_InputActive; }

private:
    const sf::Font& m_Font;
    
    float m_ScrollOffset = 0.f;
    float m_MaxScroll = 0.f;
    bool m_AutoScroll = true;
    size_t m_LastMessageCount = 0;

    bool m_ScrollbarDragging = false;
    float m_DragStartY = 0.f;
    float m_DragStartScroll = 0.f;
    sf::FloatRect m_ScrollbarBounds;

    std::string m_InputBuffer;
    bool m_InputActive = false;
    sf::FloatRect m_InputBounds;
    sf::FloatRect m_Bounds;
    
    void ExecuteCommand(const std::string& command);
    
    static std::vector<ConsoleMessage> s_Messages;
    static std::mutex s_Mutex;
    static std::unique_ptr<ConsoleRedirector> s_CoutRedirector;
    static std::unique_ptr<ConsoleRedirector> s_CerrRedirector;
    static bool s_Initialized;
};

#endif // CONSOLEPANEL_H
