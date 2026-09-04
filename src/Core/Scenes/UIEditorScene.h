#ifndef UIEDITORSCENE_H
#define UIEDITORSCENE_H

#include <vector>
#include <string>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/View.hpp>

#include "../Scenes/Scene.h"
#include "../Scenes/SceneManager.h"
#include "../UI/UIManager.h"

class UIEditorScene : public Scene
{
public:
    UIEditorScene(SceneManager &manager, sf::RenderWindow &window);

    void HandleEvent(const sf::Event &event) override;
    void Update(float deltaTime) override;
    void Render(sf::RenderWindow &window) override;
    void OnEnter() override;
    void OnExit() override;

private:
    sf::RenderWindow &m_Window;
    
    std::shared_ptr<sf::Font> m_Font;
    
    // UI Panels Layout
    static constexpr float ToolbarHeight = 34.f;
    static constexpr float PaletteWidth = 200.f;
    static constexpr float InspectorWidth = 270.f;
    static constexpr float HierarchyHeight = 350.f;

    sf::FloatRect m_CanvasBounds;
    sf::FloatRect m_PaletteBounds;
    sf::FloatRect m_InspectorBounds;
    sf::FloatRect m_HierarchyBounds;
    
    // Virtual Canvas view
    sf::View m_CanvasView;
    sf::Vector2f m_CanvasSize = {1920.f, 1080.f};
    bool m_Panning = false;
    sf::Vector2f m_PanStart;
    bool m_ViewInitialized = false;
    
    // Input / Selection
    sf::Vector2f m_MouseScreenPos;
    sf::Vector2f m_MouseCanvasPos;
    
    UIElement* m_SelectedElement = nullptr;
    bool m_Dragging = false;
    sf::Vector2f m_DragOffset;
    
    // Resizing
    bool m_Resizing = false;
    int m_ResizeHandle = -1;
    sf::Vector2f m_ResizeMouseStart;
    sf::Vector2f m_ResizeObjOrigin;
    sf::Vector2f m_ResizeObjSize;
    
    // Editing
    enum class EditField
    {
        None,
        Id,
        TransformX,
        TransformY,
        ZIndex,
        SizeW,
        SizeH,
        ColorR,
        ColorG,
        ColorB,
        UIText
    };

    EditField m_ActiveField = EditField::None;
    std::string m_ActiveInputText;
    
    // Hitboxes
    struct ButtonHitbox { sf::FloatRect bounds; std::string action; };
    std::vector<ButtonHitbox> m_PaletteHitboxes;
    std::vector<ButtonHitbox> m_InspectorHitboxes;
    std::vector<ButtonHitbox> m_ToolbarHitboxes;
    std::vector<std::pair<sf::FloatRect, UIElement*>> m_HierarchyHitboxes;

    void UpdateBounds();
    
    // Rendering parts
    void DrawToolbar(sf::RenderWindow &window);
    void DrawPalette(sf::RenderWindow &window);
    void DrawHierarchy(sf::RenderWindow &window);
    void DrawInspector(sf::RenderWindow &window);
    void DrawCanvas(sf::RenderWindow &window);
    void DrawResizeHandles(sf::RenderWindow &window);
    
    // UI Helpers
    float DrawSectionHeader(sf::RenderWindow &window, const std::string &title, sf::Color accent, float x, float y);
    float DrawRow(sf::RenderWindow &window, const std::string &key, const std::string &val, float x, float y);
    float DrawEditableRow(sf::RenderWindow &window, const std::string &key, const std::string &val,
                          const std::string &action, float x, float y);
    float DrawActionButton(sf::RenderWindow &window, const std::string &label, const std::string &action, float x,
                           float y, sf::Color fillColor, sf::Color borderColor);
    void DrawPill(sf::RenderWindow &window, const sf::FloatRect &r, sf::Color fill, sf::Color outline);
    
    // Interaction
    void HandleAction(const std::string &action);
    int GetResizeHandle(sf::Vector2f canvasPos) const;
    void DeleteSelected();
    
    std::string NextId(UIElementType type);
};

#endif
