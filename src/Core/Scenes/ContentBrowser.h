#ifndef CONTENTBROWSER_H
#define CONTENTBROWSER_H

#include <string>
#include <vector>
#include <filesystem>

#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/Text.hpp"
#include "SFML/Graphics/Font.hpp"

namespace fs = std::filesystem;

enum class AssetType { Folder, Script, Scene, Image, Unknown };

struct ContentEntry
{
    std::string name;
    std::string fullPath;
    AssetType type;
    bool isDirectory;
};

struct DraggedAsset
{
    std::string path;
    AssetType type;
    bool active = false;
    sf::Vector2f pos;
};

class ContentBrowser
{
public:
    ContentBrowser(const sf::Font& font, const std::string& rootPath);

    void HandleEvent(const sf::Event& event, sf::Vector2f mouseScreenPos);
    void Render(sf::RenderWindow& window, float x, float y, float width, float height);

    bool HasDraggedAsset() const { return m_Drag.active; }
    DraggedAsset GetDraggedAsset() const { return m_Drag; }
    void ClearDrag() { m_Drag.active = false; }

    void Refresh();

private:
    const sf::Font& m_Font;
    std::string m_RootPath;
    std::string m_CurrentPath;
    std::vector<ContentEntry> m_Entries;

    DraggedAsset m_Drag;
    sf::Vector2f m_MousePos;

    float m_ScrollOffset = 0.f;
    double m_LastClickTime = 0.0;
    std::string m_LastClickedPath;

    std::vector<std::pair<sf::FloatRect, size_t>> m_ItemBounds;
    sf::FloatRect m_RefreshBtnBounds;

    static AssetType TypeFromExtension(const std::string& ext);
    static sf::Color ColorForType(AssetType type);
    static std::string LabelForType(AssetType type);

    void NavigateTo(const std::string& path);
    void OpenEntry(const ContentEntry& entry);
};

#endif