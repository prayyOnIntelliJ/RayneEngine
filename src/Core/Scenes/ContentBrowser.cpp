#include "ContentBrowser.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <chrono>
#include <cmath>
#include <set>

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

static const sf::Color C_BG_CANVAS = sf::Color(18, 20, 23);
static const sf::Color C_BG_PANEL = sf::Color(26, 29, 34);
static const sf::Color C_BG_ELEVATED = sf::Color(33, 37, 43);
static const sf::Color C_BG_INPUT = sf::Color(20, 23, 27);
static const sf::Color C_BORDER = sf::Color(42, 46, 53);
static const sf::Color C_BORDER_LIGHT = sf::Color(58, 63, 72);
static const sf::Color C_TEXT_PRIMARY = sf::Color(232, 234, 237);
static const sf::Color C_TEXT_SECONDARY = sf::Color(154, 160, 172);
static const sf::Color C_TEXT_MUTED = sf::Color(92, 97, 107);
static const sf::Color C_ACCENT = sf::Color(124, 108, 240);
static const sf::Color C_ACCENT_HOV = sf::Color(146, 132, 245);
static const sf::Color C_ACCENT_ACT = sf::Color(100, 85, 217);
static const sf::Color C_ACCENT_DIM = sf::Color(40, 35, 80, 200);
static const sf::Color C_ACCENT_BRIGHT = sf::Color(146, 132, 245);
static const sf::Color C_ACCENT2 = sf::Color(67, 217, 200);
static const sf::Color C_SUCCESS = sf::Color(74, 222, 128);
static const sf::Color C_SUCCESS_DIM = sf::Color(20, 55, 35, 200);
static const sf::Color C_WARNING = sf::Color(245, 185, 77);
static const sf::Color C_DANGER = sf::Color(241, 104, 94);
static const sf::Color C_DANGER_DIM = sf::Color(70, 20, 18, 200);
static const sf::Color C_GRID_MINOR = sf::Color(38, 43, 51);
static const sf::Color C_GRID_MAJOR = sf::Color(51, 58, 69);

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

bool ContentBrowser::IsBuildMode() const
{
    if (m_ForceBuildMode.has_value()) return m_ForceBuildMode.value();

    std::error_code ec;
    fs::path root(m_RootPath);
    fs::path projRoot = root.parent_path();
    // If CMakeLists.txt or src exists in project root (or root), we are running in the source / dev environment
    if (fs::exists(projRoot / "CMakeLists.txt", ec) || fs::exists(root / "CMakeLists.txt", ec)) return false;
    if (fs::exists(projRoot / "src", ec) || fs::exists(root / "src", ec)) return false;

    return true;
}

bool ContentBrowser::IsReadOnlyPath(const std::string &path) const
{
    // Read-only is only enforced in the build environment!
    if (!IsBuildMode()) return false;

    std::error_code ec;
    fs::path p = fs::canonical(path, ec);
    if (ec) p = fs::path(path);

    fs::path rootP = fs::canonical(m_RootPath, ec);
    if (ec) rootP = fs::path(m_RootPath);

    fs::path scriptP = fs::canonical(rootP / "scripting", ec);
    if (ec) scriptP = rootP / "scripting";

    auto isSubOrEqual = [](const fs::path &child, const fs::path &parent) {
        std::string cStr = child.generic_string();
        std::string pStr = parent.generic_string();
        if (cStr == pStr) return true;
        if (cStr.size() > pStr.size() && cStr.compare(0, pStr.size(), pStr) == 0)
        {
            if (cStr[pStr.size()] == '/') return true;
        }
        return false;
    };

    if (isSubOrEqual(p, scriptP))
        return true;

    for (auto it = p; it != rootP && it.has_parent_path(); it = it.parent_path())
    {
        std::string fn = it.filename().string();
        std::transform(fn.begin(), fn.end(), fn.begin(), ::tolower);
        if (fn == "scripting") return true;
    }

    return false;
}

bool ContentBrowser::IsCurrentPathReadOnly() const
{
    return IsReadOnlyPath(m_CurrentPath);
}

