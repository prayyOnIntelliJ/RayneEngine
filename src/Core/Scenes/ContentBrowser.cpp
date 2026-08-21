#include "ContentBrowser.h"

#include <algorithm>
#include <iostream>
#include <chrono>

#include "SFML/Window/Event.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#undef CreateWindow
#endif

static double CurrentTimeSeconds()
{
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

ContentBrowser::ContentBrowser(const sf::Font& font, const std::string& rootPath)
    : m_Font(font)
{
    fs::create_directories(rootPath);
    std::error_code ec;
    m_RootPath = fs::canonical(rootPath, ec).string();
    if (ec) m_RootPath = rootPath;
    m_CurrentPath = m_RootPath;
    Refresh();
}

void ContentBrowser::Refresh()
{
    m_Entries.clear();

    fs::path current(m_CurrentPath);
    fs::path root(m_RootPath);
    if (current != root && current.has_parent_path())
    {
        fs::path parent = current.parent_path();
        if (parent.string().find(m_RootPath) != std::string::npos || parent == root)
        {
            ContentEntry back;
            back.name = "..";
            back.fullPath = parent.string();
            back.type = AssetType::Folder;
            back.isDirectory = true;
            m_Entries.push_back(back);
        }
    }

    if (!fs::exists(m_CurrentPath)) return;

    std::vector<ContentEntry> dirs, files;

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(m_CurrentPath, ec))
    {
        if (ec) break;
        ContentEntry ce;
        ce.name = entry.path().filename().string();
        ce.fullPath = entry.path().string();
        ce.isDirectory = entry.is_directory(ec);
        if (ec) { ec.clear(); continue; }

        if (ce.isDirectory)
        {
            ce.type = AssetType::Folder;
            dirs.push_back(ce);
        }
        else
        {
            ce.type = TypeFromExtension(entry.path().extension().string());
            files.push_back(ce);
        }
    }

    auto sortByName = [](const ContentEntry& a, const ContentEntry& b) {
        return a.name < b.name;
    };
    std::sort(dirs.begin(), dirs.end(), sortByName);
    std::sort(files.begin(), files.end(), sortByName);

    for (auto& d : dirs) m_Entries.push_back(d);
    for (auto& f : files) m_Entries.push_back(f);
}

void ContentBrowser::HandleEvent(const sf::Event& event, sf::Vector2f mouseScreenPos)
{
    m_MousePos = mouseScreenPos;

    if (event.type == sf::Event::MouseWheelScrolled)
    {
        m_ScrollOffset -= event.mouseWheelScroll.delta * 20.f;
        if (m_ScrollOffset < 0.f) m_ScrollOffset = 0.f;
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        // Refresh button
        if (m_RefreshBtnBounds.contains(mouseScreenPos))
        {
            Refresh();
            return;
        }

        for (auto& [rect, idx] : m_ItemBounds)
        {
            if (!rect.contains(mouseScreenPos)) continue;
            const ContentEntry& entry = m_Entries[idx];

            double now = CurrentTimeSeconds();
            bool doubleClick = (m_LastClickedPath == entry.fullPath &&
                                now - m_LastClickTime < 0.4);
            m_LastClickTime = now;
            m_LastClickedPath = entry.fullPath;

            if (doubleClick)
            {
                OpenEntry(entry);
                m_Drag.active = false;
            }
            else if (entry.isDirectory)
            {
                NavigateTo(entry.fullPath);
            }
            else
            {
                m_Drag.active = true;
                m_Drag.path = entry.fullPath;
                m_Drag.type = entry.type;
                m_Drag.pos = mouseScreenPos;
            }
            break;
        }
    }
}

