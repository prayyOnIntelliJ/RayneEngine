#ifndef RAYNEENGINE_UIMANAGER_H
#define RAYNEENGINE_UIMANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <SFML/Graphics.hpp>

enum class UIElementType { Text, Panel, Button };

struct UIElement
{
    std::string id;
    UIElementType type;
    sf::Vector2f position;
    sf::Vector2f size;
    sf::Color color;
    int zIndex = 0;
    
    // Text properties
    std::string text;
    unsigned int characterSize = 16;
    sf::Color textColor = sf::Color::White;
    
    // Button properties
    sf::Color normalColor = sf::Color(100, 100, 100);
    sf::Color hoverColor = sf::Color(150, 150, 150);
    sf::Color pressedColor = sf::Color(80, 80, 80);
    bool isHovered = false;
    bool isPressed = false;
    
    // Internal rendering
    sf::RectangleShape shape;
    sf::Text drawableText;
    std::shared_ptr<sf::Font> font;
    
    void UpdateDrawables();
};

class UIManager
{
public:
    static UIManager& Get()
    {
        static UIManager instance;
        return instance;
    }

    void Init(std::shared_ptr<sf::Font> defaultFont);
    void Update(float dt, sf::Vector2f mousePos, bool mouseClicked, bool mouseReleased);
    void Render(sf::RenderWindow& window);
    
    void Save(const std::string& path);
    void Load(const std::string& path);
    
    UIElement* CreateElement(const std::string& id, UIElementType type);
    void RemoveElement(const std::string& id);
    UIElement* GetElement(const std::string& id);
    std::vector<UIElement>& GetElements() { return m_Elements; }
    
    // Lua API
    void SetText(const std::string& id, const std::string& text);
    std::string GetText(const std::string& id);
    void SetPosition(const std::string& id, float x, float y);
    void SetSize(const std::string& id, float w, float h);
    void SetColor(const std::string& id, int r, int g, int b, int a = 255);
    void SetZIndex(const std::string& id, int z);
    int GetZIndex(const std::string& id);
    bool IsButtonClicked(const std::string& id);
    
    void ClearClickedButton() { m_LastClickedButton.clear(); }

private:
    UIManager() = default;
    
    std::vector<UIElement> m_Elements;
    std::shared_ptr<sf::Font> m_DefaultFont;
    std::string m_LastClickedButton;
};

#endif
