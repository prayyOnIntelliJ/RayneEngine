#include "ContentBrowser.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <chrono>

#include "SFML/Window/Event.hpp"
#include <SFML/Window/Clipboard.hpp>
#include "../Resources/ResourceManager.h"
#include "../Audio/AudioManager.h"

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

ContentBrowser::ContentBrowser(const sf::Font &font, const std::string &rootPath)
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

    if (!fs::exists(m_CurrentPath)) return;

    std::vector<ContentEntry> dirs, files;
    std::error_code ec;

    for (const auto &entry: fs::directory_iterator(m_CurrentPath, ec))
    {
        if (ec) break;
        ContentEntry ce;
        ce.name = entry.path().filename().string();
        ce.fullPath = entry.path().string();
        ce.isDirectory = entry.is_directory(ec);
        if (ec)
        {
            ec.clear();
            continue;
        }

        if (!ce.isDirectory)
        {
            ce.fileSize = entry.file_size(ec);
            if (ec)
            {
                ce.fileSize = 0;
                ec.clear();
            }
        }

        if (ce.isDirectory)
        {
            ce.type = AssetType::Folder;
            dirs.push_back(ce);
        } else
        {
            ce.type = TypeFromExtension(entry.path().extension().string());
            files.push_back(ce);
        }
    }

    auto sortByName = [](const ContentEntry &a, const ContentEntry &b) {
        std::string nameA = a.name, nameB = b.name;
        std::transform(nameA.begin(), nameA.end(), nameA.begin(), ::tolower);
        std::transform(nameB.begin(), nameB.end(), nameB.begin(), ::tolower);
        return nameA < nameB;
    };

    std::sort(dirs.begin(), dirs.end(), sortByName);
    std::sort(files.begin(), files.end(), sortByName);

    for (auto &d: dirs) m_Entries.push_back(d);
    for (auto &f: files) m_Entries.push_back(f);

    std::cout << "[INFO] [ContentBrowser] Refreshed directory: " << m_CurrentPath << " (Found " << dirs.size() <<
            " folders, " << files.size() << " files)\n";

    UpdateFilteredEntries();
}

void ContentBrowser::UpdateFilteredEntries()
{
    m_FilteredEntries.clear();

    std::string q = m_SearchQuery;
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);

    for (const auto &entry: m_Entries)
    {
        if (m_CurrentFilter == AssetFilter::Images && entry.type != AssetType::Image && !entry.isDirectory) continue;
        if (m_CurrentFilter == AssetFilter::Scripts && entry.type != AssetType::Script && !entry.isDirectory) continue;
        if (m_CurrentFilter == AssetFilter::Audio && entry.type != AssetType::Audio && !entry.isDirectory) continue;
        if (m_CurrentFilter == AssetFilter::Scenes && entry.type != AssetType::Scene && !entry.isDirectory) continue;

        if (!q.empty())
        {
            std::string entryName = entry.name;
            std::transform(entryName.begin(), entryName.end(), entryName.begin(), ::tolower);
            if (entryName.find(q) == std::string::npos) continue;
        }

        m_FilteredEntries.push_back(entry);
    }
}

