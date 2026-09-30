#include "UIEditorScene.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include "../Resources/ResourceManager.h"
#include "../Application/Application.h"
#include <nlohmann/json.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Clipboard.hpp>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#undef CreateWindow
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")

static std::string OpenImageFileDialog(HWND hwnd)
{
    char filename[MAX_PATH] = "";
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "Image Files (*.png;*.jpg;*.jpeg;*.jfif;*.bmp)\0*.png;*.jpg;*.jpeg;*.jfif;*.bmp\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    std::filesystem::path curDir = std::filesystem::current_path();
    std::filesystem::path assetsDir = curDir / "assets";
    std::string initDir = std::filesystem::exists(assetsDir) ? assetsDir.string() : curDir.string();
    ofn.lpstrInitialDir = initDir.c_str();

    if (GetOpenFileNameA(&ofn))
    {
        std::error_code ec;
        std::filesystem::path fullPath(filename);
        std::string relPath = std::filesystem::proximate(fullPath, curDir, ec).generic_string();
        if (ec || relPath.empty())
            relPath = fullPath.generic_string();
        return relPath;
    }
    return "";
}

static bool OpenColorPickerDialog(sf::Color &ioColor, HWND hwnd = nullptr)
{
    static COLORREF customColors[16] = {0};
    CHOOSECOLORA cc;
    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hwnd;
    cc.lpCustColors = customColors;
    cc.rgbResult = RGB(ioColor.r, ioColor.g, ioColor.b);
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColorA(&cc))
    {
        ioColor.r = GetRValue(cc.rgbResult);
        ioColor.g = GetGValue(cc.rgbResult);
        ioColor.b = GetBValue(cc.rgbResult);
        return true;
    }
    return false;
}
#endif

static std::filesystem::path GetAppDir()
{
#ifdef _WIN32
    char pathBuf[MAX_PATH];
    if (GetModuleFileNameA(NULL, pathBuf, MAX_PATH)) {
        return std::filesystem::path(pathBuf).parent_path();
    }
#endif
    return std::filesystem::current_path();
}

static std::filesystem::path FindProjectRoot()
{
    std::error_code ec;
    std::filesystem::path cur = std::filesystem::current_path(ec);
    std::filesystem::path appDir = GetAppDir();

    if (std::filesystem::exists(cur / "CMakeLists.txt") && std::filesystem::exists(cur / "assets")) return cur;
    if (std::filesystem::exists(cur.parent_path() / "CMakeLists.txt") && std::filesystem::exists(cur.parent_path() / "assets")) return cur.parent_path();
    
    if (std::filesystem::exists(appDir / "CMakeLists.txt") && std::filesystem::exists(appDir / "assets")) return appDir;
    if (std::filesystem::exists(appDir.parent_path() / "CMakeLists.txt") && std::filesystem::exists(appDir.parent_path() / "assets")) return appDir.parent_path();

    if (!ec && std::filesystem::exists(cur / "assets")) return cur;
    if (std::filesystem::exists(appDir / "assets")) return appDir;

    return cur;
}

#ifdef _WIN32
static std::string OpenScriptFileDialog(HWND hwnd)
{
    char filename[MAX_PATH] = "";
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "Lua Scripts (*.lua)\0*.lua\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    std::filesystem::path rootDir = FindProjectRoot();
    std::filesystem::path assetsDir = rootDir / "assets";
    std::string initDir = std::filesystem::exists(assetsDir) ? assetsDir.string() : rootDir.string();
    ofn.lpstrInitialDir = initDir.c_str();

    if (GetOpenFileNameA(&ofn))
    {
        std::error_code ec;
        std::filesystem::path fullPath(filename);
        std::string relPath = std::filesystem::relative(fullPath, rootDir, ec).generic_string();
        if (ec || relPath.empty())
            relPath = fullPath.generic_string();
        return relPath;
    }
    return "";
}
#endif