bool ContentBrowser::IsIgnoredEntry(const std::string &name, const std::string &fullPath, bool isDirectory) const
{
    if (name.empty()) return true;
    if (name[0] == '.') return true;

    return false;
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
        std::string name = entry.path().filename().string();
        std::string fullPath = entry.path().string();
        bool isDir = entry.is_directory(ec);
        if (ec)
        {
            ec.clear();
            continue;
        }

        if (IsIgnoredEntry(name, fullPath, isDir))
            continue;

        ContentEntry ce;
        ce.name = name;
        ce.fullPath = fullPath;
        ce.isDirectory = isDir;
        ce.isReadOnly = IsReadOnlyPath(fullPath);

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
            ce.type = TypeFromFile(entry.path().string());
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

    if (m_SearchActive &&event.type == sf::Event::TextEntered) {
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

    if (m_NewScriptPrompt &&event.type == sf::Event::TextEntered) {
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

    if (m_NewScenePrompt &&event.type == sf::Event::TextEntered) {
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

    if (m_NewFolderPrompt &&event.type == sf::Event::TextEntered) {
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

    if (m_RenamePrompt &&event.type == sf::Event::TextEntered) {
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

    if (m_DeletePrompt &&event.type == sf::Event::TextEntered) {
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
            if (m_DeletePrompt &&event.key.code == sf::Keyboard::Enter) {
                DeleteAsset(m_DeleteTarget);
                m_DeletePrompt = false;
                return;
            }
        } else if (!m_SearchActive)
        {
            if (event.key.code == sf::Keyboard::F2 && !m_SelectedPath.empty())
            {
                if (IsReadOnlyPath(m_SelectedPath))
                {
                    SetStatusMessage("Cannot rename read-only asset");
                    return;
                }
                m_RenameTarget = m_SelectedPath;
                m_RenameInput = fs::path(m_SelectedPath).filename().string();
                m_RenamePrompt = true;
                return;
            }
            if (event.key.code == sf::Keyboard::Delete && !m_SelectedPath.empty())
            {
                if (IsReadOnlyPath(m_SelectedPath))
                {
                    SetStatusMessage("Cannot delete read-only asset");
                    return;
                }
                m_DeleteTarget = m_SelectedPath;
                m_DeletePrompt = true;
                return;
            }
            if (event.key.control && event.key.code == sf::Keyboard::D && !m_SelectedPath.empty())
            {
                if (IsReadOnlyPath(m_SelectedPath))
                {
                    SetStatusMessage("Cannot duplicate read-only asset");
                    return;
                }
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
            if (IsCurrentPathReadOnly())
            {
                SetStatusMessage("Cannot create folder in read-only directory");
                return;
            }
            m_NewFolderPrompt = true;
            m_NewFolderName = "NewFolder";
            return;
        }

        if (m_NewScriptBtnBounds.contains(mouseScreenPos))
        {
            if (IsCurrentPathReadOnly())
            {
                SetStatusMessage("Cannot create script in read-only directory");
                return;
            }
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

    const bool blink = (static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count() / 500) % 2) == 0;
    const std::string cursor = blink ? "|" : "";

    sf::RectangleShape bg({width, height});
    bg.setPosition(x, y);
    bg.setFillColor(sf::Color(C_BG_CANVAS.r, C_BG_CANVAS.g, C_BG_CANVAS.b, 252));
    bg.setOutlineColor(C_BORDER);
    bg.setOutlineThickness(1.f);
    window.draw(bg);

    const float toolbarH = 32.f;
    sf::RectangleShape toolbar({width, toolbarH});
    toolbar.setPosition(x, y);
    toolbar.setFillColor(C_BG_PANEL);
    window.draw(toolbar);

    sf::RectangleShape toolbarBorder({width, 1.f});
    toolbarBorder.setPosition(x, y + toolbarH);
    toolbarBorder.setFillColor(C_BORDER);
    window.draw(toolbarBorder);

    float curX = x + 8.f;
    const float curY = y + 5.f;

    bool canGoUp = (m_CurrentPath != m_RootPath);
    m_UpBtnBounds = sf::FloatRect(curX, curY, 24.f, 22.f);
    bool upHovered = m_UpBtnBounds.contains(m_MousePos) && canGoUp;

    sf::RectangleShape upBtn({m_UpBtnBounds.width, m_UpBtnBounds.height});
    upBtn.setPosition(m_UpBtnBounds.left, m_UpBtnBounds.top);
    upBtn.setFillColor(canGoUp ? (upHovered ? C_BG_ELEVATED : C_BORDER) : C_BG_PANEL);
    upBtn.setOutlineColor(upHovered ? C_BORDER_LIGHT : C_BORDER);
    upBtn.setOutlineThickness(1.f);
    window.draw(upBtn);

    sf::Text upText;
    upText.setFont(m_Font);
    upText.setCharacterSize(12);
    upText.setFillColor(canGoUp ? (upHovered ? sf::Color::White : C_TEXT_SECONDARY) : C_TEXT_MUTED);
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
        crumbBg.setFillColor(isHovered ? C_BG_ELEVATED : C_BG_INPUT);
        crumbBg.setOutlineColor(isHovered ? C_BORDER_LIGHT : C_BORDER);
        crumbBg.setOutlineThickness(1.f);
        window.draw(crumbBg);

        cText.setFillColor(isLast ? C_TEXT_PRIMARY : isHovered ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
        cText.setPosition(curX + 6.f, curY + 4.f);
        window.draw(cText);

        curX += tw + 4.f;

        if (!isLast)
        {
            sf::Text sep;
            sep.setFont(m_Font);
            sep.setCharacterSize(11);
            sep.setFillColor(C_TEXT_MUTED);
            sep.setString("/");
            sep.setPosition(curX, curY + 4.f);
            window.draw(sep);
            curX += 10.f;
        }
    }

    bool isReadOnlyDir = IsCurrentPathReadOnly();
    float rightX = x + width - 8.f;

    rightX -= 26.f;
    m_RefreshBtnBounds = sf::FloatRect(rightX, curY, 26.f, 22.f);
    bool refHover = m_RefreshBtnBounds.contains(m_MousePos);
    sf::RectangleShape refBtn({m_RefreshBtnBounds.width, m_RefreshBtnBounds.height});
    refBtn.setPosition(m_RefreshBtnBounds.left, m_RefreshBtnBounds.top);
    refBtn.setFillColor(refHover ? C_ACCENT_DIM : C_BG_INPUT);
    refBtn.setOutlineColor(refHover ? C_ACCENT_HOV : C_ACCENT);
    refBtn.setOutlineThickness(1.f);
    window.draw(refBtn);

    sf::Text refText;
    refText.setFont(m_Font);
    refText.setCharacterSize(12);
    refText.setFillColor(refHover ? C_TEXT_PRIMARY : C_ACCENT_BRIGHT);
    refText.setString("R");
    refText.setPosition(m_RefreshBtnBounds.left + 8.f, m_RefreshBtnBounds.top + 2.f);
    window.draw(refText);

    rightX -= 68.f;
    m_NewScriptBtnBounds = sf::FloatRect(rightX, curY, 62.f, 22.f);
    bool newScriptHover = m_NewScriptBtnBounds.contains(m_MousePos) && !isReadOnlyDir;
    sf::RectangleShape newScriptBtn({m_NewScriptBtnBounds.width, m_NewScriptBtnBounds.height});
    newScriptBtn.setPosition(m_NewScriptBtnBounds.left, m_NewScriptBtnBounds.top);
    newScriptBtn.setFillColor(isReadOnlyDir ? C_BG_PANEL : (newScriptHover ? C_BG_ELEVATED : C_BG_INPUT));
    newScriptBtn.setOutlineColor(isReadOnlyDir ? C_BORDER : (newScriptHover ? C_BORDER_LIGHT : C_BORDER));
    newScriptBtn.setOutlineThickness(1.f);
    window.draw(newScriptBtn);

    sf::Text newScriptText;
    newScriptText.setFont(m_Font);
    newScriptText.setCharacterSize(10);
    newScriptText.setFillColor(isReadOnlyDir ? C_TEXT_MUTED : (newScriptHover ? C_TEXT_PRIMARY : C_TEXT_SECONDARY));
    newScriptText.setString("+ Script");
    newScriptText.setPosition(m_NewScriptBtnBounds.left + 8.f, m_NewScriptBtnBounds.top + 4.f);
    window.draw(newScriptText);

    rightX -= 64.f;
    m_NewFolderBtnBounds = sf::FloatRect(rightX, curY, 58.f, 22.f);
    bool newFolderHover = m_NewFolderBtnBounds.contains(m_MousePos) && !isReadOnlyDir;
    sf::RectangleShape newFolderBtn({m_NewFolderBtnBounds.width, m_NewFolderBtnBounds.height});
    newFolderBtn.setPosition(m_NewFolderBtnBounds.left, m_NewFolderBtnBounds.top);
    newFolderBtn.setFillColor(isReadOnlyDir ? C_BG_PANEL : (newFolderHover ? C_BG_ELEVATED : C_BG_INPUT));
    newFolderBtn.setOutlineColor(isReadOnlyDir ? C_BORDER : (newFolderHover ? C_BORDER_LIGHT : C_BORDER));
    newFolderBtn.setOutlineThickness(1.f);
    window.draw(newFolderBtn);

    sf::Text newFolderText;
    newFolderText.setFont(m_Font);
    newFolderText.setCharacterSize(10);
    newFolderText.setFillColor(isReadOnlyDir ? C_TEXT_MUTED : (newFolderHover ? C_TEXT_PRIMARY : C_TEXT_SECONDARY));
    newFolderText.setString("+ Folder");
    newFolderText.setPosition(m_NewFolderBtnBounds.left + 7.f, m_NewFolderBtnBounds.top + 4.f);
    window.draw(newFolderText);

    rightX -= 120.f;
    m_SearchBoxBounds = sf::FloatRect(rightX, curY, 114.f, 22.f);
    bool searchHover = m_SearchBoxBounds.contains(m_MousePos);
    sf::RectangleShape searchBox({m_SearchBoxBounds.width, m_SearchBoxBounds.height});
    searchBox.setPosition(m_SearchBoxBounds.left, m_SearchBoxBounds.top);
    searchBox.setFillColor(m_SearchActive
                               ? C_BG_ELEVATED
                               : searchHover
                                     ? C_BG_ELEVATED
                                     : C_BG_INPUT);
    searchBox.setOutlineColor(m_SearchActive
                                  ? C_ACCENT
                                  : searchHover
                                        ? C_BORDER_LIGHT
                                        : C_BORDER);
    searchBox.setOutlineThickness(1.f);
    window.draw(searchBox);

    sf::Text searchContent;
    searchContent.setFont(m_Font);
    searchContent.setCharacterSize(10);
    if (m_SearchQuery.empty() && !m_SearchActive)
    {
        searchContent.setFillColor(C_TEXT_MUTED);
        searchContent.setString("Search...");
    } else
    {
        searchContent.setFillColor(C_TEXT_PRIMARY);
        searchContent.setString(m_SearchQuery + (m_SearchActive ? cursor : ""));
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
        clearX.setFillColor(C_TEXT_MUTED);
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
        fChip.setFillColor(isActive ? C_ACCENT_DIM : isHover ? C_BG_ELEVATED : C_BG_PANEL);
        fChip.setOutlineColor(isActive
                                  ? C_ACCENT
                                  : isHover
                                        ? C_BORDER_LIGHT
                                        : C_BORDER);
        fChip.setOutlineThickness(1.f);
        window.draw(fChip);

        sf::Text fText;
        fText.setFont(m_Font);
        fText.setCharacterSize(10);
        fText.setFillColor(isActive
                               ? C_TEXT_PRIMARY
                               : isHover
                                     ? C_TEXT_PRIMARY
                                     : C_TEXT_SECONDARY);
        fText.setString(filters[i].label);
        fText.setPosition(fRect.left + (fRect.width - fText.getLocalBounds().width) / 2.f, fRect.top + 4.f);
        window.draw(fText);
    }

    const float statusBarH = 22.f;
    const float cardW = 100.f;
    const float cardH = 110.f;
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

            // Hover lift effect: shift card 2px up when hovered
            float liftY = (hovered && !selected) ? -2.f : 0.f;
            float drawY = iy + liftY;

            const float radius = 6.f;

            // Glow effect behind card on hover
            if (hovered || selected)
            {
                sf::Color glowColor = selected ? sf::Color(C_ACCENT.r, C_ACCENT.g, C_ACCENT.b, 35)
                                               : sf::Color(C_ACCENT.r, C_ACCENT.g, C_ACCENT.b, 20);
                DrawRoundedRect(window, ix - 3.f, drawY - 3.f, cardW + 6.f, cardH + 6.f,
                                radius + 3.f, glowColor);
            }

            // Card background with rounded corners
            sf::Color cardFill = selected
                                     ? C_ACCENT_DIM
                                     : hovered
                                           ? C_BG_ELEVATED
                                           : sf::Color(C_BG_PANEL.r, C_BG_PANEL.g, C_BG_PANEL.b, 200);
            sf::Color cardOutline = selected
                                        ? C_ACCENT
                                        : hovered
                                              ? C_ACCENT
                                              : C_BORDER;
            float outlineThick = selected ? 1.5f : 1.f;
            DrawRoundedRect(window, ix, drawY, cardW, cardH, radius, cardFill, cardOutline, outlineThick);

            const float previewW = 80.f;
            const float previewH = 60.f;
            const float previewX = ix + (cardW - previewW) / 2.f;
            const float previewY = drawY + 6.f;

            if (entry.type == AssetType::Image)
            {
                auto tex = ResourceManager::Get().GetTexture(entry.fullPath);
                if (tex && tex->getSize().x > 0 && tex->getSize().y > 0)
                {
                    // Subtle gradient background for image preview area
                    sf::Color imgColor = ColorForType(AssetType::Image);
                    DrawGradientRect(window, previewX, previewY, previewW, previewH,
                                     sf::Color(imgColor.r, imgColor.g, imgColor.b, 15),
                                     sf::Color(imgColor.r, imgColor.g, imgColor.b, 5));

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
                    previewFrame.setOutlineColor(sf::Color(C_BORDER_LIGHT.r, C_BORDER_LIGHT.g, C_BORDER_LIGHT.b, 120));
                    previewFrame.setOutlineThickness(1.f);
                    window.draw(previewFrame);
                }

                // Extension badge for images
                std::string ext = ExtensionLabel(entry.fullPath, entry.type);
                if (!ext.empty())
                    DrawExtensionBadge(window, ext, ColorForType(entry.type),
                                       ix + cardW - 4.f, previewY + previewH - 2.f);
            } else
            {
                sf::Color typeColor = ColorForType(entry.type);

                // Gradient background for the preview area
                DrawGradientRect(window, previewX, previewY, previewW, previewH,
                                 sf::Color(typeColor.r, typeColor.g, typeColor.b, 30),
                                 sf::Color(typeColor.r, typeColor.g, typeColor.b, 8));

                // Subtle border around preview area
                sf::RectangleShape previewBorder({previewW, previewH});
                previewBorder.setPosition(previewX, previewY);
                previewBorder.setFillColor(sf::Color::Transparent);
                previewBorder.setOutlineColor(sf::Color(typeColor.r, typeColor.g, typeColor.b, 50));
                previewBorder.setOutlineThickness(1.f);
                window.draw(previewBorder);

                // Hover glow inside preview area
                if (hovered)
                {
                    DrawGradientRect(window, previewX, previewY, previewW, previewH,
                                     sf::Color(typeColor.r, typeColor.g, typeColor.b, 20),
                                     sf::Color(typeColor.r, typeColor.g, typeColor.b, 45));
                }

                // Draw unique procedural icon silhouette
                float iconCX = previewX + previewW / 2.f;
                float iconCY = previewY + previewH / 2.f;
                float iconSize = 28.f;

                DrawIconForType(window, entry.type, typeColor, iconCX, iconCY, iconSize);

                // Folder content dots
                if (entry.type == AssetType::Folder)
                {
                    DrawFolderContentDots(window, entry.fullPath, iconCX, previewY + previewH - 6.f);
                }

                // Extension badge (bottom-right of preview area)
                if (entry.type != AssetType::Folder)
                {
                    std::string ext = ExtensionLabel(entry.fullPath, entry.type);
                    if (!ext.empty())
                        DrawExtensionBadge(window, ext, typeColor,
                                           ix + cardW - 4.f, previewY + previewH - 2.f);
                }
            }

            // Name label
            sf::Text nameText;
            nameText.setFont(m_Font);
            nameText.setCharacterSize(10);
            nameText.setFillColor(selected
                                      ? C_TEXT_PRIMARY
                                      : hovered
                                            ? sf::Color::White
                                            : C_TEXT_SECONDARY);

            std::string displayName = entry.name;
            if (displayName.size() > 12) displayName = displayName.substr(0, 11) + "...";
            nameText.setString(displayName);
            nameText.setPosition(ix + (cardW - nameText.getLocalBounds().width) / 2.f, drawY + 72.f);
            window.draw(nameText);

            // Subtitle (Folder or file size)
            sf::Text subText;
            subText.setFont(m_Font);
            subText.setCharacterSize(8);
            subText.setFillColor(C_TEXT_MUTED);
            if (entry.isDirectory)
                subText.setString(entry.isReadOnly ? "Folder [RO]" : "Folder");
            else
                subText.setString(entry.isReadOnly ? (FormatFileSize(entry.fileSize) + " [RO]") : FormatFileSize(entry.fileSize));
            subText.setPosition(ix + (cardW - subText.getLocalBounds().width) / 2.f, drawY + 88.f);
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
        emptyText.setFillColor(C_TEXT_MUTED);
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
        track.setFillColor(C_BG_INPUT);
        window.draw(track);

        float thumbRatio = gridHeight / (gridHeight + m_MaxScroll);
        float thumbH = std::max(18.f, sbH * thumbRatio);
        float thumbY = sbY + (m_ScrollOffset / m_MaxScroll) * (sbH - thumbH);

        sf::RectangleShape thumb({sbW, thumbH});
        thumb.setPosition(sbX, thumbY);
        thumb.setFillColor(C_BORDER_LIGHT);
        window.draw(thumb);
    }

    const float statusBarY = y + height - statusBarH;
    sf::RectangleShape statusBg({width, statusBarH});
    statusBg.setPosition(x, statusBarY);
    statusBg.setFillColor(C_BG_PANEL);
    statusBg.setOutlineColor(C_BORDER);
    statusBg.setOutlineThickness(1.f);
    window.draw(statusBg);

    sf::Text statusText;
    statusText.setFont(m_Font);
    statusText.setCharacterSize(10);

    double now = CurrentTimeSeconds();
    if (!m_StatusMessage.empty() && (now - m_StatusMessageTime < 3.5))
    {
        statusText.setFillColor(C_SUCCESS);
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
            statusText.setFillColor(C_TEXT_PRIMARY);
            std::string infoStr = "[" + LabelForType(infoEntry->type) + "] " + infoEntry->name;
            if (!infoEntry->isDirectory)
                infoStr += "  |  " + FormatFileSize(infoEntry->fileSize);
            if (infoEntry->isReadOnly)
                infoStr += "  |  [Read-Only]";
            statusText.setString(infoStr);
        } else
        {
            statusText.setFillColor(C_TEXT_MUTED);
            std::string countStr = std::to_string(m_FilteredEntries.size()) + " items";
            if (m_FilteredEntries.size() != m_Entries.size())
                countStr += " (filtered from " + std::to_string(m_Entries.size()) + ")";
            if (IsCurrentPathReadOnly())
                countStr += "  |  [Read-Only Folder]";
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
                                   : TypeFromFile(m_ContextMenuTarget);

        bool targetReadOnly = IsReadOnlyPath(m_ContextMenuTarget);
        bool currentReadOnly = IsCurrentPathReadOnly();

        std::vector<std::pair<std::string, std::string> > actions;
        if (!isDir)
        {
            actions.push_back({"Open", "open"});
            if (targetType == AssetType::Audio)
                actions.push_back({"Play Audio", "play_audio"});
        }
        if (m_ContextMenuTarget != m_CurrentPath)
        {
            if (!targetReadOnly)
            {
                actions.push_back({"Rename (F2)", "rename"});
                if (!isDir) { actions.push_back({"Duplicate (Ctrl+D)", "duplicate"}); }
            }
            actions.push_back({"Copy Relative Path (Ctrl+C)", "copy_path"});
        }
        if (!currentReadOnly)
        {
            actions.push_back({"New Folder", "new_folder"});
            actions.push_back({"New Script", "new_script"});
            actions.push_back({"New Scene", "new_scene"});
        }
        actions.push_back({"Reveal in Explorer", "reveal"});
        if (m_ContextMenuTarget != m_CurrentPath && m_ContextMenuTarget != m_RootPath && !targetReadOnly)
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
        menuBg.setFillColor(C_BG_ELEVATED);
        menuBg.setOutlineColor(C_BORDER_LIGHT);
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
                rowHighlight.setFillColor(act == "delete" ? C_DANGER_DIM : C_ACCENT_DIM);
                window.draw(rowHighlight);
            }

            sf::Text rowText;
            rowText.setFont(m_Font);
            rowText.setCharacterSize(10);
            rowText.setFillColor(act == "delete"
                                     ? C_DANGER
                                     : isRowHover
                                           ? sf::Color::White
                                           : C_TEXT_PRIMARY);
            rowText.setString(label);
            rowText.setPosition(itemRect.left + 8.f, itemRect.top + 4.f);
            window.draw(rowText);

            rowY += itemRowH;
        }
    }

    if (m_NewScriptPrompt)
    {
        sf::Text title;
        title.setFont(m_Font);
        title.setCharacterSize(11);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(C_TEXT_PRIMARY);
        title.setString("Create Lua Script (Enter to save, Esc to cancel)");

        const float titleW = title.getLocalBounds().width;
        const float modalW = std::max(380.f, titleW + 36.f);
        const float modalH = 84.f;
        const float modalX = x + (width - modalW) / 2.f;
        const float modalY = y + (height - modalH) / 2.f;

        sf::RectangleShape modalDim({width, height});
        modalDim.setPosition(x, y);
        modalDim.setFillColor(sf::Color(0, 0, 0, 160));
        window.draw(modalDim);

        sf::RectangleShape modalBg({modalW, modalH});
        modalBg.setPosition(modalX, modalY);
        modalBg.setFillColor(C_BG_ELEVATED);
        modalBg.setOutlineColor(C_BORDER_LIGHT);
        modalBg.setOutlineThickness(1.5f);
        window.draw(modalBg);

        title.setPosition(modalX + 14.f, modalY + 12.f);
        window.draw(title);

        sf::RectangleShape inputField({modalW - 28.f, 26.f});
        inputField.setPosition(modalX + 14.f, modalY + 38.f);
        inputField.setFillColor(C_BG_INPUT);
        inputField.setOutlineColor(C_ACCENT);
        inputField.setOutlineThickness(1.f);
        window.draw(inputField);

        sf::Text inputText;
        inputText.setFont(m_Font);
        inputText.setCharacterSize(11);
        inputText.setFillColor(sf::Color::White);
        inputText.setString(m_NewScriptName + cursor);
        inputText.setPosition(modalX + 22.f, modalY + 43.f);
        window.draw(inputText);
    }

    if (m_NewScenePrompt)
    {
        sf::Text title;
        title.setFont(m_Font);
        title.setCharacterSize(11);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(C_SUCCESS);
        title.setString("Create Scene (Enter to save, Esc to cancel)");

        const float titleW = title.getLocalBounds().width;
        const float modalW = std::max(380.f, titleW + 36.f);
        const float modalH = 84.f;
        const float modalX = x + (width - modalW) / 2.f;
        const float modalY = y + (height - modalH) / 2.f;

        sf::RectangleShape modalDim({width, height});
        modalDim.setPosition(x, y);
        modalDim.setFillColor(sf::Color(0, 0, 0, 160));
        window.draw(modalDim);

        sf::RectangleShape modalBg({modalW, modalH});
        modalBg.setPosition(modalX, modalY);
        modalBg.setFillColor(C_BG_ELEVATED);
        modalBg.setOutlineColor(C_SUCCESS);
        modalBg.setOutlineThickness(1.5f);
        window.draw(modalBg);

        title.setPosition(modalX + 14.f, modalY + 12.f);
        window.draw(title);

        sf::RectangleShape inputField({modalW - 28.f, 26.f});
        inputField.setPosition(modalX + 14.f, modalY + 38.f);
        inputField.setFillColor(C_BG_INPUT);
        inputField.setOutlineColor(C_SUCCESS);
        inputField.setOutlineThickness(1.f);
        window.draw(inputField);

        sf::Text inputText;
        inputText.setFont(m_Font);
        inputText.setCharacterSize(11);
        inputText.setFillColor(sf::Color::White);
        inputText.setString(m_NewSceneName + cursor);
        inputText.setPosition(modalX + 22.f, modalY + 43.f);
        window.draw(inputText);
    }

    if (m_NewFolderPrompt)
    {
        sf::Text title;
        title.setFont(m_Font);
        title.setCharacterSize(11);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(C_WARNING);
        title.setString("Create Folder (Enter to create, Esc to cancel)");

        const float titleW = title.getLocalBounds().width;
        const float modalW = std::max(380.f, titleW + 36.f);
        const float modalH = 84.f;
        const float modalX = x + (width - modalW) / 2.f;
        const float modalY = y + (height - modalH) / 2.f;

        sf::RectangleShape modalDim({width, height});
        modalDim.setPosition(x, y);
        modalDim.setFillColor(sf::Color(0, 0, 0, 160));
        window.draw(modalDim);

        sf::RectangleShape modalBg({modalW, modalH});
        modalBg.setPosition(modalX, modalY);
        modalBg.setFillColor(C_BG_ELEVATED);
        modalBg.setOutlineColor(C_WARNING);
        modalBg.setOutlineThickness(1.5f);
        window.draw(modalBg);

        title.setPosition(modalX + 14.f, modalY + 12.f);
        window.draw(title);

        sf::RectangleShape inputField({modalW - 28.f, 26.f});
        inputField.setPosition(modalX + 14.f, modalY + 38.f);
        inputField.setFillColor(C_BG_INPUT);
        inputField.setOutlineColor(C_WARNING);
        inputField.setOutlineThickness(1.f);
        window.draw(inputField);

        sf::Text inputText;
        inputText.setFont(m_Font);
        inputText.setCharacterSize(11);
        inputText.setFillColor(sf::Color::White);
        inputText.setString(m_NewFolderName + cursor);
        inputText.setPosition(modalX + 22.f, modalY + 43.f);
        window.draw(inputText);
    }

    if (m_RenamePrompt)
    {
        sf::Text title;
        title.setFont(m_Font);
        title.setCharacterSize(11);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(C_TEXT_PRIMARY);
        title.setString("Rename (Enter to save, Esc to cancel)");

        const float titleW = title.getLocalBounds().width;
        const float modalW = std::max(380.f, titleW + 36.f);
        const float modalH = 84.f;
        const float modalX = x + (width - modalW) / 2.f;
        const float modalY = y + (height - modalH) / 2.f;

        sf::RectangleShape modalDim({width, height});
        modalDim.setPosition(x, y);
        modalDim.setFillColor(sf::Color(0, 0, 0, 170));
        window.draw(modalDim);

        sf::RectangleShape modalBg({modalW, modalH});
        modalBg.setPosition(modalX, modalY);
        modalBg.setFillColor(C_BG_ELEVATED);
        modalBg.setOutlineColor(C_TEXT_PRIMARY);
        modalBg.setOutlineThickness(1.5f);
        window.draw(modalBg);

        title.setPosition(modalX + 14.f, modalY + 12.f);
        window.draw(title);

        sf::RectangleShape inputField({modalW - 28.f, 26.f});
        inputField.setPosition(modalX + 14.f, modalY + 38.f);
        inputField.setFillColor(C_BG_INPUT);
        inputField.setOutlineColor(C_ACCENT);
        inputField.setOutlineThickness(1.f);
        window.draw(inputField);

        sf::Text inputText;
        inputText.setFont(m_Font);
        inputText.setCharacterSize(11);
        inputText.setFillColor(sf::Color::White);
        inputText.setString(m_RenameInput + cursor);
        inputText.setPosition(modalX + 22.f, modalY + 43.f);
        window.draw(inputText);
    }

    if (m_DeletePrompt)
    {
        sf::Text title;
        title.setFont(m_Font);
        title.setCharacterSize(11);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(C_DANGER);
        title.setString("Delete Item? (Enter to delete, Esc to cancel)");

        const float titleW = title.getLocalBounds().width;
        const float modalW = std::max(380.f, titleW + 36.f);
        const float modalH = 88.f;
        const float modalX = x + (width - modalW) / 2.f;
        const float modalY = y + (height - modalH) / 2.f;

        sf::RectangleShape modalDim({width, height});
        modalDim.setPosition(x, y);
        modalDim.setFillColor(sf::Color(0, 0, 0, 170));
        window.draw(modalDim);

        sf::RectangleShape modalBg({modalW, modalH});
        modalBg.setPosition(modalX, modalY);
        modalBg.setFillColor(C_BG_ELEVATED);
        modalBg.setOutlineColor(C_DANGER);
        modalBg.setOutlineThickness(1.5f);
        window.draw(modalBg);

        title.setPosition(modalX + 14.f, modalY + 12.f);
        window.draw(title);

        std::string fname = fs::path(m_DeleteTarget).filename().string();
        if (fname.size() > 38) fname = fname.substr(0, 36) + "...";

        sf::Text info;
        info.setFont(m_Font);
        info.setCharacterSize(11);
        info.setFillColor(C_TEXT_PRIMARY);
        info.setString("\"" + fname + "\"");
        info.setPosition(modalX + 14.f, modalY + 38.f);
        window.draw(info);

        sf::Text sub;
        sub.setFont(m_Font);
        sub.setCharacterSize(9);
        sub.setFillColor(C_TEXT_SECONDARY);
        sub.setString("This action cannot be undone.");
        sub.setPosition(modalX + 14.f, modalY + 62.f);
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
        const float ghostW = thumbSz + 4.f;
        const float ghostH = thumbSz + 22.f;

        // Shadow
        DrawRoundedRect(window, ox + 3.f, oy + 3.f, ghostW, ghostH, 4.f,
                        sf::Color(0, 0, 0, 100));

        // Card
        DrawRoundedRect(window, ox, oy, ghostW, ghostH, 4.f,
                        C_BG_ELEVATED, accent, 1.5f);

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
            ph.setFillColor(C_BORDER);
            window.draw(ph);
        }

        if (gname.size() > 12) gname = gname.substr(0, 11) + "..";
        sf::Text label;
        label.setFont(m_Font);
        label.setCharacterSize(9);
        label.setFillColor(C_TEXT_PRIMARY);
        label.setString(gname);
        label.setPosition(ox + 3.f, oy + thumbSz + 7.f);
        window.draw(label);
    } else
    {
        if (gname.size() > 18) gname = gname.substr(0, 17) + "...";
        const float pw = 140.f, ph = 30.f;

        // Shadow
        DrawRoundedRect(window, ox + 2.f, oy + 2.f, pw + 2.f, ph + 2.f, 6.f,
                        sf::Color(0, 0, 0, 80));

        // Pill background
        DrawRoundedRect(window, ox, oy, pw, ph, 6.f,
                        C_BG_ELEVATED, accent, 1.5f);

        // Draw mini icon silhouette instead of plain dot
        DrawIconForType(window, m_Drag.type, accent,
                        ox + 15.f, oy + ph / 2.f, 10.f);

        sf::Text label;
        label.setFont(m_Font);
        label.setCharacterSize(10);
        label.setFillColor(C_TEXT_PRIMARY);
        label.setString(gname);
        label.setPosition(ox + 28.f, oy + 8.f);
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

    if ((entry.type == AssetType::Scene || entry.type == AssetType::UIScene) && onSceneLoadRequest)
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
    if (IsCurrentPathReadOnly())
    {
        SetStatusMessage("Cannot create script in read-only directory");
        return;
    }
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
    if (IsCurrentPathReadOnly())
    {
        SetStatusMessage("Cannot create scene in read-only directory");
        return;
    }
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
    if (IsCurrentPathReadOnly())
    {
        SetStatusMessage("Cannot create folder in read-only directory");
        return;
    }
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
    if (IsReadOnlyPath(oldPath))
    {
        SetStatusMessage("Cannot rename read-only asset");
        return;
    }
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
    if (IsReadOnlyPath(path))
    {
        SetStatusMessage("Cannot duplicate read-only asset");
        return;
    }
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
    if (path == m_RootPath || path.empty() || IsReadOnlyPath(path))
    {
        SetStatusMessage("Cannot delete read-only asset");
        return;
    }
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
                      : TypeFromFile(m_ContextMenuTarget);
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

AssetType ContentBrowser::TypeFromFile(const std::string &fullPath)
{
    std::string e = fs::path(fullPath).extension().string();
    std::transform(e.begin(), e.end(), e.begin(), ::tolower);

    if (e == ".lua") return AssetType::Script;
    if (e == ".png" || e == ".jpg" ||
        e == ".jpeg" || e == ".bmp")
        return AssetType::Image;
    if (e == ".wav" || e == ".ogg" ||
        e == ".mp3" || e == ".flac")
        return AssetType::Audio;
    if (e == ".ttf" || e == ".otf") return AssetType::Font;

    if (e == ".json")
    {
        // Read the first few lines to determine if it's a UI scene or regular scene
        std::ifstream file(fullPath);
        if (file.is_open())
        {
            std::string content;
            char buffer[256];
            file.read(buffer, 255);
            std::streamsize bytesRead = file.gcount();
            if (bytesRead > 0)
            {
                content.assign(buffer, bytesRead);
                if (content.find("\"ui_elements\"") != std::string::npos)
                {
                    return AssetType::UIScene;
                }
            }
        }
        return AssetType::Scene;
    }

    return AssetType::Unknown;
}

sf::Color ContentBrowser::ColorForType(AssetType type)
{
    switch (type)
    {
        case AssetType::Folder: return sf::Color(235, 190, 85);
        case AssetType::Script: return sf::Color(100, 215, 130);
        case AssetType::Scene: return sf::Color(90, 170, 255);
        case AssetType::UIScene: return sf::Color(240, 100, 140); // A pink/magenta color for UI
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
        case AssetType::UIScene: return "UI";
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

std::string ContentBrowser::ExtensionLabel(const std::string &path, AssetType type)
{
    if (type == AssetType::Scene) return "SCENE";
    if (type == AssetType::UIScene) return "UI";

    std::string ext = fs::path(path).extension().string();
    if (ext.empty()) return "";
    // Remove leading dot and uppercase
    ext = ext.substr(1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
    return "." + ext;
}

void ContentBrowser::DrawRoundedRect(sf::RenderWindow &window, float x, float y, float w, float h,
                                     float radius, sf::Color fillColor, sf::Color outlineColor,
                                     float outlineThickness)
{
    if (radius < 1.f)
    {
        sf::RectangleShape rect({w, h});
        rect.setPosition(x, y);
        rect.setFillColor(fillColor);
        rect.setOutlineColor(outlineColor);
        rect.setOutlineThickness(outlineThickness);
        window.draw(rect);
        return;
    }

    radius = std::min(radius, std::min(w, h) / 2.f);
    const int segments = 6; // segments per corner arc

    sf::ConvexShape shape;
    shape.setPointCount(segments * 4);

    // Top-right corner
    for (int i = 0; i < segments; ++i)
    {
        float angle = static_cast<float>(i) / static_cast<float>(segments - 1) * 90.f;
        float rad = angle * 3.14159265f / 180.f;
        shape.setPoint(i, sf::Vector2f(
            x + w - radius + std::cos(rad) * radius,
            y + radius - std::sin(rad) * radius
        ));
    }
    // Top-left corner
    for (int i = 0; i < segments; ++i)
    {
        float angle = static_cast<float>(i) / static_cast<float>(segments - 1) * 90.f;
        float rad = (90.f + angle) * 3.14159265f / 180.f;
        shape.setPoint(segments + i, sf::Vector2f(
            x + radius + std::cos(rad) * radius,
            y + radius - std::sin(rad) * radius
        ));
    }
    // Bottom-left corner
    for (int i = 0; i < segments; ++i)
    {
        float angle = static_cast<float>(i) / static_cast<float>(segments - 1) * 90.f;
        float rad = (180.f + angle) * 3.14159265f / 180.f;
        shape.setPoint(segments * 2 + i, sf::Vector2f(
            x + radius + std::cos(rad) * radius,
            y + h - radius - std::sin(rad) * radius
        ));
    }
    // Bottom-right corner
    for (int i = 0; i < segments; ++i)
    {
        float angle = static_cast<float>(i) / static_cast<float>(segments - 1) * 90.f;
        float rad = (270.f + angle) * 3.14159265f / 180.f;
        shape.setPoint(segments * 3 + i, sf::Vector2f(
            x + w - radius + std::cos(rad) * radius,
            y + h - radius - std::sin(rad) * radius
        ));
    }

    shape.setFillColor(fillColor);
    shape.setOutlineColor(outlineColor);
    shape.setOutlineThickness(outlineThickness);
    window.draw(shape);
}

void ContentBrowser::DrawGradientRect(sf::RenderWindow &window, float x, float y, float w, float h,
                                      sf::Color topColor, sf::Color bottomColor)
{
    sf::VertexArray quad(sf::Quads, 4);
    quad[0].position = {x, y};
    quad[0].color = topColor;
    quad[1].position = {x + w, y};
    quad[1].color = topColor;
    quad[2].position = {x + w, y + h};
    quad[2].color = bottomColor;
    quad[3].position = {x, y + h};
    quad[3].color = bottomColor;
    window.draw(quad);
}

void ContentBrowser::DrawIconForType(sf::RenderWindow &window, AssetType type, sf::Color color,
                                     float cx, float cy, float size) const
{
    switch (type)
    {
        case AssetType::Folder: DrawFolderIcon(window, color, cx, cy, size); break;
        case AssetType::Script: DrawScriptIcon(window, color, cx, cy, size); break;
        case AssetType::Scene:  DrawSceneIcon(window, color, cx, cy, size);  break;
        case AssetType::UIScene: DrawUISceneIcon(window, color, cx, cy, size); break;
        case AssetType::Audio:  DrawAudioIcon(window, color, cx, cy, size);  break;
        case AssetType::Font:   DrawFontIcon(window, color, cx, cy, size);   break;
        case AssetType::Image:  DrawImageIcon(window, color, cx, cy, size);  break;
        default:                DrawUnknownIcon(window, color, cx, cy, size); break;
    }
}

void ContentBrowser::DrawFolderIcon(sf::RenderWindow &window, sf::Color color,
                                    float cx, float cy, float size) const
{
    // Classic folder shape with tab
    float w = size * 1.3f;
    float h = size * 0.9f;
    float tabW = w * 0.4f;
    float tabH = h * 0.2f;

    // Folder body (main rectangle)
    sf::ConvexShape body;
    body.setPointCount(4);
    body.setPoint(0, {cx - w / 2.f, cy - h / 2.f + tabH});
    body.setPoint(1, {cx + w / 2.f, cy - h / 2.f + tabH});
    body.setPoint(2, {cx + w / 2.f, cy + h / 2.f});
    body.setPoint(3, {cx - w / 2.f, cy + h / 2.f});
    body.setFillColor(sf::Color(color.r, color.g, color.b, 60));
    body.setOutlineColor(color);
    body.setOutlineThickness(1.5f);
    window.draw(body);

    // Folder tab (top-left trapezoid)
    sf::ConvexShape tab;
    tab.setPointCount(4);
    tab.setPoint(0, {cx - w / 2.f, cy - h / 2.f});
    tab.setPoint(1, {cx - w / 2.f + tabW, cy - h / 2.f});
    tab.setPoint(2, {cx - w / 2.f + tabW + tabH * 0.5f, cy - h / 2.f + tabH});
    tab.setPoint(3, {cx - w / 2.f, cy - h / 2.f + tabH});
    tab.setFillColor(sf::Color(color.r, color.g, color.b, 100));
    tab.setOutlineColor(color);
    tab.setOutlineThickness(1.5f);
    window.draw(tab);
}

void ContentBrowser::DrawScriptIcon(sf::RenderWindow &window, sf::Color color,
                                    float cx, float cy, float size) const
{
    // Curly braces { }
    sf::Text leftBrace;
    leftBrace.setFont(m_Font);
    leftBrace.setCharacterSize(static_cast<unsigned int>(size * 1.2f));
    leftBrace.setStyle(sf::Text::Bold);
    leftBrace.setFillColor(color);
    leftBrace.setString("{");
    auto lb = leftBrace.getLocalBounds();
    leftBrace.setPosition(cx - lb.width - 3.f, cy - lb.height / 2.f - lb.top);
    window.draw(leftBrace);

    sf::Text rightBrace;
    rightBrace.setFont(m_Font);
    rightBrace.setCharacterSize(static_cast<unsigned int>(size * 1.2f));
    rightBrace.setStyle(sf::Text::Bold);
    rightBrace.setFillColor(color);
    rightBrace.setString("}");
    auto rb = rightBrace.getLocalBounds();
    rightBrace.setPosition(cx + 3.f, cy - rb.height / 2.f - rb.top);
    window.draw(rightBrace);

    // Small code lines between braces
    for (int i = 0; i < 3; ++i)
    {
        float lineW = (i == 1) ? size * 0.3f : size * 0.2f;
        float lineY = cy - size * 0.2f + i * (size * 0.2f);
        sf::RectangleShape codeLine({lineW, 1.5f});
        codeLine.setPosition(cx - lineW / 2.f, lineY);
        codeLine.setFillColor(sf::Color(color.r, color.g, color.b, 150));
        window.draw(codeLine);
    }
}

void ContentBrowser::DrawSceneIcon(sf::RenderWindow &window, sf::Color color,
                                   float cx, float cy, float size) const
{
    // Layer stack / film clapperboard style
    float w = size * 1.1f;
    float h = size * 0.7f;

    // Back layer (offset)
    sf::RectangleShape backLayer({w, h});
    backLayer.setPosition(cx - w / 2.f + 3.f, cy - h / 2.f - 3.f);
    backLayer.setFillColor(sf::Color(color.r, color.g, color.b, 30));
    backLayer.setOutlineColor(sf::Color(color.r, color.g, color.b, 80));
    backLayer.setOutlineThickness(1.f);
    window.draw(backLayer);

    // Middle layer
    sf::RectangleShape midLayer({w, h});
    midLayer.setPosition(cx - w / 2.f + 1.5f, cy - h / 2.f - 1.5f);
    midLayer.setFillColor(sf::Color(color.r, color.g, color.b, 40));
    midLayer.setOutlineColor(sf::Color(color.r, color.g, color.b, 100));
    midLayer.setOutlineThickness(1.f);
    window.draw(midLayer);

    // Front layer
    sf::RectangleShape frontLayer({w, h});
    frontLayer.setPosition(cx - w / 2.f, cy - h / 2.f);
    frontLayer.setFillColor(sf::Color(color.r, color.g, color.b, 60));
    frontLayer.setOutlineColor(color);
    frontLayer.setOutlineThickness(1.5f);
    window.draw(frontLayer);

    // Play triangle in center of front layer
    float triSize = size * 0.25f;
    sf::ConvexShape triangle;
    triangle.setPointCount(3);
    triangle.setPoint(0, {cx - triSize * 0.4f, cy - triSize * 0.5f});
    triangle.setPoint(1, {cx + triSize * 0.6f, cy});
    triangle.setPoint(2, {cx - triSize * 0.4f, cy + triSize * 0.5f});
    triangle.setFillColor(color);
    window.draw(triangle);
}

void ContentBrowser::DrawUISceneIcon(sf::RenderWindow &window, sf::Color color,
                                     float cx, float cy, float size) const
{
    // Window/UI Layout icon
    float w = size * 1.2f;
    float h = size * 0.9f;

    // Main window background
    sf::RectangleShape windowBg({w, h});
    windowBg.setPosition(cx - w / 2.f, cy - h / 2.f);
    windowBg.setFillColor(sf::Color(color.r, color.g, color.b, 40));
    windowBg.setOutlineColor(color);
    windowBg.setOutlineThickness(1.5f);
    window.draw(windowBg);

    // Window title bar
    sf::RectangleShape titleBar({w, h * 0.25f});
    titleBar.setPosition(cx - w / 2.f, cy - h / 2.f);
    titleBar.setFillColor(sf::Color(color.r, color.g, color.b, 100));
    window.draw(titleBar);

    // Inner UI elements (a sidebar and a content area)
    // Sidebar
    sf::RectangleShape sidebar({w * 0.25f, h * 0.55f});
    sidebar.setPosition(cx - w / 2.f + w * 0.1f, cy - h / 2.f + h * 0.35f);
    sidebar.setFillColor(sf::Color(color.r, color.g, color.b, 80));
    window.draw(sidebar);

    // Content area / Button
    sf::RectangleShape contentArea({w * 0.45f, h * 0.25f});
    contentArea.setPosition(cx - w / 2.f + w * 0.45f, cy - h / 2.f + h * 0.35f);
    contentArea.setFillColor(sf::Color(color.r, color.g, color.b, 120));
    window.draw(contentArea);
    
    // Checkbox or slider
    sf::RectangleShape bottomArea({w * 0.45f, h * 0.15f});
    bottomArea.setPosition(cx - w / 2.f + w * 0.45f, cy - h / 2.f + h * 0.7f);
    bottomArea.setFillColor(sf::Color(color.r, color.g, color.b, 80));
    window.draw(bottomArea);
}

void ContentBrowser::DrawAudioIcon(sf::RenderWindow &window, sf::Color color,
                                   float cx, float cy, float size) const
{
    // Music note symbol
    float noteHeadR = size * 0.22f;
    float stemH = size * 0.7f;

    // Note head (filled ellipse approximated by circle)
    sf::CircleShape noteHead(noteHeadR);
    noteHead.setOrigin(noteHeadR, noteHeadR);
    noteHead.setScale(1.3f, 1.f);
    noteHead.setPosition(cx - size * 0.1f, cy + size * 0.2f);
    noteHead.setFillColor(color);
    window.draw(noteHead);

    // Stem
    sf::RectangleShape stem({2.f, stemH});
    stem.setPosition(cx - size * 0.1f + noteHeadR * 1.3f - 2.f, cy + size * 0.2f - stemH);
    stem.setFillColor(color);
    window.draw(stem);

    // Flag (small curve at top of stem)
    float flagX = cx - size * 0.1f + noteHeadR * 1.3f;
    float flagY = cy + size * 0.2f - stemH;
    sf::ConvexShape flag;
    flag.setPointCount(4);
    flag.setPoint(0, {flagX, flagY});
    flag.setPoint(1, {flagX + size * 0.3f, flagY + size * 0.15f});
    flag.setPoint(2, {flagX + size * 0.2f, flagY + size * 0.35f});
    flag.setPoint(3, {flagX, flagY + size * 0.25f});
    flag.setFillColor(sf::Color(color.r, color.g, color.b, 180));
    window.draw(flag);

    // Sound wave arcs (small lines to the right)
    for (int i = 1; i <= 2; ++i)
    {
        float arcX = cx + size * 0.3f + i * 4.f;
        float arcH = size * (0.15f + i * 0.1f);
        sf::RectangleShape wave({1.5f, arcH});
        wave.setPosition(arcX, cy - arcH / 2.f);
        wave.setFillColor(sf::Color(color.r, color.g, color.b, static_cast<sf::Uint8>(200 - i * 50)));
        window.draw(wave);
    }
}

void ContentBrowser::DrawFontIcon(sf::RenderWindow &window, sf::Color color,
                                  float cx, float cy, float size) const
{
    // Large "Aa" text
    sf::Text fontText;
    fontText.setFont(m_Font);
    fontText.setCharacterSize(static_cast<unsigned int>(size * 1.0f));
    fontText.setStyle(sf::Text::Bold);
    fontText.setFillColor(color);
    fontText.setString("Aa");
    auto bounds = fontText.getLocalBounds();
    fontText.setPosition(cx - bounds.width / 2.f - bounds.left, cy - bounds.height / 2.f - bounds.top);
    window.draw(fontText);

    // Underline decoration
    sf::RectangleShape underline({bounds.width + 6.f, 2.f});
    underline.setPosition(cx - bounds.width / 2.f - 3.f, cy + bounds.height / 2.f + 3.f);
    underline.setFillColor(sf::Color(color.r, color.g, color.b, 120));
    window.draw(underline);
}

void ContentBrowser::DrawImageIcon(sf::RenderWindow &window, sf::Color color,
                                   float cx, float cy, float size) const
{
    // Mountain/landscape icon (photo symbol)
    float w = size * 1.2f;
    float h = size * 0.85f;

    // Frame
    sf::RectangleShape frame({w, h});
    frame.setPosition(cx - w / 2.f, cy - h / 2.f);
    frame.setFillColor(sf::Color(color.r, color.g, color.b, 40));
    frame.setOutlineColor(color);
    frame.setOutlineThickness(1.5f);
    window.draw(frame);

    // Mountain silhouette
    sf::ConvexShape mountain;
    mountain.setPointCount(5);
    float baseY = cy + h / 2.f - 2.f;
    mountain.setPoint(0, {cx - w / 2.f + 2.f, baseY});
    mountain.setPoint(1, {cx - size * 0.15f, cy - size * 0.1f});
    mountain.setPoint(2, {cx + size * 0.05f, cy + size * 0.05f});
    mountain.setPoint(3, {cx + size * 0.3f, cy - size * 0.2f});
    mountain.setPoint(4, {cx + w / 2.f - 2.f, baseY});
    mountain.setFillColor(sf::Color(color.r, color.g, color.b, 100));
    window.draw(mountain);

    // Sun circle (top-right)
    sf::CircleShape sun(size * 0.1f);
    sun.setOrigin(size * 0.1f, size * 0.1f);
    sun.setPosition(cx + w / 2.f - size * 0.35f, cy - h / 2.f + size * 0.25f);
    sun.setFillColor(color);
    window.draw(sun);
}

void ContentBrowser::DrawUnknownIcon(sf::RenderWindow &window, sf::Color color,
                                     float cx, float cy, float size) const
{
    // Generic document with dog-ear
    float w = size * 0.85f;
    float h = size * 1.1f;
    float ear = size * 0.25f;

    sf::ConvexShape doc;
    doc.setPointCount(5);
    doc.setPoint(0, {cx - w / 2.f, cy - h / 2.f});
    doc.setPoint(1, {cx + w / 2.f - ear, cy - h / 2.f});
    doc.setPoint(2, {cx + w / 2.f, cy - h / 2.f + ear});
    doc.setPoint(3, {cx + w / 2.f, cy + h / 2.f});
    doc.setPoint(4, {cx - w / 2.f, cy + h / 2.f});
    doc.setFillColor(sf::Color(color.r, color.g, color.b, 50));
    doc.setOutlineColor(color);
    doc.setOutlineThickness(1.5f);
    window.draw(doc);

    // Dog-ear fold
    sf::ConvexShape fold;
    fold.setPointCount(3);
    fold.setPoint(0, {cx + w / 2.f - ear, cy - h / 2.f});
    fold.setPoint(1, {cx + w / 2.f, cy - h / 2.f + ear});
    fold.setPoint(2, {cx + w / 2.f - ear, cy - h / 2.f + ear});
    fold.setFillColor(sf::Color(color.r, color.g, color.b, 100));
    fold.setOutlineColor(color);
    fold.setOutlineThickness(1.f);
    window.draw(fold);

    // Content lines
    for (int i = 0; i < 3; ++i)
    {
        float lineW = w * (0.5f - i * 0.08f);
        float lineY = cy - size * 0.1f + i * (size * 0.22f);
        sf::RectangleShape line({lineW, 1.5f});
        line.setPosition(cx - w / 2.f + size * 0.15f, lineY);
        line.setFillColor(sf::Color(color.r, color.g, color.b, 120));
        window.draw(line);
    }
}

void ContentBrowser::DrawExtensionBadge(sf::RenderWindow &window, const std::string &ext,
                                        sf::Color color, float cardRight, float cardBottom) const
{
    sf::Text badgeText;
    badgeText.setFont(m_Font);
    badgeText.setCharacterSize(8);
    badgeText.setStyle(sf::Text::Bold);
    badgeText.setFillColor(sf::Color::White);
    badgeText.setString(ext);

    float tw = badgeText.getLocalBounds().width;
    float badgeW = tw + 8.f;
    float badgeH = 14.f;
    float badgeX = cardRight - badgeW;
    float badgeY = cardBottom - badgeH + 2.f;

    // Badge background
    DrawRoundedRect(window, badgeX, badgeY, badgeW, badgeH, 3.f,
                    sf::Color(color.r, color.g, color.b, 200));

    badgeText.setPosition(badgeX + 4.f, badgeY + 1.f);
    window.draw(badgeText);
}

void ContentBrowser::DrawFolderContentDots(sf::RenderWindow &window, const std::string &folderPath,
                                           float cx, float bottomY) const
{
    auto types = GetFolderContentTypes(folderPath);
    if (types.empty()) return;

    float dotR = 3.f;
    float spacing = 9.f;
    float totalW = static_cast<float>(types.size()) * spacing - (spacing - dotR * 2.f);
    float startX = cx - totalW / 2.f;

    int idx = 0;
    for (AssetType t : types)
    {
        sf::CircleShape dot(dotR);
        dot.setOrigin(dotR, dotR);
        dot.setPosition(startX + idx * spacing + dotR, bottomY);
        dot.setFillColor(ColorForType(t));
        window.draw(dot);
        ++idx;
    }
}

std::set<AssetType> ContentBrowser::GetFolderContentTypes(const std::string &folderPath) const
{
    std::set<AssetType> types;
    std::error_code ec;

    if (!fs::exists(folderPath, ec) || ec) return types;
    if (!fs::is_directory(folderPath, ec) || ec) return types;

    for (const auto &entry : fs::directory_iterator(folderPath, ec))
    {
        if (ec) break;
        if (entry.is_directory(ec))
        {
            // Don't add folder type as a content indicator
            ec.clear();
            continue;
        }
        AssetType t = TypeFromFile(entry.path().string());
        types.insert(t);
        if (types.size() >= 5) break; // Cap at 5 dots max
    }

    return types;
}