void ContentBrowser::HandleEvent(const sf::Event &event, sf::Vector2f mouseScreenPos)
{
    m_MousePos = mouseScreenPos;

    if (event.type == sf::Event::MouseButtonReleased &&
        event.mouseButton.button == sf::Mouse::Left) { m_Drag.active = false; }

    if (m_SearchActive && event.type == sf::Event::TextEntered)
    {
        if (event.text.unicode == '\b')
        {
            if (!m_SearchQuery.empty())
            {
                m_SearchQuery.pop_back();
                UpdateFilteredEntries();
            }
        } else if (event.text.unicode == 27 || event.text.unicode == '\r' || event.text.unicode ==
                   '\n') { m_SearchActive = false; } else if (event.text.unicode >= 32 && event.text.unicode < 128)
        {
            m_SearchQuery += static_cast<char>(event.text.unicode);
            UpdateFilteredEntries();
        }
        return;
    }

    if (m_NewScriptPrompt && event.type == sf::Event::TextEntered)
    {
        if (event.text.unicode == '\b') { if (!m_NewScriptName.empty()) m_NewScriptName.pop_back(); } else if (
            event.text.unicode == 27)
        {
            m_NewScriptPrompt = false;
            m_NewScriptName.clear();
        } else if (event.text.unicode == '\r' || event.text.unicode == '\n')
        {
            if (!m_NewScriptName.empty()) { CreateNewScript(m_NewScriptName); }
            m_NewScriptPrompt = false;
            m_NewScriptName.clear();
        } else if (event.text.unicode >= 32 && event.text.unicode < 128)
        {
            char c = static_cast<char>(event.text.unicode);
            if (std::isalnum(c) || c == '_' || c == '-')
                m_NewScriptName += c;
        }
        return;
    }

    if (m_NewScenePrompt && event.type == sf::Event::TextEntered)
    {
        if (event.text.unicode == '\b') { if (!m_NewSceneName.empty()) m_NewSceneName.pop_back(); } else if (
            event.text.unicode == 27)
        {
            m_NewScenePrompt = false;
            m_NewSceneName.clear();
        } else if (event.text.unicode == '\r' || event.text.unicode == '\n')
        {
            if (!m_NewSceneName.empty()) { CreateNewScene(m_NewSceneName); }
            m_NewScenePrompt = false;
            m_NewSceneName.clear();
        } else if (event.text.unicode >= 32 && event.text.unicode < 128)
        {
            char c = static_cast<char>(event.text.unicode);
            if (std::isalnum(c) || c == '_' || c == '-')
                m_NewSceneName += c;
        }
        return;
    }

    if (m_NewFolderPrompt && event.type == sf::Event::TextEntered)
    {
        if (event.text.unicode == '\b') { if (!m_NewFolderName.empty()) m_NewFolderName.pop_back(); } else if (
            event.text.unicode == 27)
        {
            m_NewFolderPrompt = false;
            m_NewFolderName.clear();
        } else if (event.text.unicode == '\r' || event.text.unicode == '\n')
        {
            if (!m_NewFolderName.empty()) { CreateNewFolder(m_NewFolderName); }
            m_NewFolderPrompt = false;
            m_NewFolderName.clear();
        } else if (event.text.unicode >= 32 && event.text.unicode < 128)
        {
            char c = static_cast<char>(event.text.unicode);
            if (std::isalnum(c) || c == '_' || c == '-' || c == ' ')
                m_NewFolderName += c;
        }
        return;
    }

    if (m_RenamePrompt && event.type == sf::Event::TextEntered)
    {
        if (event.text.unicode == '\b') { if (!m_RenameInput.empty()) m_RenameInput.pop_back(); } else if (
            event.text.unicode == 27)
        {
            m_RenamePrompt = false;
            m_RenameInput.clear();
        } else if (event.text.unicode == '\r' || event.text.unicode == '\n')
        {
            if (!m_RenameInput.empty()) { RenameAsset(m_RenameTarget, m_RenameInput); }
            m_RenamePrompt = false;
            m_RenameInput.clear();
        } else if (event.text.unicode >= 32 && event.text.unicode < 128)
        {
            char c = static_cast<char>(event.text.unicode);
            if (c != '/' && c != '\\' && c != ':' && c != '*' && c != '?' && c != '"' && c != '<' && c != '>' && c !=
                '|')
                m_RenameInput += c;
        }
        return;
    }

    if (m_DeletePrompt && event.type == sf::Event::TextEntered)
    {
        if (event.text.unicode == 27 || event.text.unicode == 'n' || event.text.unicode == 'N')
        {
            m_DeletePrompt = false;
        } else if (event.text.unicode == '\r' || event.text.unicode == '\n' || event.text.unicode == 'y' || event.text.
                   unicode == 'Y')
        {
            DeleteAsset(m_DeleteTarget);
            m_DeletePrompt = false;
        }
        return;
    }

    if (event.type == sf::Event::KeyPressed)
    {
        if (m_RenamePrompt || m_NewScriptPrompt || m_NewScenePrompt || m_NewFolderPrompt || m_DeletePrompt)
        {
            if (event.key.code == sf::Keyboard::Escape)
            {
                m_RenamePrompt = false;
                m_NewScriptPrompt = false;
                m_NewScenePrompt = false;
                m_NewFolderPrompt = false;
                m_DeletePrompt = false;
                return;
            }
            if (m_DeletePrompt && event.key.code == sf::Keyboard::Enter)
            {
                DeleteAsset(m_DeleteTarget);
                m_DeletePrompt = false;
                return;
            }
        } else if (!m_SearchActive)
        {
            if (event.key.code == sf::Keyboard::F2 && !m_SelectedPath.empty())
            {
                m_RenameTarget = m_SelectedPath;
                m_RenameInput = fs::path(m_SelectedPath).filename().string();
                m_RenamePrompt = true;
                return;
            }
            if (event.key.code == sf::Keyboard::Delete && !m_SelectedPath.empty())
            {
                m_DeleteTarget = m_SelectedPath;
                m_DeletePrompt = true;
                return;
            }
            if (event.key.control && event.key.code == sf::Keyboard::D && !m_SelectedPath.empty())
            {
                DuplicateAsset(m_SelectedPath);
                return;
            }
            if (event.key.control && event.key.code == sf::Keyboard::C && !m_SelectedPath.empty())
            {
                CopyAssetPath(m_SelectedPath);
                return;
            }
        }
    }

    if (event.type == sf::Event::MouseWheelScrolled)
    {
        m_ScrollOffset -= event.mouseWheelScroll.delta * 28.f;
        if (m_ScrollOffset < 0.f) m_ScrollOffset = 0.f;
        if (m_ScrollOffset > m_MaxScroll) m_ScrollOffset = m_MaxScroll;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
    {
        if (m_ContextMenuOpen)
        {
            bool clickedMenu = false;
            for (const auto &item: m_ContextMenuItems)
            {
                if (item.bounds.contains(mouseScreenPos))
                {
                    HandleContextMenuAction(item.action);
                    clickedMenu = true;
                    break;
                }
            }
            m_ContextMenuOpen = false;
            if (clickedMenu) return;
        }

        if (m_SearchBoxBounds.contains(mouseScreenPos))
        {
            m_SearchActive = true;
            return;
        }
        m_SearchActive = false;

        if (!m_SearchQuery.empty() && m_SearchClearBounds.contains(mouseScreenPos))
        {
            m_SearchQuery.clear();
            UpdateFilteredEntries();
            return;
        }

        if (m_UpBtnBounds.contains(mouseScreenPos))
        {
            fs::path current(m_CurrentPath);
            fs::path root(m_RootPath);
            if (current != root && current.has_parent_path()) { NavigateTo(current.parent_path().string()); }
            return;
        }

        if (m_RefreshBtnBounds.contains(mouseScreenPos))
        {
            Refresh();
            return;
        }

        if (m_NewFolderBtnBounds.contains(mouseScreenPos))
        {
            m_NewFolderPrompt = true;
            m_NewFolderName = "NewFolder";
            return;
        }

        if (m_NewScriptBtnBounds.contains(mouseScreenPos))
        {
            m_NewScriptPrompt = true;
            m_NewScriptName = "new_script";
            return;
        }

        for (const auto &crumb: m_Breadcrumbs)
        {
            if (crumb.bounds.contains(mouseScreenPos))
            {
                NavigateTo(crumb.fullPath);
                return;
            }
        }

        for (const auto &[fRect, filter]: m_FilterBounds)
        {
            if (fRect.contains(mouseScreenPos))
            {
                m_CurrentFilter = filter;
                m_ScrollOffset = 0.f;
                UpdateFilteredEntries();
                return;
            }
        }

        for (auto &[rect, idx]: m_ItemBounds)
        {
            if (!rect.contains(mouseScreenPos)) continue;
            if (idx >= m_FilteredEntries.size()) continue;
            const ContentEntry &entry = m_FilteredEntries[idx];

            m_SelectedPath = entry.fullPath;
            double now = CurrentTimeSeconds();
            bool doubleClick = (m_LastClickedPath == entry.fullPath && now - m_LastClickTime < 0.35);
            m_LastClickTime = now;
            m_LastClickedPath = entry.fullPath;

            if (doubleClick)
            {
                OpenEntry(entry);
                m_Drag.active = false;
            } else if (entry.isDirectory) { NavigateTo(entry.fullPath); } else
            {
                m_Drag.active = true;
                m_Drag.path = entry.fullPath;
                m_Drag.type = entry.type;
                m_Drag.pos = mouseScreenPos;
            }
            return;
        }

        m_SelectedPath.clear();
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right)
    {
        m_ContextMenuOpen = false;

        if (m_Bounds.contains(mouseScreenPos))
        {
            for (auto &[rect, idx]: m_ItemBounds)
            {
                if (rect.contains(mouseScreenPos) && idx < m_FilteredEntries.size())
                {
                    const ContentEntry &entry = m_FilteredEntries[idx];
                    m_SelectedPath = entry.fullPath;
                    m_ContextMenuTarget = entry.fullPath;
                    m_ContextMenuPos = mouseScreenPos;
                    m_ContextMenuOpen = true;
                    return;
                }
            }

            m_ContextMenuTarget = m_CurrentPath;
            m_ContextMenuPos = mouseScreenPos;
            m_ContextMenuOpen = true;
        }
    }
}

void ContentBrowser::Render(sf::RenderWindow &window, float x, float y, float width, float height)
{
    m_Bounds = sf::FloatRect(x, y, width, height);
    m_ItemBounds.clear();
    m_Breadcrumbs.clear();
    m_FilterBounds.clear();

    sf::RectangleShape bg({width, height});
    bg.setPosition(x, y);
    bg.setFillColor(sf::Color(16, 17, 24, 252));
    bg.setOutlineColor(sf::Color(44, 46, 62));
    bg.setOutlineThickness(1.f);
    window.draw(bg);

    const float toolbarH = 32.f;
    sf::RectangleShape toolbar({width, toolbarH});
    toolbar.setPosition(x, y);
    toolbar.setFillColor(sf::Color(24, 25, 36, 255));
    window.draw(toolbar);

    sf::RectangleShape toolbarBorder({width, 1.f});
    toolbarBorder.setPosition(x, y + toolbarH);
    toolbarBorder.setFillColor(sf::Color(45, 48, 66));
    window.draw(toolbarBorder);

    float curX = x + 8.f;
    const float curY = y + 5.f;

    bool canGoUp = (m_CurrentPath != m_RootPath);
    m_UpBtnBounds = sf::FloatRect(curX, curY, 24.f, 22.f);
    bool upHovered = m_UpBtnBounds.contains(m_MousePos) && canGoUp;

    sf::RectangleShape upBtn({m_UpBtnBounds.width, m_UpBtnBounds.height});
    upBtn.setPosition(m_UpBtnBounds.left, m_UpBtnBounds.top);
    upBtn.setFillColor(canGoUp ? (upHovered ? sf::Color(60, 65, 90) : sf::Color(38, 41, 56)) : sf::Color(28, 30, 42));
    upBtn.setOutlineColor(upHovered ? sf::Color(100, 110, 150) : sf::Color(45, 48, 66));
    upBtn.setOutlineThickness(1.f);
    window.draw(upBtn);

    sf::Text upText;
    upText.setFont(m_Font);
    upText.setCharacterSize(12);
    upText.setFillColor(canGoUp ? (upHovered ? sf::Color::White : sf::Color(180, 185, 210)) : sf::Color(80, 85, 110));
    upText.setString("<");
    upText.setPosition(m_UpBtnBounds.left + 8.f, m_UpBtnBounds.top + 2.f);
    window.draw(upText);

    curX += 30.f;

    std::string relPath = m_CurrentPath;
    if (relPath.find(m_RootPath) == 0)
    {
        relPath = relPath.substr(m_RootPath.size());
        if (!relPath.empty() && (relPath[0] == '/' || relPath[0] == '\\'))
            relPath = relPath.substr(1);
    }

    std::vector<std::pair<std::string, std::string> > crumbs;
    crumbs.push_back({"assets", m_RootPath});

    if (!relPath.empty())
    {
        fs::path p = m_RootPath;
        std::string accum;
        for (size_t i = 0; i <= relPath.size(); ++i)
        {
            if (i == relPath.size() || relPath[i] == '/' || relPath[i] == '\\')
            {
                if (!accum.empty())
                {
                    p /= accum;
                    crumbs.push_back({accum, p.string()});
                    accum.clear();
                }
            } else { accum += relPath[i]; }
        }
    }

    for (size_t i = 0; i < crumbs.size(); ++i)
    {
        const auto &[cName, cPath] = crumbs[i];
        sf::Text cText;
        cText.setFont(m_Font);
        cText.setCharacterSize(11);
        cText.setString(cName);
        float tw = cText.getLocalBounds().width + 12.f;

        sf::FloatRect crumbBounds(curX, curY, tw, 22.f);
        m_Breadcrumbs.push_back({cName, cPath, crumbBounds});
        bool isHovered = crumbBounds.contains(m_MousePos);
        bool isLast = (i == crumbs.size() - 1);

        sf::RectangleShape crumbBg({crumbBounds.width, crumbBounds.height});
        crumbBg.setPosition(crumbBounds.left, crumbBounds.top);
        crumbBg.setFillColor(isHovered ? sf::Color(50, 55, 78) : sf::Color(32, 34, 48));
        crumbBg.setOutlineColor(isHovered ? sf::Color(80, 90, 130) : sf::Color(45, 48, 66));
        crumbBg.setOutlineThickness(1.f);
        window.draw(crumbBg);

        cText.setFillColor(isLast ? sf::Color(100, 180, 255) : isHovered ? sf::Color::White : sf::Color(170, 175, 200));
        cText.setPosition(curX + 6.f, curY + 4.f);
        window.draw(cText);

        curX += tw + 4.f;

        if (!isLast)
        {
            sf::Text sep;
            sep.setFont(m_Font);
            sep.setCharacterSize(11);
            sep.setFillColor(sf::Color(80, 85, 110));
            sep.setString("/");
            sep.setPosition(curX, curY + 4.f);
            window.draw(sep);
            curX += 10.f;
        }
    }

    float rightX = x + width - 8.f;

    rightX -= 26.f;
    m_RefreshBtnBounds = sf::FloatRect(rightX, curY, 26.f, 22.f);
    bool refHover = m_RefreshBtnBounds.contains(m_MousePos);
    sf::RectangleShape refBtn({m_RefreshBtnBounds.width, m_RefreshBtnBounds.height});
    refBtn.setPosition(m_RefreshBtnBounds.left, m_RefreshBtnBounds.top);
    refBtn.setFillColor(refHover ? sf::Color(40, 70, 50) : sf::Color(28, 45, 34));
    refBtn.setOutlineColor(refHover ? sf::Color(80, 180, 100) : sf::Color(40, 80, 50));
    refBtn.setOutlineThickness(1.f);
    window.draw(refBtn);

    sf::Text refText;
    refText.setFont(m_Font);
    refText.setCharacterSize(12);
    refText.setFillColor(refHover ? sf::Color(140, 255, 160) : sf::Color(100, 200, 120));
    refText.setString("R");
    refText.setPosition(m_RefreshBtnBounds.left + 8.f, m_RefreshBtnBounds.top + 2.f);
    window.draw(refText);

    rightX -= 68.f;
    m_NewScriptBtnBounds = sf::FloatRect(rightX, curY, 62.f, 22.f);
    bool newScriptHover = m_NewScriptBtnBounds.contains(m_MousePos);
    sf::RectangleShape newScriptBtn({m_NewScriptBtnBounds.width, m_NewScriptBtnBounds.height});
    newScriptBtn.setPosition(m_NewScriptBtnBounds.left, m_NewScriptBtnBounds.top);
    newScriptBtn.setFillColor(newScriptHover ? sf::Color(40, 60, 95) : sf::Color(28, 40, 65));
    newScriptBtn.setOutlineColor(newScriptHover ? sf::Color(80, 140, 230) : sf::Color(45, 70, 110));
    newScriptBtn.setOutlineThickness(1.f);
    window.draw(newScriptBtn);

    sf::Text newScriptText;
    newScriptText.setFont(m_Font);
    newScriptText.setCharacterSize(10);
    newScriptText.setFillColor(newScriptHover ? sf::Color(140, 200, 255) : sf::Color(100, 160, 230));
    newScriptText.setString("+ Script");
    newScriptText.setPosition(m_NewScriptBtnBounds.left + 8.f, m_NewScriptBtnBounds.top + 4.f);
    window.draw(newScriptText);

    rightX -= 64.f;
    m_NewFolderBtnBounds = sf::FloatRect(rightX, curY, 58.f, 22.f);
    bool newFolderHover = m_NewFolderBtnBounds.contains(m_MousePos);
    sf::RectangleShape newFolderBtn({m_NewFolderBtnBounds.width, m_NewFolderBtnBounds.height});
    newFolderBtn.setPosition(m_NewFolderBtnBounds.left, m_NewFolderBtnBounds.top);
    newFolderBtn.setFillColor(newFolderHover ? sf::Color(65, 55, 30) : sf::Color(45, 38, 20));
    newFolderBtn.setOutlineColor(newFolderHover ? sf::Color(220, 180, 70) : sf::Color(140, 110, 40));
    newFolderBtn.setOutlineThickness(1.f);
    window.draw(newFolderBtn);

    sf::Text newFolderText;
    newFolderText.setFont(m_Font);
    newFolderText.setCharacterSize(10);
    newFolderText.setFillColor(newFolderHover ? sf::Color(255, 225, 130) : sf::Color(200, 170, 90));
    newFolderText.setString("+ Folder");
    newFolderText.setPosition(m_NewFolderBtnBounds.left + 7.f, m_NewFolderBtnBounds.top + 4.f);
    window.draw(newFolderText);

    rightX -= 120.f;
    m_SearchBoxBounds = sf::FloatRect(rightX, curY, 114.f, 22.f);
    bool searchHover = m_SearchBoxBounds.contains(m_MousePos);
    sf::RectangleShape searchBox({m_SearchBoxBounds.width, m_SearchBoxBounds.height});
    searchBox.setPosition(m_SearchBoxBounds.left, m_SearchBoxBounds.top);
    searchBox.setFillColor(m_SearchActive
                               ? sf::Color(35, 38, 55)
                               : searchHover
                                     ? sf::Color(30, 32, 45)
                                     : sf::Color(22, 24, 34));
    searchBox.setOutlineColor(m_SearchActive
                                  ? sf::Color(100, 160, 255)
                                  : searchHover
                                        ? sf::Color(70, 75, 105)
                                        : sf::Color(45, 48, 66));
    searchBox.setOutlineThickness(1.f);
    window.draw(searchBox);

    sf::Text searchContent;
    searchContent.setFont(m_Font);
    searchContent.setCharacterSize(10);
    if (m_SearchQuery.empty() && !m_SearchActive)
    {
        searchContent.setFillColor(sf::Color(90, 95, 120));
        searchContent.setString("Search...");
    } else
    {
        searchContent.setFillColor(sf::Color(220, 225, 245));
        searchContent.setString(m_SearchQuery + (m_SearchActive ? "|" : ""));
    }
    searchContent.setPosition(m_SearchBoxBounds.left + 6.f, m_SearchBoxBounds.top + 4.f);
    window.draw(searchContent);

    if (!m_SearchQuery.empty())
    {
        m_SearchClearBounds = sf::FloatRect(m_SearchBoxBounds.left + m_SearchBoxBounds.width - 16.f,
                                            m_SearchBoxBounds.top + 3.f, 14.f, 16.f);
        sf::Text clearX;
        clearX.setFont(m_Font);
        clearX.setCharacterSize(10);
        clearX.setFillColor(sf::Color(160, 160, 180));
        clearX.setString("x");
        clearX.setPosition(m_SearchClearBounds.left + 2.f, m_SearchClearBounds.top + 1.f);
        window.draw(clearX);
    }

    struct FilterOption
    {
        const char *label;
        AssetFilter filter;
        float w;
    };
    FilterOption filters[] = {
        {"All", AssetFilter::All, 34.f},
        {"Sprites", AssetFilter::Images, 52.f},
        {"Scripts", AssetFilter::Scripts, 50.f},
        {"Audio", AssetFilter::Audio, 46.f},
        {"Scenes", AssetFilter::Scenes, 48.f}
    };

    for (int i = 4; i >= 0; --i)
    {
        rightX -= (filters[i].w + 4.f);
        if (rightX < curX + 10.f) break;

        sf::FloatRect fRect(rightX, curY, filters[i].w, 22.f);
        m_FilterBounds.emplace_back(fRect, filters[i].filter);

        bool isActive = (m_CurrentFilter == filters[i].filter);
        bool isHover = fRect.contains(m_MousePos);

        sf::RectangleShape fChip({fRect.width, fRect.height});
        fChip.setPosition(fRect.left, fRect.top);
        fChip.setFillColor(isActive ? sf::Color(45, 60, 95) : isHover ? sf::Color(35, 38, 52) : sf::Color(26, 28, 38));
        fChip.setOutlineColor(isActive
                                  ? sf::Color(100, 160, 255)
                                  : isHover
                                        ? sf::Color(65, 70, 95)
                                        : sf::Color(40, 42, 58));
        fChip.setOutlineThickness(1.f);
        window.draw(fChip);

        sf::Text fText;
        fText.setFont(m_Font);
        fText.setCharacterSize(10);
        fText.setFillColor(isActive
                               ? sf::Color(140, 200, 255)
                               : isHover
                                     ? sf::Color(200, 205, 225)
                                     : sf::Color(130, 135, 155));
        fText.setString(filters[i].label);
        fText.setPosition(fRect.left + (fRect.width - fText.getLocalBounds().width) / 2.f, fRect.top + 4.f);
        window.draw(fText);
    }

    const float statusBarH = 22.f;
    const float cardW = 86.f;
    const float cardH = 92.f;
    const float pad = 10.f;
    const float gridStartY = y + toolbarH + 8.f;
    const float gridHeight = height - toolbarH - statusBarH - 12.f;

    float ix = x + pad;
    float iy = gridStartY - m_ScrollOffset;
    float totalContentHeight = 0.f;

    for (size_t i = 0; i < m_FilteredEntries.size(); ++i)
    {
        const auto &entry = m_FilteredEntries[i];

        sf::FloatRect cardRect(ix, iy, cardW, cardH);
        m_ItemBounds.emplace_back(cardRect, i);

        if (iy + cardH >= gridStartY && iy <= y + height - statusBarH)
        {
            bool hovered = cardRect.contains(m_MousePos);
            bool selected = (entry.fullPath == m_SelectedPath);

            sf::RectangleShape cardBg({cardW, cardH});
            cardBg.setPosition(ix, iy);
            cardBg.setFillColor(selected
                                    ? sf::Color(35, 48, 75, 240)
                                    : hovered
                                          ? sf::Color(30, 33, 48, 230)
                                          : sf::Color(22, 23, 33, 200));
            cardBg.setOutlineColor(selected
                                       ? sf::Color(100, 160, 255)
                                       : hovered
                                             ? sf::Color(70, 85, 125)
                                             : sf::Color(38, 40, 56));
            cardBg.setOutlineThickness(selected ? 1.5f : 1.f);
            window.draw(cardBg);

            const float previewW = 68.f;
            const float previewH = 50.f;
            const float previewX = ix + (cardW - previewW) / 2.f;
            const float previewY = iy + 6.f;

            if (entry.type == AssetType::Image)
            {
                auto tex = ResourceManager::Get().GetTexture(entry.fullPath);
                if (tex && tex->getSize().x > 0 && tex->getSize().y > 0)
                {
                    sf::Sprite previewSprite(*tex);
                    float scaleX = previewW / static_cast<float>(tex->getSize().x);
                    float scaleY = previewH / static_cast<float>(tex->getSize().y);
                    float scale = std::min(scaleX, scaleY);

                    previewSprite.setScale(scale, scale);
                    float drawnW = tex->getSize().x * scale;
                    float drawnH = tex->getSize().y * scale;
                    previewSprite.setPosition(
                        previewX + (previewW - drawnW) / 2.f,
                        previewY + (previewH - drawnH) / 2.f
                    );
                    window.draw(previewSprite);

                    sf::RectangleShape previewFrame({previewW, previewH});
                    previewFrame.setPosition(previewX, previewY);
                    previewFrame.setFillColor(sf::Color::Transparent);
                    previewFrame.setOutlineColor(sf::Color(55, 60, 85));
                    previewFrame.setOutlineThickness(1.f);
                    window.draw(previewFrame);
                }
            } else
            {
                sf::Color typeColor = ColorForType(entry.type);
                sf::RectangleShape iconBox({previewW, previewH});
                iconBox.setPosition(previewX, previewY);
                iconBox.setFillColor(sf::Color(typeColor.r, typeColor.g, typeColor.b, 25));
                iconBox.setOutlineColor(sf::Color(typeColor.r, typeColor.g, typeColor.b, 100));
                iconBox.setOutlineThickness(1.f);
                window.draw(iconBox);

                sf::RectangleShape pill({42.f, 18.f});
                pill.setPosition(previewX + (previewW - 42.f) / 2.f, previewY + 16.f);
                pill.setFillColor(sf::Color(typeColor.r, typeColor.g, typeColor.b, 50));
                pill.setOutlineColor(typeColor);
                pill.setOutlineThickness(1.f);
                window.draw(pill);

                sf::Text typeLabel;
                typeLabel.setFont(m_Font);
                typeLabel.setCharacterSize(9);
                typeLabel.setStyle(sf::Text::Bold);
                typeLabel.setFillColor(typeColor);
                typeLabel.setString(LabelForType(entry.type));
                typeLabel.setPosition(
                    pill.getPosition().x + (pill.getSize().x - typeLabel.getLocalBounds().width) / 2.f,
                    pill.getPosition().y + 2.f
                );
                window.draw(typeLabel);
            }

            sf::Text nameText;
            nameText.setFont(m_Font);
            nameText.setCharacterSize(10);
            nameText.setFillColor(selected
                                      ? sf::Color(140, 200, 255)
                                      : hovered
                                            ? sf::Color::White
                                            : sf::Color(185, 190, 210));

            std::string displayName = entry.name;
            if (displayName.size() > 11) displayName = displayName.substr(0, 10) + "...";
            nameText.setString(displayName);
            nameText.setPosition(ix + (cardW - nameText.getLocalBounds().width) / 2.f, iy + 60.f);
            window.draw(nameText);

            sf::Text subText;
            subText.setFont(m_Font);
            subText.setCharacterSize(8);
            subText.setFillColor(sf::Color(110, 115, 140));
            if (entry.isDirectory)
                subText.setString("Folder");
            else
                subText.setString(FormatFileSize(entry.fileSize));
            subText.setPosition(ix + (cardW - subText.getLocalBounds().width) / 2.f, iy + 74.f);
            window.draw(subText);
        }

        ix += cardW + pad;
        if (ix + cardW > x + width - 16.f)
        {
            ix = x + pad;
            iy += cardH + pad;
        }
    }

    totalContentHeight = (iy + cardH + pad) - (gridStartY - m_ScrollOffset);
    m_MaxScroll = std::max(0.f, totalContentHeight - gridHeight);

    if (m_FilteredEntries.empty())
    {
        sf::Text emptyText;
        emptyText.setFont(m_Font);
        emptyText.setCharacterSize(11);
        emptyText.setFillColor(sf::Color(100, 105, 130));
        emptyText.setString(m_SearchQuery.empty() ? "Folder is empty" : "No matching assets found");
        emptyText.setPosition(x + (width - emptyText.getLocalBounds().width) / 2.f, y + toolbarH + 40.f);
        window.draw(emptyText);
    }

    if (m_MaxScroll > 0.f)
    {
        const float sbW = 6.f;
        const float sbX = x + width - sbW - 2.f;
        const float sbY = gridStartY;
        const float sbH = gridHeight;

        sf::RectangleShape track({sbW, sbH});
        track.setPosition(sbX, sbY);
        track.setFillColor(sf::Color(20, 22, 30, 160));
        window.draw(track);

        float thumbRatio = gridHeight / (gridHeight + m_MaxScroll);
        float thumbH = std::max(18.f, sbH * thumbRatio);
        float thumbY = sbY + (m_ScrollOffset / m_MaxScroll) * (sbH - thumbH);

        sf::RectangleShape thumb({sbW, thumbH});
        thumb.setPosition(sbX, thumbY);
        thumb.setFillColor(sf::Color(65, 75, 105));
        window.draw(thumb);
    }

    const float statusBarY = y + height - statusBarH;
    sf::RectangleShape statusBg({width, statusBarH});
    statusBg.setPosition(x, statusBarY);
    statusBg.setFillColor(sf::Color(20, 21, 30, 255));
    statusBg.setOutlineColor(sf::Color(38, 41, 56));
    statusBg.setOutlineThickness(1.f);
    window.draw(statusBg);

    sf::Text statusText;
    statusText.setFont(m_Font);
    statusText.setCharacterSize(10);

    double now = CurrentTimeSeconds();
    if (!m_StatusMessage.empty() && (now - m_StatusMessageTime < 3.5))
    {
        statusText.setFillColor(sf::Color(120, 230, 160));
        statusText.setString(m_StatusMessage);
    } else
    {
        const ContentEntry *infoEntry = nullptr;
        for (const auto &[rect, idx]: m_ItemBounds)
        {
            if (rect.contains(m_MousePos) && idx < m_FilteredEntries.size())
            {
                infoEntry = &m_FilteredEntries[idx];
                break;
            }
        }
        if (!infoEntry && !m_SelectedPath.empty())
        {
            for (const auto &e: m_FilteredEntries)
            {
                if (e.fullPath == m_SelectedPath)
                {
                    infoEntry = &e;
                    break;
                }
            }
        }

        if (infoEntry)
        {
            statusText.setFillColor(sf::Color(190, 200, 225));
            std::string infoStr = "[" + LabelForType(infoEntry->type) + "] " + infoEntry->name;
            if (!infoEntry->isDirectory)
                infoStr += "  |  " + FormatFileSize(infoEntry->fileSize);
            statusText.setString(infoStr);
        } else
        {
            statusText.setFillColor(sf::Color(110, 115, 140));
            std::string countStr = std::to_string(m_FilteredEntries.size()) + " items";
            if (m_FilteredEntries.size() != m_Entries.size())
                countStr += " (filtered from " + std::to_string(m_Entries.size()) + ")";
            statusText.setString(countStr);
        }
    }
    statusText.setPosition(x + 10.f, statusBarY + 4.f);
    window.draw(statusText);

    if (m_ContextMenuOpen)
    {
        m_ContextMenuItems.clear();
        bool isDir = fs::is_directory(m_ContextMenuTarget);
        AssetType targetType = isDir
                                   ? AssetType::Folder
                                   : TypeFromExtension(fs::path(m_ContextMenuTarget).extension().string());

        std::vector<std::pair<std::string, std::string> > actions;
        if (!isDir)
        {
            actions.push_back({"Open", "open"});
            if (targetType == AssetType::Audio)
                actions.push_back({"Play Audio", "play_audio"});
        }
        if (m_ContextMenuTarget != m_CurrentPath)
        {
            actions.push_back({"Rename (F2)", "rename"});
            if (!isDir) { actions.push_back({"Duplicate (Ctrl+D)", "duplicate"}); }
            actions.push_back({"Copy Relative Path (Ctrl+C)", "copy_path"});
        }
        actions.push_back({"New Folder", "new_folder"});
        actions.push_back({"New Script", "new_script"});
        actions.push_back({"New Scene", "new_scene"});
        actions.push_back({"Reveal in Explorer", "reveal"});
        if (m_ContextMenuTarget != m_CurrentPath && m_ContextMenuTarget != m_RootPath)
        {
            actions.push_back({"Delete (Del)", "delete"});
        }

        const float menuW = 190.f;
        const float itemRowH = 22.f;
        const float menuH = actions.size() * itemRowH + 6.f;
        float menuX = std::min(m_ContextMenuPos.x, x + width - menuW - 4.f);
        float menuY = std::min(m_ContextMenuPos.y, y + height - menuH - 4.f);

        sf::RectangleShape menuBg({menuW, menuH});
        menuBg.setPosition(menuX, menuY);
        menuBg.setFillColor(sf::Color(25, 27, 38, 255));
        menuBg.setOutlineColor(sf::Color(65, 75, 110));
        menuBg.setOutlineThickness(1.f);
        window.draw(menuBg);

        float rowY = menuY + 3.f;
        for (const auto &[label, act]: actions)
        {
            sf::FloatRect itemRect(menuX + 3.f, rowY, menuW - 6.f, itemRowH);
            m_ContextMenuItems.push_back({label, act, itemRect});
            bool isRowHover = itemRect.contains(m_MousePos);

            if (isRowHover)
            {
                sf::RectangleShape rowHighlight({itemRect.width, itemRect.height});
                rowHighlight.setPosition(itemRect.left, itemRect.top);
                rowHighlight.setFillColor(act == "delete" ? sf::Color(80, 30, 30) : sf::Color(45, 60, 95));
                window.draw(rowHighlight);
            }

            sf::Text rowText;
            rowText.setFont(m_Font);
            rowText.setCharacterSize(10);
            rowText.setFillColor(act == "delete"
                                     ? sf::Color(255, 120, 120)
                                     : isRowHover
                                           ? sf::Color::White
                                           : sf::Color(200, 205, 225));
            rowText.setString(label);
            rowText.setPosition(itemRect.left + 8.f, itemRect.top + 4.f);
            window.draw(rowText);

            rowY += itemRowH;
        }
    }

    if (m_NewScriptPrompt)
    {
        const float modalW = 280.f;
        const float modalH = 80.f;
        const float modalX = x + (width - modalW) / 2.f;
        const float modalY = y + (height - modalH) / 2.f;

        sf::RectangleShape modalDim({width, height});
        modalDim.setPosition(x, y);
        modalDim.setFillColor(sf::Color(0, 0, 0, 160));
        window.draw(modalDim);

        sf::RectangleShape modalBg({modalW, modalH});
        modalBg.setPosition(modalX, modalY);
        modalBg.setFillColor(sf::Color(25, 27, 40, 255));
        modalBg.setOutlineColor(sf::Color(80, 130, 220));
        modalBg.setOutlineThickness(1.5f);
        window.draw(modalBg);

        sf::Text title;
        title.setFont(m_Font);
        title.setCharacterSize(11);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color(140, 190, 255));
        title.setString("Create Lua Script (Enter to save, Esc to cancel)");
        title.setPosition(modalX + 10.f, modalY + 8.f);
        window.draw(title);

        sf::RectangleShape inputField({modalW - 20.f, 24.f});
        inputField.setPosition(modalX + 10.f, modalY + 32.f);
        inputField.setFillColor(sf::Color(16, 18, 28));
        inputField.setOutlineColor(sf::Color(100, 160, 255));
        inputField.setOutlineThickness(1.f);
        window.draw(inputField);

        sf::Text inputText;
        inputText.setFont(m_Font);
        inputText.setCharacterSize(11);
        inputText.setFillColor(sf::Color::White);
        inputText.setString(m_NewScriptName + ".lua |");
        inputText.setPosition(modalX + 16.f, modalY + 36.f);
        window.draw(inputText);
    }

    if (m_NewScenePrompt)
    {
        const float modalW = 280.f;
        const float modalH = 80.f;
        const float modalX = x + (width - modalW) / 2.f;
        const float modalY = y + (height - modalH) / 2.f;

        sf::RectangleShape modalDim({width, height});
        modalDim.setPosition(x, y);
        modalDim.setFillColor(sf::Color(0, 0, 0, 160));
        window.draw(modalDim);

        sf::RectangleShape modalBg({modalW, modalH});
        modalBg.setPosition(modalX, modalY);
        modalBg.setFillColor(sf::Color(25, 27, 40, 255));
        modalBg.setOutlineColor(sf::Color(80, 220, 130));
        modalBg.setOutlineThickness(1.5f);
        window.draw(modalBg);

        sf::Text title;
        title.setFont(m_Font);
        title.setCharacterSize(11);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color(140, 255, 190));
        title.setString("Create Scene (Enter to save, Esc to cancel)");
        title.setPosition(modalX + 10.f, modalY + 8.f);
        window.draw(title);

        sf::RectangleShape inputField({modalW - 20.f, 24.f});
        inputField.setPosition(modalX + 10.f, modalY + 32.f);
        inputField.setFillColor(sf::Color(16, 18, 28));
        inputField.setOutlineColor(sf::Color(100, 255, 160));
        inputField.setOutlineThickness(1.f);
        window.draw(inputField);

        sf::Text inputText;
        inputText.setFont(m_Font);
        inputText.setCharacterSize(11);
        inputText.setFillColor(sf::Color::White);
        inputText.setString(m_NewSceneName + ".json |");
        inputText.setPosition(modalX + 16.f, modalY + 36.f);
        window.draw(inputText);
    }

    if (m_NewFolderPrompt)
    {
        const float modalW = 280.f;
        const float modalH = 80.f;
        const float modalX = x + (width - modalW) / 2.f;
        const float modalY = y + (height - modalH) / 2.f;

        sf::RectangleShape modalDim({width, height});
        modalDim.setPosition(x, y);
        modalDim.setFillColor(sf::Color(0, 0, 0, 160));
        window.draw(modalDim);

        sf::RectangleShape modalBg({modalW, modalH});
        modalBg.setPosition(modalX, modalY);
        modalBg.setFillColor(sf::Color(25, 27, 40, 255));
        modalBg.setOutlineColor(sf::Color(220, 180, 80));
        modalBg.setOutlineThickness(1.5f);
        window.draw(modalBg);

        sf::Text title;
        title.setFont(m_Font);
        title.setCharacterSize(11);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color(235, 200, 100));
        title.setString("Create Folder (Enter to create, Esc to cancel)");
        title.setPosition(modalX + 10.f, modalY + 8.f);
        window.draw(title);

        sf::RectangleShape inputField({modalW - 20.f, 24.f});
        inputField.setPosition(modalX + 10.f, modalY + 32.f);
        inputField.setFillColor(sf::Color(16, 18, 28));
        inputField.setOutlineColor(sf::Color(220, 180, 80));
        inputField.setOutlineThickness(1.f);
        window.draw(inputField);

        sf::Text inputText;
        inputText.setFont(m_Font);
        inputText.setCharacterSize(11);
        inputText.setFillColor(sf::Color::White);
        inputText.setString(m_NewFolderName + " |");
        inputText.setPosition(modalX + 16.f, modalY + 36.f);
        window.draw(inputText);
    }

    if (m_RenamePrompt)
    {
        const float modalW = 320.f;
        const float modalH = 86.f;
        const float modalX = x + (width - modalW) / 2.f;
        const float modalY = y + (height - modalH) / 2.f;

        sf::RectangleShape modalDim({width, height});
        modalDim.setPosition(x, y);
        modalDim.setFillColor(sf::Color(0, 0, 0, 170));
        window.draw(modalDim);

        sf::RectangleShape modalBg({modalW, modalH});
        modalBg.setPosition(modalX, modalY);
        modalBg.setFillColor(sf::Color(25, 27, 40, 255));
        modalBg.setOutlineColor(sf::Color(140, 190, 255));
        modalBg.setOutlineThickness(1.5f);
        window.draw(modalBg);

        sf::Text title;
        title.setFont(m_Font);
        title.setCharacterSize(11);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color(140, 190, 255));
        title.setString("Rename (Enter to save, Esc to cancel)");
        title.setPosition(modalX + 10.f, modalY + 8.f);
        window.draw(title);

        sf::RectangleShape inputField({modalW - 20.f, 24.f});
        inputField.setPosition(modalX + 10.f, modalY + 34.f);
        inputField.setFillColor(sf::Color(16, 18, 28));
        inputField.setOutlineColor(sf::Color(100, 160, 255));
        inputField.setOutlineThickness(1.f);
        window.draw(inputField);

        sf::Text inputText;
        inputText.setFont(m_Font);
        inputText.setCharacterSize(11);
        inputText.setFillColor(sf::Color::White);
        inputText.setString(m_RenameInput + " |");
        inputText.setPosition(modalX + 16.f, modalY + 38.f);
        window.draw(inputText);
    }

    if (m_DeletePrompt)
    {
        const float modalW = 320.f;
        const float modalH = 84.f;
        const float modalX = x + (width - modalW) / 2.f;
        const float modalY = y + (height - modalH) / 2.f;

        sf::RectangleShape modalDim({width, height});
        modalDim.setPosition(x, y);
        modalDim.setFillColor(sf::Color(0, 0, 0, 170));
        window.draw(modalDim);

        sf::RectangleShape modalBg({modalW, modalH});
        modalBg.setPosition(modalX, modalY);
        modalBg.setFillColor(sf::Color(32, 22, 26, 255));
        modalBg.setOutlineColor(sf::Color(240, 80, 80));
        modalBg.setOutlineThickness(1.5f);
        window.draw(modalBg);

        sf::Text title;
        title.setFont(m_Font);
        title.setCharacterSize(11);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color(255, 120, 120));
        title.setString("Delete Item? (Enter to delete, Esc to cancel)");
        title.setPosition(modalX + 10.f, modalY + 8.f);
        window.draw(title);

        std::string fname = fs::path(m_DeleteTarget).filename().string();
        if (fname.size() > 34) fname = fname.substr(0, 32) + "...";

        sf::Text info;
        info.setFont(m_Font);
        info.setCharacterSize(11);
        info.setFillColor(sf::Color(230, 200, 205));
        info.setString("\"" + fname + "\"");
        info.setPosition(modalX + 12.f, modalY + 36.f);
        window.draw(info);

        sf::Text sub;
        sub.setFont(m_Font);
        sub.setCharacterSize(9);
        sub.setFillColor(sf::Color(160, 130, 140));
        sub.setString("This action cannot be undone.");
        sub.setPosition(modalX + 12.f, modalY + 58.f);
        window.draw(sub);
    }

    if (m_Drag.active)
        m_Drag.pos = m_MousePos;
}

