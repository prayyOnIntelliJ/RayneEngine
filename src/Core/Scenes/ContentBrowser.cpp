#include "ContentBrowser.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <chrono>
#include <cmath>

#include "SFML/Window/Event.hpp"
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

    std::cout << "[INFO] [ContentBrowser] Refreshed directory: " << m_CurrentPath << " (Found " << dirs.size() << " folders, " << files.size() << " files)\n";

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

    const float cardW = 86.f;
    const float cardH = 92.f;
    const float pad = 10.f;
    const float gridStartY = y + toolbarH + 8.f;
    const float gridHeight = height - toolbarH - 10.f;

    float ix = x + pad;
    float iy = gridStartY - m_ScrollOffset;
    float totalContentHeight = 0.f;

    for (size_t i = 0; i < m_FilteredEntries.size(); ++i)
    {
        const auto &entry = m_FilteredEntries[i];

        sf::FloatRect cardRect(ix, iy, cardW, cardH);
        m_ItemBounds.emplace_back(cardRect, i);

        if (iy + cardH >= gridStartY && iy <= y + height)
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
        actions.push_back({"New Script Here", "new_script"});
        actions.push_back({"Reveal in Explorer", "reveal"});
        if (!isDir) { actions.push_back({"Delete", "delete"}); }

        const float menuW = 140.f;
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

    if (m_Drag.active)
    {
        m_Drag.pos = m_MousePos;
        sf::RectangleShape ghost({100.f, 24.f});
        ghost.setPosition(m_Drag.pos.x + 10.f, m_Drag.pos.y - 12.f);
        ghost.setFillColor(sf::Color(30, 35, 55, 230));
        ghost.setOutlineColor(ColorForType(m_Drag.type));
        ghost.setOutlineThickness(1.5f);
        window.draw(ghost);

        sf::Text ghostText;
        ghostText.setFont(m_Font);
        ghostText.setCharacterSize(10);
        ghostText.setFillColor(sf::Color(220, 230, 255));
        std::string gname = fs::path(m_Drag.path).filename().string();
        if (gname.size() > 14) gname = gname.substr(0, 13) + "...";
        ghostText.setString(gname);
        ghostText.setPosition(m_Drag.pos.x + 16.f, m_Drag.pos.y - 8.f);
        window.draw(ghostText);
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
        file.close();
        std::cout << "[INFO] [ContentBrowser] Created new Lua script file: " << fullPath << "\n";
    }

    Refresh();
    m_SelectedPath = fullPath;
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
    std::error_code ec;
    fs::remove_all(path, ec);
    if (!ec)
    {
        std::cout << "[INFO] [ContentBrowser] Deleted file/folder: " << path << "\n";
        Refresh();
    }
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
        action == "reveal") { RevealInExplorer(m_ContextMenuTarget); } else if (action == "new_script")
    {
        m_NewScriptPrompt = true;
        m_NewScriptName = "new_script";
    } else if (action == "delete") { DeleteAsset(m_ContextMenuTarget); }
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
