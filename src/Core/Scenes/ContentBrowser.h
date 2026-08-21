#ifndef CONTENTBROWSER_H
#define CONTENTBROWSER_H

#include <string>
#include <vector>
#include <filesystem>
#include <memory>

#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/CircleShape.hpp"
#include "SFML/Graphics/Text.hpp"
#include "SFML/Graphics/Font.hpp"
#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/Texture.hpp"

namespace fs = std::filesystem;

enum class AssetType
{
    Folder,
    Script,
    Scene,
    Image,
    Audio,
    Font,
    Unknown
};

enum class AssetFilter
{
    All,
    Images,
    Scripts,
    Audio,
    Scenes
};

struct ContentEntry
{
    std::string name;
    std::string fullPath;
    AssetType type;
    bool isDirectory;
    uintmax_t fileSize = 0;
};

struct DraggedAsset
{
    std::string path;
    AssetType type;
    bool active = false;
    sf::Vector2f pos;
};

struct BreadcrumbSegment
{
    std::string name;
    std::string fullPath;
    sf::FloatRect bounds;
};

struct ContextMenuItem
{
    std::string label;
    std::string action;
    sf::FloatRect bounds;
};

class ContentBrowser
{
public:
    ContentBrowser(const sf::Font &font, const std::string &rootPath);

    void HandleEvent(const sf::Event &event, sf::Vector2f mouseScreenPos);

    void Render(sf::RenderWindow &window, float x, float y, float width, float height);

    bool HasDraggedAsset() const { return m_Drag.active; }
    DraggedAsset GetDraggedAsset() const { return m_Drag; }
    void ClearDrag() { m_Drag.active = false; }

    void Refresh();

    bool IsInputActive() const { return m_SearchActive || m_NewScriptPrompt; }

private:
    const sf::Font &m_Font;
    std::string m_RootPath;
    std::string m_CurrentPath;
    std::vector<ContentEntry> m_Entries;
    std::vector<ContentEntry> m_FilteredEntries;

    DraggedAsset m_Drag;
    sf::Vector2f m_MousePos;

    float m_ScrollOffset = 0.f;
    float m_MaxScroll = 0.f;
    double m_LastClickTime = 0.0;
    std::string m_LastClickedPath;
    std::string m_SelectedPath;

    AssetFilter m_CurrentFilter = AssetFilter::All;
    std::string m_SearchQuery;
    bool m_SearchActive = false;

    std::vector<std::pair<sf::FloatRect, size_t> > m_ItemBounds;
    std::vector<BreadcrumbSegment> m_Breadcrumbs;
    std::vector<std::pair<sf::FloatRect, AssetFilter> > m_FilterBounds;
    sf::FloatRect m_UpBtnBounds;
    sf::FloatRect m_RefreshBtnBounds;
    sf::FloatRect m_NewScriptBtnBounds;
    sf::FloatRect m_SearchBoxBounds;
    sf::FloatRect m_SearchClearBounds;

    bool m_ContextMenuOpen = false;
    sf::Vector2f m_ContextMenuPos;
    std::string m_ContextMenuTarget;
    std::vector<ContextMenuItem> m_ContextMenuItems;

    bool m_NewScriptPrompt = false;
    std::string m_NewScriptName;

    static AssetType TypeFromExtension(const std::string &ext);

    static sf::Color ColorForType(AssetType type);

    static std::string LabelForType(AssetType type);

    static std::string FormatFileSize(uintmax_t bytes);

    void UpdateFilteredEntries();

    void NavigateTo(const std::string &path);

    void OpenEntry(const ContentEntry &entry);

    void CreateNewScript(const std::string &name);

    void RevealInExplorer(const std::string &path);

    void DeleteAsset(const std::string &path);

    void HandleContextMenuAction(const std::string &action);
};

#endif // CONTENTBROWSER_H