void ContentBrowser::RenderDragGhost(sf::RenderWindow &window)
{
    if (!m_Drag.active) return;

    const float ox = m_Drag.pos.x + 14.f;
    const float oy = m_Drag.pos.y + 8.f;
    const sf::Color accent = ColorForType(m_Drag.type);
    std::string gname = fs::path(m_Drag.path).filename().string();

    if (m_Drag.type == AssetType::Image)
    {
        const float thumbSz = 64.f;

        sf::RectangleShape shadow({thumbSz + 4.f, thumbSz + 22.f});
        shadow.setPosition(ox + 3.f, oy + 3.f);
        shadow.setFillColor(sf::Color(0, 0, 0, 100));
        window.draw(shadow);

        sf::RectangleShape card({thumbSz + 4.f, thumbSz + 22.f});
        card.setPosition(ox, oy);
        card.setFillColor(sf::Color(22, 26, 42, 240));
        card.setOutlineColor(accent);
        card.setOutlineThickness(1.5f);
        window.draw(card);

        auto tex = ResourceManager::Get().GetTexture(m_Drag.path);
        if (tex)
        {
            sf::Sprite preview;
            preview.setTexture(*tex);
            const sf::Vector2u ts = tex->getSize();
            if (ts.x > 0 && ts.y > 0)
            {
                const float scale = thumbSz / static_cast<float>(std::max(ts.x, ts.y));
                preview.setScale(scale, scale);
                preview.setPosition(
                    ox + 2.f + (thumbSz - ts.x * scale) / 2.f,
                    oy + 2.f + (thumbSz - ts.y * scale) / 2.f);
                preview.setColor(sf::Color(255, 255, 255, 230));
            }
            window.draw(preview);
        } else
        {
            sf::RectangleShape ph({thumbSz - 4.f, thumbSz - 4.f});
            ph.setPosition(ox + 4.f, oy + 4.f);
            ph.setFillColor(sf::Color(40, 45, 70));
            window.draw(ph);
        }

        if (gname.size() > 12) gname = gname.substr(0, 11) + "..";
        sf::Text label;
        label.setFont(m_Font);
        label.setCharacterSize(9);
        label.setFillColor(sf::Color(200, 210, 255, 240));
        label.setString(gname);
        label.setPosition(ox + 3.f, oy + thumbSz + 7.f);
        window.draw(label);
    } else
    {
        if (gname.size() > 18) gname = gname.substr(0, 17) + "...";
        const float pw = 124.f, ph = 24.f;

        sf::RectangleShape shadow({pw + 2.f, ph + 2.f});
        shadow.setPosition(ox + 2.f, oy + 2.f);
        shadow.setFillColor(sf::Color(0, 0, 0, 80));
        window.draw(shadow);

        sf::RectangleShape pill({pw, ph});
        pill.setPosition(ox, oy);
        pill.setFillColor(sf::Color(22, 26, 42, 240));
        pill.setOutlineColor(accent);
        pill.setOutlineThickness(1.5f);
        window.draw(pill);

        sf::CircleShape dot(4.f);
        dot.setFillColor(accent);
        dot.setPosition(ox + 6.f, oy + 8.f);
        window.draw(dot);

        sf::Text label;
        label.setFont(m_Font);
        label.setCharacterSize(10);
        label.setFillColor(sf::Color(210, 220, 255, 240));
        label.setString(gname);
        label.setPosition(ox + 18.f, oy + 6.f);
        window.draw(label);
    }
}