static void LaunchProcessDetached(const std::string &commandLine)
{
#ifdef _WIN32
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    std::string fullCmd = "cmd.exe /c " + commandLine;
    std::vector<char> cmdBuf(fullCmd.begin(), fullCmd.end());
    cmdBuf.push_back('\0');

    if (CreateProcessA(NULL, cmdBuf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
#elif __APPLE__
    std::string cmd = commandLine + " &";
    system(cmd.c_str());
#else
    std::string cmd = commandLine + " &";
    system(cmd.c_str());
#endif
}

static bool IsExecutableInPath(const std::string &name)
{
#ifdef _WIN32
    char buf[MAX_PATH];
    if (SearchPathA(NULL, name.c_str(), ".cmd", MAX_PATH, buf, NULL) > 0) return true;
    if (SearchPathA(NULL, name.c_str(), ".exe", MAX_PATH, buf, NULL) > 0) return true;
    if (SearchPathA(NULL, name.c_str(), ".bat", MAX_PATH, buf, NULL) > 0) return true;
    return false;
#else
    std::string checkCmd = "which " + name + " >/dev/null 2>&1";
    return system(checkCmd.c_str()) == 0;
#endif
}

static std::string FormatFloat(float value, int precision = 2)
{
    if (std::abs(value) < 1e-6f) value = 0.0f;
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", precision, value);
    return std::string(buf);
}

static std::shared_ptr<sf::Texture> LoadTextureRobust(const std::string &path)
{
    if (path.empty()) return nullptr;
    auto tex = ResourceManager::Get().GetTexture(path);
    if (!tex) tex = ResourceManager::Get().GetTexture(std::string(ASSET_PATH) + "/" + path);
    return tex;
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
static const sf::Color C_SUCCESS = sf::Color(74, 222, 128);
static const sf::Color C_SUCCESS_DIM = sf::Color(20, 55, 35, 200);
static const sf::Color C_WARNING = sf::Color(251, 191, 36);
static const sf::Color C_WARNING_DIM = sf::Color(75, 55, 10, 200);
static const sf::Color C_DANGER = sf::Color(241, 104, 94);
static const sf::Color C_DANGER_DIM = sf::Color(70, 20, 18, 200);
static const sf::Color C_GRID_MINOR = sf::Color(38, 43, 51);
static const sf::Color C_GRID_MAJOR = sf::Color(51, 58, 69);

enum class ActionParamType {
    None,
    Bool,
    String,
    Float
};

struct DropdownOption {
    std::string label;
    std::string code;
    ActionParamType paramType;
    std::string defaultParam;
};

static const std::vector<DropdownOption> UTILITY_ACTIONS = {
    {"None", "", ActionParamType::None, ""},
    {"Load Scene", "LoadScene", ActionParamType::String, "scenes/level1.json"},
    {"Restart Scene", "Restart", ActionParamType::None, ""},
    {"Quit Game", "Quit", ActionParamType::None, ""},
    {"Play Sound", "PlaySound", ActionParamType::String, "audio/click.wav"},
    {"Play Music", "PlayMusic", ActionParamType::String, "audio/music.ogg"},
    {"Stop Music", "StopMusic", ActionParamType::None, ""},
    {"Toggle Pause", "TogglePause", ActionParamType::None, ""},
    {"Pause Game", "Pause", ActionParamType::None, ""},
    {"Resume Game", "Resume", ActionParamType::None, ""},
    {"Set TimeScale", "SetTimeScale", ActionParamType::Float, "1.0"},
    {"Toggle Fullscreen", "ToggleFullscreen", ActionParamType::None, ""},
    {"Set Fullscreen", "SetFullscreen", ActionParamType::Bool, "true"},
    {"Show FPS", "ShowFPS", ActionParamType::Bool, "true"},
    {"Set Cursor Visible", "SetCursorVisible", ActionParamType::Bool, "true"},
    {"Log To Screen", "LogToScreen", ActionParamType::String, "Button Clicked!"},
    {"Log Message", "Log", ActionParamType::String, "Hello from UI!"},
    {"Open URL", "OpenURL", ActionParamType::String, "https://rayne3d.com"},
    {"Take Screenshot", "TakeScreenshot", ActionParamType::None, ""},
    {"Toggle UI Element", "ToggleElement", ActionParamType::String, "menu_panel"},
    {"Show UI Element", "ShowElement", ActionParamType::String, "menu_panel"},
    {"Hide UI Element", "HideElement", ActionParamType::String, "menu_panel"},
    {"Execute Lua", "ExecuteLua", ActionParamType::String, "print('custom action')"}
};

UIEditorScene::UIEditorScene(SceneManager &manager, sf::RenderWindow &window)
    : Scene(manager), m_Window(window)
{
    m_Font = ResourceManager::Get().GetFont(ENGINE_ASSET_PATH "/fonts/Merriweather.ttf");
    m_CanvasView = window.getDefaultView();
    m_ContentBrowser = std::make_unique<ContentBrowser>(*m_Font, ASSET_PATH);
    InitMenus();
    UpdateBounds();
}

void UIEditorScene::AutoDetectPreferredIDE()
{
    std::filesystem::path rootDir = FindProjectRoot();
    std::vector<std::filesystem::path> settingsPaths = {
        rootDir / "engine_content/editor/editor_settings.json",
        GetAppDir() / "engine_content/editor/editor_settings.json"
    };
    for (const auto &sp : settingsPaths)
    {
        std::error_code ec;
        if (std::filesystem::exists(sp, ec))
        {
            std::ifstream f(sp);
            if (f.is_open())
            {
                try {
                    nlohmann::json j = nlohmann::json::parse(f);
                    if (j.contains("editor") && j["editor"].contains("preferredIDE"))
                    {
                        m_PreferredIDE = j["editor"]["preferredIDE"].get<std::string>();
                        if (!m_PreferredIDE.empty()) break;
                    }
                } catch (...) {}
            }
        }
    }

    if (m_PreferredIDE.empty() || !IsExecutableInPath(m_PreferredIDE))
    {
        if (IsExecutableInPath("clion")) m_PreferredIDE = "clion";
        else if (IsExecutableInPath("code")) m_PreferredIDE = "code";
        else if (IsExecutableInPath("rider")) m_PreferredIDE = "rider";
    }
}

void UIEditorScene::SyncFileToRuntime(const std::filesystem::path &sourceRelPath)
{
    std::filesystem::path rootDir = FindProjectRoot();
    std::filesystem::path srcFile = rootDir / sourceRelPath;
    std::filesystem::path appDir = GetAppDir();
    std::filesystem::path curDir = std::filesystem::current_path();

    std::vector<std::filesystem::path> targets;
    if (appDir != rootDir) targets.push_back(appDir / sourceRelPath);
    if (curDir != rootDir && curDir != appDir) targets.push_back(curDir / sourceRelPath);

    std::error_code ec;
    if (std::filesystem::exists(srcFile, ec))
    {
        for (const auto &tgt : targets)
        {
            std::filesystem::create_directories(tgt.parent_path(), ec);
            std::filesystem::copy_file(srcFile, tgt, std::filesystem::copy_options::overwrite_existing, ec);
        }
    }
}

void UIEditorScene::OnEnter()
{
    std::cout << "[INFO] [UIEditorScene] Entered UI Editor\n";
    AutoDetectPreferredIDE();
    UpdateBounds();
    SelectElement(nullptr);
    if (m_ContentBrowser) m_ContentBrowser->Refresh();

    if (UIManager::Get().GetCurrentUIPath().empty())
    {
        std::filesystem::path rootDir = FindProjectRoot();
        if (std::filesystem::exists(rootDir / "assets/scenes/game_ui.json"))
            UIManager::Get().SetCurrentUIPath("assets/scenes/game_ui.json");
    }
    SetDirty(false);
}

void UIEditorScene::OnExit() { std::cout << "[INFO] [UIEditorScene] Exited UI Editor\n"; }

void UIEditorScene::UpdateBounds()
{
    const float winW = static_cast<float>(m_Window.getSize().x);
    const float winH = static_cast<float>(m_Window.getSize().y);

    m_PaletteBounds = {0.f, TopBarHeight, PaletteWidth, winH - TopBarHeight};
    m_InspectorBounds = {winW - InspectorWidth, TopBarHeight, InspectorWidth, winH - TopBarHeight - HierarchyHeight};
    m_HierarchyBounds = {winW - InspectorWidth, winH - HierarchyHeight, InspectorWidth, HierarchyHeight};

    m_CanvasBounds = {PaletteWidth, TopBarHeight, winW - PaletteWidth - InspectorWidth, winH - TopBarHeight - BrowserHeight};
    m_BrowserBounds = {PaletteWidth, winH - BrowserHeight, winW - PaletteWidth - InspectorWidth, BrowserHeight};

    sf::FloatRect vp(
        PaletteWidth / winW,
        TopBarHeight / winH,
        m_CanvasBounds.width / winW,
        m_CanvasBounds.height / winH
    );
    m_CanvasView.setViewport(vp);

    if (!m_ViewInitialized)
    {
        const float margin = 50.f;
        const float availW = std::max(100.f, m_CanvasBounds.width - margin * 2.f);
        const float availH = std::max(100.f, m_CanvasBounds.height - margin * 2.f);
        float scale = std::min(availW / m_CanvasSize.x, availH / m_CanvasSize.y);
        if (scale <= 0.001f) scale = 0.5f;

        m_CanvasView.setSize(m_CanvasBounds.width / scale, m_CanvasBounds.height / scale);
        m_CanvasView.setCenter(m_CanvasSize.x / 2.f, m_CanvasSize.y / 2.f);
        m_ViewInitialized = true;
    }

    sf::FloatRect inspVp(
        m_InspectorBounds.left / winW,
        m_InspectorBounds.top / winH,
        m_InspectorBounds.width / winW,
        m_InspectorBounds.height / winH
    );
    m_InspectorView.setViewport(inspVp);
    m_InspectorView.setSize(m_InspectorBounds.width, m_InspectorBounds.height);
}

void UIEditorScene::HandleEvent(const sf::Event &event)
{
    if (event.type == sf::Event::Resized)
    {
        UpdateBounds();
        return;
    }

    const sf::Vector2i pixelPos = sf::Mouse::getPosition(m_Window);
    const float winW = static_cast<float>(m_Window.getSize().x);
    const float winH = static_cast<float>(m_Window.getSize().y);
    const sf::View uiView(sf::FloatRect(0.f, 0.f, winW, winH));

    m_MouseScreenPos = m_Window.mapPixelToCoords(pixelPos, uiView);
    m_MouseCanvasPos = m_Window.mapPixelToCoords(pixelPos, m_CanvasView);

    if (m_ShowDeleteModal)
    {
        if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
        {
            if (m_DeleteModalCascadeBtn.contains(m_MouseScreenPos))
            {
                ConfirmDeleteCascade();
                return;
            }
            if (m_DeleteModalUnparentBtn.contains(m_MouseScreenPos))
            {
                ConfirmDeleteUnparent();
                return;
            }
            if (m_DeleteModalCancelBtn.contains(m_MouseScreenPos))
            {
                m_ShowDeleteModal = false;
                return;
            }
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
        {
            m_ShowDeleteModal = false;
            return;
        }
        return;
    }

    if (m_ContentBrowser)
    {
        if (event.type == sf::Event::MouseMoved && m_ContentBrowser->HasDraggedAsset())
        {
            m_ContentBrowser->HandleEvent(event, m_MouseScreenPos);
        }
    }

    if (event.type == sf::Event::MouseWheelScrolled)
    {
        if (m_BrowserBounds.contains(m_MouseScreenPos))
        {
            if (m_ContentBrowser) m_ContentBrowser->HandleEvent(event, m_MouseScreenPos);
            return;
        }
        else if (m_CanvasBounds.contains(m_MouseScreenPos))
        {
            float delta = event.mouseWheelScroll.delta;
            float factor = delta > 0 ? 0.9f : 1.1f;
            m_CanvasView.zoom(factor);
        }
        else if (m_InspectorBounds.contains(m_MouseScreenPos))
        {
            m_InspectorTargetScroll -= event.mouseWheelScroll.delta * 40.f;
            m_InspectorTargetScroll = std::max(0.f, std::min(m_InspectorTargetScroll, m_InspectorMaxScroll));
        }
        return;
    }

    if (m_ContentBrowser)
    {
        if (m_BrowserBounds.contains(m_MouseScreenPos) ||
            m_ContentBrowser->IsInputActive() ||
            m_ContentBrowser->IsContextMenuOpen())
        {
            m_ContentBrowser->HandleEvent(event, m_MouseScreenPos);
            if (event.type == sf::Event::MouseButtonPressed ||
                event.type == sf::Event::MouseButtonReleased ||
                m_ContentBrowser->IsInputActive() ||
                m_ContentBrowser->IsContextMenuOpen())
            {
                return;
            }
        }
    }

    if (event.type == sf::Event::MouseButtonPressed)
    {
        if (event.mouseButton.button == sf::Mouse::Left)
        {
            if (!m_ActiveDropdown.empty())
            {
                if (m_DropdownRect.contains(m_MouseScreenPos) && m_SelectedElement)
                {
                    if (m_ActiveDropdown == "script_select")
                    {
                        auto scripts = GetAvailableScripts();
                        std::vector<std::pair<std::string, std::string>> scriptOptions;
                        scriptOptions.push_back({"assets/scripts/ui_interactions.lua", "+ ui_interactions.lua (Default)"});
                        scriptOptions.push_back({"assets/scripts/" + m_SelectedElement->id + ".lua", "+ " + m_SelectedElement->id + ".lua (New)"});
                        scriptOptions.push_back({"__browse__", "[.. Browse File on Disk...]"});
                        for (const auto &s : scripts)
                        {
                            if (s != "assets/scripts/ui_interactions.lua" && s != "assets/scripts/" + m_SelectedElement->id + ".lua")
                                scriptOptions.push_back({s, s});
                        }

                        float y = m_DropdownRect.top + 4.f;
                        for (const auto &opt : scriptOptions)
                        {
                            sf::FloatRect r(m_DropdownRect.left, y, m_DropdownRect.width, 24.f);
                            if (r.contains(m_MouseScreenPos))
                            {
                                if (opt.first == "__browse__")
                                {
                                    HandleAction("browse_script_dialog");
                                }
                                else
                                {
                                    m_SelectedElement->scriptPath = opt.first;
                                }
                                break;
                            }
                            y += 24.f;
                        }
                    }
                    else
                    {
                        float y = m_DropdownRect.top + 4.f;
                        for (const auto& opt : UTILITY_ACTIONS)
                        {
                            sf::FloatRect r(m_DropdownRect.left, y, m_DropdownRect.width, 24.f);
                            if (r.contains(m_MouseScreenPos))
                            {
                                if (m_ActiveDropdown == "onclick") {
                                    m_SelectedElement->onClickAction = opt.code;
                                    m_SelectedElement->onClickParam = opt.defaultParam;
                                }
                                else if (m_ActiveDropdown == "onhover") {
                                    m_SelectedElement->onHoverAction = opt.code;
                                    m_SelectedElement->onHoverParam = opt.defaultParam;
                                }
                                break;
                            }
                            y += 24.f;
                        }
                    }
                }
                m_ActiveDropdown = "";
                return;
            }

            if (m_OpenMenuIndex >= 0)
            {
                for (auto &[r, a]: m_MenuItemHitboxes)
                {
                    if (r.contains(m_MouseScreenPos))
                    {
                        HandleMenuAction(a);
                        m_OpenMenuIndex = -1;
                        return;
                    }
                }
                m_OpenMenuIndex = -1;
            }

            if (m_MouseScreenPos.y < MenuBarHeight)
            {
                for (int i = 0; i < (int)m_Menus.size(); i++)
                {
                    if (m_Menus[i].bounds.contains(m_MouseScreenPos))
                    {
                        m_OpenMenuIndex = i;
                        return;
                    }
                }
            }

            if (m_ActiveField != EditField::None && m_ActiveInputBounds.contains(m_MouseScreenPos))
            {
                float textStartX = m_ActiveInputBounds.left + 4.f;
                float localX = m_MouseScreenPos.x - textStartX;
                sf::Text t(m_ActiveInputText, *m_Font, 12);
                int bestIdx = 0;
                float bestDist = 1e9f;
                for (int i = 0; i <= (int)m_ActiveInputText.size(); ++i) {
                    float cx = t.findCharacterPos(i).x;
                    float d = std::abs(localX - cx);
                    if (d < bestDist) { bestDist = d; bestIdx = i; }
                }
                m_InputSelectionStart = bestIdx;
                m_InputSelectionEnd = bestIdx;
                m_IsSelectingText = true;
                return;
            }

            m_InputSelectionStart = -1;
            m_InputSelectionEnd = -1;
            m_IsSelectingText = false;

            if (m_PaletteBounds.contains(m_MouseScreenPos))
            {
                for (auto &btn: m_PaletteHitboxes)
                {
                    if (btn.bounds.contains(m_MouseScreenPos))
                    {
                        HandleAction(btn.action);
                        return;
                    }
                }
            }
            if (m_InspectorBounds.contains(m_MouseScreenPos))
            {
                for (auto &btn: m_InspectorHitboxes)
                {
                    if (btn.bounds.contains(m_MouseScreenPos))
                    {
                        HandleAction(btn.action);
                        return;
                    }
                }
            }
            if (m_ToolbarHitboxes.size() > 0 && m_MouseScreenPos.y > MenuBarHeight && m_MouseScreenPos.y < TopBarHeight)
            {
                for (auto &btn: m_ToolbarHitboxes)
                {
                    if (btn.bounds.contains(m_MouseScreenPos))
                    {
                        HandleAction(btn.action);
                        return;
                    }
                }
            }
            if (m_HierarchyBounds.contains(m_MouseScreenPos))
            {
                for (auto &[rect, id] : m_HierarchyFoldHitboxes)
                {
                    if (rect.contains(m_MouseScreenPos))
                    {
                        if (m_HierarchyCollapsed.count(id)) m_HierarchyCollapsed.erase(id);
                        else m_HierarchyCollapsed.insert(id);
                        return;
                    }
                }
                for (auto &[rect, el]: m_HierarchyHitboxes)
                {
                    if (rect.contains(m_MouseScreenPos))
                    {
                        SelectElement(el);
                        m_HierarchyPotentialDrag = true;
                        m_HierarchyDragStartPos = m_MouseScreenPos;
                        m_HierarchyDragSourceId = el->id;
                        m_HierarchyDragTargetId.clear();
                        return;
                    }
                }
            }

            if (m_CanvasBounds.contains(m_MouseScreenPos))
            {
                if (m_SelectedElement)
                {
                    m_ResizeHandle = GetResizeHandle(m_MouseCanvasPos);
                    if (m_ResizeHandle != -1)
                    {
                        m_Resizing = true;
                        m_ResizeMouseStart = m_MouseCanvasPos;
                        m_ResizeObjOrigin = m_SelectedElement->position;
                        m_ResizeObjSize = m_SelectedElement->size;
                        return;
                    }
                }

                UIElement *clicked = nullptr;
                std::vector<UIElement *> candidates;
                for (auto &el: UIManager::Get().GetElements()) candidates.push_back(&el);
                std::stable_sort(candidates.begin(), candidates.end(), [](const UIElement *a, const UIElement *b) {
                    return a->zIndex > b->zIndex;
                });
                for (auto *el: candidates)
                {
                    sf::FloatRect bounds(el->worldPosition.x, el->worldPosition.y, el->size.x, el->size.y);
                    if (bounds.contains(m_MouseCanvasPos))
                    {
                        clicked = el;
                        break;
                    }
                }

                if (clicked)
                {
                    SelectElement(clicked);
                    m_Dragging = true;
                    m_DragOffset = m_MouseCanvasPos - clicked->worldPosition;
                } else { SelectElement(nullptr); }
            }
        } else if (event.mouseButton.button == sf::Mouse::Middle || event.mouseButton.button == sf::Mouse::Right)
        {
            if (m_CanvasBounds.contains(m_MouseScreenPos))
            {
                m_Panning = true;
                m_PanStart = m_MouseCanvasPos;
            }
        }
    }

    if (event.type == sf::Event::MouseMoved)
    {
        if (m_OpenMenuIndex >= 0 && m_MouseScreenPos.y < MenuBarHeight)
        {
            for (int i = 0; i < (int)m_Menus.size(); i++)
            {
                if (m_Menus[i].bounds.contains(m_MouseScreenPos))
                {
                    m_OpenMenuIndex = i;
                    break;
                }
            }
        }

        if (m_IsSelectingText && m_ActiveField != EditField::None)
        {
            float textStartX = m_ActiveInputBounds.left + 4.f;
            float localX = m_MouseScreenPos.x - textStartX;
            sf::Text t(m_ActiveInputText, *m_Font, 12);
            int bestIdx = 0;
            float bestDist = 1e9f;
            for (int i = 0; i <= (int)m_ActiveInputText.size(); ++i) {
                float cx = t.findCharacterPos(i).x;
                float d = std::abs(localX - cx);
                if (d < bestDist) { bestDist = d; bestIdx = i; }
            }
            m_InputSelectionEnd = bestIdx;
        }

        if (m_HierarchyPotentialDrag && !m_HierarchyDragging)
        {
            if (std::hypot(m_MouseScreenPos.x - m_HierarchyDragStartPos.x, m_MouseScreenPos.y - m_HierarchyDragStartPos.y) > 4.f)
            {
                m_HierarchyDragging = true;
            }
        }
        if (m_HierarchyDragging)
        {
            m_HierarchyDragTargetId.clear();
            for (auto &[rect, el] : m_HierarchyHitboxes)
            {
                if (rect.contains(m_MouseScreenPos) && el->id != m_HierarchyDragSourceId &&
                    !UIManager::Get().IsDescendantOf(el->id, m_HierarchyDragSourceId))
                {
                    m_HierarchyDragTargetId = el->id;
                    break;
                }
            }
        }

        if (m_Panning)
        {
            sf::Vector2f newPos = m_Window.mapPixelToCoords(pixelPos, m_CanvasView);
            m_CanvasView.move(m_PanStart - newPos);
        }
        if (m_Dragging && m_SelectedElement)
        {
            sf::Vector2f newWorldPos = m_MouseCanvasPos - m_DragOffset;
            if (m_SelectedElement->parent.empty() || UIManager::Get().GetElement(m_SelectedElement->parent) == nullptr) {
                m_SelectedElement->position = newWorldPos;
            } else {
                auto* parentEl = UIManager::Get().GetElement(m_SelectedElement->parent);
                m_SelectedElement->position = newWorldPos - parentEl->worldPosition;
            }
            UIManager::Get().ApplyLayouts();
        }
        if (m_Resizing && m_SelectedElement)
        {
            sf::Vector2f delta = m_MouseCanvasPos - m_ResizeMouseStart;
            sf::Vector2f newPos = m_ResizeObjOrigin;
            sf::Vector2f newSize = m_ResizeObjSize;
            const float minSize = 4.f;

            switch (m_ResizeHandle)
            {
                case 0:
                    newPos.x = std::min(m_ResizeObjOrigin.x + delta.x,
                                        m_ResizeObjOrigin.x + m_ResizeObjSize.x - minSize);
                    newPos.y = std::min(m_ResizeObjOrigin.y + delta.y,
                                        m_ResizeObjOrigin.y + m_ResizeObjSize.y - minSize);
                    newSize.x = std::max(m_ResizeObjSize.x - delta.x, minSize);
                    newSize.y = std::max(m_ResizeObjSize.y - delta.y, minSize);
                    break;
                case 1:
                    newPos.y = std::min(m_ResizeObjOrigin.y + delta.y,
                                        m_ResizeObjOrigin.y + m_ResizeObjSize.y - minSize);
                    newSize.y = std::max(m_ResizeObjSize.y - delta.y, minSize);
                    break;
                case 2:
                    newPos.y = std::min(m_ResizeObjOrigin.y + delta.y,
                                        m_ResizeObjOrigin.y + m_ResizeObjSize.y - minSize);
                    newSize.x = std::max(m_ResizeObjSize.x + delta.x, minSize);
                    newSize.y = std::max(m_ResizeObjSize.y - delta.y, minSize);
                    break;
                case 3:
                    newPos.x = std::min(m_ResizeObjOrigin.x + delta.x,
                                        m_ResizeObjOrigin.x + m_ResizeObjSize.x - minSize);
                    newSize.x = std::max(m_ResizeObjSize.x - delta.x, minSize);
                    break;
                case 4:
                    newSize.x = std::max(m_ResizeObjSize.x + delta.x, minSize);
                    break;
                case 5:
                    newPos.x = std::min(m_ResizeObjOrigin.x + delta.x,
                                        m_ResizeObjOrigin.x + m_ResizeObjSize.x - minSize);
                    newSize.x = std::max(m_ResizeObjSize.x - delta.x, minSize);
                    newSize.y = std::max(m_ResizeObjSize.y + delta.y, minSize);
                    break;
                case 6:
                    newSize.y = std::max(m_ResizeObjSize.y + delta.y, minSize);
                    break;
                case 7:
                    newSize.x = std::max(m_ResizeObjSize.x + delta.x, minSize);
                    newSize.y = std::max(m_ResizeObjSize.y + delta.y, minSize);
                    break;
            }

            m_SelectedElement->position = newPos;
            m_SelectedElement->size = newSize;
            m_SelectedElement->UpdateDrawables();
        }
    }

    if (event.type == sf::Event::MouseButtonReleased)
    {
        if (event.mouseButton.button == sf::Mouse::Left)
        {
            if (m_HierarchyDragging)
            {
                if (!m_HierarchyDragTargetId.empty())
                {
                    UIManager::Get().SetParent(m_HierarchyDragSourceId, m_HierarchyDragTargetId, true);
                    SetDirty(true);
                }
                else if (m_HierarchyBounds.contains(m_MouseScreenPos))
                {
                    UIManager::Get().SetParent(m_HierarchyDragSourceId, "", true);
                    SetDirty(true);
                }
            }
            m_HierarchyDragging = false;
            m_HierarchyPotentialDrag = false;
            m_HierarchyDragSourceId.clear();
            m_HierarchyDragTargetId.clear();

            m_IsSelectingText = false;
            if (m_InputSelectionStart == m_InputSelectionEnd) {
                m_InputSelectionStart = -1;
                m_InputSelectionEnd = -1;
            }
        }
        if (m_Dragging || m_Resizing) {
            SetDirty(true);
        }
        m_Dragging = false;
        m_Panning = false;
        m_Resizing = false;
        m_ResizeHandle = -1;

        if (event.mouseButton.button == sf::Mouse::Left &&
            m_ContentBrowser && m_ContentBrowser->HasDraggedAsset())
        {
            DraggedAsset drag = m_ContentBrowser->GetDraggedAsset();
            m_ContentBrowser->ClearDrag();

            std::error_code ec;
            std::filesystem::path pt(drag.path);
            std::filesystem::path root = std::filesystem::absolute(ASSET_PATH, ec);
            std::string relPath = std::filesystem::proximate(pt, root, ec).generic_string();
            if (ec) relPath = drag.path;

            if (drag.type == AssetType::Image)
            {
                auto tex = ResourceManager::Get().GetTexture(drag.path);
                if (!tex && !relPath.empty()) tex = ResourceManager::Get().GetTexture(relPath);
                if (!tex && !relPath.empty()) tex = ResourceManager::Get().GetTexture(std::string(ASSET_PATH) + "/" + relPath);

                if (m_InspectorBounds.contains(m_MouseScreenPos) && m_SelectedElement)
                {
                    bool assignedSlot = false;
                    for (const auto &hb : m_InspectorHitboxes)
                    {
                        if (hb.bounds.contains(m_MouseScreenPos))
                        {
                            if (hb.action == "edit_hovertexturepath" || hb.action == "slot_hover_texture")
                            {
                                m_SelectedElement->hoverTexturePath = relPath;
                                m_SelectedElement->hoverTexture = tex;
                                assignedSlot = true;
                                break;
                            }
                            else if (hb.action == "edit_pressedtexturepath" || hb.action == "slot_pressed_texture")
                            {
                                m_SelectedElement->pressedTexturePath = relPath;
                                m_SelectedElement->pressedTexture = tex;
                                assignedSlot = true;
                                break;
                            }
                            else if (hb.action == "edit_checkedtexturepath" || hb.action == "slot_checked_texture")
                            {
                                m_SelectedElement->checkedTexturePath = relPath;
                                m_SelectedElement->checkedTexture = tex;
                                assignedSlot = true;
                                break;
                            }
                            else if (hb.action == "edit_knobtexturepath" || hb.action == "slot_knob_texture")
                            {
                                m_SelectedElement->knobTexturePath = relPath;
                                m_SelectedElement->knobTexture = tex;
                                assignedSlot = true;
                                break;
                            }
                            else if (hb.action == "edit_filltexturepath" || hb.action == "slot_fill_texture")
                            {
                                m_SelectedElement->fillTexturePath = relPath;
                                m_SelectedElement->fillTexture = tex;
                                assignedSlot = true;
                                break;
                            }
                            else if (hb.action == "edit_texturepath" || hb.action == "slot_texture")
                            {
                                m_SelectedElement->texturePath = relPath;
                                m_SelectedElement->texture = tex;
                                if (m_SelectedElement->type == UIElementType::Button && m_SelectedElement->normalColor == sf::Color(100, 100, 100))
                                {
                                    m_SelectedElement->normalColor = sf::Color::White;
                                    m_SelectedElement->hoverColor = sf::Color(230, 230, 230);
                                    m_SelectedElement->pressedColor = sf::Color(180, 180, 180);
                                }
                                assignedSlot = true;
                                break;
                            }
                        }
                    }
                    if (!assignedSlot)
                    {
                        if (m_SelectedElement->type == UIElementType::Checkbox && m_SelectedElement->isChecked)
                        {
                            m_SelectedElement->checkedTexturePath = relPath;
                            m_SelectedElement->checkedTexture = tex;
                        }
                        else if (m_SelectedElement->type == UIElementType::Slider)
                        {
                            m_SelectedElement->knobTexturePath = relPath;
                            m_SelectedElement->knobTexture = tex;
                        }
                        else if (m_SelectedElement->type == UIElementType::ProgressBar)
                        {
                            m_SelectedElement->fillTexturePath = relPath;
                            m_SelectedElement->fillTexture = tex;
                        }
                        else if (m_SelectedElement->type == UIElementType::Button)
                        {
                            m_SelectedElement->texturePath = relPath;
                            m_SelectedElement->texture = tex;
                            if (m_SelectedElement->normalColor == sf::Color(100, 100, 100))
                            {
                                m_SelectedElement->normalColor = sf::Color::White;
                                m_SelectedElement->hoverColor = sf::Color(230, 230, 230);
                                m_SelectedElement->pressedColor = sf::Color(180, 180, 180);
                            }
                        }
                        else
                        {
                            m_SelectedElement->texturePath = relPath;
                            m_SelectedElement->texture = tex;
                        }
                    }
                    m_SelectedElement->UpdateDrawables();
                }
                else if (m_HierarchyBounds.contains(m_MouseScreenPos))
                {
                    for (auto &[rect, el] : m_HierarchyHitboxes)
                    {
                        if (rect.contains(m_MouseScreenPos))
                        {
                            el->texturePath = relPath;
                            el->texture = tex;
                            el->UpdateDrawables();
                            m_SelectedElement = el;
                            break;
                        }
                    }
                }
                else if (m_CanvasBounds.contains(m_MouseScreenPos))
                {
                    UIElement *hit = nullptr;
                    std::vector<UIElement *> candidates;
                    for (auto &el : UIManager::Get().GetElements()) candidates.push_back(&el);
                    std::stable_sort(candidates.begin(), candidates.end(), [](const UIElement *a, const UIElement *b) {
                        return a->zIndex > b->zIndex;
                    });
                    for (auto *el : candidates)
                    {
                        sf::FloatRect bounds(el->position.x, el->position.y, el->size.x, el->size.y);
                        if (bounds.contains(m_MouseCanvasPos))
                        {
                            hit = el;
                            break;
                        }
                    }

                    if (hit)
                    {
                        if (hit->type == UIElementType::Checkbox)
                        {
                            if (hit->isChecked)
                            {
                                hit->checkedTexturePath = relPath;
                                hit->checkedTexture = tex;
                            }
                            else
                            {
                                hit->texturePath = relPath;
                                hit->texture = tex;
                            }
                        }
                        else if (hit->type == UIElementType::Slider)
                        {
                            hit->knobTexturePath = relPath;
                            hit->knobTexture = tex;
                        }
                        else if (hit->type == UIElementType::ProgressBar)
                        {
                            hit->fillTexturePath = relPath;
                            hit->fillTexture = tex;
                        }
                        else
                        {
                            hit->texturePath = relPath;
                            hit->texture = tex;
                        }
                        hit->UpdateDrawables();
                        SelectElement(hit);
                    }
                    else
                    {
                        int maxZ = 0;
                        for (auto &el : UIManager::Get().GetElements()) maxZ = std::max(maxZ, el.zIndex);
                        UIElement *newImg = UIManager::Get().CreateElement(NextId(UIElementType::Image), UIElementType::Image);
                        if (newImg)
                        {
                            newImg->texturePath = relPath;
                            newImg->texture = tex;
                            sf::Vector2f imgSize(100.f, 100.f);
                            if (newImg->texture)
                            {
                                imgSize = {static_cast<float>(newImg->texture->getSize().x),
                                           static_cast<float>(newImg->texture->getSize().y)};
                                if (imgSize.x > 400.f || imgSize.y > 400.f)
                                {
                                    float s = std::min(400.f / imgSize.x, 400.f / imgSize.y);
                                    imgSize *= s;
                                }
                            }
                            newImg->size = imgSize;
                            newImg->position = m_MouseCanvasPos - imgSize / 2.f;
                            newImg->color = sf::Color::White;
                            newImg->zIndex = maxZ + 1;
                            newImg->UpdateDrawables();
                            SelectElement(newImg);
                        }
                    }
                }
            }
            else if (drag.type == AssetType::Font)
            {
                auto f = ResourceManager::Get().GetFont(drag.path);
                if (m_CanvasBounds.contains(m_MouseScreenPos))
                {
                    for (auto &el : UIManager::Get().GetElements())
                    {
                        sf::FloatRect bounds(el.position.x, el.position.y, el.size.x, el.size.y);
                        if (bounds.contains(m_MouseCanvasPos))
                        {
                            el.font = f;
                            el.UpdateDrawables();
                            SelectElement(&el);
                            break;
                        }
                    }
                }
                else if (m_InspectorBounds.contains(m_MouseScreenPos) && m_SelectedElement)
                {
                    m_SelectedElement->font = f;
                    m_SelectedElement->UpdateDrawables();
                }
            }
            SetDirty(true);
            return;
        }
    }

    if (m_ContentBrowser && m_ContentBrowser->IsInputActive())
        return;

    if (event.type == sf::Event::KeyPressed)
    {
        const bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);

        if (m_ActiveField != EditField::None)
        {
            if (ctrl && event.key.code == sf::Keyboard::A)
            {
                m_InputSelectionStart = 0;
                m_InputSelectionEnd = static_cast<int>(m_ActiveInputText.size());
                return;
            }
            if (ctrl && event.key.code == sf::Keyboard::C && HasTextSelection())
            {
                int sMin = std::clamp(GetSelectionMin(), 0, (int)m_ActiveInputText.size());
                int sMax = std::clamp(GetSelectionMax(), 0, (int)m_ActiveInputText.size());
                sf::Clipboard::setString(m_ActiveInputText.substr(sMin, sMax - sMin));
                return;
            }
            if (ctrl && event.key.code == sf::Keyboard::V)
            {
                if (HasTextSelection()) DeleteActiveSelection();
                std::string pasteStr = sf::Clipboard::getString().toAnsiString();
                for (char c : pasteStr) {
                    if (c >= 32 && c < 127) m_ActiveInputText += c;
                }
                return;
            }
            if (event.key.code == sf::Keyboard::Delete && HasTextSelection())
            {
                DeleteActiveSelection();
                return;
            }
        }

        if (ctrl && event.key.code == sf::Keyboard::S)
        {
            HandleAction("save");
            return;
        }
        if (ctrl && event.key.code == sf::Keyboard::C)
        {
            CopySelection();
            return;
        }
        if (ctrl && event.key.code == sf::Keyboard::V)
        {
            PasteClipboard();
            return;
        }

        if (event.key.code == sf::Keyboard::Escape)
        {
            if (m_ActiveField != EditField::None)
            {
                m_ActiveField = EditField::None;
                m_ActiveInputText.clear();
                m_InputSelectionStart = -1;
                m_InputSelectionEnd = -1;
            }
            else
            {
                SelectElement(nullptr);
            }
        }
        else if (event.key.code == sf::Keyboard::Delete && m_SelectedElement && m_ActiveField == EditField::None)
        {
            DeleteSelected();
        }
        else if (event.key.code == sf::Keyboard::Enter && m_ActiveField != EditField::None)
        {
            m_ActiveField = EditField::None;
            m_ActiveInputText.clear();
            m_InputSelectionStart = -1;
            m_InputSelectionEnd = -1;
        }
        else if (event.key.code == sf::Keyboard::Backspace && m_ActiveField != EditField::None)
        {
            if (HasTextSelection())
            {
                DeleteActiveSelection();
            }
            else if (!m_ActiveInputText.empty())
            {
                m_ActiveInputText.pop_back();
            }
            if (m_SelectedElement)
            {
                if (m_ActiveField == EditField::Id) m_SelectedElement->id = m_ActiveInputText;
                else if (m_ActiveField == EditField::UIText) m_SelectedElement->text = m_ActiveInputText;
                else if (m_ActiveField == EditField::ScriptPath) m_SelectedElement->scriptPath = m_ActiveInputText;
                else if (m_ActiveField == EditField::ScriptMethod) m_SelectedElement->scriptMethod = m_ActiveInputText;
                m_SelectedElement->UpdateDrawables();
                SetDirty(true);
            }
        }
    }

    if (event.type == sf::Event::TextEntered && m_ActiveField != EditField::None && m_SelectedElement)
    {
        if (event.text.unicode == 8 || event.text.unicode == 13 || event.text.unicode == 27) return;

        if (HasTextSelection())
        {
            DeleteActiveSelection();
        }

        char c = static_cast<char>(event.text.unicode);
        if (m_ActiveField == EditField::Id || m_ActiveField == EditField::UIText || 
            m_ActiveField == EditField::OnClickParam || m_ActiveField == EditField::OnHoverParam ||
            m_ActiveField == EditField::ScriptPath ||
            m_ActiveField == EditField::ScriptMethod)
            m_ActiveInputText += c;
        else if (std::isdigit(c) || c == '-' || c == '.')
            m_ActiveInputText += c;

        if (m_ActiveField == EditField::Id) { m_SelectedElement->id = m_ActiveInputText; }
        else if (m_ActiveField == EditField::UIText) { m_SelectedElement->text = m_ActiveInputText; }
        else if (m_ActiveField == EditField::ScriptPath) { m_SelectedElement->scriptPath = m_ActiveInputText; }
        else if (m_ActiveField == EditField::ScriptMethod) { m_SelectedElement->scriptMethod = m_ActiveInputText; }
        else if (m_ActiveField == EditField::TransformX)
        {
            try { m_SelectedElement->position.x = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::TransformY)
        {
            try { m_SelectedElement->position.y = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::ZIndex)
        {
            try { m_SelectedElement->zIndex = std::stoi(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::SizeW)
        {
            try { m_SelectedElement->size.x = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::SizeH)
        {
            try { m_SelectedElement->size.y = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::ColorR)
        {
            try { m_SelectedElement->color.r = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::ColorG)
        {
            try { m_SelectedElement->color.g = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::ColorB)
        {
            try { m_SelectedElement->color.b = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::ColorA)
        {
            try { m_SelectedElement->color.a = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::Opacity)
        {
            try { m_SelectedElement->opacity = std::clamp(std::stof(m_ActiveInputText), 0.f, 255.f); } catch (...) {}
        }
        else if (m_ActiveField == EditField::CharacterSize)
        {
            try { m_SelectedElement->characterSize = static_cast<unsigned int>(std::max(1, std::stoi(m_ActiveInputText))); } catch (...) {}
        }
        else if (m_ActiveField == EditField::LetterSpacing)
        {
            try { m_SelectedElement->letterSpacing = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::LineSpacing)
        {
            try { m_SelectedElement->lineSpacing = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::TextOutlineR)
        {
            try { m_SelectedElement->textOutlineColor.r = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::TextOutlineG)
        {
            try { m_SelectedElement->textOutlineColor.g = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::TextOutlineB)
        {
            try { m_SelectedElement->textOutlineColor.b = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::TextOutlineThickness)
        {
            try { m_SelectedElement->textOutlineThickness = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::TextOffsetX)
        {
            try { m_SelectedElement->textOffset.x = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::TextOffsetY)
        {
            try { m_SelectedElement->textOffset.y = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::OutlineR)
        {
            try { m_SelectedElement->outlineColor.r = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::OutlineG)
        {
            try { m_SelectedElement->outlineColor.g = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::OutlineB)
        {
            try { m_SelectedElement->outlineColor.b = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::OutlineThickness)
        {
            try { m_SelectedElement->outlineThickness = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::BorderR)
        {
            try { m_SelectedElement->borderColor.r = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::BorderG)
        {
            try { m_SelectedElement->borderColor.g = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::BorderB)
        {
            try { m_SelectedElement->borderColor.b = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::BorderThickness)
        {
            try { m_SelectedElement->borderThickness = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::HoverR)
        {
            try { m_SelectedElement->hoverColor.r = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::HoverG)
        {
            try { m_SelectedElement->hoverColor.g = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::HoverB)
        {
            try { m_SelectedElement->hoverColor.b = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::PressedR)
        {
            try { m_SelectedElement->pressedColor.r = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::PressedG)
        {
            try { m_SelectedElement->pressedColor.g = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::PressedB)
        {
            try { m_SelectedElement->pressedColor.b = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::NormalR)
        {
            try { m_SelectedElement->normalColor.r = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::NormalG)
        {
            try { m_SelectedElement->normalColor.g = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::NormalB)
        {
            try { m_SelectedElement->normalColor.b = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::TextColorR)
        {
            try { m_SelectedElement->textColor.r = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::TextColorG)
        {
            try { m_SelectedElement->textColor.g = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::TextColorB)
        {
            try { m_SelectedElement->textColor.b = static_cast<sf::Uint8>(std::clamp(std::stoi(m_ActiveInputText), 0, 255)); } catch (...) {}
        }
        else if (m_ActiveField == EditField::OnClickParam)
        {
            m_SelectedElement->onClickParam = m_ActiveInputText;
        }
        else if (m_ActiveField == EditField::OnHoverParam)
        {
            m_SelectedElement->onHoverParam = m_ActiveInputText;
        }
        else if (m_ActiveField == EditField::TexturePath)
        {
            m_SelectedElement->texturePath = m_ActiveInputText;
            m_SelectedElement->texture = LoadTextureRobust(m_ActiveInputText);
            if (m_SelectedElement->type == UIElementType::Button && m_SelectedElement->normalColor == sf::Color(100, 100, 100))
            {
                m_SelectedElement->normalColor = sf::Color::White;
                m_SelectedElement->hoverColor = sf::Color(230, 230, 230);
                m_SelectedElement->pressedColor = sf::Color(180, 180, 180);
            }
        }
        else if (m_ActiveField == EditField::HoverTexturePath)
        {
            m_SelectedElement->hoverTexturePath = m_ActiveInputText;
            m_SelectedElement->hoverTexture = LoadTextureRobust(m_ActiveInputText);
        }
        else if (m_ActiveField == EditField::PressedTexturePath)
        {
            m_SelectedElement->pressedTexturePath = m_ActiveInputText;
            m_SelectedElement->pressedTexture = LoadTextureRobust(m_ActiveInputText);
        }
        else if (m_ActiveField == EditField::CheckedTexturePath)
        {
            m_SelectedElement->checkedTexturePath = m_ActiveInputText;
            m_SelectedElement->checkedTexture = LoadTextureRobust(m_ActiveInputText);
        }
        else if (m_ActiveField == EditField::KnobTexturePath)
        {
            m_SelectedElement->knobTexturePath = m_ActiveInputText;
            m_SelectedElement->knobTexture = LoadTextureRobust(m_ActiveInputText);
        }
        else if (m_ActiveField == EditField::FillTexturePath)
        {
            m_SelectedElement->fillTexturePath = m_ActiveInputText;
            m_SelectedElement->fillTexture = LoadTextureRobust(m_ActiveInputText);
        }
        else if (m_ActiveField == EditField::SliderValue)
        {
            try { m_SelectedElement->sliderValue = std::clamp(std::stof(m_ActiveInputText), m_SelectedElement->sliderMin, m_SelectedElement->sliderMax); } catch (...) {}
        }
        else if (m_ActiveField == EditField::SliderMin)
        {
            try { m_SelectedElement->sliderMin = std::stof(m_ActiveInputText); m_SelectedElement->sliderValue = std::max(m_SelectedElement->sliderMin, m_SelectedElement->sliderValue); } catch (...) {}
        }
        else if (m_ActiveField == EditField::SliderMax)
        {
            try { m_SelectedElement->sliderMax = std::stof(m_ActiveInputText); m_SelectedElement->sliderValue = std::min(m_SelectedElement->sliderMax, m_SelectedElement->sliderValue); } catch (...) {}
        }
        else if (m_ActiveField == EditField::ProgressValue)
        {
            try { m_SelectedElement->progressValue = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::ProgressMax)
        {
            try { m_SelectedElement->progressMax = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::LayoutSpacing)
        {
            try { m_SelectedElement->layoutSpacing = std::stof(m_ActiveInputText); } catch (...) {}
        }
        else if (m_ActiveField == EditField::LayoutPadding)
        {
            try { m_SelectedElement->layoutPadding = std::stof(m_ActiveInputText); } catch (...) {}
        }

        m_SelectedElement->UpdateDrawables();
        SetDirty(true);
    }
}

void UIEditorScene::Update(float deltaTime)
{
    m_InspectorScrollOffset += (m_InspectorTargetScroll - m_InspectorScrollOffset) * 15.f * deltaTime;
    if (m_SaveFeedbackTimer > 0.f)
        m_SaveFeedbackTimer -= deltaTime;

    std::string uiPath = UIManager::Get().GetCurrentUIPath();
    if (uiPath.empty()) uiPath = "game_ui.json";
    std::string title = uiPath + (m_HasUnsavedChanges ? "*" : "") + " [UI Editor] - RayneEngine";
    m_Window.setTitle(title);
}

void UIEditorScene::Render(sf::RenderWindow &window)
{
    const float winW = static_cast<float>(window.getSize().x);
    const float winH = static_cast<float>(window.getSize().y);
    const sf::View uiView(sf::FloatRect(0.f, 0.f, winW, winH));

    window.setView(uiView);
    UpdateBounds();
    window.clear(C_BG_CANVAS);

    DrawCanvas(window);

    window.setView(uiView);

    DrawToolbar(window);
    DrawPalette(window);
    DrawInspector(window);
    DrawHierarchy(window);

    if (m_ContentBrowser)
    {
        m_ContentBrowser->Render(window, m_BrowserBounds.left, m_BrowserBounds.top, m_BrowserBounds.width, m_BrowserBounds.height);
    }

    DrawMenuBar(window);
    DrawDropdownOverlay(window);

    if (m_ContentBrowser && m_ContentBrowser->HasDraggedAsset())
    {
        if (m_InspectorBounds.contains(m_MouseScreenPos) && m_SelectedElement)
        {
            sf::RectangleShape glow({m_InspectorBounds.width, m_InspectorBounds.height});
            glow.setPosition(m_InspectorBounds.left, m_InspectorBounds.top);
            glow.setFillColor(sf::Color(C_ACCENT.r, C_ACCENT.g, C_ACCENT.b, 20));
            glow.setOutlineColor(C_ACCENT_BRIGHT);
            glow.setOutlineThickness(2.f);
            window.draw(glow);
        }
        else if (m_CanvasBounds.contains(m_MouseScreenPos))
        {
            sf::RectangleShape glow({m_CanvasBounds.width, m_CanvasBounds.height});
            glow.setPosition(m_CanvasBounds.left, m_CanvasBounds.top);
            glow.setFillColor(sf::Color(C_ACCENT.r, C_ACCENT.g, C_ACCENT.b, 10));
            glow.setOutlineColor(sf::Color(C_ACCENT.r, C_ACCENT.g, C_ACCENT.b, 100));
            glow.setOutlineThickness(1.f);
            window.draw(glow);
        }

        m_ContentBrowser->RenderDragGhost(window);
    }

    if (m_SaveFeedbackTimer > 0.f)
    {
        sf::Text asText;
        if (m_Font) asText.setFont(*m_Font);
        asText.setCharacterSize(14);
        asText.setFillColor(C_TEXT_PRIMARY);
        asText.setString(m_FeedbackMessage.empty() ? "UI Saved successfully!" : m_FeedbackMessage);

        float tw = asText.getLocalBounds().width;
        float th = asText.getLocalBounds().height;
        float pW = tw + 40.f;
        float pH = 40.f;
        float pX = (m_Window.getSize().x - pW) / 2.f;
        float pY = m_Window.getSize().y - 100.f;

        sf::RectangleShape asBg({pW, pH});
        asBg.setFillColor(C_BG_ELEVATED);
        asBg.setOutlineColor(C_BORDER_LIGHT);
        asBg.setOutlineThickness(1.f);
        asBg.setPosition(pX, pY);

        sf::RectangleShape successBar({4.f, pH});
        successBar.setFillColor(C_SUCCESS);
        successBar.setPosition(pX, pY);

        asText.setPosition(pX + 20.f, pY + (pH - th) / 2.f - 4.f);

        sf::Uint8 alpha = static_cast<sf::Uint8>(std::clamp(m_SaveFeedbackTimer / 0.5f, 0.f, 1.f) * 255.f);
        asBg.setFillColor(sf::Color(C_BG_ELEVATED.r, C_BG_ELEVATED.g, C_BG_ELEVATED.b, alpha));
        asBg.setOutlineColor(sf::Color(C_BORDER_LIGHT.r, C_BORDER_LIGHT.g, C_BORDER_LIGHT.b, alpha));
        successBar.setFillColor(sf::Color(C_SUCCESS.r, C_SUCCESS.g, C_SUCCESS.b, alpha));
        asText.setFillColor(sf::Color(C_TEXT_PRIMARY.r, C_TEXT_PRIMARY.g, C_TEXT_PRIMARY.b, alpha));

        window.draw(asBg);
        window.draw(successBar);
        window.draw(asText);
    }

    DrawTooltip(window);
    DrawDeleteModal(window);
}

void UIEditorScene::DrawToolbar(sf::RenderWindow &window)
{
    m_ToolbarHitboxes.clear();
    const float winW = static_cast<float>(window.getSize().x);

    sf::RectangleShape bar({winW, ToolbarHeight});
    bar.setFillColor(C_BG_PANEL);
    bar.setPosition(0, MenuBarHeight);
    window.draw(bar);

    sf::RectangleShape border({winW, 1.f});
    border.setFillColor(C_BORDER);
    border.setPosition(0, TopBarHeight - 1.f);
    window.draw(border);

    sf::FloatRect backRect(10.f, MenuBarHeight + 4.f, 120.f, ToolbarHeight - 8.f);
    DrawActionButton(window, "Game Editor", "back", backRect.left, backRect.top, C_ACCENT_DIM, C_ACCENT);
    m_ToolbarHitboxes.push_back({backRect, "back"});

    sf::FloatRect saveRect(140.f, MenuBarHeight + 4.f, 80.f, ToolbarHeight - 8.f);
    DrawActionButton(window, "Save UI", "save", saveRect.left, saveRect.top, C_SUCCESS_DIM, C_SUCCESS);
    m_ToolbarHitboxes.push_back({saveRect, "save"});
}

void UIEditorScene::DrawPalette(sf::RenderWindow &window)
{
    m_PaletteHitboxes.clear();

    sf::RectangleShape panel({PaletteWidth, m_PaletteBounds.height});
    panel.setFillColor(C_BG_PANEL);
    panel.setPosition(m_PaletteBounds.left, m_PaletteBounds.top);
    window.draw(panel);

    sf::RectangleShape border({1.f, m_PaletteBounds.height});
    border.setFillColor(C_BORDER);
    border.setPosition(m_PaletteBounds.left + PaletteWidth - 1.f, m_PaletteBounds.top);
    window.draw(border);

    float y = m_PaletteBounds.top + 10.f;
    DrawSectionHeader(window, "PALETTE", C_ACCENT, m_PaletteBounds.left, y);
    y += 30.f;

    auto drawAddBtn = [&](const std::string &label, const std::string &action) {
        sf::FloatRect r(m_PaletteBounds.left + 10.f, y, PaletteWidth - 20.f, 28.f);
        DrawActionButton(window, "+ " + label, action, r.left, r.top, C_BG_ELEVATED, C_BORDER);
        m_PaletteHitboxes.push_back({r, action});
        y += 34.f;
    };

    drawAddBtn("Panel", "add_panel");
    drawAddBtn("Text", "add_text");
    drawAddBtn("Button", "add_button");
    drawAddBtn("Image", "add_image");
    drawAddBtn("Checkbox", "add_checkbox");
    drawAddBtn("Slider", "add_slider");
    drawAddBtn("Progress Bar", "add_progressbar");
    drawAddBtn("Text Input", "add_textinput");
    drawAddBtn("Vertical Box", "add_verticalbox");
    drawAddBtn("Horizontal Box", "add_horizontalbox");
}

void UIEditorScene::DrawHierarchy(sf::RenderWindow &window)
{
    m_HierarchyHitboxes.clear();
    m_HierarchyFoldHitboxes.clear();

    sf::RectangleShape panel({InspectorWidth, HierarchyHeight});
    panel.setFillColor(C_BG_PANEL);
    panel.setPosition(m_HierarchyBounds.left, m_HierarchyBounds.top);
    window.draw(panel);

    sf::RectangleShape borderTop({InspectorWidth, 1.f});
    borderTop.setFillColor(C_BORDER);
    borderTop.setPosition(m_HierarchyBounds.left, m_HierarchyBounds.top);
    window.draw(borderTop);

    float y = m_HierarchyBounds.top + 10.f;
    DrawSectionHeader(window, m_HasUnsavedChanges ? "UI HIERARCHY *" : "UI HIERARCHY", m_HasUnsavedChanges ? C_WARNING : C_ACCENT, m_HierarchyBounds.left, y);
    y += 30.f;

    auto getChildren = [](const std::string& parentId) -> std::vector<UIElement*> {
        std::vector<UIElement*> ch;
        for (auto &el : UIManager::Get().GetElements()) {
            if (el.parent == parentId && el.id != parentId) {
                ch.push_back(&el);
            }
        }
        std::stable_sort(ch.begin(), ch.end(), [](const UIElement* a, const UIElement* b) {
            return a->zIndex > b->zIndex;
        });
        return ch;
    };

    std::function<void(UIElement*, int)> drawNode = [&](UIElement* el, int depth) {
        if (!el) return;
        if (y + 24.f > m_HierarchyBounds.top + HierarchyHeight) return;

        float indent = depth * 16.f;
        sf::FloatRect fullRowRect(m_HierarchyBounds.left + 4.f, y, InspectorWidth - 8.f, 24.f);
        sf::FloatRect textRect(m_HierarchyBounds.left + 4.f + indent, y, InspectorWidth - 8.f - indent, 24.f);

        bool hov = fullRowRect.contains(m_MouseScreenPos);
        bool sel = (m_SelectedElement == el);
        bool isDropTarget = (m_HierarchyDragging && m_HierarchyDragTargetId == el->id);

        if (hov || sel || isDropTarget)
        {
            sf::RectangleShape bg(fullRowRect.getSize());
            bg.setPosition(fullRowRect.getPosition());
            if (isDropTarget) {
                bg.setFillColor(sf::Color(80, 120, 240, 140));
                bg.setOutlineColor(C_ACCENT_BRIGHT);
                bg.setOutlineThickness(1.5f);
            } else {
                bg.setFillColor(sel ? C_ACCENT_DIM : C_BG_ELEVATED);
            }
            window.draw(bg);

            if (sel && !isDropTarget) {
                sf::RectangleShape indicator({2.f, 24.f});
                indicator.setPosition(fullRowRect.left, fullRowRect.top);
                indicator.setFillColor(C_ACCENT);
                window.draw(indicator);
            }
        }

        std::vector<UIElement*> children = getChildren(el->id);
        bool hasChildren = !children.empty();
        bool isCollapsed = m_HierarchyCollapsed.count(el->id) > 0;

        if (hasChildren) {
            sf::FloatRect foldRect(m_HierarchyBounds.left + 4.f + indent, y, 14.f, 24.f);
            m_HierarchyFoldHitboxes.push_back({foldRect, el->id});

            sf::Text arrowText;
            arrowText.setFont(*m_Font);
            arrowText.setCharacterSize(10);
            arrowText.setFillColor(C_TEXT_MUTED);
            arrowText.setString(isCollapsed ? ">" : "v");
            arrowText.setPosition(foldRect.left + 3.f, foldRect.top + 6.f);
            window.draw(arrowText);
        }

        std::string display = "[Z:" + std::to_string(el->zIndex) + "] " + (el->id.empty() ? "Unnamed" : el->id);
        sf::Text t;
        t.setFont(*m_Font);
        t.setCharacterSize(12);
        t.setFillColor(sel ? sf::Color::White : C_TEXT_PRIMARY);
        t.setString(display);
        t.setPosition(textRect.left + (hasChildren ? 16.f : 6.f), textRect.top + 4.f);
        window.draw(t);

        m_HierarchyHitboxes.push_back({fullRowRect, el});
        y += 26.f;

        if (hasChildren && !isCollapsed) {
            for (auto* child : children) {
                drawNode(child, depth + 1);
            }
        }
    };

    std::vector<UIElement*> rootElements;
    for (auto &el : UIManager::Get().GetElements()) {
        if (el.parent.empty() || UIManager::Get().GetElement(el.parent) == nullptr) {
            rootElements.push_back(&el);
        }
    }
    std::stable_sort(rootElements.begin(), rootElements.end(), [](const UIElement* a, const UIElement* b) {
        return a->zIndex > b->zIndex;
    });

    for (auto* root : rootElements) {
        drawNode(root, 0);
    }

    if (y < m_HierarchyBounds.top + HierarchyHeight) {
        m_HierarchyRootDropZone = sf::FloatRect(m_HierarchyBounds.left, y, InspectorWidth, m_HierarchyBounds.top + HierarchyHeight - y);
        if (m_HierarchyDragging && m_HierarchyDragTargetId.empty() && m_HierarchyRootDropZone.contains(m_MouseScreenPos)) {
            sf::RectangleShape rootIndicator({InspectorWidth - 8.f, 20.f});
            rootIndicator.setPosition(m_HierarchyBounds.left + 4.f, y + 4.f);
            rootIndicator.setFillColor(sf::Color(80, 120, 240, 60));
            rootIndicator.setOutlineColor(C_ACCENT);
            rootIndicator.setOutlineThickness(1.f);
            window.draw(rootIndicator);

            sf::Text rText;
            rText.setFont(*m_Font);
            rText.setCharacterSize(10);
            rText.setFillColor(C_TEXT_MUTED);
            rText.setString("Detach to Root");
            rText.setPosition(m_HierarchyBounds.left + 12.f, y + 7.f);
            window.draw(rText);
        }
    } else {
        m_HierarchyRootDropZone = sf::FloatRect(0, 0, 0, 0);
    }

    if (m_HierarchyDragging && !m_HierarchyDragSourceId.empty()) {
        sf::RectangleShape ghost({130.f, 22.f});
        ghost.setPosition(m_MouseScreenPos.x + 12.f, m_MouseScreenPos.y + 12.f);
        ghost.setFillColor(sf::Color(35, 38, 46, 230));
        ghost.setOutlineColor(C_ACCENT_BRIGHT);
        ghost.setOutlineThickness(1.5f);
        window.draw(ghost);

        sf::Text gt;
        gt.setFont(*m_Font);
        gt.setCharacterSize(11);
        gt.setFillColor(sf::Color::White);
        gt.setString(m_HierarchyDragSourceId);
        gt.setPosition(m_MouseScreenPos.x + 18.f, m_MouseScreenPos.y + 15.f);
        window.draw(gt);
    }
}

void UIEditorScene::DrawInspector(sf::RenderWindow &window)
{
    m_InspectorHitboxes.clear();
    m_ActiveTooltip.clear();

    sf::RectangleShape panel({InspectorWidth, m_InspectorBounds.height});
    panel.setFillColor(C_BG_PANEL);
    panel.setPosition(m_InspectorBounds.left, m_InspectorBounds.top);
    window.draw(panel);

    sf::RectangleShape border({1.f, m_InspectorBounds.height});
    border.setFillColor(C_BORDER);
    border.setPosition(m_InspectorBounds.left, m_InspectorBounds.top);
    window.draw(border);

    if (!m_SelectedElement)
    {
        m_InspectorClipTop    = 0.f;
        m_InspectorClipBottom = 99999.f;
        return;
    }

    const float clipTop    = m_InspectorBounds.top;
    const float clipBottom = m_InspectorBounds.top + m_InspectorBounds.height;

    m_InspectorClipTop    = clipTop;
    m_InspectorClipBottom = clipBottom;

    float scrollOff = m_InspectorScrollOffset;
    float y = m_InspectorBounds.top + 10.f - scrollOff;
    float px = m_InspectorBounds.left;

    y = DrawSectionHeader(window, "PROPERTIES", C_ACCENT, px, y);

    std::string idDisplay = (m_ActiveField == EditField::Id && !m_ActiveInputText.empty())
                                ? m_ActiveInputText + "|"
                                : (m_ActiveField == EditField::Id ? "|" : m_SelectedElement->id);
    y = DrawEditableRow(window, "ID", idDisplay, "edit_id", px, y);

    std::string parentDisplay = m_SelectedElement->parent.empty() ? "(None)" : m_SelectedElement->parent;
    y = DrawRow(window, "Parent", parentDisplay, px, y);
    if (!m_SelectedElement->parent.empty())
    {
        DrawActionButton(window, "Detach Parent", "clear_parent", px + 10.f, y, C_DANGER_DIM, C_DANGER);
        y += 30.f;
    }

    if (m_SelectedElement->type == UIElementType::VerticalBox || m_SelectedElement->type == UIElementType::HorizontalBox)
    {
        y += 6.f;
        y = DrawSectionHeader(window, "BOX LAYOUT", sf::Color(120, 220, 160), px, y);

        std::string spacingDisplay = (m_ActiveField == EditField::LayoutSpacing && !m_ActiveInputText.empty())
                                         ? m_ActiveInputText + "|"
                                         : (m_ActiveField == EditField::LayoutSpacing ? "|" : FormatFloat(m_SelectedElement->layoutSpacing, 1));
        y = DrawEditableRow(window, "Spacing", spacingDisplay, "edit_layoutspacing", px, y);

        std::string paddingDisplay = (m_ActiveField == EditField::LayoutPadding && !m_ActiveInputText.empty())
                                         ? m_ActiveInputText + "|"
                                         : (m_ActiveField == EditField::LayoutPadding ? "|" : FormatFloat(m_SelectedElement->layoutPadding, 1));
        y = DrawEditableRow(window, "Padding", paddingDisplay, "edit_layoutpadding", px, y);
    }

    y += 10.f;
    y = DrawSectionHeader(window, "TRANSFORM", sf::Color(100, 200, 255), px, y);

    std::string xDisplay = (m_ActiveField == EditField::TransformX && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::TransformX
                                      ? "|"
                                      : std::to_string((int) m_SelectedElement->position.x));
    y = DrawEditableRow(window, "Position X", xDisplay, "edit_x", px, y);
    std::string yDisplay = (m_ActiveField == EditField::TransformY && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::TransformY
                                      ? "|"
                                      : std::to_string((int) m_SelectedElement->position.y));
    y = DrawEditableRow(window, "Position Y", yDisplay, "edit_y", px, y);

    std::string zDisplay = (m_ActiveField == EditField::ZIndex && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::ZIndex ? "|" : std::to_string(m_SelectedElement->zIndex));
    y = DrawEditableRow(window, "Z-Index (Depth)", zDisplay, "edit_z", px, y);

    y += 4.f;
    DrawActionButton(window, "+ Forward", "layer_forward", px + 10.f, y, C_BG_ELEVATED, C_BORDER_LIGHT);
    DrawActionButton(window, "- Backward", "layer_backward", px + 110.f, y, C_BG_ELEVATED, C_BORDER_LIGHT);
    y += 30.f;

    std::string wDisplay = (m_ActiveField == EditField::SizeW && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::SizeW
                                      ? "|"
                                      : std::to_string((int) m_SelectedElement->size.x));
    y = DrawEditableRow(window, "Width", wDisplay, "edit_w", px, y);
    std::string hDisplay = (m_ActiveField == EditField::SizeH && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::SizeH
                                      ? "|"
                                      : std::to_string((int) m_SelectedElement->size.y));
    y = DrawEditableRow(window, "Height", hDisplay, "edit_h", px, y);

    y += 10.f;
    y = DrawSectionHeader(window, "APPEARANCE", sf::Color(255, 200, 100), px, y);

    bool isVisible = m_SelectedElement->visible;
    DrawActionButton(window, isVisible ? "[Visible]" : "[Hidden]", "visible_toggle",
                     px + 10.f, y,
                     isVisible ? C_SUCCESS_DIM : C_BG_ELEVATED,
                     isVisible ? C_SUCCESS : C_BORDER_LIGHT);
    y += 30.f;

    std::string opacityDisplay = (m_ActiveField == EditField::Opacity && !m_ActiveInputText.empty())
                                     ? m_ActiveInputText + "|"
                                     : (m_ActiveField == EditField::Opacity
                                            ? "|"
                                            : std::to_string((int)m_SelectedElement->opacity));
    y = DrawEditableRow(window, "Opacity", opacityDisplay, "edit_opacity", px, y);

    if (m_SelectedElement->type == UIElementType::Panel || m_SelectedElement->type == UIElementType::Image ||
        m_SelectedElement->type == UIElementType::VerticalBox || m_SelectedElement->type == UIElementType::HorizontalBox)
    {
        std::string colorHeader = (m_SelectedElement->type == UIElementType::Image) ? "TINT COLOR" : "BG COLOR";
        y += 6.f;
        y = DrawSectionHeader(window, colorHeader, sf::Color(200, 200, 255), px, y);
        y = DrawColorPickerRow(window, "Color", m_SelectedElement->color, "pick_color_main", px, y);

        std::string rDisplay = (m_ActiveField == EditField::ColorR && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::ColorR ? "|" : std::to_string(m_SelectedElement->color.r));
        y = DrawEditableRow(window, "Color Red", rDisplay, "edit_r", px, y);
        std::string gDisplay = (m_ActiveField == EditField::ColorG && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::ColorG ? "|" : std::to_string(m_SelectedElement->color.g));
        y = DrawEditableRow(window, "Color Green", gDisplay, "edit_g", px, y);
        std::string bDisplay = (m_ActiveField == EditField::ColorB && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::ColorB ? "|" : std::to_string(m_SelectedElement->color.b));
        y = DrawEditableRow(window, "Color Blue", bDisplay, "edit_b", px, y);
        std::string aDisplay = (m_ActiveField == EditField::ColorA && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::ColorA ? "|" : std::to_string(m_SelectedElement->color.a));
        y = DrawEditableRow(window, "Alpha", aDisplay, "edit_a", px, y);

        y += 6.f;
        y = DrawSectionHeader(window, "TEXTURE", sf::Color(255, 150, 100), px, y);
        y = DrawTextureSlot(window, "Texture", m_SelectedElement->texturePath, "edit_texturepath", "clear_texture", "browse_texture", px, y);

        y += 6.f;
        y = DrawSectionHeader(window, "OUTLINE", sf::Color(180, 180, 180), px, y);
        y = DrawColorPickerRow(window, "Outline Color", m_SelectedElement->outlineColor, "pick_color_outline", px, y);
        std::string outRDisplay = (m_ActiveField == EditField::OutlineR && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::OutlineR ? "|" : std::to_string(m_SelectedElement->outlineColor.r));
        y = DrawEditableRow(window, "Outline Red", outRDisplay, "edit_outline_r", px, y);
        std::string outGDisplay = (m_ActiveField == EditField::OutlineG && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::OutlineG ? "|" : std::to_string(m_SelectedElement->outlineColor.g));
        y = DrawEditableRow(window, "Outline Green", outGDisplay, "edit_outline_g", px, y);
        std::string outBDisplay = (m_ActiveField == EditField::OutlineB && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::OutlineB ? "|" : std::to_string(m_SelectedElement->outlineColor.b));
        y = DrawEditableRow(window, "Outline Blue", outBDisplay, "edit_outline_b", px, y);
        std::string outThkDisplay = (m_ActiveField == EditField::OutlineThickness && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::OutlineThickness ? "|" : std::to_string((int)m_SelectedElement->outlineThickness));
        y = DrawEditableRow(window, "Outline Thickness", outThkDisplay, "edit_outline_thickness", px, y);
    }

    if (m_SelectedElement->type == UIElementType::Text || m_SelectedElement->type == UIElementType::TextInput || m_SelectedElement->type == UIElementType::Button)
    {
        y += 10.f;
        std::string textSecTitle = (m_SelectedElement->type == UIElementType::TextInput) ? "TEXT INPUT" : "TEXT";
        y = DrawSectionHeader(window, textSecTitle, sf::Color(255, 100, 150), px, y);

        std::string txtLabel = (m_SelectedElement->type == UIElementType::TextInput) ? "Default Text" : "Text";
        std::string txtDisplay = (m_ActiveField == EditField::UIText && !m_ActiveInputText.empty())
                                     ? m_ActiveInputText + "|"
                                     : (m_ActiveField == EditField::UIText ? "|" : m_SelectedElement->text);
        y = DrawEditableRow(window, txtLabel, txtDisplay, "edit_text", px, y);

        std::string fsDisplay = (m_ActiveField == EditField::CharacterSize && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::CharacterSize ? "|" : std::to_string(m_SelectedElement->characterSize));
        y = DrawEditableRow(window, "Font Size", fsDisplay, "edit_fontsize", px, y);

        y += 4.f;
        bool isLeft   = m_SelectedElement->textAlign == TextAlign::Left;
        bool isCenter = m_SelectedElement->textAlign == TextAlign::Center;
        bool isRight  = m_SelectedElement->textAlign == TextAlign::Right;
        DrawActionButton(window, "Left",   "align_left",   px + 10.f,  y, isLeft   ? C_ACCENT_DIM : C_BG_ELEVATED, isLeft   ? C_ACCENT : C_BORDER_LIGHT);
        DrawActionButton(window, "Center", "align_center", px + 70.f,  y, isCenter ? C_ACCENT_DIM : C_BG_ELEVATED, isCenter ? C_ACCENT : C_BORDER_LIGHT);
        DrawActionButton(window, "Right",  "align_right",  px + 150.f, y, isRight  ? C_ACCENT_DIM : C_BG_ELEVATED, isRight  ? C_ACCENT : C_BORDER_LIGHT);
        y += 30.f;

        if (m_SelectedElement->type == UIElementType::Text)
        {
            bool isTop    = m_SelectedElement->textVAlign == TextVAlign::Top;
            bool isMiddle = m_SelectedElement->textVAlign == TextVAlign::Middle;
            bool isBottom = m_SelectedElement->textVAlign == TextVAlign::Bottom;
            DrawActionButton(window, "Top",    "valign_top",    px + 10.f,  y, isTop    ? C_ACCENT_DIM : C_BG_ELEVATED, isTop    ? C_ACCENT : C_BORDER_LIGHT);
            DrawActionButton(window, "Middle", "valign_middle", px + 70.f,  y, isMiddle ? C_ACCENT_DIM : C_BG_ELEVATED, isMiddle ? C_ACCENT : C_BORDER_LIGHT);
            DrawActionButton(window, "Bottom", "valign_bottom", px + 150.f, y, isBottom ? C_ACCENT_DIM : C_BG_ELEVATED, isBottom ? C_ACCENT : C_BORDER_LIGHT);
            y += 30.f;
        }

        if (m_SelectedElement->type != UIElementType::TextInput)
        {
            bool isBold      = (m_SelectedElement->textStyle & sf::Text::Bold) != 0;
            bool isItalic    = (m_SelectedElement->textStyle & sf::Text::Italic) != 0;
            bool isUnderline = (m_SelectedElement->textStyle & sf::Text::Underlined) != 0;
            bool isUpperCase = m_SelectedElement->textUpperCase;
            DrawActionButton(window, "Bold",      "style_bold_toggle",      px + 10.f,  y, isBold      ? C_ACCENT_DIM : C_BG_ELEVATED, isBold      ? C_ACCENT : C_BORDER_LIGHT);
            DrawActionButton(window, "Italic",    "style_italic_toggle",    px + 70.f,  y, isItalic    ? C_ACCENT_DIM : C_BG_ELEVATED, isItalic    ? C_ACCENT : C_BORDER_LIGHT);
            DrawActionButton(window, "Underline", "style_underline_toggle", px + 130.f, y, isUnderline ? C_ACCENT_DIM : C_BG_ELEVATED, isUnderline ? C_ACCENT : C_BORDER_LIGHT);
            y += 30.f;
            DrawActionButton(window, "UpperCase", "uppercase_toggle", px + 10.f, y, isUpperCase ? C_ACCENT_DIM : C_BG_ELEVATED, isUpperCase ? C_ACCENT : C_BORDER_LIGHT);
            y += 30.f;

            std::string lsDisplay = (m_ActiveField == EditField::LetterSpacing && !m_ActiveInputText.empty())
                                        ? m_ActiveInputText + "|"
                                        : (m_ActiveField == EditField::LetterSpacing ? "|" : FormatFloat(m_SelectedElement->letterSpacing, 2));
            y = DrawEditableRow(window, "Letter Spacing", lsDisplay, "edit_letterspacing", px, y);
            std::string lineDisplay = (m_ActiveField == EditField::LineSpacing && !m_ActiveInputText.empty())
                                          ? m_ActiveInputText + "|"
                                          : (m_ActiveField == EditField::LineSpacing ? "|" : FormatFloat(m_SelectedElement->lineSpacing, 2));
            y = DrawEditableRow(window, "Line Spacing", lineDisplay, "edit_linespacing", px, y);

            y = DrawColorPickerRow(window, "Text Outline Color", m_SelectedElement->textOutlineColor, "pick_color_textoutline", px, y);
            std::string toRDisplay = (m_ActiveField == EditField::TextOutlineR && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::TextOutlineR ? "|" : std::to_string(m_SelectedElement->textOutlineColor.r));
            y = DrawEditableRow(window, "Text Outline Red", toRDisplay, "edit_textoutline_r", px, y);
            std::string toGDisplay = (m_ActiveField == EditField::TextOutlineG && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::TextOutlineG ? "|" : std::to_string(m_SelectedElement->textOutlineColor.g));
            y = DrawEditableRow(window, "Text Outline Green", toGDisplay, "edit_textoutline_g", px, y);
            std::string toBDisplay = (m_ActiveField == EditField::TextOutlineB && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::TextOutlineB ? "|" : std::to_string(m_SelectedElement->textOutlineColor.b));
            y = DrawEditableRow(window, "Text Outline Blue", toBDisplay, "edit_textoutline_b", px, y);
            std::string toThkDisplay = (m_ActiveField == EditField::TextOutlineThickness && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::TextOutlineThickness ? "|" : std::to_string((int)m_SelectedElement->textOutlineThickness));
            y = DrawEditableRow(window, "Text Outline Thickness", toThkDisplay, "edit_textoutline_thickness", px, y);

            std::string txDisplay = (m_ActiveField == EditField::TextOffsetX && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::TextOffsetX ? "|" : std::to_string((int)m_SelectedElement->textOffset.x));
            y = DrawEditableRow(window, "Text Offset X", txDisplay, "edit_textoffset_x", px, y);
            std::string tyDisplay = (m_ActiveField == EditField::TextOffsetY && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::TextOffsetY ? "|" : std::to_string((int)m_SelectedElement->textOffset.y));
            y = DrawEditableRow(window, "Text Offset Y", tyDisplay, "edit_textoffset_y", px, y);
        }

        y = DrawColorPickerRow(window, "Text Color", m_SelectedElement->textColor, "pick_color_text", px, y);
        std::string tcRDisplay = (m_ActiveField == EditField::TextColorR && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::TextColorR ? "|" : std::to_string(m_SelectedElement->textColor.r));
        y = DrawEditableRow(window, "Text Red", tcRDisplay, "edit_textcolor_r", px, y);
        std::string tcGDisplay = (m_ActiveField == EditField::TextColorG && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::TextColorG ? "|" : std::to_string(m_SelectedElement->textColor.g));
        y = DrawEditableRow(window, "Text Green", tcGDisplay, "edit_textcolor_g", px, y);
        std::string tcBDisplay = (m_ActiveField == EditField::TextColorB && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::TextColorB ? "|" : std::to_string(m_SelectedElement->textColor.b));
        y = DrawEditableRow(window, "Text Blue", tcBDisplay, "edit_textcolor_b", px, y);
    }

    if (m_SelectedElement->type == UIElementType::TextInput)
    {
        y += 10.f;
        y = DrawSectionHeader(window, "INPUT FIELD STYLE", sf::Color(100, 220, 255), px, y);

        y = DrawColorPickerRow(window, "Background Color", m_SelectedElement->normalColor, "pick_color_normal", px, y);
        std::string nrDisplay = (m_ActiveField == EditField::NormalR && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::NormalR ? "|" : std::to_string(m_SelectedElement->normalColor.r));
        y = DrawEditableRow(window, "Background Red", nrDisplay, "edit_normalr", px, y);
        std::string ngDisplay = (m_ActiveField == EditField::NormalG && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::NormalG ? "|" : std::to_string(m_SelectedElement->normalColor.g));
        y = DrawEditableRow(window, "Background Green", ngDisplay, "edit_normalg", px, y);
        std::string nbDisplay = (m_ActiveField == EditField::NormalB && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::NormalB ? "|" : std::to_string(m_SelectedElement->normalColor.b));
        y = DrawEditableRow(window, "Background Blue", nbDisplay, "edit_normalb", px, y);

        y = DrawColorPickerRow(window, "Focus Color", m_SelectedElement->pressedColor, "pick_color_pressed", px, y);
        std::string prDisplay = (m_ActiveField == EditField::PressedR && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::PressedR ? "|" : std::to_string(m_SelectedElement->pressedColor.r));
        y = DrawEditableRow(window, "Focus Red", prDisplay, "edit_pressedr", px, y);
        std::string pgDisplay = (m_ActiveField == EditField::PressedG && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::PressedG ? "|" : std::to_string(m_SelectedElement->pressedColor.g));
        y = DrawEditableRow(window, "Focus Green", pgDisplay, "edit_pressedg", px, y);
        std::string pbDisplay = (m_ActiveField == EditField::PressedB && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::PressedB ? "|" : std::to_string(m_SelectedElement->pressedColor.b));
        y = DrawEditableRow(window, "Focus Blue", pbDisplay, "edit_pressedb", px, y);

        y = DrawColorPickerRow(window, "Border Color", m_SelectedElement->borderColor, "pick_color_border", px, y);
        std::string brDisplay = (m_ActiveField == EditField::BorderR && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::BorderR ? "|" : std::to_string(m_SelectedElement->borderColor.r));
        y = DrawEditableRow(window, "Border Red", brDisplay, "edit_borderr", px, y);
        std::string bgDisplay = (m_ActiveField == EditField::BorderG && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::BorderG ? "|" : std::to_string(m_SelectedElement->borderColor.g));
        y = DrawEditableRow(window, "Border Green", bgDisplay, "edit_borderg", px, y);
        std::string bbDisplay = (m_ActiveField == EditField::BorderB && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::BorderB ? "|" : std::to_string(m_SelectedElement->borderColor.b));
        y = DrawEditableRow(window, "Border Blue", bbDisplay, "edit_borderb", px, y);
        std::string bThkDisplay = (m_ActiveField == EditField::BorderThickness && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::BorderThickness ? "|" : std::to_string((int)m_SelectedElement->borderThickness));
        y = DrawEditableRow(window, "Border Thickness", bThkDisplay, "edit_borderthickness", px, y);
    }

    if (m_SelectedElement->type == UIElementType::Button)
    {
        y += 10.f;
        y = DrawSectionHeader(window, "BUTTON STYLE", sf::Color(100, 220, 255), px, y);

        y = DrawTextureSlot(window, "Normal Texture", m_SelectedElement->texturePath, "edit_texturepath", "clear_texture", "browse_texture", px, y);
        y = DrawTextureSlot(window, "Hover Texture", m_SelectedElement->hoverTexturePath, "edit_hovertexturepath", "clear_hovertexture", "browse_hovertexture", px, y);
        y = DrawTextureSlot(window, "Pressed Texture", m_SelectedElement->pressedTexturePath, "edit_pressedtexturepath", "clear_pressedtexture", "browse_pressedtexture", px, y);
        y += 4.f;

        y = DrawColorPickerRow(window, "Normal Color", m_SelectedElement->normalColor, "pick_color_normal", px, y);
        std::string nrDisplay = (m_ActiveField == EditField::NormalR && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::NormalR ? "|" : std::to_string(m_SelectedElement->normalColor.r));
        y = DrawEditableRow(window, "Normal Red", nrDisplay, "edit_normalr", px, y);
        std::string ngDisplay = (m_ActiveField == EditField::NormalG && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::NormalG ? "|" : std::to_string(m_SelectedElement->normalColor.g));
        y = DrawEditableRow(window, "Normal Green", ngDisplay, "edit_normalg", px, y);
        std::string nbDisplay = (m_ActiveField == EditField::NormalB && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::NormalB ? "|" : std::to_string(m_SelectedElement->normalColor.b));
        y = DrawEditableRow(window, "Normal Blue", nbDisplay, "edit_normalb", px, y);

        y = DrawColorPickerRow(window, "Hover Color", m_SelectedElement->hoverColor, "pick_color_hover", px, y);
        std::string hrDisplay = (m_ActiveField == EditField::HoverR && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::HoverR ? "|" : std::to_string(m_SelectedElement->hoverColor.r));
        y = DrawEditableRow(window, "Hover Red", hrDisplay, "edit_hoverr", px, y);
        std::string hgDisplay = (m_ActiveField == EditField::HoverG && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::HoverG ? "|" : std::to_string(m_SelectedElement->hoverColor.g));
        y = DrawEditableRow(window, "Hover Green", hgDisplay, "edit_hoverg", px, y);
        std::string hbDisplay = (m_ActiveField == EditField::HoverB && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::HoverB ? "|" : std::to_string(m_SelectedElement->hoverColor.b));
        y = DrawEditableRow(window, "Hover Blue", hbDisplay, "edit_hoverb", px, y);

        y = DrawColorPickerRow(window, "Pressed Color", m_SelectedElement->pressedColor, "pick_color_pressed", px, y);
        std::string prDisplay = (m_ActiveField == EditField::PressedR && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::PressedR ? "|" : std::to_string(m_SelectedElement->pressedColor.r));
        y = DrawEditableRow(window, "Pressed Red", prDisplay, "edit_pressedr", px, y);
        std::string pgDisplay = (m_ActiveField == EditField::PressedG && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::PressedG ? "|" : std::to_string(m_SelectedElement->pressedColor.g));
        y = DrawEditableRow(window, "Pressed Green", pgDisplay, "edit_pressedg", px, y);
        std::string pbDisplay = (m_ActiveField == EditField::PressedB && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::PressedB ? "|" : std::to_string(m_SelectedElement->pressedColor.b));
        y = DrawEditableRow(window, "Pressed Blue", pbDisplay, "edit_pressedb", px, y);

        y = DrawColorPickerRow(window, "Border Color", m_SelectedElement->borderColor, "pick_color_border", px, y);
        std::string brDisplay = (m_ActiveField == EditField::BorderR && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::BorderR ? "|" : std::to_string(m_SelectedElement->borderColor.r));
        y = DrawEditableRow(window, "Border Red", brDisplay, "edit_borderr", px, y);
        std::string bgDisplay = (m_ActiveField == EditField::BorderG && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::BorderG ? "|" : std::to_string(m_SelectedElement->borderColor.g));
        y = DrawEditableRow(window, "Border Green", bgDisplay, "edit_borderg", px, y);
        std::string bbDisplay = (m_ActiveField == EditField::BorderB && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::BorderB ? "|" : std::to_string(m_SelectedElement->borderColor.b));
        y = DrawEditableRow(window, "Border Blue", bbDisplay, "edit_borderb", px, y);
        std::string bThkDisplay = (m_ActiveField == EditField::BorderThickness && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::BorderThickness ? "|" : std::to_string((int)m_SelectedElement->borderThickness));
        y = DrawEditableRow(window, "Border Thickness", bThkDisplay, "edit_borderthickness", px, y);

        y += 4.f;
        bool isDisabled = m_SelectedElement->disabled;
        DrawActionButton(window, isDisabled ? "[Disabled]" : "[Enabled]", "disabled_toggle",
                         px + 10.f, y,
                         isDisabled ? C_DANGER_DIM : C_SUCCESS_DIM,
                         isDisabled ? C_DANGER : C_SUCCESS);
        y += 30.f;
    }

    if (m_SelectedElement->type == UIElementType::Checkbox)
    {
        y += 10.f;
        y = DrawSectionHeader(window, "CHECKBOX", sf::Color(100, 255, 200), px, y);

        y += 4.f;
        bool isChecked = m_SelectedElement->isChecked;
        DrawActionButton(window, isChecked ? "[Checked]" : "[Unchecked]", "checked_toggle",
                         px + 10.f, y,
                         isChecked ? C_SUCCESS_DIM : C_BG_ELEVATED,
                         isChecked ? C_SUCCESS : C_BORDER_LIGHT);
        y += 30.f;

        y = DrawTextureSlot(window, "Unchecked Texture", m_SelectedElement->texturePath, "edit_texturepath", "clear_texture", "browse_texture", px, y);
        y = DrawTextureSlot(window, "Checked Texture", m_SelectedElement->checkedTexturePath, "edit_checkedtexturepath", "clear_checkedtexture", "browse_checkedtexture", px, y);

        y += 6.f;
        y = DrawColorPickerRow(window, "Box Color", m_SelectedElement->normalColor, "pick_color_normal", px, y);
        std::string nrDisplay = (m_ActiveField == EditField::NormalR && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::NormalR ? "|" : std::to_string(m_SelectedElement->normalColor.r));
        y = DrawEditableRow(window, "Box Red", nrDisplay, "edit_normalr", px, y);
        std::string ngDisplay = (m_ActiveField == EditField::NormalG && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::NormalG ? "|" : std::to_string(m_SelectedElement->normalColor.g));
        y = DrawEditableRow(window, "Box Green", ngDisplay, "edit_normalg", px, y);
        std::string nbDisplay = (m_ActiveField == EditField::NormalB && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::NormalB ? "|" : std::to_string(m_SelectedElement->normalColor.b));
        y = DrawEditableRow(window, "Box Blue", nbDisplay, "edit_normalb", px, y);

        y = DrawColorPickerRow(window, "Check Color", m_SelectedElement->textColor, "pick_color_text", px, y);
        std::string tcRDisplay = (m_ActiveField == EditField::TextColorR && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::TextColorR ? "|" : std::to_string(m_SelectedElement->textColor.r));
        y = DrawEditableRow(window, "Check Red", tcRDisplay, "edit_textcolor_r", px, y);
        std::string tcGDisplay = (m_ActiveField == EditField::TextColorG && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::TextColorG ? "|" : std::to_string(m_SelectedElement->textColor.g));
        y = DrawEditableRow(window, "Check Green", tcGDisplay, "edit_textcolor_g", px, y);
        std::string tcBDisplay = (m_ActiveField == EditField::TextColorB && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::TextColorB ? "|" : std::to_string(m_SelectedElement->textColor.b));
        y = DrawEditableRow(window, "Check Blue", tcBDisplay, "edit_textcolor_b", px, y);

        y = DrawColorPickerRow(window, "Border Color", m_SelectedElement->borderColor, "pick_color_border", px, y);
        std::string brDisplay = (m_ActiveField == EditField::BorderR && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::BorderR ? "|" : std::to_string(m_SelectedElement->borderColor.r));
        y = DrawEditableRow(window, "Border Red", brDisplay, "edit_borderr", px, y);
        std::string bgDisplay = (m_ActiveField == EditField::BorderG && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::BorderG ? "|" : std::to_string(m_SelectedElement->borderColor.g));
        y = DrawEditableRow(window, "Border Green", bgDisplay, "edit_borderg", px, y);
        std::string bbDisplay = (m_ActiveField == EditField::BorderB && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::BorderB ? "|" : std::to_string(m_SelectedElement->borderColor.b));
        y = DrawEditableRow(window, "Border Blue", bbDisplay, "edit_borderb", px, y);
        std::string bThkDisplay = (m_ActiveField == EditField::BorderThickness && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::BorderThickness ? "|" : std::to_string((int)m_SelectedElement->borderThickness));
        y = DrawEditableRow(window, "Border Thickness", bThkDisplay, "edit_borderthickness", px, y);
    }

    if (m_SelectedElement->type == UIElementType::Slider)
    {
        y += 10.f;
        y = DrawSectionHeader(window, "SLIDER", sf::Color(100, 200, 255), px, y);

        std::string svDisplay = (m_ActiveField == EditField::SliderValue && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::SliderValue ? "|" : FormatFloat(m_SelectedElement->sliderValue, 2));
        y = DrawEditableRow(window, "Value", svDisplay, "edit_slidervalue", px, y);
        std::string smDisplay = (m_ActiveField == EditField::SliderMin && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::SliderMin ? "|" : FormatFloat(m_SelectedElement->sliderMin, 2));
        y = DrawEditableRow(window, "Min", smDisplay, "edit_slidermin", px, y);
        std::string smaxDisplay = (m_ActiveField == EditField::SliderMax && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::SliderMax ? "|" : FormatFloat(m_SelectedElement->sliderMax, 2));
        y = DrawEditableRow(window, "Max", smaxDisplay, "edit_slidermax", px, y);

        y = DrawTextureSlot(window, "Track Texture", m_SelectedElement->texturePath, "edit_texturepath", "clear_texture", "browse_texture", px, y);
        y = DrawTextureSlot(window, "Knob Texture", m_SelectedElement->knobTexturePath, "edit_knobtexturepath", "clear_knobtexture", "browse_knobtexture", px, y);

        y = DrawColorPickerRow(window, "Track Color", m_SelectedElement->color, "pick_color_main", px, y);
        std::string rDisplay = (m_ActiveField == EditField::ColorR && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::ColorR ? "|" : std::to_string(m_SelectedElement->color.r));
        y = DrawEditableRow(window, "Track Red", rDisplay, "edit_r", px, y);
        std::string gDisplay = (m_ActiveField == EditField::ColorG && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::ColorG ? "|" : std::to_string(m_SelectedElement->color.g));
        y = DrawEditableRow(window, "Track Green", gDisplay, "edit_g", px, y);
        std::string bDisplay = (m_ActiveField == EditField::ColorB && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::ColorB ? "|" : std::to_string(m_SelectedElement->color.b));
        y = DrawEditableRow(window, "Track Blue", bDisplay, "edit_b", px, y);

        y = DrawColorPickerRow(window, "Knob Color", m_SelectedElement->normalColor, "pick_color_normal", px, y);
        std::string nrDisplay = (m_ActiveField == EditField::NormalR && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::NormalR ? "|" : std::to_string(m_SelectedElement->normalColor.r));
        y = DrawEditableRow(window, "Knob Red", nrDisplay, "edit_normalr", px, y);
        std::string ngDisplay = (m_ActiveField == EditField::NormalG && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::NormalG ? "|" : std::to_string(m_SelectedElement->normalColor.g));
        y = DrawEditableRow(window, "Knob Green", ngDisplay, "edit_normalg", px, y);
        std::string nbDisplay = (m_ActiveField == EditField::NormalB && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::NormalB ? "|" : std::to_string(m_SelectedElement->normalColor.b));
        y = DrawEditableRow(window, "Knob Blue", nbDisplay, "edit_normalb", px, y);
    }

    if (m_SelectedElement->type == UIElementType::ProgressBar)
    {
        y += 10.f;
        y = DrawSectionHeader(window, "PROGRESS BAR", sf::Color(255, 100, 255), px, y);

        std::string pvDisplay = (m_ActiveField == EditField::ProgressValue && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::ProgressValue ? "|" : FormatFloat(m_SelectedElement->progressValue, 2));
        y = DrawEditableRow(window, "Progress", pvDisplay, "edit_progressvalue", px, y);
        std::string pmaxDisplay = (m_ActiveField == EditField::ProgressMax && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::ProgressMax ? "|" : FormatFloat(m_SelectedElement->progressMax, 2));
        y = DrawEditableRow(window, "Max", pmaxDisplay, "edit_progressmax", px, y);

        y = DrawTextureSlot(window, "Background Texture", m_SelectedElement->texturePath, "edit_texturepath", "clear_texture", "browse_texture", px, y);
        y = DrawTextureSlot(window, "Fill Texture", m_SelectedElement->fillTexturePath, "edit_filltexturepath", "clear_filltexture", "browse_filltexture", px, y);

        y = DrawColorPickerRow(window, "Track Color", m_SelectedElement->color, "pick_color_main", px, y);
        std::string rDisplay = (m_ActiveField == EditField::ColorR && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::ColorR ? "|" : std::to_string(m_SelectedElement->color.r));
        y = DrawEditableRow(window, "Track Red", rDisplay, "edit_r", px, y);
        std::string gDisplay = (m_ActiveField == EditField::ColorG && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::ColorG ? "|" : std::to_string(m_SelectedElement->color.g));
        y = DrawEditableRow(window, "Track Green", gDisplay, "edit_g", px, y);
        std::string bDisplay = (m_ActiveField == EditField::ColorB && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::ColorB ? "|" : std::to_string(m_SelectedElement->color.b));
        y = DrawEditableRow(window, "Track Blue", bDisplay, "edit_b", px, y);

        y = DrawColorPickerRow(window, "Fill Color", m_SelectedElement->normalColor, "pick_color_normal", px, y);
        std::string nrDisplay = (m_ActiveField == EditField::NormalR && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::NormalR ? "|" : std::to_string(m_SelectedElement->normalColor.r));
        y = DrawEditableRow(window, "Fill Red", nrDisplay, "edit_normalr", px, y);
        std::string ngDisplay = (m_ActiveField == EditField::NormalG && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::NormalG ? "|" : std::to_string(m_SelectedElement->normalColor.g));
        y = DrawEditableRow(window, "Fill Green", ngDisplay, "edit_normalg", px, y);
        std::string nbDisplay = (m_ActiveField == EditField::NormalB && !m_ActiveInputText.empty()) ? m_ActiveInputText + "|" : (m_ActiveField == EditField::NormalB ? "|" : std::to_string(m_SelectedElement->normalColor.b));
        y = DrawEditableRow(window, "Fill Blue", nbDisplay, "edit_normalb", px, y);
    }

    if (m_SelectedElement->type == UIElementType::Button || m_SelectedElement->type == UIElementType::Checkbox)
    {
        y += 10.f;
        y = DrawSectionHeader(window, "ACTIONS", sf::Color(150, 255, 150), px, y);

        std::string clickLabel = m_SelectedElement->onClickAction.empty() ? "None" : m_SelectedElement->onClickAction;
        ActionParamType clickParamType = ActionParamType::None;
        for (const auto& opt : UTILITY_ACTIONS) {
            if (opt.code == m_SelectedElement->onClickAction) { clickLabel = opt.label; clickParamType = opt.paramType; break; }
        }
        if (clickLabel.length() > 18) clickLabel = clickLabel.substr(0, 15) + "..";
        y = DrawEditableRow(window, "onClick", clickLabel, "dropdown_onclick", px, y);

        if (clickParamType == ActionParamType::Bool) {
            y += 4.f;
            bool isParamTrue = (m_SelectedElement->onClickParam == "true");
            DrawActionButton(window, isParamTrue ? "True" : "False", "clickparam_bool", px + 110.f, y, isParamTrue ? C_SUCCESS_DIM : C_DANGER_DIM, isParamTrue ? C_SUCCESS : C_DANGER);
            y += 30.f;
        } else if (clickParamType == ActionParamType::String || clickParamType == ActionParamType::Float) {
            std::string cpDisplay = (m_ActiveField == EditField::OnClickParam && !m_ActiveInputText.empty())
                                        ? m_ActiveInputText + "|"
                                        : (m_ActiveField == EditField::OnClickParam ? "|" : m_SelectedElement->onClickParam);
            y = DrawEditableRow(window, "  Param", cpDisplay, "edit_clickparam", px, y);
        }

        std::string hoverLabel = m_SelectedElement->onHoverAction.empty() ? "None" : m_SelectedElement->onHoverAction;
        ActionParamType hoverParamType = ActionParamType::None;
        for (const auto& opt : UTILITY_ACTIONS) {
            if (opt.code == m_SelectedElement->onHoverAction) { hoverLabel = opt.label; hoverParamType = opt.paramType; break; }
        }
        if (hoverLabel.length() > 18) hoverLabel = hoverLabel.substr(0, 15) + "..";
        y = DrawEditableRow(window, "onHover", hoverLabel, "dropdown_onhover", px, y);

        if (hoverParamType == ActionParamType::Bool) {
            y += 4.f;
            bool isParamTrue = (m_SelectedElement->onHoverParam == "true");
            DrawActionButton(window, isParamTrue ? "True" : "False", "hoverparam_bool", px + 110.f, y, isParamTrue ? C_SUCCESS_DIM : C_DANGER_DIM, isParamTrue ? C_SUCCESS : C_DANGER);
            y += 30.f;
        } else if (hoverParamType == ActionParamType::String || hoverParamType == ActionParamType::Float) {
            std::string hpDisplay = (m_ActiveField == EditField::OnHoverParam && !m_ActiveInputText.empty())
                                        ? m_ActiveInputText + "|"
                                        : (m_ActiveField == EditField::OnHoverParam ? "|" : m_SelectedElement->onHoverParam);
        }
    }

    y += 10.f;
    y = DrawSectionHeader(window, m_HasUnsavedChanges ? "SCRIPT INTERACTION *" : "SCRIPT INTERACTION", m_HasUnsavedChanges ? C_WARNING : sf::Color(120, 200, 255), px, y);

    std::string scriptDisplay = (m_ActiveField == EditField::ScriptPath && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::ScriptPath ? "|" : (m_SelectedElement->scriptPath.empty() ? "None (Click to set)" : (m_SelectedElement->scriptPath + (m_HasUnsavedChanges ? " *" : ""))));
    y = DrawEditableRow(window, "Script File", scriptDisplay, "edit_script_path", px, y);

    y = DrawActionButton(window, "Select Script File...", "browse_scripts", px + 10.f, y, C_BG_ELEVATED, C_BORDER_LIGHT);

    std::string defaultMethod = m_SelectedElement->scriptMethod.empty() ? GetDefaultMethodName(m_SelectedElement) : m_SelectedElement->scriptMethod;
    std::string methodDisplay = (m_ActiveField == EditField::ScriptMethod && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::ScriptMethod ? "|" : defaultMethod);
    y = DrawEditableRow(window, "Method Name", methodDisplay, "edit_script_method", px, y);

    y = DrawActionButton(window, "+ Insert Method into Script", "insert_script_method", px + 10.f, y, C_ACCENT_DIM, C_ACCENT);

    if (!m_SelectedElement->scriptPath.empty())
    {
        y = DrawActionButton(window, "Open Script in Editor", "open_script", px + 10.f, y, C_BG_ELEVATED, C_BORDER_LIGHT);
    }

    float totalContentBottom = y + scrollOff;
    m_InspectorMaxScroll = std::max(0.f, totalContentBottom - m_InspectorBounds.top - m_InspectorBounds.height + 20.f);

    sf::RectangleShape borderFront({1.f, m_InspectorBounds.height});
    borderFront.setFillColor(C_BORDER);
    borderFront.setPosition(m_InspectorBounds.left, m_InspectorBounds.top);
    window.draw(borderFront);

    if (m_InspectorMaxScroll > 0.f)
    {
        float ratio     = m_InspectorBounds.height / (m_InspectorBounds.height + m_InspectorMaxScroll);
        float barH      = std::max(20.f, m_InspectorBounds.height * ratio);
        float barOffset = (m_InspectorScrollOffset / m_InspectorMaxScroll) * (m_InspectorBounds.height - barH);
        sf::RectangleShape scrollBar({4.f, barH});
        scrollBar.setFillColor(sf::Color(120, 120, 180, 180));
        scrollBar.setPosition(m_InspectorBounds.left + InspectorWidth - 5.f, m_InspectorBounds.top + barOffset);
        window.draw(scrollBar);
    }

    m_InspectorClipTop    = 0.f;
    m_InspectorClipBottom = 99999.f;
}

void UIEditorScene::DrawCanvas(sf::RenderWindow &window)
{
    window.setView(m_CanvasView);

    sf::RectangleShape shadow(m_CanvasSize);
    shadow.setPosition(8.f, 8.f);
    shadow.setFillColor(sf::Color(0, 0, 0, 120));
    window.draw(shadow);

    sf::RectangleShape canvasBg(m_CanvasSize);
    canvasBg.setPosition(0, 0);
    canvasBg.setFillColor(C_BG_CANVAS);
    canvasBg.setOutlineColor(sf::Color(C_ACCENT.r, C_ACCENT.g, C_ACCENT.b, 200));
    canvasBg.setOutlineThickness(2.f);
    window.draw(canvasBg);

    sf::Text badge;
    badge.setFont(*m_Font);
    badge.setCharacterSize(16);
    badge.setFillColor(C_TEXT_MUTED);
    badge.setString("Screen Canvas (1920 x 1080) - 16:9  [Stretched in Game]");
    badge.setPosition(0.f, -28.f);
    window.draw(badge);

    sf::RectangleShape line;
    line.setFillColor(C_GRID_MINOR);
    for (float i = 100.f; i < m_CanvasSize.x; i += 100.f)
    {
        line.setPosition(i, 0);
        line.setSize({1.f, m_CanvasSize.y});
        window.draw(line);
    }
    for (float i = 100.f; i < m_CanvasSize.y; i += 100.f)
    {
        line.setPosition(0, i);
        line.setSize({m_CanvasSize.x, 1.f});
        window.draw(line);
    }

    UIManager::Get().Render(window);

    if (m_SelectedElement)
    {
        sf::RectangleShape outline(m_SelectedElement->size);
        outline.setPosition(m_SelectedElement->worldPosition);
        outline.setFillColor(sf::Color::Transparent);
        outline.setOutlineColor(sf::Color(255, 220, 60));
        outline.setOutlineThickness(2.f);
        window.draw(outline);

        DrawResizeHandles(window);
    }
}

void UIEditorScene::DrawResizeHandles(sf::RenderWindow &window)
{
    if (!m_SelectedElement) return;

    const sf::FloatRect bounds(m_SelectedElement->worldPosition, m_SelectedElement->size);
    const float hw = 4.f;

    sf::Vector2f positions[8] = {
        {bounds.left - hw, bounds.top - hw},
        {bounds.left + bounds.width / 2.f - hw, bounds.top - hw},
        {bounds.left + bounds.width - hw, bounds.top - hw},
        {bounds.left - hw, bounds.top + bounds.height / 2.f - hw},
        {bounds.left + bounds.width - hw, bounds.top + bounds.height / 2.f - hw},
        {bounds.left - hw, bounds.top + bounds.height - hw},
        {bounds.left + bounds.width / 2.f - hw, bounds.top + bounds.height - hw},
        {bounds.left + bounds.width - hw, bounds.top + bounds.height - hw}
    };

    sf::RectangleShape handle({hw * 2.f, hw * 2.f});
    handle.setFillColor(sf::Color(80, 165, 255, 240));
    handle.setOutlineColor(sf::Color(25, 30, 45, 255));
    handle.setOutlineThickness(1.f);

    for (const auto &pos: positions)
    {
        handle.setPosition(pos);
        window.draw(handle);
    }
}

int UIEditorScene::GetResizeHandle(sf::Vector2f worldPos) const
{
    if (!m_SelectedElement) return -1;
    const sf::FloatRect bounds(m_SelectedElement->worldPosition, m_SelectedElement->size);
    const float hw = 6.f;
    sf::Vector2f positions[8] = {
        {bounds.left, bounds.top},
        {bounds.left + bounds.width / 2.f, bounds.top},
        {bounds.left + bounds.width, bounds.top},
        {bounds.left, bounds.top + bounds.height / 2.f},
        {bounds.left + bounds.width, bounds.top + bounds.height / 2.f},
        {bounds.left, bounds.top + bounds.height},
        {bounds.left + bounds.width / 2.f, bounds.top + bounds.height},
        {bounds.left + bounds.width, bounds.top + bounds.height}
    };
    for (int i = 0; i < 8; ++i)
    {
        sf::FloatRect hr(positions[i].x - hw, positions[i].y - hw, hw * 2.f, hw * 2.f);
        if (hr.contains(worldPos)) return i;
    }
    return -1;
}

std::string UIEditorScene::NextId(UIElementType type)
{
    static int idCounter = 1;
    std::string prefix = "Panel_";
    if (type == UIElementType::Text) prefix = "Text_";
    else if (type == UIElementType::Button) prefix = "Button_";
    else if (type == UIElementType::Image) prefix = "Image_";
    else if (type == UIElementType::Checkbox) prefix = "Checkbox_";
    else if (type == UIElementType::Slider) prefix = "Slider_";
    else if (type == UIElementType::ProgressBar) prefix = "ProgressBar_";
    else if (type == UIElementType::TextInput) prefix = "TextInput_";
    else if (type == UIElementType::VerticalBox) prefix = "VBox_";
    else if (type == UIElementType::HorizontalBox) prefix = "HBox_";

    return prefix + std::to_string(idCounter++);
}

void UIEditorScene::HandleAction(const std::string &action)
{
    m_InputSelectionStart = -1;
    m_InputSelectionEnd = -1;
    m_IsSelectingText = false;

    if (action.rfind("add_", 0) == 0 || action == "clear_parent" || action == "delete" || action == "paste" ||
        action.rfind("align_", 0) == 0 || action.rfind("valign_", 0) == 0 || action.rfind("style_", 0) == 0 ||
        action == "uppercase_toggle" || action == "visible_toggle" || action == "disabled_toggle" ||
        action == "layer_forward" || action == "layer_backward" || action == "checked_toggle" ||
        action.rfind("clear_", 0) == 0 || action.rfind("pick_color", 0) == 0 ||
        action == "clickparam_bool" || action == "hoverparam_bool" || action == "browse_script_dialog")
    {
        SetDirty(true);
    }

    if (action == "back") { m_manager.SwitchSceneTo("editor"); } else if (action == "save")
    {
        std::string path = UIManager::Get().GetCurrentUIPath();
        if (path.empty()) path = "assets/scenes/game_ui.json";
        std::filesystem::path rootDir = FindProjectRoot();
        std::filesystem::path fullPath = rootDir / path;
        try {
            std::error_code ec;
            std::filesystem::create_directories(fullPath.parent_path(), ec);
            UIManager::Get().Save(fullPath.string());
            SyncFileToRuntime(path);
            std::cout << "[INFO] [UIEditorScene] UI Saved to " << fullPath.string() << "\n";
            m_FeedbackMessage = "UI Saved successfully!";
            m_SaveFeedbackTimer = 2.0f;
            SetDirty(false);
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] [UIEditorScene] Failed to save UI: " << e.what() << "\n";
        }
    } else if (action == "add_panel")
    {
        int maxZ = 0;
        for (auto &el: UIManager::Get().GetElements()) maxZ = std::max(maxZ, el.zIndex);
        m_SelectedElement = UIManager::Get().CreateElement(NextId(UIElementType::Panel), UIElementType::Panel);
        if (m_SelectedElement)
        {
            m_SelectedElement->size = {300.f, 200.f};
            m_SelectedElement->position = {m_CanvasSize.x / 2.f - 150.f, m_CanvasSize.y / 2.f - 100.f};
            m_SelectedElement->color = sf::Color(35, 35, 55, 230);
            m_SelectedElement->zIndex = maxZ + 1;
            m_SelectedElement->UpdateDrawables();
            SelectElement(m_SelectedElement);
        }
    } else if (action == "add_text")
    {
        int maxZ = 0;
        for (auto &el: UIManager::Get().GetElements()) maxZ = std::max(maxZ, el.zIndex);
        m_SelectedElement = UIManager::Get().CreateElement(NextId(UIElementType::Text), UIElementType::Text);
        if (m_SelectedElement)
        {
            m_SelectedElement->size = {200.f, 40.f};
            m_SelectedElement->position = {m_CanvasSize.x / 2.f - 100.f, m_CanvasSize.y / 2.f - 20.f};
            m_SelectedElement->characterSize = 24;
            m_SelectedElement->zIndex = maxZ + 1;
            m_SelectedElement->UpdateDrawables();
            SelectElement(m_SelectedElement);
        }
    } else if (action == "add_button")
    {
        int maxZ = 0;
        for (auto &el: UIManager::Get().GetElements()) maxZ = std::max(maxZ, el.zIndex);
        m_SelectedElement = UIManager::Get().CreateElement(NextId(UIElementType::Button), UIElementType::Button);
        if (m_SelectedElement)
        {
            m_SelectedElement->size = {180.f, 50.f};
            m_SelectedElement->position = {m_CanvasSize.x / 2.f - 90.f, m_CanvasSize.y / 2.f - 25.f};
            m_SelectedElement->zIndex = maxZ + 1;
            m_SelectedElement->UpdateDrawables();
            SelectElement(m_SelectedElement);
        }
    } else if (action == "add_image")
    {
        int maxZ = 0;
        for (auto &el: UIManager::Get().GetElements()) maxZ = std::max(maxZ, el.zIndex);
        m_SelectedElement = UIManager::Get().CreateElement(NextId(UIElementType::Image), UIElementType::Image);
        if (m_SelectedElement)
        {
            m_SelectedElement->size = {100.f, 100.f};
            m_SelectedElement->position = {m_CanvasSize.x / 2.f - 50.f, m_CanvasSize.y / 2.f - 50.f};
            m_SelectedElement->color = sf::Color::White;
            m_SelectedElement->zIndex = maxZ + 1;
            m_SelectedElement->UpdateDrawables();
            SelectElement(m_SelectedElement);
        }
    } else if (action == "add_checkbox")
    {
        int maxZ = 0;
        for (auto &el: UIManager::Get().GetElements()) maxZ = std::max(maxZ, el.zIndex);
        m_SelectedElement = UIManager::Get().CreateElement(NextId(UIElementType::Checkbox), UIElementType::Checkbox);
        if (m_SelectedElement)
        {
            m_SelectedElement->size = {40.f, 40.f};
            m_SelectedElement->position = {m_CanvasSize.x / 2.f - 20.f, m_CanvasSize.y / 2.f - 20.f};
            m_SelectedElement->zIndex = maxZ + 1;
            m_SelectedElement->UpdateDrawables();
            SelectElement(m_SelectedElement);
        }
    } else if (action == "add_slider")
    {
        int maxZ = 0;
        for (auto &el: UIManager::Get().GetElements()) maxZ = std::max(maxZ, el.zIndex);
        m_SelectedElement = UIManager::Get().CreateElement(NextId(UIElementType::Slider), UIElementType::Slider);
        if (m_SelectedElement)
        {
            m_SelectedElement->size = {200.f, 20.f};
            m_SelectedElement->position = {m_CanvasSize.x / 2.f - 100.f, m_CanvasSize.y / 2.f - 10.f};
            m_SelectedElement->color = sf::Color(80, 80, 90);
            m_SelectedElement->normalColor = sf::Color(200, 200, 210);
            m_SelectedElement->zIndex = maxZ + 1;
            m_SelectedElement->UpdateDrawables();
            SelectElement(m_SelectedElement);
        }
    } else if (action == "add_progressbar")
    {
        int maxZ = 0;
        for (auto &el: UIManager::Get().GetElements()) maxZ = std::max(maxZ, el.zIndex);
        m_SelectedElement = UIManager::Get().CreateElement(NextId(UIElementType::ProgressBar), UIElementType::ProgressBar);
        if (m_SelectedElement)
        {
            m_SelectedElement->size = {200.f, 30.f};
            m_SelectedElement->position = {m_CanvasSize.x / 2.f - 100.f, m_CanvasSize.y / 2.f - 15.f};
            m_SelectedElement->color = sf::Color(50, 50, 60);
            m_SelectedElement->normalColor = C_SUCCESS;
            m_SelectedElement->progressValue = 0.5f;
            m_SelectedElement->zIndex = maxZ + 1;
            m_SelectedElement->UpdateDrawables();
            SelectElement(m_SelectedElement);
        }
    } else if (action == "add_textinput")
    {
        int maxZ = 0;
        for (auto &el: UIManager::Get().GetElements()) maxZ = std::max(maxZ, el.zIndex);
        m_SelectedElement = UIManager::Get().CreateElement(NextId(UIElementType::TextInput), UIElementType::TextInput);
        if (m_SelectedElement)
        {
            m_SelectedElement->size = {200.f, 40.f};
            m_SelectedElement->position = {m_CanvasSize.x / 2.f - 100.f, m_CanvasSize.y / 2.f - 20.f};
            m_SelectedElement->normalColor = C_BG_INPUT;
            m_SelectedElement->pressedColor = sf::Color(30, 35, 42);
            m_SelectedElement->borderColor = C_BORDER;
            m_SelectedElement->borderThickness = 1.f;
            m_SelectedElement->characterSize = 18;
            m_SelectedElement->textAlign = TextAlign::Left;
            m_SelectedElement->zIndex = maxZ + 1;
            m_SelectedElement->UpdateDrawables();
            SelectElement(m_SelectedElement);
        }
    } else if (action == "add_verticalbox")
    {
        int maxZ = 0;
        for (auto &el: UIManager::Get().GetElements()) maxZ = std::max(maxZ, el.zIndex);
        m_SelectedElement = UIManager::Get().CreateElement(NextId(UIElementType::VerticalBox), UIElementType::VerticalBox);
        if (m_SelectedElement)
        {
            m_SelectedElement->size = {220.f, 260.f};
            m_SelectedElement->position = {m_CanvasSize.x / 2.f - 110.f, m_CanvasSize.y / 2.f - 130.f};
            m_SelectedElement->color = sf::Color(30, 32, 40, 180);
            m_SelectedElement->borderColor = C_ACCENT;
            m_SelectedElement->borderThickness = 1.f;
            m_SelectedElement->layoutSpacing = 8.f;
            m_SelectedElement->layoutPadding = 10.f;
            m_SelectedElement->zIndex = maxZ + 1;
            m_SelectedElement->UpdateDrawables();
            SelectElement(m_SelectedElement);
        }
    } else if (action == "add_horizontalbox")
    {
        int maxZ = 0;
        for (auto &el: UIManager::Get().GetElements()) maxZ = std::max(maxZ, el.zIndex);
        m_SelectedElement = UIManager::Get().CreateElement(NextId(UIElementType::HorizontalBox), UIElementType::HorizontalBox);
        if (m_SelectedElement)
        {
            m_SelectedElement->size = {300.f, 80.f};
            m_SelectedElement->position = {m_CanvasSize.x / 2.f - 150.f, m_CanvasSize.y / 2.f - 40.f};
            m_SelectedElement->color = sf::Color(30, 32, 40, 180);
            m_SelectedElement->borderColor = C_ACCENT;
            m_SelectedElement->borderThickness = 1.f;
            m_SelectedElement->layoutSpacing = 8.f;
            m_SelectedElement->layoutPadding = 10.f;
            m_SelectedElement->zIndex = maxZ + 1;
            m_SelectedElement->UpdateDrawables();
            SelectElement(m_SelectedElement);
        }
    } else if (action == "edit_id")
    {
        m_ActiveField = EditField::Id;
        m_ActiveInputText = m_SelectedElement->id;
    } else if (action == "clear_parent")
    {
        if (m_SelectedElement) { UIManager::Get().SetParent(m_SelectedElement->id, "", true); }
    } else if (action == "edit_layoutspacing")
    {
        m_ActiveField = EditField::LayoutSpacing;
        m_ActiveInputText = FormatFloat(m_SelectedElement->layoutSpacing, 1);
    } else if (action == "edit_layoutpadding")
    {
        m_ActiveField = EditField::LayoutPadding;
        m_ActiveInputText = FormatFloat(m_SelectedElement->layoutPadding, 1);
    } else if (action == "edit_x")
    {
        m_ActiveField = EditField::TransformX;
        m_ActiveInputText = std::to_string((int) m_SelectedElement->position.x);
    } else if (action == "edit_y")
    {
        m_ActiveField = EditField::TransformY;
        m_ActiveInputText = std::to_string((int) m_SelectedElement->position.y);
    } else if (action == "edit_z")
    {
        m_ActiveField = EditField::ZIndex;
        m_ActiveInputText = std::to_string(m_SelectedElement->zIndex);
    }
    else if (action == "layer_forward") { if (m_SelectedElement) { m_SelectedElement->zIndex += 1; } }
    else if (action == "layer_backward") { if (m_SelectedElement) { m_SelectedElement->zIndex -= 1; } }
    else if (action == "edit_w")
    {
        m_ActiveField = EditField::SizeW;
        m_ActiveInputText = std::to_string((int) m_SelectedElement->size.x);
    }
    else if (action == "edit_h")
    {
        m_ActiveField = EditField::SizeH;
        m_ActiveInputText = std::to_string((int) m_SelectedElement->size.y);
    }
    else if (action == "edit_r")
    {
        m_ActiveField = EditField::ColorR;
        m_ActiveInputText = std::to_string(m_SelectedElement->color.r);
    }
    else if (action == "edit_g")
    {
        m_ActiveField = EditField::ColorG;
        m_ActiveInputText = std::to_string(m_SelectedElement->color.g);
    }
    else if (action == "edit_b")
    {
        m_ActiveField = EditField::ColorB;
        m_ActiveInputText = std::to_string(m_SelectedElement->color.b);
    }
    else if (action == "edit_a")
    {
        m_ActiveField = EditField::ColorA;
        m_ActiveInputText = std::to_string(m_SelectedElement->color.a);
    }
    else if (action == "edit_opacity")
    {
        m_ActiveField = EditField::Opacity;
        m_ActiveInputText = std::to_string((int)m_SelectedElement->opacity);
    }
    else if (action == "edit_text")
    {
        m_ActiveField = EditField::UIText;
        m_ActiveInputText = m_SelectedElement->text;
    }
    else if (action == "edit_fontsize")
    {
        m_ActiveField = EditField::CharacterSize;
        m_ActiveInputText = std::to_string(m_SelectedElement->characterSize);
    }
    else if (action == "edit_letterspacing")
    {
        m_ActiveField = EditField::LetterSpacing;
        m_ActiveInputText = FormatFloat(m_SelectedElement->letterSpacing, 2);
    }
    else if (action == "edit_linespacing")
    {
        m_ActiveField = EditField::LineSpacing;
        m_ActiveInputText = FormatFloat(m_SelectedElement->lineSpacing, 2);
    }
    else if (action == "align_left")
    {
        if (m_SelectedElement) { m_SelectedElement->textAlign = TextAlign::Left; m_SelectedElement->UpdateDrawables(); }
    }
    else if (action == "align_center")
    {
        if (m_SelectedElement) { m_SelectedElement->textAlign = TextAlign::Center; m_SelectedElement->UpdateDrawables(); }
    }
    else if (action == "align_right")
    {
        if (m_SelectedElement) { m_SelectedElement->textAlign = TextAlign::Right; m_SelectedElement->UpdateDrawables(); }
    }
    else if (action == "valign_top")
    {
        if (m_SelectedElement) { m_SelectedElement->textVAlign = TextVAlign::Top; m_SelectedElement->UpdateDrawables(); }
    }
    else if (action == "valign_middle")
    {
        if (m_SelectedElement) { m_SelectedElement->textVAlign = TextVAlign::Middle; m_SelectedElement->UpdateDrawables(); }
    }
    else if (action == "valign_bottom")
    {
        if (m_SelectedElement) { m_SelectedElement->textVAlign = TextVAlign::Bottom; m_SelectedElement->UpdateDrawables(); }
    }
    else if (action == "style_bold_toggle")
    {
        if (m_SelectedElement)
        {
            m_SelectedElement->textStyle ^= sf::Text::Bold;
            m_SelectedElement->UpdateDrawables();
        }
    }
    else if (action == "style_italic_toggle")
    {
        if (m_SelectedElement)
        {
            m_SelectedElement->textStyle ^= sf::Text::Italic;
            m_SelectedElement->UpdateDrawables();
        }
    }
    else if (action == "style_underline_toggle")
    {
        if (m_SelectedElement)
        {
            m_SelectedElement->textStyle ^= sf::Text::Underlined;
            m_SelectedElement->UpdateDrawables();
        }
    }
    else if (action == "uppercase_toggle")
    {
        if (m_SelectedElement) { m_SelectedElement->textUpperCase = !m_SelectedElement->textUpperCase; m_SelectedElement->UpdateDrawables(); }
    }
    else if (action == "visible_toggle")
    {
        if (m_SelectedElement) { m_SelectedElement->visible = !m_SelectedElement->visible; m_SelectedElement->UpdateDrawables(); }
    }
    else if (action == "disabled_toggle")
    {
        if (m_SelectedElement) { m_SelectedElement->disabled = !m_SelectedElement->disabled; m_SelectedElement->UpdateDrawables(); }
    }
    else if (action == "edit_textoutline_r")
    {
        m_ActiveField = EditField::TextOutlineR;
        m_ActiveInputText = std::to_string(m_SelectedElement->textOutlineColor.r);
    }
    else if (action == "edit_textoutline_g")
    {
        m_ActiveField = EditField::TextOutlineG;
        m_ActiveInputText = std::to_string(m_SelectedElement->textOutlineColor.g);
    }
    else if (action == "edit_textoutline_b")
    {
        m_ActiveField = EditField::TextOutlineB;
        m_ActiveInputText = std::to_string(m_SelectedElement->textOutlineColor.b);
    }
    else if (action == "edit_textoutline_thickness")
    {
        m_ActiveField = EditField::TextOutlineThickness;
        m_ActiveInputText = std::to_string(m_SelectedElement->textOutlineThickness);
    }
    else if (action == "edit_textoffset_x")
    {
        m_ActiveField = EditField::TextOffsetX;
        m_ActiveInputText = std::to_string((int)m_SelectedElement->textOffset.x);
    }
    else if (action == "edit_textoffset_y")
    {
        m_ActiveField = EditField::TextOffsetY;
        m_ActiveInputText = std::to_string((int)m_SelectedElement->textOffset.y);
    }
    else if (action == "edit_outline_r")
    {
        m_ActiveField = EditField::OutlineR;
        m_ActiveInputText = std::to_string(m_SelectedElement->outlineColor.r);
    }
    else if (action == "edit_outline_g")
    {
        m_ActiveField = EditField::OutlineG;
        m_ActiveInputText = std::to_string(m_SelectedElement->outlineColor.g);
    }
    else if (action == "edit_outline_b")
    {
        m_ActiveField = EditField::OutlineB;
        m_ActiveInputText = std::to_string(m_SelectedElement->outlineColor.b);
    }
    else if (action == "edit_outline_thickness")
    {
        m_ActiveField = EditField::OutlineThickness;
        m_ActiveInputText = std::to_string(m_SelectedElement->outlineThickness);
    }
    else if (action == "edit_borderr")
    {
        m_ActiveField = EditField::BorderR;
        m_ActiveInputText = std::to_string(m_SelectedElement->borderColor.r);
    }
    else if (action == "edit_borderg")
    {
        m_ActiveField = EditField::BorderG;
        m_ActiveInputText = std::to_string(m_SelectedElement->borderColor.g);
    }
    else if (action == "edit_borderb")
    {
        m_ActiveField = EditField::BorderB;
        m_ActiveInputText = std::to_string(m_SelectedElement->borderColor.b);
    }
    else if (action == "edit_borderthickness")
    {
        m_ActiveField = EditField::BorderThickness;
        m_ActiveInputText = std::to_string(m_SelectedElement->borderThickness);
    }
    else if (action == "edit_hoverr")
    {
        m_ActiveField = EditField::HoverR;
        m_ActiveInputText = std::to_string(m_SelectedElement->hoverColor.r);
    }
    else if (action == "edit_hoverg")
    {
        m_ActiveField = EditField::HoverG;
        m_ActiveInputText = std::to_string(m_SelectedElement->hoverColor.g);
    }
    else if (action == "edit_hoverb")
    {
        m_ActiveField = EditField::HoverB;
        m_ActiveInputText = std::to_string(m_SelectedElement->hoverColor.b);
    }
    else if (action == "edit_pressedr")
    {
        m_ActiveField = EditField::PressedR;
        m_ActiveInputText = std::to_string(m_SelectedElement->pressedColor.r);
    }
    else if (action == "edit_pressedg")
    {
        m_ActiveField = EditField::PressedG;
        m_ActiveInputText = std::to_string(m_SelectedElement->pressedColor.g);
    }
    else if (action == "edit_pressedb")
    {
        m_ActiveField = EditField::PressedB;
        m_ActiveInputText = std::to_string(m_SelectedElement->pressedColor.b);
    }
    else if (action == "edit_normalr")
    {
        m_ActiveField = EditField::NormalR;
        m_ActiveInputText = std::to_string(m_SelectedElement->normalColor.r);
    }
    else if (action == "edit_normalg")
    {
        m_ActiveField = EditField::NormalG;
        m_ActiveInputText = std::to_string(m_SelectedElement->normalColor.g);
    }
    else if (action == "edit_normalb")
    {
        m_ActiveField = EditField::NormalB;
        m_ActiveInputText = std::to_string(m_SelectedElement->normalColor.b);
    }
    else if (action == "edit_textcolor_r")
    {
        m_ActiveField = EditField::TextColorR;
        m_ActiveInputText = std::to_string(m_SelectedElement->textColor.r);
    }
    else if (action == "edit_textcolor_g")
    {
        m_ActiveField = EditField::TextColorG;
        m_ActiveInputText = std::to_string(m_SelectedElement->textColor.g);
    }
    else if (action == "edit_textcolor_b")
    {
        m_ActiveField = EditField::TextColorB;
        m_ActiveInputText = std::to_string(m_SelectedElement->textColor.b);
    }
    else if (action == "dropdown_onclick")
    {
        m_ActiveDropdown = "onclick";
        for (auto& hb : m_InspectorHitboxes) {
            if (hb.action == "dropdown_onclick") {
                m_DropdownRect = sf::FloatRect(hb.bounds.left, hb.bounds.top + hb.bounds.height, hb.bounds.width, UTILITY_ACTIONS.size() * 24.f + 8.f);
                break;
            }
        }
    }
    else if (action == "dropdown_onhover")
    {
        m_ActiveDropdown = "onhover";
        for (auto& hb : m_InspectorHitboxes) {
            if (hb.action == "dropdown_onhover") {
                m_DropdownRect = sf::FloatRect(hb.bounds.left, hb.bounds.top + hb.bounds.height, hb.bounds.width, UTILITY_ACTIONS.size() * 24.f + 8.f);
                break;
            }
        }
    }
    else if (action == "edit_clickparam")
    {
        m_ActiveField = EditField::OnClickParam;
        m_ActiveInputText = m_SelectedElement->onClickParam;
    }
    else if (action == "edit_hoverparam")
    {
        m_ActiveField = EditField::OnHoverParam;
        m_ActiveInputText = m_SelectedElement->onHoverParam;
    }
    else if (action == "clickparam_bool")
    {
        m_SelectedElement->onClickParam = (m_SelectedElement->onClickParam == "true") ? "false" : "true";
    }
    else if (action == "hoverparam_bool")
    {
        m_SelectedElement->onHoverParam = (m_SelectedElement->onHoverParam == "true") ? "false" : "true";
    }
    else if (action == "edit_script_path")
    {
        m_ActiveField = EditField::ScriptPath;
        m_ActiveInputText = m_SelectedElement->scriptPath;
    }
    else if (action == "edit_script_method")
    {
        m_ActiveField = EditField::ScriptMethod;
        m_ActiveInputText = m_SelectedElement->scriptMethod.empty() ? GetDefaultMethodName(m_SelectedElement) : m_SelectedElement->scriptMethod;
    }
    else if (action == "browse_scripts")
    {
        m_ActiveDropdown = "script_select";
        for (auto &hb : m_InspectorHitboxes)
        {
            if (hb.action == "browse_scripts")
            {
                auto scripts = GetAvailableScripts();
                float dropH = (scripts.size() + 3) * 24.f + 8.f;
                float dropW = InspectorWidth - 28.f;
                m_DropdownRect = sf::FloatRect(m_InspectorBounds.left + 14.f, hb.bounds.top + hb.bounds.height + 2.f, dropW, std::min(dropH, 240.f));
                break;
            }
        }
    }
    else if (action == "browse_script_dialog")
    {
        if (m_SelectedElement)
        {
#ifdef _WIN32
            std::string picked = OpenScriptFileDialog(reinterpret_cast<HWND>(m_Window.getSystemHandle()));
            if (!picked.empty())
            {
                m_SelectedElement->scriptPath = picked;
            }
#endif
        }
    }
    else if (action == "insert_script_method")
    {
        if (m_SelectedElement)
        {
            std::string path = m_SelectedElement->scriptPath.empty() ? "assets/scripts/ui_interactions.lua" : m_SelectedElement->scriptPath;
            std::string method = m_SelectedElement->scriptMethod.empty() ? GetDefaultMethodName(m_SelectedElement) : m_SelectedElement->scriptMethod;
            if (InsertScriptMethod(m_SelectedElement, path, method))
            {
                m_FeedbackMessage = "Inserted " + m_SelectedElement->scriptMethod + " into " + m_SelectedElement->scriptPath + "!";
                m_SaveFeedbackTimer = 3.0f;
                OpenScriptInIDE(m_SelectedElement->scriptPath);
            }
            else
            {
                m_FeedbackMessage = "Failed to insert script method!";
                m_SaveFeedbackTimer = 3.0f;
            }
        }
    }
    else if (action == "open_script")
    {
        if (m_SelectedElement && !m_SelectedElement->scriptPath.empty())
        {
            OpenScriptInIDE(m_SelectedElement->scriptPath);
        }
    }
    else if (action == "edit_texturepath")
    {
        m_ActiveField = EditField::TexturePath;
        m_ActiveInputText = m_SelectedElement->texturePath;
    }
    else if (action == "edit_hovertexturepath")
    {
        m_ActiveField = EditField::HoverTexturePath;
        m_ActiveInputText = m_SelectedElement->hoverTexturePath;
    }
    else if (action == "edit_pressedtexturepath")
    {
        m_ActiveField = EditField::PressedTexturePath;
        m_ActiveInputText = m_SelectedElement->pressedTexturePath;
    }
    else if (action == "browse_texture")
    {
        if (m_SelectedElement)
        {
#ifdef _WIN32
            std::string picked = OpenImageFileDialog(reinterpret_cast<HWND>(m_Window.getSystemHandle()));
            if (!picked.empty())
            {
                m_SelectedElement->texturePath = picked;
                m_SelectedElement->texture = LoadTextureRobust(picked);
                if (m_SelectedElement->type == UIElementType::Button && m_SelectedElement->normalColor == sf::Color(100, 100, 100))
                {
                    m_SelectedElement->normalColor = sf::Color::White;
                    m_SelectedElement->hoverColor = sf::Color(230, 230, 230);
                    m_SelectedElement->pressedColor = sf::Color(180, 180, 180);
                }
                m_SelectedElement->UpdateDrawables();
            }
#endif
        }
    }
    else if (action == "browse_hovertexture")
    {
        if (m_SelectedElement)
        {
#ifdef _WIN32
            std::string picked = OpenImageFileDialog(reinterpret_cast<HWND>(m_Window.getSystemHandle()));
            if (!picked.empty())
            {
                m_SelectedElement->hoverTexturePath = picked;
                m_SelectedElement->hoverTexture = LoadTextureRobust(picked);
                m_SelectedElement->UpdateDrawables();
            }
#endif
        }
    }
    else if (action == "browse_pressedtexture")
    {
        if (m_SelectedElement)
        {
#ifdef _WIN32
            std::string picked = OpenImageFileDialog(reinterpret_cast<HWND>(m_Window.getSystemHandle()));
            if (!picked.empty())
            {
                m_SelectedElement->pressedTexturePath = picked;
                m_SelectedElement->pressedTexture = LoadTextureRobust(picked);
                m_SelectedElement->UpdateDrawables();
            }
#endif
        }
    }
    else if (action == "browse_checkedtexture")
    {
        if (m_SelectedElement)
        {
#ifdef _WIN32
            std::string picked = OpenImageFileDialog(reinterpret_cast<HWND>(m_Window.getSystemHandle()));
            if (!picked.empty())
            {
                m_SelectedElement->checkedTexturePath = picked;
                m_SelectedElement->checkedTexture = LoadTextureRobust(picked);
                m_SelectedElement->UpdateDrawables();
            }
#endif
        }
    }
    else if (action == "browse_knobtexture")
    {
        if (m_SelectedElement)
        {
#ifdef _WIN32
            std::string picked = OpenImageFileDialog(reinterpret_cast<HWND>(m_Window.getSystemHandle()));
            if (!picked.empty())
            {
                m_SelectedElement->knobTexturePath = picked;
                m_SelectedElement->knobTexture = LoadTextureRobust(picked);
                m_SelectedElement->UpdateDrawables();
            }
#endif
        }
    }
    else if (action == "browse_filltexture")
    {
        if (m_SelectedElement)
        {
#ifdef _WIN32
            std::string picked = OpenImageFileDialog(reinterpret_cast<HWND>(m_Window.getSystemHandle()));
            if (!picked.empty())
            {
                m_SelectedElement->fillTexturePath = picked;
                m_SelectedElement->fillTexture = LoadTextureRobust(picked);
                m_SelectedElement->UpdateDrawables();
            }
#endif
        }
    }
    else if (action == "checked_toggle")
    {
        if (m_SelectedElement) { m_SelectedElement->isChecked = !m_SelectedElement->isChecked; m_SelectedElement->UpdateDrawables(); }
    }
    else if (action == "edit_checkedtexturepath")
    {
        m_ActiveField = EditField::CheckedTexturePath;
        m_ActiveInputText = m_SelectedElement->checkedTexturePath;
    }
    else if (action == "edit_slidervalue")
    {
        m_ActiveField = EditField::SliderValue;
        m_ActiveInputText = FormatFloat(m_SelectedElement->sliderValue, 2);
    }
    else if (action == "edit_slidermin")
    {
        m_ActiveField = EditField::SliderMin;
        m_ActiveInputText = FormatFloat(m_SelectedElement->sliderMin, 2);
    }
    else if (action == "edit_slidermax")
    {
        m_ActiveField = EditField::SliderMax;
        m_ActiveInputText = FormatFloat(m_SelectedElement->sliderMax, 2);
    }
    else if (action == "edit_knobtexturepath")
    {
        m_ActiveField = EditField::KnobTexturePath;
        m_ActiveInputText = m_SelectedElement->knobTexturePath;
    }
    else if (action == "edit_progressvalue")
    {
        m_ActiveField = EditField::ProgressValue;
        m_ActiveInputText = FormatFloat(m_SelectedElement->progressValue, 2);
    }
    else if (action == "edit_progressmax")
    {
        m_ActiveField = EditField::ProgressMax;
        m_ActiveInputText = FormatFloat(m_SelectedElement->progressMax, 2);
    }
    else if (action == "pick_color_main")
    {
        if (m_SelectedElement)
        {
#ifdef _WIN32
            HWND hwnd = reinterpret_cast<HWND>(m_Window.getSystemHandle());
            if (OpenColorPickerDialog(m_SelectedElement->color, hwnd))
            {
                m_SelectedElement->UpdateDrawables();
            }
#endif
        }
    }
    else if (action == "pick_color_outline")
    {
        if (m_SelectedElement)
        {
#ifdef _WIN32
            HWND hwnd = reinterpret_cast<HWND>(m_Window.getSystemHandle());
            if (OpenColorPickerDialog(m_SelectedElement->outlineColor, hwnd))
            {
                m_SelectedElement->UpdateDrawables();
            }
#endif
        }
    }
    else if (action == "pick_color_textoutline")
    {
        if (m_SelectedElement)
        {
#ifdef _WIN32
            HWND hwnd = reinterpret_cast<HWND>(m_Window.getSystemHandle());
            if (OpenColorPickerDialog(m_SelectedElement->textOutlineColor, hwnd))
            {
                m_SelectedElement->UpdateDrawables();
            }
#endif
        }
    }
    else if (action == "pick_color_text")
    {
        if (m_SelectedElement)
        {
#ifdef _WIN32
            HWND hwnd = reinterpret_cast<HWND>(m_Window.getSystemHandle());
            if (OpenColorPickerDialog(m_SelectedElement->textColor, hwnd))
            {
                m_SelectedElement->UpdateDrawables();
            }
#endif
        }
    }
    else if (action == "pick_color_normal")
    {
        if (m_SelectedElement)
        {
#ifdef _WIN32
            HWND hwnd = reinterpret_cast<HWND>(m_Window.getSystemHandle());
            if (OpenColorPickerDialog(m_SelectedElement->normalColor, hwnd))
            {
                m_SelectedElement->UpdateDrawables();
            }
#endif
        }
    }
    else if (action == "pick_color_hover")
    {
        if (m_SelectedElement)
        {
#ifdef _WIN32
            HWND hwnd = reinterpret_cast<HWND>(m_Window.getSystemHandle());
            if (OpenColorPickerDialog(m_SelectedElement->hoverColor, hwnd))
            {
                m_SelectedElement->UpdateDrawables();
            }
#endif
        }
    }
    else if (action == "pick_color_pressed")
    {
        if (m_SelectedElement)
        {
#ifdef _WIN32
            HWND hwnd = reinterpret_cast<HWND>(m_Window.getSystemHandle());
            if (OpenColorPickerDialog(m_SelectedElement->pressedColor, hwnd))
            {
                m_SelectedElement->UpdateDrawables();
            }
#endif
        }
    }
    else if (action == "pick_color_border")
    {
        if (m_SelectedElement)
        {
#ifdef _WIN32
            HWND hwnd = reinterpret_cast<HWND>(m_Window.getSystemHandle());
            if (OpenColorPickerDialog(m_SelectedElement->borderColor, hwnd))
            {
                m_SelectedElement->UpdateDrawables();
            }
#endif
        }
    }
    else if (action == "edit_filltexturepath")
    {
        m_ActiveField = EditField::FillTexturePath;
        m_ActiveInputText = m_SelectedElement->fillTexturePath;
    }
    else if (action == "clear_texture")
    {
        if (m_SelectedElement) { m_SelectedElement->texturePath.clear(); m_SelectedElement->texture.reset(); m_SelectedElement->UpdateDrawables(); }
    }
    else if (action == "clear_hovertexture")
    {
        if (m_SelectedElement) { m_SelectedElement->hoverTexturePath.clear(); m_SelectedElement->hoverTexture.reset(); m_SelectedElement->UpdateDrawables(); }
    }
    else if (action == "clear_pressedtexture")
    {
        if (m_SelectedElement) { m_SelectedElement->pressedTexturePath.clear(); m_SelectedElement->pressedTexture.reset(); m_SelectedElement->UpdateDrawables(); }
    }
    else if (action == "clear_checkedtexture")
    {
        if (m_SelectedElement) { m_SelectedElement->checkedTexturePath.clear(); m_SelectedElement->checkedTexture.reset(); m_SelectedElement->UpdateDrawables(); }
    }
    else if (action == "clear_knobtexture")
    {
        if (m_SelectedElement) { m_SelectedElement->knobTexturePath.clear(); m_SelectedElement->knobTexture.reset(); m_SelectedElement->UpdateDrawables(); }
    }
    else if (action == "clear_filltexture")
    {
        if (m_SelectedElement) { m_SelectedElement->fillTexturePath.clear(); m_SelectedElement->fillTexture.reset(); m_SelectedElement->UpdateDrawables(); }
    }
}


void UIEditorScene::SelectElement(UIElement *el)
{
    if (m_SelectedElement != el)
    {
        m_InspectorScrollOffset = 0.f;
        m_InspectorTargetScroll = 0.f;
    }
    m_SelectedElement = el;
    m_ActiveField = EditField::None;
    m_ActiveInputText.clear();
    m_InputSelectionStart = -1;
    m_InputSelectionEnd = -1;
    m_IsSelectingText = false;
}

void UIEditorScene::CopySelection()
{
    if (!m_SelectedElement) return;
    m_ClipboardElement = std::make_unique<UIElement>(*m_SelectedElement);
}

void UIEditorScene::PasteClipboard()
{
    if (!m_ClipboardElement) return;

    UIElement* newEl = UIManager::Get().CreateElement(NextId(m_ClipboardElement->type), m_ClipboardElement->type);
    if (!newEl) return;

    std::string newId = newEl->id;
    *newEl = *m_ClipboardElement;
    newEl->id = newId;
    newEl->position += sf::Vector2f(20.f, 20.f);
    newEl->UpdateDrawables();

    SelectElement(newEl);
    SetDirty(true);
}

void UIEditorScene::DeleteElementWithPrompt(UIElement *el)
{
    if (!el) return;
    auto children = UIManager::Get().GetChildren(el->id);
    if (!children.empty())
    {
        m_ShowDeleteModal = true;
        m_DeleteModalTargetId = el->id;
        m_DeleteModalDescendantIds = children;
    }
    else
    {
        std::string id = el->id;
        SelectElement(nullptr);
        UIManager::Get().RemoveElement(id);
        SetDirty(true);
    }
}

void UIEditorScene::DeleteSelected()
{
    if (m_SelectedElement)
    {
        DeleteElementWithPrompt(m_SelectedElement);
    }
}

void UIEditorScene::ConfirmDeleteCascade()
{
    if (m_DeleteModalTargetId.empty()) return;
    std::vector<std::string> toDelete;
    auto collect = [&](auto& self, const std::string& parentId) -> void {
        for (const auto& ch : UIManager::Get().GetChildren(parentId)) {
            self(self, ch);
            toDelete.push_back(ch);
        }
    };
    collect(collect, m_DeleteModalTargetId);
    toDelete.push_back(m_DeleteModalTargetId);

    SelectElement(nullptr);
    for (const auto& id : toDelete) {
        UIManager::Get().RemoveElement(id);
    }
    SetDirty(true);
    m_ShowDeleteModal = false;
    m_DeleteModalTargetId.clear();
    m_DeleteModalDescendantIds.clear();
}

void UIEditorScene::ConfirmDeleteUnparent()
{
    if (m_DeleteModalTargetId.empty()) return;
    auto* target = UIManager::Get().GetElement(m_DeleteModalTargetId);
    std::string grandParent = target ? target->parent : "";

    for (const auto& chId : UIManager::Get().GetChildren(m_DeleteModalTargetId)) {
        UIManager::Get().SetParent(chId, grandParent, true);
    }

    SelectElement(nullptr);
    UIManager::Get().RemoveElement(m_DeleteModalTargetId);
    SetDirty(true);
    m_ShowDeleteModal = false;
    m_DeleteModalTargetId.clear();
    m_DeleteModalDescendantIds.clear();
}

void UIEditorScene::DrawDeleteModal(sf::RenderWindow &window)
{
    if (!m_ShowDeleteModal) return;

    sf::RectangleShape dim({static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y)});
    dim.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(dim);

    float modalW = 440.f;
    float modalH = 170.f;
    float mx = (window.getSize().x - modalW) / 2.f;
    float my = (window.getSize().y - modalH) / 2.f;

    sf::RectangleShape box({modalW, modalH});
    box.setPosition(mx, my);
    box.setFillColor(C_BG_PANEL);
    box.setOutlineColor(C_BORDER_LIGHT);
    box.setOutlineThickness(1.5f);
    window.draw(box);

    sf::Text title;
    title.setFont(*m_Font);
    title.setCharacterSize(15);
    title.setStyle(sf::Text::Bold);
    title.setFillColor(C_TEXT_PRIMARY);
    title.setString("Delete Element with Children");
    title.setPosition(mx + 20.f, my + 18.f);
    window.draw(title);

    sf::Text msg;
    msg.setFont(*m_Font);
    msg.setCharacterSize(12);
    msg.setFillColor(C_TEXT_SECONDARY);
    msg.setString("Element '" + m_DeleteModalTargetId + "' has " + std::to_string(m_DeleteModalDescendantIds.size()) +
                  " direct child element(s).\nHow would you like to proceed?");
    msg.setPosition(mx + 20.f, my + 50.f);
    window.draw(msg);

    m_DeleteModalCascadeBtn = sf::FloatRect(mx + 20.f, my + 110.f, 115.f, 32.f);
    bool hov1 = m_DeleteModalCascadeBtn.contains(m_MouseScreenPos);
    sf::RectangleShape btn1(m_DeleteModalCascadeBtn.getSize());
    btn1.setPosition(m_DeleteModalCascadeBtn.getPosition());
    btn1.setFillColor(hov1 ? sf::Color(180, 40, 40) : sf::Color(140, 30, 30));
    window.draw(btn1);

    sf::Text t1;
    t1.setFont(*m_Font);
    t1.setCharacterSize(11);
    t1.setFillColor(sf::Color::White);
    t1.setString("Delete All");
    t1.setPosition(m_DeleteModalCascadeBtn.left + 22.f, m_DeleteModalCascadeBtn.top + 8.f);
    window.draw(t1);

    m_DeleteModalUnparentBtn = sf::FloatRect(mx + 145.f, my + 110.f, 150.f, 32.f);
    bool hov2 = m_DeleteModalUnparentBtn.contains(m_MouseScreenPos);
    sf::RectangleShape btn2(m_DeleteModalUnparentBtn.getSize());
    btn2.setPosition(m_DeleteModalUnparentBtn.getPosition());
    btn2.setFillColor(hov2 ? C_ACCENT : C_BG_ELEVATED);
    btn2.setOutlineColor(C_BORDER);
    btn2.setOutlineThickness(1.f);
    window.draw(btn2);

    sf::Text t2;
    t2.setFont(*m_Font);
    t2.setCharacterSize(11);
    t2.setFillColor(sf::Color::White);
    t2.setString("Unparent Children");
    t2.setPosition(m_DeleteModalUnparentBtn.left + 14.f, m_DeleteModalUnparentBtn.top + 8.f);
    window.draw(t2);

    m_DeleteModalCancelBtn = sf::FloatRect(mx + 305.f, my + 110.f, 100.f, 32.f);
    bool hov3 = m_DeleteModalCancelBtn.contains(m_MouseScreenPos);
    sf::RectangleShape btn3(m_DeleteModalCancelBtn.getSize());
    btn3.setPosition(m_DeleteModalCancelBtn.getPosition());
    btn3.setFillColor(hov3 ? C_BG_ELEVATED : C_BG_PANEL);
    btn3.setOutlineColor(C_BORDER);
    btn3.setOutlineThickness(1.f);
    window.draw(btn3);

    sf::Text t3;
    t3.setFont(*m_Font);
    t3.setCharacterSize(11);
    t3.setFillColor(C_TEXT_PRIMARY);
    t3.setString("Cancel");
    t3.setPosition(m_DeleteModalCancelBtn.left + 26.f, m_DeleteModalCancelBtn.top + 8.f);
    window.draw(t3);
}

void UIEditorScene::DrawPill(sf::RenderWindow &window, const sf::FloatRect &r, sf::Color fill, sf::Color outline)
{
    float radius = 4.f;
    const int cornerPoints = 8;
    sf::ConvexShape shape(32);

    const float PI = 3.141592654f;
    auto addArc = [&](int startIndex, float cx, float cy, float startAngle, float endAngle) {
        for (int i = 0; i < cornerPoints; ++i)
        {
            float t = static_cast<float>(i) / (cornerPoints - 1);
            float angle = startAngle + (endAngle - startAngle) * t;
            shape.setPoint(startIndex + i, sf::Vector2f(cx + std::cos(angle) * radius, cy + std::sin(angle) * radius));
        }
    };

    addArc(0, r.left + radius, r.top + radius, PI, PI * 1.5f);
    addArc(cornerPoints, r.left + r.width - radius, r.top + radius, PI * 1.5f, PI * 2.f);
    addArc(cornerPoints * 2, r.left + r.width - radius, r.top + r.height - radius, 0.f, PI * 0.5f);
    addArc(cornerPoints * 3, r.left + radius, r.top + r.height - radius, PI * 0.5f, PI);

    shape.setFillColor(fill);
    shape.setOutlineColor(outline);
    shape.setOutlineThickness(1.f);
    window.draw(shape);
}

float UIEditorScene::DrawSectionHeader(sf::RenderWindow &window, const std::string &title, sf::Color accent, float x,
                                       float y)
{
    if (y + 20.f <= m_InspectorClipTop || y >= m_InspectorClipBottom)
        return y + 20.f;

    sf::RectangleShape hairline({InspectorWidth, 1.f});
    hairline.setFillColor(C_BORDER);
    hairline.setPosition(x, y + 2.f);
    window.draw(hairline);
    y += 4.f;

    sf::Text text;
    text.setFont(*m_Font);
    text.setCharacterSize(10);
    text.setFillColor(accent);
    text.setStyle(sf::Text::Bold);
    text.setString(title);
    text.setPosition(x + 10.f + 2.f, y + 4.f);
    window.draw(text);

    return y + 20.f;
}

static std::string GetUIInspectorTooltip(const std::string &key)
{
    if (key == "Element ID" || key == "ID") return "Unique identifier of the UI element";
    if (key == "Type") return "Type of the UI element";
    if (key == "Parent" || key == "Parent Box") return "ID of parent element (for layout containers)";
    if (key == "Position X" || key == "X") return "Horizontal position in pixels";
    if (key == "Position Y" || key == "Y") return "Vertical position in pixels";
    if (key == "Z-Index (Depth)" || key == "Z-Index" || key == "Z") return "Render layer: higher values render in front";
    if (key == "Width" || key == "W") return "Width of the element in pixels";
    if (key == "Height" || key == "H") return "Height of the element in pixels";
    if (key == "Text" || key == "Default Text") return "Displayed text of the element";
    if (key == "Font Size" || key == "Char Size") return "Font size in pixels";
    if (key == "Spacing" || key == "Layout Spacing") return "Spacing between child elements in layout boxes";
    if (key == "Padding" || key == "Layout Padding") return "Internal padding of the layout container in pixels";
    if (key == "Alignment") return "Text alignment (Left, Center, Right)";
    if (key == "Vertical Alignment") return "Vertical text alignment (Top, Middle, Bottom)";
    if (key == "Letter Spacing") return "Horizontal spacing between letters";
    if (key == "Line Spacing") return "Vertical spacing between text lines";
    if (key == "Text Outline Color") return "Outline color for text glyphs";
    if (key.find("Text Outline") != std::string::npos || key.find("TxtOut") != std::string::npos) return "Text outline properties";
    if (key.find("Text Offset") != std::string::npos) return "Fine-tuning offset for text alignment";
    if (key.find("Text Color") != std::string::npos || key.find("Text ") != std::string::npos) return "Text color component";
    if (key.find("Background") != std::string::npos || key.find("Bg ") != std::string::npos) return "Background property";
    if (key.find("Focus") != std::string::npos) return "Focus state property";
    if (key.find("Border") != std::string::npos) return "Border property";
    if (key.find("Normal") != std::string::npos) return "Normal button state property";
    if (key.find("Hover") != std::string::npos) return "Hover button state property";
    if (key.find("Pressed") != std::string::npos) return "Pressed button state property";
    if (key.find("Color Red") != std::string::npos || key == "R") return "Red color component (0-255)";
    if (key.find("Color Green") != std::string::npos || key == "G") return "Green color component (0-255)";
    if (key.find("Color Blue") != std::string::npos || key == "B") return "Blue color component (0-255)";
    if (key == "Alpha" || key == "A" || key == "Opacity") return "Element opacity / alpha transparency";
    if (key == "Value" || key == "Progress") return "Current value (slider or progress bar)";
    if (key == "Min") return "Minimum value of the slider";
    if (key == "Max") return "Maximum value of the slider or progress bar";
    if (key == "Checked") return "Checkbox state (checked / unchecked)";
    if (key.find("Thickness") != std::string::npos || key.find("Thk") != std::string::npos) return "Thickness in pixels";
    if (key == "OnClick Action" || key == "onClick" || key == "Action") return "Action to trigger when clicked";
    if (key == "onHover") return "Action to trigger on hover";
    if (key.find("Param") != std::string::npos) return "Parameter value for the selected action";
    if (key == "Script File") return "Target Lua script for handling this UI element's events";
    if (key == "Method Name") return "Name of the handler function generated in the script";
    return "";
}

float UIEditorScene::DrawRow(sf::RenderWindow &window, const std::string &key, const std::string &val, float x, float y)
{
    sf::Text keyText;
    keyText.setFont(*m_Font);
    keyText.setCharacterSize(12);
    keyText.setFillColor(C_TEXT_SECONDARY);
    keyText.setString(key);

    const float labelWidth = keyText.getLocalBounds().width;
    const bool wrapField = (labelWidth + 18.f > InspectorWidth * 0.44f);
    const float rowH = wrapField ? 38.f : 20.f;

    const sf::FloatRect rowRect(x, y, InspectorWidth, rowH);
    if (rowRect.contains(m_MouseScreenPos))
    {
        std::string tip = GetUIInspectorTooltip(key);
        if (!tip.empty()) m_ActiveTooltip = tip;
    }

    if (wrapField)
    {
        keyText.setPosition(x + 14.f, y + 2.f);
    }
    else
    {
        keyText.setPosition(x + 14.f, y + 3.f);
    }
    window.draw(keyText);

    sf::Text valText;
    valText.setFont(*m_Font);
    valText.setCharacterSize(12);
    valText.setFillColor(C_TEXT_PRIMARY);
    valText.setString(val);

    if (wrapField)
    {
        valText.setPosition(x + 14.f, y + 18.f);
    }
    else
    {
        const float valX = x + InspectorWidth * 0.44f;
        valText.setPosition(valX + 4.f, y + 3.f);
    }
    window.draw(valText);

    return y + rowH;
}

float UIEditorScene::DrawEditableRow(sf::RenderWindow &window, const std::string &key, const std::string &val,
                                     const std::string &action, float x, float y)
{
    sf::Text keyText;
    keyText.setFont(*m_Font);
    keyText.setCharacterSize(12);
    keyText.setFillColor(C_TEXT_SECONDARY);
    keyText.setString(key);

    const float labelWidth = keyText.getLocalBounds().width;
    const bool wrapField = (labelWidth + 18.f > InspectorWidth * 0.44f);
    const float rowH = wrapField ? 44.f : 24.f;

    if (y + rowH <= m_InspectorClipTop || y >= m_InspectorClipBottom)
        return y + rowH;

    const sf::FloatRect rowRect(x, y, InspectorWidth, rowH);
    if (rowRect.contains(m_MouseScreenPos))
    {
        std::string tip = GetUIInspectorTooltip(key);
        if (!tip.empty()) m_ActiveTooltip = tip;
    }

    float valX, valY, valW;
    if (wrapField)
    {
        keyText.setPosition(x + 14.f, y + 2.f);
        valX = x + 14.f;
        valY = y + 18.f;
        valW = InspectorWidth - 28.f;
    }
    else
    {
        keyText.setPosition(x + 14.f, y + 3.f);
        valX = x + InspectorWidth * 0.44f;
        valY = y;
        valW = InspectorWidth - InspectorWidth * 0.44f - 10.f;
    }
    window.draw(keyText);

    const sf::FloatRect fieldRect(valX, valY, valW, 20.f);
    const bool hovered = fieldRect.contains(m_MouseScreenPos);

    sf::Color fieldFill = hovered ? C_BG_ELEVATED : C_BG_INPUT;
    sf::Color fieldBorder = hovered ? C_ACCENT : C_BORDER;

    sf::RectangleShape field({fieldRect.width, fieldRect.height});
    field.setPosition(fieldRect.left, fieldRect.top);
    field.setFillColor(fieldFill);
    field.setOutlineColor(fieldBorder);
    field.setOutlineThickness(1.f);
    window.draw(field);

    const bool isFieldActive = (!val.empty() && val.back() == '|');
    if (isFieldActive)
    {
        m_ActiveInputBounds = fieldRect;
        if (HasTextSelection())
        {
            sf::Text t(m_ActiveInputText, *m_Font, 12);
            int sMin = std::clamp(GetSelectionMin(), 0, (int)m_ActiveInputText.size());
            int sMax = std::clamp(GetSelectionMax(), 0, (int)m_ActiveInputText.size());
            float x1 = t.findCharacterPos(sMin).x;
            float x2 = t.findCharacterPos(sMax).x;
            sf::RectangleShape selBox({x2 - x1, 14.f});
            selBox.setPosition(valX + 4.f + x1, valY + 3.f);
            selBox.setFillColor(sf::Color(60, 120, 240, 140));
            window.draw(selBox);
        }
    }

    std::string displayVal = val;
    if (!displayVal.empty() && displayVal.back() == '|')
    {
        const bool blink = (static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() / 500) % 2) == 0;
        if (!blink)
            displayVal.pop_back();
    }

    sf::Text valText;
    valText.setFont(*m_Font);
    valText.setCharacterSize(12);
    valText.setFillColor(hovered ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
    valText.setString(displayVal);
    valText.setPosition(valX + 4.f, valY + 3.f);
    window.draw(valText);

    m_InspectorHitboxes.push_back({fieldRect, action});
    return y + rowH;
}

float UIEditorScene::DrawTextureSlot(sf::RenderWindow &window, const std::string &label, const std::string &path,
                                     const std::string &action, const std::string &clearAction,
                                     const std::string &browseAction, float x, float y)
{
    sf::Text keyText;
    keyText.setFont(*m_Font);
    keyText.setCharacterSize(11);
    keyText.setFillColor(C_TEXT_SECONDARY);
    keyText.setString(label);

    const float labelWidth = keyText.getLocalBounds().width;
    const bool wrapField = (labelWidth + 18.f > InspectorWidth * 0.44f);
    const float rowH = wrapField ? 44.f : 24.f;

    if (y + rowH <= m_InspectorClipTop || y >= m_InspectorClipBottom)
        return y + rowH;

    const sf::FloatRect rowRect(x, y, InspectorWidth, rowH);
    if (rowRect.contains(m_MouseScreenPos))
    {
        m_ActiveTooltip = "Assign texture path, browse (..) or clear (x)";
    }

    float valX, valY, totalW;
    if (wrapField)
    {
        keyText.setPosition(x + 14.f, y + 2.f);
        valX = x + 14.f;
        valY = y + 18.f;
        totalW = InspectorWidth - 28.f;
    }
    else
    {
        keyText.setPosition(x + 14.f, y + 4.f);
        valX = x + InspectorWidth * 0.44f;
        valY = y + 1.f;
        totalW = InspectorWidth - InspectorWidth * 0.44f - 14.f;
    }
    window.draw(keyText);

    const float browseBtnW = browseAction.empty() ? 0.f : 24.f;
    const float clearBtnW = path.empty() ? 0.f : 20.f;
    const float valW = totalW - clearBtnW - browseBtnW;
    const sf::FloatRect fieldRect(valX, valY, valW, 20.f);
    const sf::FloatRect rowSlotRect(valX, valY, valW, 20.f);
    const bool hovered = fieldRect.contains(m_MouseScreenPos);

    bool isDragHover = false;
    if (m_ContentBrowser && m_ContentBrowser->HasDraggedAsset())
    {
        if (m_ContentBrowser->GetDraggedAsset().type == AssetType::Image &&
            (fieldRect.contains(m_MouseScreenPos) || rowSlotRect.contains(m_MouseScreenPos)))
        {
            isDragHover = true;
        }
    }

    sf::Color fieldFill = isDragHover ? C_ACCENT_DIM : (hovered ? C_BG_ELEVATED : C_BG_INPUT);
    sf::Color fieldBorder = isDragHover ? C_ACCENT_BRIGHT : (hovered ? C_ACCENT : C_BORDER);

    sf::RectangleShape field({fieldRect.width, fieldRect.height});
    field.setPosition(fieldRect.left, fieldRect.top);
    field.setFillColor(fieldFill);
    field.setOutlineColor(fieldBorder);
    field.setOutlineThickness(1.f);
    window.draw(field);

    std::string display = isDragHover ? "+ Drop Here" : "[Drop / ..]";
    if (!path.empty())
    {
        std::filesystem::path p(path);
        display = p.filename().string();
        if (display.length() > 12) display = display.substr(0, 9) + "..";
    }

    sf::Text valText;
    valText.setFont(*m_Font);
    valText.setCharacterSize(11);
    valText.setFillColor(path.empty() ? (isDragHover ? C_ACCENT_BRIGHT : C_TEXT_MUTED) : (hovered ? C_TEXT_PRIMARY : C_TEXT_SECONDARY));
    valText.setString(display);
    valText.setPosition(valX + 4.f, valY + 3.f);
    window.draw(valText);

    m_InspectorHitboxes.push_back({rowSlotRect, action});

    float curRight = valX + valW + 2.f;

    if (!browseAction.empty())
    {
        const sf::FloatRect browseRect(curRight, valY, 22.f, 20.f);
        const bool browseHov = browseRect.contains(m_MouseScreenPos);
        sf::RectangleShape browseBox({22.f, 20.f});
        browseBox.setPosition(browseRect.left, browseRect.top);
        browseBox.setFillColor(browseHov ? C_ACCENT_DIM : C_BG_INPUT);
        browseBox.setOutlineColor(browseHov ? C_ACCENT : C_BORDER);
        browseBox.setOutlineThickness(1.f);
        window.draw(browseBox);

        sf::Text bText;
        bText.setFont(*m_Font);
        bText.setCharacterSize(10);
        bText.setFillColor(browseHov ? C_ACCENT_BRIGHT : C_TEXT_SECONDARY);
        bText.setString("..");
        bText.setPosition(browseRect.left + 6.f, browseRect.top + 3.f);
        window.draw(bText);

        m_InspectorHitboxes.push_back({browseRect, browseAction});
        curRight += 24.f;
    }

    if (!path.empty())
    {
        const sf::FloatRect clearRect(curRight, valY, 18.f, 20.f);
        const bool clearHov = clearRect.contains(m_MouseScreenPos);
        sf::RectangleShape clearBox({18.f, 20.f});
        clearBox.setPosition(clearRect.left, clearRect.top);
        clearBox.setFillColor(clearHov ? C_DANGER_DIM : C_BG_INPUT);
        clearBox.setOutlineColor(clearHov ? C_DANGER : C_BORDER);
        clearBox.setOutlineThickness(1.f);
        window.draw(clearBox);

        sf::Text xText;
        xText.setFont(*m_Font);
        xText.setCharacterSize(11);
        xText.setFillColor(clearHov ? C_DANGER : C_TEXT_MUTED);
        xText.setString("x");
        xText.setPosition(clearRect.left + 5.f, clearRect.top + 3.f);
        window.draw(xText);

        m_InspectorHitboxes.push_back({clearRect, clearAction});
    }

    return y + rowH;
}

float UIEditorScene::DrawColorPickerRow(sf::RenderWindow &window, const std::string &label, const sf::Color &color,
                                       const std::string &action, float x, float y)
{
    sf::Text keyText;
    keyText.setFont(*m_Font);
    keyText.setCharacterSize(11);
    keyText.setFillColor(C_TEXT_SECONDARY);
    keyText.setString(label);

    const float labelWidth = keyText.getLocalBounds().width;
    const bool wrapField = (labelWidth + 18.f > InspectorWidth * 0.44f);
    const float rowH = wrapField ? 44.f : 24.f;

    if (y + rowH <= m_InspectorClipTop || y >= m_InspectorClipBottom)
        return y + rowH;

    const sf::FloatRect rowRect(x, y, InspectorWidth, rowH);
    if (rowRect.contains(m_MouseScreenPos))
    {
        m_ActiveTooltip = "Edit color values or open color picker";
    }

    float valX, valY, totalW;
    if (wrapField)
    {
        keyText.setPosition(x + 14.f, y + 2.f);
        valX = x + 14.f;
        valY = y + 18.f;
        totalW = InspectorWidth - 28.f;
    }
    else
    {
        keyText.setPosition(x + 14.f, y + 4.f);
        valX = x + InspectorWidth * 0.44f;
        valY = y + 1.f;
        totalW = InspectorWidth - InspectorWidth * 0.44f - 14.f;
    }
    window.draw(keyText);

    const float pickBtnW = 46.f;
    const float swatchW = totalW - pickBtnW - 4.f;

    const sf::FloatRect swatchRect(valX, valY, swatchW, 20.f);
    const bool swatchHov = swatchRect.contains(m_MouseScreenPos);
    sf::RectangleShape swatch({swatchRect.width, swatchRect.height});
    swatch.setPosition(swatchRect.left, swatchRect.top);
    swatch.setFillColor(color);
    swatch.setOutlineColor(swatchHov ? C_ACCENT : C_BORDER);
    swatch.setOutlineThickness(swatchHov ? 2.f : 1.f);
    window.draw(swatch);
    m_InspectorHitboxes.push_back({swatchRect, action});

    const sf::FloatRect btnRect(valX + swatchW + 4.f, valY, pickBtnW, 20.f);
    const bool btnHov = btnRect.contains(m_MouseScreenPos);
    DrawPill(window, btnRect, btnHov ? C_BG_ELEVATED : C_BG_INPUT, btnHov ? C_ACCENT : C_BORDER_LIGHT);
    sf::Text btnText;
    btnText.setFont(*m_Font);
    btnText.setCharacterSize(10);
    btnText.setFillColor(btnHov ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
    btnText.setString("Pick");
    btnText.setPosition(btnRect.left + (btnRect.width - btnText.getLocalBounds().width) / 2.f, btnRect.top + 3.f);
    window.draw(btnText);
    m_InspectorHitboxes.push_back({btnRect, action});

    return y + rowH;
}

float UIEditorScene::DrawActionButton(sf::RenderWindow &window, const std::string &label, const std::string &action,
                                      float x, float y, sf::Color fillColor, sf::Color borderColor)
{
    if (y + 30.f <= m_InspectorClipTop || y >= m_InspectorClipBottom)
        return y + 30.f;

    sf::FloatRect r(x, y, 0.f, 24.f);
    sf::Text t;
    t.setFont(*m_Font);
    t.setCharacterSize(12);
    t.setString(label);
    r.width = t.getLocalBounds().width + 24.f;

    bool hov = r.contains(m_MouseScreenPos);
    if (hov)
    {
        if (action == "layer_forward") m_ActiveTooltip = "Bring element forward (+1 layer)";
        else if (action == "layer_backward") m_ActiveTooltip = "Send element backward (-1 layer)";
        else if (action == "delete_element") m_ActiveTooltip = "Delete selected UI element";
        else if (label.find("Links") != std::string::npos || label.find("Mitte") != std::string::npos || label.find("Rechts") != std::string::npos ||
                 label.find("Left") != std::string::npos || label.find("Center") != std::string::npos || label.find("Right") != std::string::npos)
            m_ActiveTooltip = "Change horizontal text alignment";
        else if (label.find("Top") != std::string::npos || label.find("Middle") != std::string::npos || label.find("Bottom") != std::string::npos)
            m_ActiveTooltip = "Change vertical text alignment";
    }
    bool active = (fillColor != C_BG_ELEVATED);

    sf::Color fill = active
                         ? fillColor
                         : hov
                               ? C_BG_ELEVATED
                               : sf::Color::Transparent;
    sf::Color bdr = active
                        ? sf::Color(borderColor.r, borderColor.g, borderColor.b, 200)
                        : hov
                              ? C_BORDER_LIGHT
                              : sf::Color::Transparent;
    DrawPill(window, r, fill, bdr);

    t.setFillColor(active
                       ? sf::Color(borderColor.r, borderColor.g, borderColor.b, 255)
                       : hov
                             ? C_TEXT_PRIMARY
                             : C_TEXT_SECONDARY);
    t.setPosition(r.left + (r.width - t.getLocalBounds().width) / 2.f,
                  r.top + (r.height - t.getLocalBounds().height) / 2.f - 2.f);
    window.draw(t);

    m_InspectorHitboxes.push_back({r, action});
    return y + 30.f;
}
void UIEditorScene::InitMenus()
{
    MenuEntry datei;
    datei.label = "File";
    datei.items = {
        {"Save UI", "save", false, "Ctrl+S"},
        {"", "", true, ""},
        {"Quit", "quit", false, ""}
    };

    MenuEntry edit;
    edit.label = "Edit";
    edit.items = {
        {"Copy", "copy", false, "Ctrl+C"},
        {"Paste", "paste", false, "Ctrl+V"},
        {"", "", true, ""},
        {"Delete Element", "delete", false, "Del"},
        {"Deselect", "deselect", false, "Esc"},
        {"", "", true, ""},
        {"Clear UI", "clear", false, ""}
    };

    MenuEntry ansicht;
    ansicht.label = "View";
    ansicht.items = {
        {"Center Camera", "center_camera", false, ""}
    };

    MenuEntry tools;
    tools.label = "Tools";
    tools.items = {
        {"Game Editor", "back", false, ""}
    };

    m_Menus = {datei, edit, ansicht, tools};
}

void UIEditorScene::HandleMenuAction(const std::string &action)
{
    if (action == "save") {
        HandleAction("save");
    } else if (action == "quit") {
        m_Window.close();
    } else if (action == "copy") {
        CopySelection();
    } else if (action == "paste") {
        PasteClipboard();
    } else if (action == "delete") {
        DeleteSelected();
    } else if (action == "deselect") {
        SelectElement(nullptr);
    } else if (action == "clear") {
        UIManager::Get().GetElements().clear();
        SelectElement(nullptr);
    } else if (action == "center_camera") {
        m_CanvasView.setCenter(m_CanvasSize.x / 2.f, m_CanvasSize.y / 2.f);
    } else if (action == "back") {
        HandleAction("back");
    }
}

void UIEditorScene::DrawDropdownOverlay(sf::RenderWindow &window)
{
    if (m_ActiveDropdown.empty()) return;

    if (m_ActiveDropdown == "script_select" && m_SelectedElement)
    {
        auto scripts = GetAvailableScripts();
        std::vector<std::pair<std::string, std::string>> scriptOptions;
        scriptOptions.push_back({"assets/scripts/ui_interactions.lua", "+ ui_interactions.lua (Default)"});
        scriptOptions.push_back({"assets/scripts/" + m_SelectedElement->id + ".lua", "+ " + m_SelectedElement->id + ".lua (New)"});
        scriptOptions.push_back({"__browse__", "[.. Browse File on Disk...]"});
        for (const auto &s : scripts)
        {
            if (s != "assets/scripts/ui_interactions.lua" && s != "assets/scripts/" + m_SelectedElement->id + ".lua")
                scriptOptions.push_back({s, s});
        }

        float totalH = std::min(static_cast<float>(scriptOptions.size() * 24.f + 8.f), 260.f);
        sf::RectangleShape bg({m_DropdownRect.width, totalH});
        bg.setFillColor(C_BG_ELEVATED);
        bg.setOutlineColor(C_BORDER_LIGHT);
        bg.setOutlineThickness(1.f);
        bg.setPosition(m_DropdownRect.left, m_DropdownRect.top);
        window.draw(bg);

        float y = m_DropdownRect.top + 4.f;
        for (const auto &opt : scriptOptions)
        {
            if (y + 24.f > m_DropdownRect.top + totalH) break;
            sf::FloatRect r(m_DropdownRect.left, y, m_DropdownRect.width, 24.f);
            bool hov = r.contains(m_MouseScreenPos);
            if (hov)
            {
                sf::RectangleShape hbg({m_DropdownRect.width - 4.f, 22.f});
                hbg.setFillColor(C_ACCENT_DIM);
                hbg.setPosition(m_DropdownRect.left + 2.f, y + 1.f);
                window.draw(hbg);
            }
            sf::Text t;
            t.setFont(*m_Font);
            t.setCharacterSize(11);
            t.setFillColor(hov ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
            std::string label = opt.second;
            if (label.length() > 36) label = "..." + label.substr(label.length() - 33);
            t.setString(label);
            t.setPosition(m_DropdownRect.left + 8.f, y + 5.f);
            window.draw(t);
            y += 24.f;
        }
        return;
    }

    sf::RectangleShape bg({m_DropdownRect.width, m_DropdownRect.height});
    bg.setFillColor(C_BG_ELEVATED);
    bg.setOutlineColor(C_BORDER_LIGHT);
    bg.setOutlineThickness(1.f);
    bg.setPosition(m_DropdownRect.left, m_DropdownRect.top);
    window.draw(bg);

    float y = m_DropdownRect.top + 4.f;
    for (const auto& opt : UTILITY_ACTIONS)
    {
        sf::FloatRect r(m_DropdownRect.left, y, m_DropdownRect.width, 24.f);
        bool hov = r.contains(m_MouseScreenPos);
        
        if (hov) {
            sf::RectangleShape hbg({m_DropdownRect.width - 4.f, 22.f});
            hbg.setFillColor(C_ACCENT_DIM);
            hbg.setPosition(m_DropdownRect.left + 2.f, y + 1.f);
            window.draw(hbg);
        }

        sf::Text t;
        t.setFont(*m_Font);
        t.setCharacterSize(12);
        t.setFillColor(hov ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
        t.setString(opt.label);
        t.setPosition(m_DropdownRect.left + 8.f, y + 4.f);
        window.draw(t);

        y += 24.f;
    }
}

void UIEditorScene::DrawMenuBar(sf::RenderWindow &window)
{
    const float w = static_cast<float>(window.getSize().x);

    sf::RectangleShape bg({w, MenuBarHeight});
    bg.setFillColor(C_BG_PANEL);
    bg.setPosition(0.f, 0.f);
    window.draw(bg);

    sf::RectangleShape border({w, 1.f});
    border.setFillColor(C_BORDER);
    border.setPosition(0.f, MenuBarHeight - 1.f);
    window.draw(border);

    m_MenuItemHitboxes.clear();

    float x = 6.f;
    for (int i = 0; i < (int) m_Menus.size(); i++)
    {
        auto &menu = m_Menus[i];
        const bool open = (m_OpenMenuIndex == i);
        const bool hovered = menu.bounds.contains(m_MouseScreenPos) || open;

        sf::Text lbl;
        lbl.setFont(*m_Font);
        lbl.setCharacterSize(12);
        lbl.setString(menu.label);

        const float itemW = lbl.getLocalBounds().width + 22.f;
        menu.bounds = sf::FloatRect(x, 0.f, itemW, MenuBarHeight);

        if (hovered)
        {
            sf::RectangleShape hbg({itemW - 2.f, MenuBarHeight - 6.f});
            hbg.setFillColor(open ? C_ACCENT_DIM : C_BG_ELEVATED);
            hbg.setPosition(x + 1.f, 3.f);
            window.draw(hbg);
        }

        lbl.setFillColor(hovered ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
        lbl.setPosition(x + 10.f, 8.f);
        window.draw(lbl);

        if (open)
        {
            float maxLabelW = 0.f;
            float maxShortW = 0.f;
            for (auto &item: menu.items)
            {
                if (item.isSeparator) continue;
                sf::Text tmp;
                tmp.setFont(*m_Font);
                tmp.setCharacterSize(12);
                tmp.setString(item.label);
                maxLabelW = std::max(maxLabelW, tmp.getLocalBounds().width);
                if (!item.shortcut.empty())
                {
                    tmp.setCharacterSize(11);
                    tmp.setString(item.shortcut);
                    maxShortW = std::max(maxShortW, tmp.getLocalBounds().width);
                }
            }

            const float dropW = std::max(maxLabelW + maxShortW + 52.f, 190.f);
            const float dropX = x;
            const float dropY = MenuBarHeight;

            float dropH = 8.f;
            for (auto &item: menu.items)
                dropH += item.isSeparator ? 8.f : 28.f;

            sf::RectangleShape dbg({dropW, dropH});
            dbg.setFillColor(C_BG_ELEVATED);
            dbg.setOutlineColor(C_BORDER_LIGHT);
            dbg.setOutlineThickness(1.f);
            dbg.setPosition(dropX, dropY);
            window.draw(dbg);

            float iy = dropY + 4.f;
            for (auto &item: menu.items)
            {
                if (item.isSeparator)
                {
                    sf::RectangleShape sep({dropW - 12.f, 1.f});
                    sep.setFillColor(C_BORDER);
                    sep.setPosition(dropX + 6.f, iy + 3.f);
                    window.draw(sep);
                    iy += 8.f;
                    continue;
                }

                const sf::FloatRect ir(dropX, iy, dropW, 28.f);
                const bool ih = ir.contains(m_MouseScreenPos);

                if (ih)
                {
                    sf::RectangleShape ibg({dropW - 6.f, 26.f});
                    ibg.setFillColor(C_ACCENT_DIM);
                    ibg.setPosition(dropX + 3.f, iy + 1.f);
                    window.draw(ibg);
                }

                sf::Text il;
                il.setFont(*m_Font);
                il.setCharacterSize(12);
                il.setFillColor(ih ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
                il.setString(item.label);
                il.setPosition(dropX + 14.f, iy + 7.f);
                window.draw(il);

                if (!item.shortcut.empty())
                {
                    sf::Text sl;
                    sl.setFont(*m_Font);
                    sl.setCharacterSize(10);
                    sl.setFillColor(C_TEXT_MUTED);
                    sl.setString(item.shortcut);
                    sl.setPosition(dropX + dropW - sl.getLocalBounds().width - 10.f, iy + 8.f);
                    window.draw(sl);
                }

                m_MenuItemHitboxes.push_back({ir, item.action});
                iy += 28.f;
            }
        }

        x += itemW;
    }

    std::string currentPath = UIManager::Get().GetCurrentUIPath();
    if (currentPath.empty()) currentPath = "game_ui.json";
    std::string wm = currentPath + (m_HasUnsavedChanges ? " *" : "") + " | RayneEngine UI";
    sf::Text rightText;
    if (m_Font) {
        rightText.setFont(*m_Font);
        rightText.setCharacterSize(11);
        rightText.setFillColor(m_HasUnsavedChanges ? C_WARNING : C_TEXT_MUTED);
        rightText.setString(wm);
        rightText.setPosition(w - rightText.getLocalBounds().width - 12.f, 9.f);
        window.draw(rightText);
    }
}

void UIEditorScene::DeleteActiveSelection()
{
    if (!HasTextSelection()) return;
    int sMin = GetSelectionMin();
    int sMax = GetSelectionMax();

    if (m_ActiveField != EditField::None)
    {
        sMin = std::clamp(sMin, 0, (int)m_ActiveInputText.size());
        sMax = std::clamp(sMax, 0, (int)m_ActiveInputText.size());
        m_ActiveInputText.erase(sMin, sMax - sMin);
    }
    m_InputSelectionStart = -1;
    m_InputSelectionEnd = -1;
}

void UIEditorScene::DrawTooltip(sf::RenderWindow &window)
{
    if (m_ActiveTooltip.empty()) return;

    sf::Text text;
    if (m_Font) text.setFont(*m_Font);
    text.setCharacterSize(11);
    text.setFillColor(C_TEXT_PRIMARY);
    text.setString(m_ActiveTooltip);

    sf::FloatRect tb = text.getLocalBounds();
    const float pad = 6.f;
    const float w = tb.width + pad * 2.f;
    const float h = tb.height + pad * 2.f + 4.f;

    sf::Vector2f pos = m_MouseScreenPos + sf::Vector2f(12.f, 16.f);
    if (pos.x + w > window.getSize().x - 4.f)
        pos.x = window.getSize().x - w - 4.f;
    if (pos.y + h > window.getSize().y - 4.f)
        pos.y = m_MouseScreenPos.y - h - 4.f;

    sf::RectangleShape bg({w, h});
    bg.setPosition(pos);
    bg.setFillColor(sf::Color(22, 25, 32, 245));
    bg.setOutlineColor(C_ACCENT);
    bg.setOutlineThickness(1.f);

    text.setPosition(pos.x + pad, pos.y + pad - tb.top);

    window.draw(bg);
    window.draw(text);
}

std::string UIEditorScene::NormalizeScriptPath(const std::string &rawPath) const
{
    if (rawPath.empty()) return "assets/scripts/ui_interactions.lua";

    std::string p = rawPath;
    for (char &c : p) { if (c == '\\') c = '/'; }
    while (!p.empty() && p.front() == '/') p.erase(p.begin());

    if (p.length() < 4 || p.substr(p.length() - 4) != ".lua")
        p += ".lua";

    std::filesystem::path rootDir = FindProjectRoot();
    std::error_code ec;

    if (p.rfind("assets/", 0) == 0) return p;

    if (std::filesystem::exists(rootDir / "assets" / p, ec))
        return "assets/" + p;

    if (std::filesystem::exists(rootDir / "assets/scripting" / p, ec))
        return "assets/scripting/" + p;

    if (std::filesystem::exists(rootDir / "assets/scripts" / p, ec))
        return "assets/scripts/" + p;

    return "assets/scripts/" + p;
}

void UIEditorScene::OpenScriptInIDE(const std::string &scriptPath)
{
    if (scriptPath.empty()) return;

    std::string normPath = NormalizeScriptPath(scriptPath);
    std::filesystem::path rootDir = FindProjectRoot();
    std::filesystem::path assetsPath = std::filesystem::absolute(rootDir / "assets");
    std::filesystem::path absScript = std::filesystem::absolute(rootDir / normPath);

    std::error_code ec;
    if (!std::filesystem::exists(absScript, ec))
    {
        std::filesystem::create_directories(absScript.parent_path(), ec);
        std::ofstream ofs(absScript);
        if (ofs.is_open())
        {
            ofs << "-- UI Script generated by RayneEngine\n";
            ofs.close();
        }
        SyncFileToRuntime(normPath);
    }

    std::string ide = m_PreferredIDE;
    if (ide.empty() || !IsExecutableInPath(ide))
    {
        if (IsExecutableInPath("clion")) ide = "clion";
        else if (IsExecutableInPath("code")) ide = "code";
        else if (IsExecutableInPath("rider")) ide = "rider";
        else ide = "";
    }

    if (!ide.empty())
    {
        std::string cmd = ide + " \"" + assetsPath.string() + "\" \"" + absScript.string() + "\"";
        LaunchProcessDetached(cmd);
        std::cout << "[INFO] [UIEditor] Opening script with " << ide << ": " << absScript.string() << " in " << assetsPath.string() << "\n";
    }
    else
    {
#ifdef _WIN32
        ShellExecuteA(nullptr, "open", absScript.string().c_str(), nullptr, nullptr, SW_SHOW);
#elif __APPLE__
        system(("open \"" + absScript.string() + "\"").c_str());
#else
        system(("xdg-open \"" + absScript.string() + "\"").c_str());
#endif
    }
}

std::vector<std::string> UIEditorScene::GetAvailableScripts()
{
    std::vector<std::string> scripts;
    std::filesystem::path rootDir = FindProjectRoot();
    std::filesystem::path assetsDir = rootDir / "assets";

    std::error_code ec;
    if (std::filesystem::exists(assetsDir, ec))
    {
        for (const auto &entry : std::filesystem::recursive_directory_iterator(assetsDir, ec))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".lua")
            {
                std::string fname = entry.path().filename().string();
                if (fname == "api_stub.lua") continue;
                std::string rel = std::filesystem::relative(entry.path(), rootDir, ec).generic_string();
                scripts.push_back(rel);
            }
        }
    }
    return scripts;
}

std::string UIEditorScene::GetDefaultMethodName(const UIElement *el) const
{
    if (!el) return "On_UI_Interaction";
    std::string safeId = el->id;
    for (char &c : safeId) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') c = '_';
    }
    if (el->type == UIElementType::Button) return "On_" + safeId + "_Clicked";
    if (el->type == UIElementType::Checkbox) return "On_" + safeId + "_Toggled";
    if (el->type == UIElementType::Slider) return "On_" + safeId + "_Changed";
    if (el->type == UIElementType::TextInput) return "On_" + safeId + "_Submitted";
    return "On_" + safeId + "_Interaction";
}

bool UIEditorScene::InsertScriptMethod(UIElement *el, const std::string &scriptPath, const std::string &methodName)
{
    if (!el) return false;

    std::string normPath = NormalizeScriptPath(scriptPath);
    std::string normMethod = methodName.empty() ? GetDefaultMethodName(el) : methodName;

    std::filesystem::path rootDir = FindProjectRoot();
    std::filesystem::path fullPath = rootDir / normPath;
    std::error_code ec;
    std::filesystem::create_directories(fullPath.parent_path(), ec);

    std::string content = "";
    if (std::filesystem::exists(fullPath, ec))
    {
        std::ifstream ifs(fullPath);
        if (ifs.is_open())
        {
            content = std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            ifs.close();
        }
    }
    else
    {
        content = "-- UI Script generated by RayneEngine\n\n";
    }

    std::string paramName = "";
    std::string logAction = "clicked!";
    if (el->type == UIElementType::Slider) {
        paramName = "value";
        logAction = "value: \" .. tostring(value)";
    } else if (el->type == UIElementType::Checkbox) {
        paramName = "checked";
        logAction = "checked: \" .. tostring(checked)";
    } else if (el->type == UIElementType::TextInput) {
        paramName = "text";
        logAction = "submitted: \" .. tostring(text)";
    }
    if (content.find("function " + normMethod) == std::string::npos)
    {
        if (!content.empty() && content.back() != '\n') content += "\n";
        content += "\n-- Handler for " + el->id + "\n";
        content += "function " + normMethod + "(" + paramName + ")\n";
        if (!paramName.empty()) {
            content += "    print(\"" + el->id + " " + logAction + ")\n";
        } else {
            content += "    print(\"" + el->id + " " + logAction + "\")\n";
        }
        content += "end\n";
    }
    std::string hookFunc = "";
    std::string hookParam = "";
    std::string callArgs = "";
    if (el->type == UIElementType::Button) {
        hookFunc = "OnButtonClicked";
        hookParam = "id";
        callArgs = "";
    } else if (el->type == UIElementType::Slider) {
        hookFunc = "OnSliderChanged";
        hookParam = "id, value";
        callArgs = "value";
    } else if (el->type == UIElementType::Checkbox) {
        hookFunc = "OnCheckboxChanged";
        hookParam = "id, checked";
        callArgs = "checked";
    } else if (el->type == UIElementType::TextInput) {
        hookFunc = "OnTextInputSubmitted";
        hookParam = "id, text";
        callArgs = "text";
    }

    if (!hookFunc.empty())
    {
        std::string checkPattern = "id == \"" + el->id + "\"";
        if (content.find(checkPattern) == std::string::npos)
        {
            size_t funcPos = content.find("function " + hookFunc);
            if (funcPos != std::string::npos)
            {
                size_t endPos = content.find("\nend", funcPos);
                if (endPos != std::string::npos)
                {
                    std::string dispatch = "    if id == \"" + el->id + "\" then\n        " + normMethod + "(" + callArgs + ")\n    end\n";
                    content.insert(endPos + 1, dispatch);
                }
            }
            else
            {
                if (!content.empty() && content.back() != '\n') content += "\n";
                content += "\nfunction " + hookFunc + "(" + hookParam + ")\n";
                content += "    if id == \"" + el->id + "\" then\n";
                content += "        " + normMethod + "(" + callArgs + ")\n";
                content += "    end\nend\n";
            }
        }
    }
    {
        std::ofstream ofs(fullPath, std::ios::trunc);
        if (!ofs.is_open())
        {
            std::cerr << "[ERROR] [UIEditor] Failed to open script for writing: " << fullPath << "\n";
            return false;
        }
        ofs << content;
        ofs.close();
    }
    SyncFileToRuntime(normPath);
    try {
        LuaState::GetLua().safe_script(content);
    } catch (...) {}
    el->scriptPath = normPath;
    el->scriptMethod = normMethod;
    if (el->type == UIElementType::Button)
    {
        el->onClickAction = normMethod;
    }
    std::string uiPath = UIManager::Get().GetCurrentUIPath();
    if (uiPath.empty()) uiPath = "assets/scenes/game_ui.json";
    UIManager::Get().SetCurrentUIPath(uiPath);
    std::filesystem::path fullUIPath = rootDir / uiPath;
    std::filesystem::create_directories(fullUIPath.parent_path(), ec);
    UIManager::Get().Save(fullUIPath.string());
    SyncFileToRuntime(uiPath);

    std::cout << "[INFO] [UIEditor] Successfully inserted " << normMethod << " into " << fullPath.string() << "\n";
    return true;
}