void ContentBrowser::Render(sf::RenderWindow& window, float x, float y, float width, float height)
{
    m_ItemBounds.clear();

    sf::RectangleShape bg({ width, height });
    bg.setPosition(x, y);
    bg.setFillColor(sf::Color(18, 18, 28, 250));
    bg.setOutlineColor(sf::Color(50, 50, 70));
    bg.setOutlineThickness(1.f);
    window.draw(bg);

    sf::RectangleShape header({ width, 24.f });
    header.setPosition(x, y);
    header.setFillColor(sf::Color(30, 30, 50, 255));
    window.draw(header);

    sf::Text headerText;
    headerText.setFont(m_Font);
    headerText.setCharacterSize(11);
    headerText.setFillColor(sf::Color(160, 160, 220));
    headerText.setStyle(sf::Text::Bold);
    std::string rel = m_CurrentPath;
    if (rel.find(m_RootPath) == 0)
        rel = "assets/" + rel.substr(m_RootPath.size());
    std::replace(rel.begin(), rel.end(), '\\', '/');
    headerText.setString("CONTENT  |  " + rel);
    headerText.setPosition(x + 8.f, y + 5.f);
    window.draw(headerText);

    m_RefreshBtnBounds = sf::FloatRect(x + width - 60.f, y + 2.f, 54.f, 20.f);
    bool refreshHovered = m_RefreshBtnBounds.contains(m_MousePos);
    sf::RectangleShape refreshShape({ m_RefreshBtnBounds.width, m_RefreshBtnBounds.height });
    refreshShape.setPosition(m_RefreshBtnBounds.left, m_RefreshBtnBounds.top);
    refreshShape.setFillColor(refreshHovered ? sf::Color(50, 70, 50) : sf::Color(35, 50, 35));
    window.draw(refreshShape);
    sf::Text refreshText;
    refreshText.setFont(m_Font);
    refreshText.setCharacterSize(10);
    refreshText.setFillColor(sf::Color(120, 200, 120));
    refreshText.setString("Refresh");
    refreshText.setPosition(m_RefreshBtnBounds.left + 6.f, m_RefreshBtnBounds.top + 4.f);
    window.draw(refreshText);

    const float itemW = 80.f;
    const float itemH = 70.f;
    const float pad = 10.f;
    float ix = x + pad;
    float iy = y + 28.f - m_ScrollOffset;

    for (size_t i = 0; i < m_Entries.size(); i++)
    {
        const auto& entry = m_Entries[i];
        if (iy + itemH < y + 24.f)
        {
            ix += itemW + pad;
            if (ix + itemW > x + width) { ix = x + pad; iy += itemH + pad; }
            continue;
        }
        if (iy > y + height) break;

        sf::FloatRect itemRect(ix, iy, itemW, itemH);
        m_ItemBounds.emplace_back(itemRect, i);
        bool hovered = itemRect.contains(m_MousePos);

        sf::RectangleShape itemBg({ itemW, itemH });
        itemBg.setPosition(ix, iy);
        itemBg.setFillColor(hovered ? sf::Color(40, 40, 65, 220) : sf::Color(28, 28, 42, 180));
        itemBg.setOutlineColor(hovered ? sf::Color(100, 100, 160) : sf::Color(45, 45, 65));
        itemBg.setOutlineThickness(1.f);
        window.draw(itemBg);

        sf::Color typeColor = ColorForType(entry.type);
        sf::RectangleShape icon({ 36.f, 30.f });
        icon.setPosition(ix + (itemW - 36.f) / 2.f, iy + 6.f);
        icon.setFillColor(sf::Color(typeColor.r, typeColor.g, typeColor.b, 60));
        icon.setOutlineColor(typeColor);
        icon.setOutlineThickness(1.5f);
        window.draw(icon);

        sf::Text typeLabel;
        typeLabel.setFont(m_Font);
        typeLabel.setCharacterSize(9);
        typeLabel.setFillColor(typeColor);
        typeLabel.setString(LabelForType(entry.type));
        typeLabel.setPosition(ix + (itemW - typeLabel.getLocalBounds().width) / 2.f, iy + 14.f);
        window.draw(typeLabel);

        sf::Text nameText;
        nameText.setFont(m_Font);
        nameText.setCharacterSize(10);
        nameText.setFillColor(sf::Color(200, 200, 210));
        std::string displayName = entry.name;
        if (displayName.size() > 10) displayName = displayName.substr(0, 9) + "~";
        nameText.setString(displayName);
        nameText.setPosition(ix + (itemW - nameText.getLocalBounds().width) / 2.f, iy + 42.f);
        window.draw(nameText);

        ix += itemW + pad;
        if (ix + itemW > x + width) { ix = x + pad; iy += itemH + pad; }
    }

    // Drag ghost
    if (m_Drag.active)
    {
        m_Drag.pos = m_MousePos;
        sf::RectangleShape ghost({ 60.f, 20.f });
        ghost.setPosition(m_Drag.pos.x + 8.f, m_Drag.pos.y - 10.f);
        ghost.setFillColor(sf::Color(60, 60, 100, 180));
        ghost.setOutlineColor(ColorForType(m_Drag.type));
        ghost.setOutlineThickness(1.f);
        window.draw(ghost);

        sf::Text ghostText;
        ghostText.setFont(m_Font);
        ghostText.setCharacterSize(10);
        ghostText.setFillColor(sf::Color(200, 200, 255));
        std::string gname = fs::path(m_Drag.path).filename().string();
        if (gname.size() > 10) gname = gname.substr(0, 9) + "~";
        ghostText.setString(gname);
        ghostText.setPosition(m_Drag.pos.x + 10.f, m_Drag.pos.y - 8.f);
        window.draw(ghostText);
    }
}