void ContentBrowser::NavigateTo(const std::string &path)
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
    m_SelectedPath.clear();
    Refresh();
}

void ContentBrowser::OpenEntry(const ContentEntry &entry)
{
    if (entry.isDirectory)
    {
        NavigateTo(entry.fullPath);
        return;
    }

    if (entry.type == AssetType::Audio)
    {
        AudioManager::Get().PlaySound(entry.fullPath);
        return;
    }

    if (entry.type == AssetType::Scene && onSceneLoadRequest)
    {
        onSceneLoadRequest(entry.fullPath);
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

void ContentBrowser::CreateNewScript(const std::string &name)
{
    std::string cleanName = name;
    if (cleanName.size() > 4 && cleanName.substr(cleanName.size() - 4) == ".lua")
        cleanName = cleanName.substr(0, cleanName.size() - 4);

    std::string fullPath = (fs::path(m_CurrentPath) / (cleanName + ".lua")).string();

    std::ofstream file(fullPath);
    if (file.is_open())
    {
        file << "-- " << cleanName << ".lua\n\n";
        file << "function OnCreate()\n";
        file << "    print(\"[Script] " << cleanName << " initialized\")\n";
        file << "end\n\n";
        file << "function OnUpdate(dt)\n";
        file << "    -- Update logic\n";
        file << "end\n\n";
        file << "function OnCollision(other)\n";
        file << "    -- Collision logic\n";
        file << "end\n";
        std::cout << "[INFO] [ContentBrowser] Created new Lua script file: " << fullPath << "\n";
    }

    Refresh();
    m_SelectedPath = fullPath;
    SetStatusMessage("Created script: " + cleanName + ".lua");
}

void ContentBrowser::CreateNewScene(const std::string &name)
{
    std::string cleanName = name;
    if (cleanName.size() > 5 && cleanName.substr(cleanName.size() - 5) == ".json")
        cleanName = cleanName.substr(0, cleanName.size() - 5);

    std::string fullPath = (fs::path(m_CurrentPath) / (cleanName + ".json")).string();

    std::ofstream file(fullPath);
    if (file.is_open())
    {
        file << "{\n    \"name\": \"" << cleanName << "\",\n    \"objects\": []\n}\n";
        file.close();
        std::cout << "[INFO] [ContentBrowser] Created new Scene file: " << fullPath << "\n";
    }

    Refresh();
    m_SelectedPath = fullPath;
    SetStatusMessage("Created scene: " + cleanName + ".json");
}

void ContentBrowser::CreateNewFolder(const std::string &name)
{
    if (name.empty()) return;
    fs::path targetDir = fs::path(m_CurrentPath) / name;
    std::error_code ec;
    if (fs::exists(targetDir, ec))
    {
        SetStatusMessage("Folder already exists: " + name);
        return;
    }
    if (fs::create_directories(targetDir, ec) && !ec)
    {
        std::cout << "[INFO] [ContentBrowser] Created folder: " << targetDir.string() << "\n";
        Refresh();
        m_SelectedPath = targetDir.string();
        SetStatusMessage("Created folder: " + name);
    } else { SetStatusMessage("Failed to create folder: " + name); }
}

void ContentBrowser::RenameAsset(const std::string &oldPath, const std::string &newName)
{
    if (newName.empty() || oldPath.empty()) return;
    std::error_code ec;
    fs::path oldP(oldPath);
    if (!fs::exists(oldP, ec) || ec)
    {
        SetStatusMessage("File does not exist");
        return;
    }

    std::string finalName = newName;
    if (!fs::is_directory(oldP, ec) && oldP.has_extension() && !fs::path(newName).has_extension())
    {
        finalName += oldP.extension().string();
    }

    fs::path parentP = oldP.parent_path();
    fs::path newP = parentP / finalName;

    if (newP == oldP) return;

    if (fs::exists(newP, ec))
    {
        SetStatusMessage("Name already exists: " + finalName);
        return;
    }

    fs::rename(oldP, newP, ec);
    if (!ec)
    {
        std::cout << "[INFO] [ContentBrowser] Renamed '" << oldPath << "' to '" << newP.string() << "'\n";
        Refresh();
        m_SelectedPath = newP.string();
        SetStatusMessage("Renamed to: " + finalName);
    } else
    {
        std::cout << "[ERROR] [ContentBrowser] Failed to rename: " << ec.message() << "\n";
        SetStatusMessage("Rename failed: " + ec.message());
    }
}

void ContentBrowser::DuplicateAsset(const std::string &path)
{
    std::error_code ec;
    fs::path src(path);
    if (!fs::exists(src, ec) || ec) return;

    fs::path parent = src.parent_path();
    std::string stem = src.stem().string();
    std::string ext = src.extension().string();

    fs::path dest = parent / (stem + "_copy" + ext);
    int count = 2;
    while (fs::exists(dest, ec)) { dest = parent / (stem + "_copy_" + std::to_string(count++) + ext); }

    if (fs::is_directory(src, ec)) { fs::copy(src, dest, fs::copy_options::recursive, ec); } else
    {
        fs::copy_file(src, dest, fs::copy_options::overwrite_existing, ec);
    }

    if (!ec)
    {
        std::cout << "[INFO] [ContentBrowser] Duplicated to '" << dest.string() << "'\n";
        Refresh();
        m_SelectedPath = dest.string();
        SetStatusMessage("Duplicated: " + dest.filename().string());
    } else { SetStatusMessage("Duplicate failed: " + ec.message()); }
}

void ContentBrowser::CopyAssetPath(const std::string &path)
{
    std::string relPath = path;
    if (relPath.find(m_RootPath) == 0)
    {
        relPath = relPath.substr(m_RootPath.size());
        if (!relPath.empty() && (relPath[0] == '/' || relPath[0] == '\\'))
            relPath = relPath.substr(1);
    }
    std::replace(relPath.begin(), relPath.end(), '\\', '/');

    sf::Clipboard::setString(relPath);
    std::cout << "[INFO] [ContentBrowser] Copied path to clipboard: " << relPath << "\n";
    SetStatusMessage("Copied to clipboard: " + relPath);
}

void ContentBrowser::SetStatusMessage(const std::string &msg)
{
    m_StatusMessage = msg;
    m_StatusMessageTime = CurrentTimeSeconds();
}

void ContentBrowser::RevealInExplorer(const std::string &path)
{
#ifdef _WIN32
    std::string cmd = "/select,\"" + path + "\"";
    ShellExecuteA(nullptr, "open", "explorer.exe", cmd.c_str(), nullptr, SW_SHOW);
#endif
}

void ContentBrowser::DeleteAsset(const std::string &path)
{
    if (path == m_RootPath || path.empty()) return;
    std::error_code ec;
    fs::remove_all(path, ec);
    if (!ec)
    {
        std::cout << "[INFO] [ContentBrowser] Deleted file/folder: " << path << "\n";
        if (m_SelectedPath == path) m_SelectedPath.clear();
        Refresh();
        SetStatusMessage("Deleted: " + fs::path(path).filename().string());
    } else { SetStatusMessage("Delete failed: " + ec.message()); }
}

void ContentBrowser::HandleContextMenuAction(const std::string &action)
{
    if (action == "open")
    {
        ContentEntry ce;
        ce.fullPath = m_ContextMenuTarget;
        ce.isDirectory = fs::is_directory(m_ContextMenuTarget);
        ce.type = ce.isDirectory
                      ? AssetType::Folder
                      : TypeFromExtension(fs::path(m_ContextMenuTarget).extension().string());
        OpenEntry(ce);
    } else if (action == "play_audio") { AudioManager::Get().PlaySound(m_ContextMenuTarget); } else if (
        action == "rename")
    {
        m_RenameTarget = m_ContextMenuTarget;
        m_RenameInput = fs::path(m_ContextMenuTarget).filename().string();
        m_RenamePrompt = true;
    } else if (action == "duplicate") { DuplicateAsset(m_ContextMenuTarget); } else if (action == "copy_path")
    {
        CopyAssetPath(m_ContextMenuTarget);
    } else if (action == "new_folder")
    {
        m_NewFolderPrompt = true;
        m_NewFolderName = "NewFolder";
    } else if (action == "new_script")
    {
        m_NewScriptPrompt = true;
        m_NewScriptName = "new_script";
    } else if (action == "new_scene")
    {
        m_NewScenePrompt = true;
        m_NewSceneName = "new_scene";
    } else if (action == "reveal") { RevealInExplorer(m_ContextMenuTarget); } else if (action == "delete")
    {
        m_DeleteTarget = m_ContextMenuTarget;
        m_DeletePrompt = true;
    }
}

AssetType ContentBrowser::TypeFromExtension(const std::string &ext)
{
    std::string e = ext;
    std::transform(e.begin(), e.end(), e.begin(), ::tolower);

    if (e == ".lua") return AssetType::Script;
    if (e == ".json") return AssetType::Scene;
    if (e == ".png" || e == ".jpg" ||
        e == ".jpeg" || e == ".bmp")
        return AssetType::Image;
    if (e == ".wav" || e == ".ogg" ||
        e == ".mp3" || e == ".flac")
        return AssetType::Audio;
    if (e == ".ttf" || e == ".otf") return AssetType::Font;

    return AssetType::Unknown;
}

sf::Color ContentBrowser::ColorForType(AssetType type)
{
    switch (type)
    {
        case AssetType::Folder: return sf::Color(235, 190, 85);
        case AssetType::Script: return sf::Color(100, 215, 130);
        case AssetType::Scene: return sf::Color(90, 170, 255);
        case AssetType::Image: return sf::Color(215, 115, 230);
        case AssetType::Audio: return sf::Color(255, 145, 75);
        case AssetType::Font: return sf::Color(75, 220, 210);
        default: return sf::Color(150, 155, 175);
    }
}

std::string ContentBrowser::LabelForType(AssetType type)
{
    switch (type)
    {
        case AssetType::Folder: return "DIR";
        case AssetType::Script: return "LUA";
        case AssetType::Scene: return "SCENE";
        case AssetType::Image: return "IMG";
        case AssetType::Audio: return "SFX";
        case AssetType::Font: return "FONT";
        default: return "FILE";
    }
}

std::string ContentBrowser::FormatFileSize(uintmax_t bytes)
{
    if (bytes < 1024)
        return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024)
        return std::to_string(bytes / 1024) + " KB";
    return std::to_string(bytes / (1024 * 1024)) + " MB";
}
