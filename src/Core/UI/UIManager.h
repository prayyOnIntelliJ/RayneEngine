#ifndef RAYNEENGINE_UIMANAGER_H
#define RAYNEENGINE_UIMANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <SFML/Graphics.hpp>

enum class UIElementType { Text, Panel, Button };

enum class TextAlign { Left, Center, Right };

struct UIElement
{
    std::string id;
    UIElementType type;
    sf::Vector2f position;
    sf::Vector2f size;
    sf::Color color;
    int zIndex = 0;

    std::string text;
    unsigned int characterSize = 16;
    sf::Color textColor = sf::Color::White;

    sf::Text::Style textStyle = sf::Text::Regular;
    TextAlign textAlign = TextAlign::Left;
    bool textUpperCase = false;
    float letterSpacing = 1.0f;
    float lineSpacing = 1.0f;
    sf::Color textOutlineColor = sf::Color::Transparent;
    float textOutlineThickness = 0.f;
    sf::Vector2f textOffset = {0.f, 0.f};

    sf::Color outlineColor = sf::Color::Transparent;
    float outlineThickness = 0.f;
    float cornerRadius = 0.f;
    float opacity = 255.f;
    bool visible = true;

    sf::Color normalColor = sf::Color(100, 100, 100);
    sf::Color hoverColor = sf::Color(150, 150, 150);
    sf::Color pressedColor = sf::Color(80, 80, 80);
    sf::Color borderColor = sf::Color::Transparent;
    float borderThickness = 0.f;
    bool disabled = false;
    sf::Color disabledColor = sf::Color(60, 60, 60, 180);
    bool isHovered = false;
    bool isPressed = false;

    std::string onClickAction = "";
    std::string onClickParam = "";
    std::string onHoverAction = "";
    std::string onHoverParam = "";

    sf::RectangleShape shape;
    sf::Text drawableText;
    std::shared_ptr<sf::Font> font;

    void UpdateDrawables();
};

class UIManager
{
public:
    static UIManager &Get()
    {
        static UIManager instance;
        return instance;
    }

    void Init(std::shared_ptr<sf::Font> defaultFont);

    void Update(float dt, sf::Vector2f mousePos, bool mouseClicked, bool mouseReleased);

    void Render(sf::RenderWindow &window);

    void Save(const std::string &path);

    void Load(const std::string &path);

    UIElement *CreateElement(const std::string &id, UIElementType type);

    void RemoveElement(const std::string &id);

    UIElement *GetElement(const std::string &id);

    std::vector<UIElement> &GetElements() { return m_Elements; }

    void SetText(const std::string &id, const std::string &text);

    std::string GetText(const std::string &id);

    void SetPosition(const std::string &id, float x, float y);

    void SetSize(const std::string &id, float w, float h);

    void SetColor(const std::string &id, int r, int g, int b, int a = 255);

    void SetZIndex(const std::string &id, int z);

    int GetZIndex(const std::string &id);

    bool IsButtonClicked(const std::string &id);

    void ClearClickedButton() { m_LastClickedButton.clear(); }

    void SetCurrentUIPath(const std::string& path) { m_CurrentUIPath = path; }
    const std::string& GetCurrentUIPath() const { return m_CurrentUIPath; }

    void SetVisible(const std::string &id, bool visible);
    bool GetVisible(const std::string &id);

    void SetOpacity(const std::string &id, float opacity);
    float GetOpacity(const std::string &id);

    void SetTextStyle(const std::string &id, int style);
    void SetTextAlign(const std::string &id, int align);

    void SetUpperCase(const std::string &id, bool upper);
    bool GetUpperCase(const std::string &id);

    void SetFontSize(const std::string &id, int size);

    void SetLetterSpacing(const std::string &id, float spacing);
    void SetLineSpacing(const std::string &id, float spacing);

    void SetTextOutline(const std::string &id, int r, int g, int b, int a, float thickness);
    void SetTextOffset(const std::string &id, float ox, float oy);
    void SetTextColor(const std::string &id, int r, int g, int b, int a);

    void SetOutline(const std::string &id, int r, int g, int b, int a, float thickness);

    void SetDisabled(const std::string &id, bool disabled);

    bool IsButtonHovered(const std::string &id);

private:
    UIManager() = default;

    std::vector<UIElement> m_Elements;
    std::shared_ptr<sf::Font> m_DefaultFont;
    std::string m_LastClickedButton;
    std::string m_CurrentUIPath;
};

#endif