void ContentBrowser::NavigateTo(const std::string& path)
{
    std::error_code ec;
    fs::path target(path);
    if (!fs::exists(target, ec) || ec) return;
    if (!fs::is_directory(target, ec) || ec) return;

    const std::string canonical = fs::canonical(target, ec).string();
    if (ec) return;
    const std::string canonicalRoot = fs::canonical(fs::path(m_RootPath), ec).string();
    if (ec) return;

    if (canonical.find(canonicalRoot) == std::string::npos) return;

    m_CurrentPath = canonical;
    m_ScrollOffset = 0.f;
    Refresh();
}

void ContentBrowser::OpenEntry(const ContentEntry& entry)
{
    if (entry.isDirectory)
    {
        NavigateTo(entry.fullPath);
        return;
    }
#ifdef _WIN32
    ShellExecuteA(nullptr, "open", entry.fullPath.c_str(), nullptr, nullptr, SW_SHOW);
#elif __APPLE__
    system(("open \"" + entry.fullPath + "\"").c_str());
#else
    system(("xdg-open \"" + entry.fullPath + "\"").c_str());
#endif
}

AssetType ContentBrowser::TypeFromExtension(const std::string& ext)
{
    if (ext == ".lua")                          return AssetType::Script;
    if (ext == ".json")                         return AssetType::Scene;
    if (ext == ".png" || ext == ".jpg" ||
        ext == ".jpeg" || ext == ".bmp")        return AssetType::Image;
    return AssetType::Unknown;
}

sf::Color ContentBrowser::ColorForType(AssetType type)
{
    switch (type)
    {
        case AssetType::Folder:  return sf::Color(220, 180, 80);
        case AssetType::Script:  return sf::Color(120, 200, 120);
        case AssetType::Scene:   return sf::Color(100, 160, 255);
        case AssetType::Image:   return sf::Color(220, 120, 220);
        default:                 return sf::Color(140, 140, 140);
    }
}

std::string ContentBrowser::LabelForType(AssetType type)
{
    switch (type)
    {
        case AssetType::Folder:  return "FOLDER";
        case AssetType::Script:  return "LUA";
        case AssetType::Scene:   return "SCENE";
        case AssetType::Image:   return "IMG";
        default:                 return "FILE";
    }
}
