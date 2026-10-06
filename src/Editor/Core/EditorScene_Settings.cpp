#include "EditorScene_Common.h"
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <map>


void EditorScene::DrawSettingsWindow(sf::RenderWindow &window)
{
    m_SettingsButtons.clear();

    const float winW = std::min(740.f, static_cast<float>(m_Window.getSize().x) - 80.f);
    const float winH = std::min(560.f, static_cast<float>(m_Window.getSize().y) - 80.f);
    const float winX = (m_Window.getSize().x - winW) / 2.f;
    const float winY = (m_Window.getSize().y - winH) / 2.f;

    sf::RectangleShape dim({(float) m_Window.getSize().x, (float) m_Window.getSize().y});
    dim.setFillColor(sf::Color(0, 0, 0, 160));
    window.draw(dim);

    sf::RectangleShape bg({winW, winH});
    bg.setPosition(winX, winY);
    bg.setFillColor(C_BG_ELEVATED);
    bg.setOutlineColor(C_BORDER_LIGHT);
    bg.setOutlineThickness(1.f);
    window.draw(bg);

    const float titleH = 38.f;
    sf::RectangleShape titleBar({winW, titleH});
    titleBar.setPosition(winX, winY);
    titleBar.setFillColor(C_BG_ELEVATED);
    window.draw(titleBar);

    sf::RectangleShape titleBorder({winW, 1.f});
    titleBorder.setPosition(winX, winY + titleH - 1.f);
    titleBorder.setFillColor(C_BORDER);
    window.draw(titleBorder);

    sf::Text titleText;
    titleText.setFont(*m_Font);
    titleText.setCharacterSize(13);
    titleText.setFillColor(C_TEXT_PRIMARY);
    titleText.setStyle(sf::Text::Bold);
    titleText.setString("Settings");
    titleText.setPosition(winX + 16.f, winY + 11.f);
    window.draw(titleText);

    const sf::FloatRect closeRect(winX + winW - 34.f, winY + 6.f, 26.f, 26.f);
    const bool closeHov = closeRect.contains(m_MouseScreenPos);
    sf::RectangleShape closeBg({26.f, 26.f});
    closeBg.setPosition(closeRect.left, closeRect.top);
    closeBg.setFillColor(closeHov ? C_DANGER_DIM : sf::Color::Transparent);
    closeBg.setOutlineColor(closeHov ? C_DANGER : sf::Color::Transparent);
    closeBg.setOutlineThickness(1.f);
    window.draw(closeBg);
    sf::Text closeText;
    closeText.setFont(*m_Font);
    closeText.setCharacterSize(14);
    closeText.setFillColor(closeHov ? C_DANGER : C_TEXT_MUTED);
    closeText.setString("x");
    closeText.setPosition(closeRect.left + 8.f, closeRect.top + 4.f);
    window.draw(closeText);
    m_SettingsButtons.push_back({closeRect, "close_settings"});

    const float tabY = winY + titleH;
    const float tabH = 32.f;
    const float tabW = winW / 5.f;
    const std::vector<std::string> tabNames = {"General", "Editor", "Rendering", "Input", "Debug"};
    const std::vector<sf::Color> tabAccents = {
        C_ACCENT, C_ACCENT, C_ACCENT, C_ACCENT, C_ACCENT
    };

    sf::RectangleShape tabBar({winW, tabH});
    tabBar.setPosition(winX, tabY);
    tabBar.setFillColor(C_BG_PANEL);
    window.draw(tabBar);
    sf::RectangleShape tabBarBorder({winW, 1.f});
    tabBarBorder.setPosition(winX, tabY + tabH - 1.f);
    tabBarBorder.setFillColor(C_BORDER);
    window.draw(tabBarBorder);

    for (int i = 0; i < (int) tabNames.size(); i++)
    {
        const sf::FloatRect tabRect(winX + i * tabW, tabY, tabW, tabH);
        const bool active = (m_SettingsTab == i);
        const bool hovered = tabRect.contains(m_MouseScreenPos);

        if (active)
        {
            sf::RectangleShape tabBg({tabW, tabH});
            tabBg.setPosition(tabRect.left, tabRect.top);
            tabBg.setFillColor(C_ACCENT_DIM);
            window.draw(tabBg);

            sf::RectangleShape accentLine({tabW, 2.f});
            accentLine.setPosition(tabRect.left, tabRect.top + tabH - 2.f);
            accentLine.setFillColor(tabAccents[i]);
            window.draw(accentLine);
        } else if (hovered)
        {
            sf::RectangleShape tabBg({tabW, tabH});
            tabBg.setPosition(tabRect.left, tabRect.top);
            tabBg.setFillColor(C_BG_ELEVATED);
            window.draw(tabBg);
        }

        sf::Text tabText;
        tabText.setFont(*m_Font);
        tabText.setCharacterSize(12);
        tabText.setFillColor(active ? tabAccents[i] : hovered ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
        tabText.setString(tabNames[i]);
        tabText.setPosition(
            tabRect.left + (tabW - tabText.getLocalBounds().width) / 2.f,
            tabRect.top + (tabH - tabText.getLocalBounds().height) / 2.f - 2.f);
        window.draw(tabText);

        m_SettingsButtons.push_back({tabRect, "tab_" + std::to_string(i)});
    }

    const float contentX = winX + 16.f;
    const float contentY = tabY + tabH + 12.f;
    const float contentW = winW - 32.f;
    float y = contentY;

    const sf::Color accent = tabAccents[m_SettingsTab];

    if (m_SettingsTab == 0)
    {
        y = DrawSettingsSectionHeader(window, "AUTOSAVE", accent, contentX, y, contentW);
        y = DrawSettingsToggle(window, "Enable AutoSave", m_AutoSaveEnabled, "toggle_autosave", contentX, y, contentW);
        y = DrawSettingsInputField(window, "Interval (Seconds)",
                                   std::to_string(static_cast<int>(m_AutoSaveIntervalSeconds)),
                                   SettingsField::AutoSaveInterval, contentX, y, contentW);
        y = DrawSettingsToggle(window, "Show Popup", m_AutoSavePopupEnabled, "toggle_autopopup", contentX, y, contentW);
        y = DrawSettingsInputField(window, "Popup Duration (Seconds)",
                                   std::to_string(static_cast<int>(m_AutoSavePopupDuration)),
                                   SettingsField::AutoSavePopupDuration, contentX, y, contentW);

        y += 16.f;
        y = DrawSettingsSectionHeader(window, "SCENE", sf::Color(100, 180, 255), contentX, y, contentW);
        y = DrawSettingsInputField(window, "Save Path", m_SceneSavePath,
                                   SettingsField::SceneSavePath, contentX, y, contentW);

        sf::Text note;
        note.setFont(*m_Font);
        note.setCharacterSize(10);
        note.setFillColor(C_TEXT_MUTED);
        note.setString("Tip: ASSET_PATH is automatically prepended.");
        note.setPosition(contentX + 2.f, y + 4.f);
        window.draw(note);
        y += 22.f;

        y += 16.f;
        y = DrawSettingsSectionHeader(window, "DISPLAY", sf::Color(200, 200, 255), contentX, y, contentW);
        y = DrawSettingsToggle(window, "AutoSave Countdown in Title Bar",
                               m_ShowAutoSaveInTitle, "toggle_title_countdown", contentX, y, contentW);
    } else if (m_SettingsTab == 1)
    {
        y = DrawSettingsSectionHeader(window, "GRID", accent, contentX, y, contentW);
        y = DrawSettingsInputField(window, "Grid Size (px)",
                                   std::to_string(static_cast<int>(m_GridSize)),
                                   SettingsField::GridSize, contentX, y, contentW);
        y = DrawSettingsInputField(window, "Grid Opacity (0-255)",
                                   std::to_string(m_GridOpacity),
                                   SettingsField::GridOpacityVal, contentX, y, contentW); {
            const sf::FloatRect gridColorRect(contentX + 2.f, y + 2.f, 60.f, 20.f);
            const bool gridHov = gridColorRect.contains(m_MouseScreenPos);
            sf::RectangleShape preview({60.f, 20.f});
            preview.setFillColor(m_GridColor);
            preview.setOutlineColor(gridHov ? C_ACCENT : C_BORDER_LIGHT);
            preview.setOutlineThickness(gridHov ? 2.f : 1.f);
            preview.setPosition(gridColorRect.left, gridColorRect.top);
            window.draw(preview);
            m_SettingsButtons.push_back({gridColorRect, "pick_grid_color"});

            sf::Text colorLabel;
            colorLabel.setFont(*m_Font);
            colorLabel.setCharacterSize(11);
            colorLabel.setFillColor(gridHov ? C_TEXT_PRIMARY : C_TEXT_MUTED);
            colorLabel.setString("Grid Color (RGB " +
                                 std::to_string(m_GridColor.r) + ", " +
                                 std::to_string(m_GridColor.g) + ", " +
                                 std::to_string(m_GridColor.b) + ") - Click to pick");
            colorLabel.setPosition(contentX + 68.f, y + 5.f);
            window.draw(colorLabel);
            y += 30.f;
        }

        y += 12.f;
        y = DrawSettingsSectionHeader(window, "OBJECTS", sf::Color(80, 200, 120), contentX, y, contentW);
        y = DrawSettingsInputField(window, "Default Object Size (px)",
                                   std::to_string(static_cast<int>(m_DefaultObjectSize)),
                                   SettingsField::DefaultObjectSize, contentX, y, contentW);

        y += 12.f;
        y = DrawSettingsSectionHeader(window, "SELECTION", sf::Color(255, 220, 60), contentX, y, contentW);
        y = DrawSettingsInputField(window, "Outline Thickness",
                                   std::to_string(static_cast<int>(m_SelectionOutlineThickness)),
                                   SettingsField::SelectionThickness, contentX, y, contentW); {
            sf::RectangleShape preview({60.f, 20.f});
            preview.setFillColor(m_SelectionOutlineColor);
            preview.setOutlineColor(C_BORDER_LIGHT);
            preview.setOutlineThickness(1.f);
            preview.setPosition(contentX + 2.f, y + 2.f);
            window.draw(preview);
            sf::Text colorLabel;
            colorLabel.setFont(*m_Font);
            colorLabel.setCharacterSize(11);
            colorLabel.setFillColor(C_TEXT_MUTED);
            colorLabel.setString("Selection Color");
            colorLabel.setPosition(contentX + 68.f, y + 5.f);
            window.draw(colorLabel);
            y += 30.f;
        }

        y += 8.f; {
            const sf::FloatRect btnRect(contentX, y + 2.f, contentW, 26.f);
            const bool hov = btnRect.contains(m_MouseScreenPos);
            sf::RectangleShape btn({contentW, 26.f});
            btn.setPosition(contentX, y + 2.f);
            btn.setFillColor(hov ? C_BG_ELEVATED : C_BG_INPUT);
            btn.setOutlineColor(hov ? C_BORDER_LIGHT : C_BORDER);
            btn.setOutlineThickness(1.f);
            window.draw(btn);
            sf::Text btnText;
            btnText.setFont(*m_Font);
            btnText.setCharacterSize(11);
            btnText.setFillColor(hov ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
            btnText.setString("Reset Editor Defaults");
            btnText.setPosition(btnRect.left + (btnRect.width - btnText.getLocalBounds().width) / 2.f, y + 7.f);
            window.draw(btnText);
            m_SettingsButtons.push_back({btnRect, "reset_editor_defaults"});
            y += 34.f;
        }
    } else if (m_SettingsTab == 2)
    {
        y = DrawSettingsSectionHeader(window, "DISPLAY", accent, contentX, y, contentW);
        y = DrawSettingsToggle(window, "Show FPS", m_ShowFPS, "toggle_fps", contentX, y, contentW);

        const std::vector<std::string> fpsCaps = {"Unlimited", "60", "120", "144", "240"};
        y = DrawSettingsDropdown(window, "FPS Cap", fpsCaps, m_FPSCapIndex, "fps_cap", contentX, y, contentW); {
            static const int capValues[] = {0, 60, 120, 144, 240};
            m_Window.setFramerateLimit(static_cast<unsigned>(capValues[m_FPSCapIndex]));
        }

        y += 12.f;
        y = DrawSettingsSectionHeader(window, "CAMERA & ZOOM", sf::Color(100, 200, 255), contentX, y, contentW);
        y = DrawSettingsSlider(window, "Zoom Sensitivity", m_ZoomSensitivity,
                               0.01f, 0.5f, "zoom_sens", contentX, y, contentW);
        y = DrawSettingsInputField(window, "Zoom Min",
                                   [this] {
                                       char buf[32];
                                       snprintf(buf, 32, "%.2f", m_ZoomMin);
                                       return std::string(buf);
                                   }(),
                                   SettingsField::ZoomMin, contentX, y, contentW);
        y = DrawSettingsInputField(window, "Zoom Max",
                                   [this] {
                                       char buf[32];
                                       snprintf(buf, 32, "%.2f", m_ZoomMax);
                                       return std::string(buf);
                                   }(),
                                   SettingsField::ZoomMax, contentX, y, contentW);

        y += 8.f; {
            const sf::FloatRect btnRect(contentX, y + 2.f, contentW, 26.f);
            const bool hov = btnRect.contains(m_MouseScreenPos);
            sf::RectangleShape btn({contentW, 26.f});
            btn.setPosition(contentX, y + 2.f);
            btn.setFillColor(hov ? C_BG_ELEVATED : C_BG_INPUT);
            btn.setOutlineColor(hov ? C_BORDER_LIGHT : C_BORDER);
            btn.setOutlineThickness(1.f);
            window.draw(btn);
            sf::Text btnText;
            btnText.setFont(*m_Font);
            btnText.setCharacterSize(11);
            btnText.setFillColor(hov ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
            btnText.setString("Reset Camera (Center 0,0 / Zoom 1x)");
            btnText.setPosition(btnRect.left + (btnRect.width - btnText.getLocalBounds().width) / 2.f, y + 7.f);
            window.draw(btnText);
            m_SettingsButtons.push_back({btnRect, "reset_camera"});
            y += 34.f;
        }
    } else if (m_SettingsTab == 3)
    {
        y = DrawSettingsSectionHeader(window, "CAMERA NAVIGATION", accent, contentX, y, contentW);

        const std::vector<std::string> panOptions = {"Middle Mouse", "Right Mouse"};
        int panIdx = m_PanOnMiddleButton ? 0 : 1;
        y = DrawSettingsDropdown(window, "Pan Button", panOptions, panIdx, "pan_button", contentX, y, contentW);
        m_PanOnMiddleButton = (panIdx == 0);

        y = DrawSettingsToggle(window, "Invert Pan", m_InvertPan, "toggle_invert_pan", contentX, y, contentW);

        y += 12.f;
        y = DrawSettingsSectionHeader(window, "SCROLL", sf::Color(80, 200, 120), contentX, y, contentW);
        y = DrawSettingsSlider(window, "Scroll Sensitivity (Hierarchy)", m_ScrollSensitivity,
                               1.f, 100.f, "scroll_sens", contentX, y, contentW);
        y = DrawSettingsSlider(window, "Zoom Sensitivity (Mouse Wheel)", m_ZoomSensitivity,
                               0.01f, 0.5f, "zoom_sens2", contentX, y, contentW);

        y += 12.f;
        y = DrawSettingsSectionHeader(window, "KEYBOARD SHORTCUTS", sf::Color(200, 110, 230), contentX, y, contentW);

        struct Shortcut
        {
            std::string key, action;
        };
        const std::vector<Shortcut> shortcuts = {
            {"Ctrl+S", "Save scene"},
            {"Ctrl+L", "Load scene"},
            {"Ctrl+D", "Duplicate"},
            {"Ctrl+,", "Open settings"},
            {"G", "Toggle grid"},
            {"F5", "Run scene"},
            {"Esc", "Deselect / Close settings"},
            {"Del", "Delete object"},
            {"Scroll", "Zoom"},
            {"Mid/Right Drag", "Pan camera"},
        };
        for (const auto &sc: shortcuts)
        {
            sf::RectangleShape row({contentW, 20.f});
            row.setPosition(contentX, y);
            row.setFillColor(sf::Color::Transparent);
            window.draw(row);

            sf::Text keyT;
            keyT.setFont(*m_Font);
            keyT.setCharacterSize(11);
            keyT.setFillColor(C_ACCENT_BRIGHT);
            keyT.setString(sc.key);
            keyT.setPosition(contentX + 4.f, y + 2.f);
            window.draw(keyT);

            sf::Text actT;
            actT.setFont(*m_Font);
            actT.setCharacterSize(11);
            actT.setFillColor(C_TEXT_SECONDARY);
            actT.setString(sc.action);
            actT.setPosition(contentX + contentW * 0.36f, y + 2.f);
            window.draw(actT);

            y += 20.f;
            if (y > winY + winH - 20.f) break;
        }
    } else if (m_SettingsTab == 4)
    {
        y = DrawSettingsSectionHeader(window, "VIEWPORT OVERLAYS", accent, contentX, y, contentW);
        y = DrawSettingsToggle(window, "Show Entity IDs", m_ShowEntityIDs, "toggle_entity_ids", contentX, y, contentW);
        y = DrawSettingsToggle(window, "Show Collider Outlines", m_ShowColliderOutlines, "toggle_colliders", contentX,
                               y, contentW);
        y = DrawSettingsToggle(window, "Show FPS", m_ShowFPS, "toggle_fps2", contentX, y, contentW);

        y += 12.f;
        y = DrawSettingsSectionHeader(window, "CONSOLE LOG", sf::Color(100, 180, 255), contentX, y, contentW);
        const std::vector<std::string> logLevels = {"Off", "Errors Only", "Info", "Verbose"};
        y = DrawSettingsDropdown(window, "Log Level", logLevels, m_LogLevel, "log_level", contentX, y, contentW);

        y += 12.f;
        y = DrawSettingsSectionHeader(window, "EDITOR INFO", sf::Color(200, 200, 100), contentX, y, contentW);

        struct InfoRow
        {
            std::string key, value;
        };
        const std::vector<InfoRow> infos = {
            {"Engine", "RayneEngine"},
            {"Build", "Debug"},
            {"C++ Standard", "C++20"},
            {"SFML", "2.6.x"},
            {"Lua", "5.4.6"},
            {"Objects", std::to_string(m_Objects.size())},
            {"Grid Size", std::to_string(static_cast<int>(m_GridSize)) + " px"},
            {"AutoSave", m_AutoSaveEnabled ? "Enabled" : "Disabled"},
        };
        for (const auto &info: infos)
        {
            sf::Text kText;
            kText.setFont(*m_Font);
            kText.setCharacterSize(11);
            kText.setFillColor(C_TEXT_MUTED);
            kText.setString(info.key);
            kText.setPosition(contentX + 4.f, y + 2.f);
            window.draw(kText);

            sf::Text vText;
            vText.setFont(*m_Font);
            vText.setCharacterSize(11);
            vText.setFillColor(C_TEXT_PRIMARY);
            vText.setString(info.value);
            vText.setPosition(contentX + contentW * 0.45f, y + 2.f);
            window.draw(vText);

            y += 20.f;
        }
    }
}


void EditorScene::HandleSettingsClick(sf::Vector2f pos)
{
    m_MouseScreenPos = pos;
    for (auto &btn: m_SettingsButtons)
    {
        if (!btn.bounds.contains(pos)) continue;

        if (btn.action == "close_settings")
        {
            m_ShowSettings = false;
            m_ActiveSettingsField = SettingsField::None;
            m_SettingsInputText.clear();
        } else if (btn.action.rfind("tab_", 0) == 0)
        {
            m_SettingsTab = std::stoi(btn.action.substr(4));
            m_ActiveSettingsField = SettingsField::None;
            m_SettingsInputText.clear();
        } else if (btn.action == "toggle_autosave") { m_AutoSaveEnabled = !m_AutoSaveEnabled; } else if (
            btn.action == "toggle_autopopup") { m_AutoSavePopupEnabled = !m_AutoSavePopupEnabled; } else if (
            btn.action == "toggle_title_countdown") { m_ShowAutoSaveInTitle = !m_ShowAutoSaveInTitle; } else if (
            btn.action == "toggle_fps") { m_ShowFPS = !m_ShowFPS; } else if (
            btn.action == "toggle_fps2") { m_ShowFPS = !m_ShowFPS; } else if (
            btn.action == "toggle_entity_ids") { m_ShowEntityIDs = !m_ShowEntityIDs; } else if (
            btn.action == "toggle_colliders") { m_ShowColliderOutlines = !m_ShowColliderOutlines; } else if (
            btn.action == "toggle_invert_pan") { m_InvertPan = !m_InvertPan; } else if (btn.action == "reset_camera")
        {
            m_camera.setCenter(0.f, 0.f);
            m_camera.setSize(
                (1.f - (InspectorWidth + HierarchyWidth) / m_Window.getSize().x) * m_Window.getSize().x,
                (1.f - BrowserHeight / m_Window.getSize().y - TopBarHeight / m_Window.getSize().y) * m_Window.getSize().
                y
            );
        } else if (btn.action == "reset_editor_defaults")
        {
            m_GridSize = 40.f;
            m_DefaultObjectSize = 40.f;
            m_GridColor = sf::Color(38, 38, 52);
            m_GridOpacity = 255;
            m_SelectionOutlineColor = sf::Color(255, 220, 60);
            m_SelectionOutlineThickness = 2.f;
        } else if (btn.action == "pick_grid_color")
        {
#ifdef _WIN32
            HWND hwnd = reinterpret_cast<HWND>(m_Window.getSystemHandle());
            OpenColorPickerDialog(m_GridColor, hwnd);
#endif
        } else if (btn.action.rfind("fps_cap", 0) == 0) {} else if (btn.action.rfind("pan_button", 0) == 0) {} else if (
            btn.action.rfind("log_level", 0) == 0) {} else if (btn.action.rfind("input_", 0) == 0)
        {
            std::string fieldStr = btn.action.substr(6);
            if (fieldStr == "AutoSaveInterval")
            {
                m_ActiveSettingsField = SettingsField::AutoSaveInterval;
                m_SettingsInputText = std::to_string(static_cast<int>(m_AutoSaveIntervalSeconds));
            } else if (fieldStr == "AutoSavePopupDuration")
            {
                m_ActiveSettingsField = SettingsField::AutoSavePopupDuration;
                m_SettingsInputText = std::to_string(static_cast<int>(m_AutoSavePopupDuration));
            } else if (fieldStr == "SceneSavePath")
            {
                m_ActiveSettingsField = SettingsField::SceneSavePath;
                m_SettingsInputText = m_SceneSavePath;
            } else if (fieldStr == "GridSize")
            {
                m_ActiveSettingsField = SettingsField::GridSize;
                m_SettingsInputText = std::to_string(static_cast<int>(m_GridSize));
            } else if (fieldStr == "DefaultObjectSize")
            {
                m_ActiveSettingsField = SettingsField::DefaultObjectSize;
                m_SettingsInputText = std::to_string(static_cast<int>(m_DefaultObjectSize));
            } else if (fieldStr == "SelectionThickness")
            {
                m_ActiveSettingsField = SettingsField::SelectionThickness;
                m_SettingsInputText = std::to_string(static_cast<int>(m_SelectionOutlineThickness));
            } else if (fieldStr == "GridOpacityVal")
            {
                m_ActiveSettingsField = SettingsField::GridOpacityVal;
                m_SettingsInputText = std::to_string(m_GridOpacity);
            } else if (fieldStr == "ZoomMin")
            {
                m_ActiveSettingsField = SettingsField::ZoomMin;
                char b[32];
                snprintf(b, 32, "%.2f", m_ZoomMin);
                m_SettingsInputText = b;
            } else if (fieldStr == "ZoomMax")
            {
                m_ActiveSettingsField = SettingsField::ZoomMax;
                char b[32];
                snprintf(b, 32, "%.2f", m_ZoomMax);
                m_SettingsInputText = b;
            } else if (fieldStr == "ScrollSensitivity")
            {
                m_ActiveSettingsField = SettingsField::ScrollSensitivity;
                m_SettingsInputText = std::to_string(static_cast<int>(m_ScrollSensitivity));
            }
        } else if (btn.action.rfind("slider_", 0) == 0)
        {
            std::string tag = btn.action.substr(7);
            const size_t sep = tag.rfind('_');
            if (sep != std::string::npos)
            {
                float norm = std::stof(tag.substr(sep + 1));
                std::string id = tag.substr(0, sep);
                if (id == "zoom_sens" || id == "zoom_sens2") m_ZoomSensitivity = 0.01f + norm * (0.5f - 0.01f);
                else if (id == "scroll_sens") m_ScrollSensitivity = 1.f + norm * (100.f - 1.f);
            }
        } else if (btn.action.rfind("dropdown_", 0) == 0)
        {
            std::string rest = btn.action.substr(9);
            const size_t sep = rest.rfind('_');
            if (sep != std::string::npos)
            {
                int idx = std::stoi(rest.substr(sep + 1));
                std::string id = rest.substr(0, sep);
                if (id == "fps_cap") m_FPSCapIndex = idx;
                else if (id == "pan_button") m_PanOnMiddleButton = (idx == 0);
                else if (id == "log_level") m_LogLevel = idx;
            }
        }
        SaveSettings();
        break;
    }
}


float EditorScene::DrawSettingsSectionHeader(sf::RenderWindow &window,
                                             const std::string &title, sf::Color accent, float x, float y, float winW)
{
    sf::RectangleShape bar({winW, 22.f});
    bar.setFillColor(sf::Color(accent.r / 10, accent.g / 10, accent.b / 10, 200));
    bar.setPosition(x, y);
    window.draw(bar);

    sf::RectangleShape accentLine({3.f, 22.f});
    accentLine.setFillColor(accent);
    accentLine.setPosition(x, y);
    window.draw(accentLine);

    sf::Text text;
    text.setFont(*m_Font);
    text.setCharacterSize(10);
    text.setFillColor(accent);
    text.setStyle(sf::Text::Bold);
    text.setString(title);
    text.setPosition(x + 10.f, y + 6.f);
    window.draw(text);

    return y + 28.f;
}


float EditorScene::DrawSettingsToggle(sf::RenderWindow &window,
                                      const std::string &label, bool &value, const std::string &action,
                                      float x, float y, float winW)
{
    const float rowH = 28.f;
    const bool hov = sf::FloatRect(x, y, winW, rowH).contains(m_MouseScreenPos);

    if (hov)
    {
        sf::RectangleShape rowBg({winW, rowH});
        rowBg.setPosition(x, y);
        rowBg.setFillColor(C_BG_ELEVATED);
        window.draw(rowBg);
    }

    sf::Text labelText;
    labelText.setFont(*m_Font);
    labelText.setCharacterSize(12);
    labelText.setFillColor(hov ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
    labelText.setString(label);
    labelText.setPosition(x + 8.f, y + 7.f);
    window.draw(labelText);

    const float pillW = 36.f, pillH = 18.f;
    const float pillX = x + winW - pillW - 8.f;
    const float pillY = y + (rowH - pillH) / 2.f;

    sf::Color pillColor = value ? C_ACCENT : C_BG_INPUT;
    sf::Color pillBorder = value ? sf::Color(60, 200, 100) : C_BORDER;
    sf::RectangleShape pill({pillW, pillH});
    pill.setPosition(pillX, pillY);
    pill.setFillColor(pillColor);
    pill.setOutlineColor(pillBorder);
    pill.setOutlineThickness(1.f);
    window.draw(pill);

    const float knobX = value ? pillX + pillW - pillH + 2.f : pillX + 2.f;
    sf::CircleShape knob(pillH / 2.f - 2.f);
    knob.setFillColor(sf::Color::White);
    knob.setPosition(knobX, pillY + 2.f);
    window.draw(knob);

    sf::RectangleShape line({winW, 1.f});
    line.setFillColor(sf::Color(C_BORDER.r, C_BORDER.g, C_BORDER.b, 60));
    line.setPosition(x, y + rowH - 1.f);
    window.draw(line);

    m_SettingsButtons.push_back({sf::FloatRect(x, y, winW, rowH), action});
    return y + rowH;
}


float EditorScene::DrawSettingsSlider(sf::RenderWindow &window,
                                      const std::string &label, float &value, float minVal, float maxVal,
                                      const std::string &action, float x, float y, float winW)
{
    const float rowH = 36.f;

    sf::Text labelText;
    labelText.setFont(*m_Font);
    labelText.setCharacterSize(12);
    labelText.setFillColor(C_TEXT_SECONDARY);
    labelText.setString(label);
    labelText.setPosition(x + 8.f, y + 4.f);
    window.draw(labelText);

    char buf[32];
    snprintf(buf, 32, "%.2f", value);
    sf::Text valText;
    valText.setFont(*m_Font);
    valText.setCharacterSize(11);
    valText.setFillColor(C_ACCENT_BRIGHT);
    valText.setString(buf);
    valText.setPosition(x + winW - 44.f, y + 4.f);
    window.draw(valText);

    const float trackX = x + 8.f;
    const float trackW = winW - 60.f;
    const float trackY = y + 22.f;
    const float trackH = 4.f;

    sf::RectangleShape track({trackW, trackH});
    track.setPosition(trackX, trackY);
    track.setFillColor(C_BG_INPUT);
    track.setOutlineColor(C_BORDER);
    track.setOutlineThickness(1.f);
    window.draw(track);

    float norm = (value - minVal) / (maxVal - minVal);
    norm = std::clamp(norm, 0.f, 1.f);

    sf::RectangleShape fill({norm * trackW, trackH});
    fill.setPosition(trackX, trackY);
    fill.setFillColor(C_ACCENT);
    window.draw(fill);

    const float knobX = trackX + norm * trackW - 6.f;
    sf::CircleShape knob(6.f);
    knob.setFillColor(C_ACCENT_BRIGHT);
    knob.setPosition(knobX, trackY - 4.f);
    window.draw(knob);

    const sf::FloatRect trackRect(trackX, trackY - 6.f, trackW, trackH + 12.f);
    if (trackRect.contains(m_MouseScreenPos) && sf::Mouse::isButtonPressed(sf::Mouse::Left))
    {
        float clickNorm = std::clamp((m_MouseScreenPos.x - trackX) / trackW, 0.f, 1.f);
        value = minVal + clickNorm * (maxVal - minVal);
    }

    sf::RectangleShape line({winW, 1.f});
    line.setFillColor(sf::Color(C_BORDER.r, C_BORDER.g, C_BORDER.b, 60));
    line.setPosition(x, y + rowH - 1.f);
    window.draw(line);

    return y + rowH;
}


float EditorScene::DrawSettingsInputField(sf::RenderWindow &window,
                                          const std::string &label, const std::string &currentVal,
                                          SettingsField field, float x, float y, float winW)
{
    const float rowH = 30.f;
    const bool active = (m_ActiveSettingsField == field);

    sf::Text labelText;
    labelText.setFont(*m_Font);
    labelText.setCharacterSize(12);
    labelText.setFillColor(C_TEXT_MUTED);
    labelText.setString(label);
    labelText.setPosition(x + 8.f, y + 7.f);
    window.draw(labelText);

    const float fieldW = winW * 0.38f;
    const float fieldX = x + winW - fieldW - 8.f;
    const sf::FloatRect fieldRect(fieldX, y + 4.f, fieldW, 22.f);
    const bool hov = fieldRect.contains(m_MouseScreenPos);

    sf::RectangleShape fieldBg({fieldW, 22.f});
    fieldBg.setPosition(fieldX, y + 4.f);
    fieldBg.setFillColor(active ? sf::Color(30, 30, 50) : hov ? C_BG_ELEVATED : C_BG_INPUT);
    fieldBg.setOutlineColor(active ? C_ACCENT : hov ? C_BORDER_LIGHT : C_BORDER);
    fieldBg.setOutlineThickness(1.f);
    window.draw(fieldBg);

    const bool blink = ((int) (m_FPSClock.getElapsedTime().asSeconds() * 2) % 2 == 0);
    std::string display = active ? (m_SettingsInputText + (blink ? "|" : "")) : currentVal;
    sf::Text valText;
    valText.setFont(*m_Font);
    valText.setCharacterSize(11);
    valText.setFillColor(active ? C_TEXT_PRIMARY : hov ? C_TEXT_PRIMARY : sf::Color(200, 200, 220));
    valText.setString(display);
    valText.setPosition(fieldX + 6.f, y + 7.f);
    window.draw(valText);

    static const std::map<SettingsField, std::string> fieldNames = {
        {SettingsField::AutoSaveInterval, "AutoSaveInterval"},
        {SettingsField::AutoSavePopupDuration, "AutoSavePopupDuration"},
        {SettingsField::SceneSavePath, "SceneSavePath"},
        {SettingsField::GridSize, "GridSize"},
        {SettingsField::DefaultObjectSize, "DefaultObjectSize"},
        {SettingsField::SelectionThickness, "SelectionThickness"},
        {SettingsField::GridOpacityVal, "GridOpacityVal"},
        {SettingsField::ZoomSensitivity, "ZoomSensitivity"},
        {SettingsField::ZoomMin, "ZoomMin"},
        {SettingsField::ZoomMax, "ZoomMax"},
        {SettingsField::ScrollSensitivity, "ScrollSensitivity"},
    };
    std::string actionStr = "input_";
    auto it = fieldNames.find(field);
    if (it != fieldNames.end()) actionStr += it->second;
    m_SettingsButtons.push_back({fieldRect, actionStr});

    sf::RectangleShape line({winW, 1.f});
    line.setFillColor(sf::Color(C_BORDER.r, C_BORDER.g, C_BORDER.b, 60));
    line.setPosition(x, y + rowH - 1.f);
    window.draw(line);

    return y + rowH;
}


float EditorScene::DrawSettingsDropdown(sf::RenderWindow &window,
                                        const std::string &label, const std::vector<std::string> &options,
                                        int &currentIdx, const std::string &action,
                                        float x, float y, float winW)
{
    const float rowH = 28.f;

    sf::Text labelText;
    labelText.setFont(*m_Font);
    labelText.setCharacterSize(12);
    labelText.setFillColor(C_TEXT_SECONDARY);
    labelText.setString(label);
    labelText.setPosition(x + 8.f, y + 6.f);
    window.draw(labelText);

    const float optW = winW * 0.44f;
    const float optH = 22.f;
    const float optItemH = 22.f;

    float optX = x + winW - optW - 8.f;

    for (int i = 0; i < (int) options.size(); i++)
    {
        const sf::FloatRect optRect(optX + i * (optW / options.size()), y + 3.f,
                                    optW / options.size() - 2.f, optH);
        const bool active = (currentIdx == i);
        const bool hov = optRect.contains(m_MouseScreenPos);

        sf::RectangleShape opt({optRect.width, optRect.height});
        opt.setPosition(optRect.left, optRect.top);
        opt.setFillColor(active ? C_ACCENT_DIM : hov ? C_BG_ELEVATED : C_BG_INPUT);
        opt.setOutlineColor(active ? C_ACCENT : hov ? C_BORDER_LIGHT : C_BORDER);
        opt.setOutlineThickness(1.f);
        window.draw(opt);

        sf::Text optText;
        optText.setFont(*m_Font);
        optText.setCharacterSize(10);
        optText.setFillColor(active ? C_ACCENT_BRIGHT : hov ? C_TEXT_PRIMARY : C_TEXT_MUTED);
        optText.setString(options[i]);
        optText.setPosition(
            optRect.left + (optRect.width - optText.getLocalBounds().width) / 2.f,
            optRect.top + 5.f);
        window.draw(optText);

        m_SettingsButtons.push_back({optRect, "dropdown_" + action + "_" + std::to_string(i)});
    }

    sf::RectangleShape line({winW, 1.f});
    line.setFillColor(sf::Color(C_BORDER.r, C_BORDER.g, C_BORDER.b, 60));
    line.setPosition(x, y + rowH - 1.f);
    window.draw(line);

    return y + rowH;
}


void EditorScene::SaveSettings()
{
    json data;
    data["general"]["autoSaveEnabled"] = m_AutoSaveEnabled;
    data["general"]["autoSaveIntervalSeconds"] = m_AutoSaveIntervalSeconds;
    data["general"]["autoSavePopupEnabled"] = m_AutoSavePopupEnabled;
    data["general"]["autoSavePopupDuration"] = m_AutoSavePopupDuration;

    data["editor"]["gridColor"] = {m_GridColor.r, m_GridColor.g, m_GridColor.b};
    data["editor"]["gridOpacity"] = m_GridOpacity;
    data["editor"]["editorBgColor"] = {m_EditorBgColor.r, m_EditorBgColor.g, m_EditorBgColor.b};
    data["editor"]["defaultObjectSize"] = m_DefaultObjectSize;
    data["editor"]["selectionOutlineColor"] = {
        m_SelectionOutlineColor.r, m_SelectionOutlineColor.g, m_SelectionOutlineColor.b
    };
    data["editor"]["selectionOutlineThickness"] = m_SelectionOutlineThickness;
    data["editor"]["preferredIDE"] = m_PreferredIDE;

    data["rendering"]["showFPS"] = m_ShowFPS;
    data["rendering"]["fpsCapIndex"] = m_FPSCapIndex;
    data["rendering"]["zoomSensitivity"] = m_ZoomSensitivity;
    data["rendering"]["zoomMin"] = m_ZoomMin;
    data["rendering"]["zoomMax"] = m_ZoomMax;

    data["input"]["panOnMiddleButton"] = m_PanOnMiddleButton;
    data["input"]["invertPan"] = m_InvertPan;
    data["input"]["scrollSensitivity"] = m_ScrollSensitivity;

    data["debug"]["showEntityIDs"] = m_ShowEntityIDs;
    data["debug"]["showColliderOutlines"] = m_ShowColliderOutlines;
    data["debug"]["logLevel"] = m_LogLevel;
    data["debug"]["showAutoSaveInTitle"] = m_ShowAutoSaveInTitle;

    std::string editorDir = ENGINE_ASSET_PATH
    "/editor";
    if (!std::filesystem::exists(editorDir)) { std::filesystem::create_directories(editorDir); }

    std::ofstream file(editorDir + "/editor_settings.json");
    if (file.is_open())
        file << data.dump(4);
}


void EditorScene::LoadSettings()
{
    std::string editorDir = ENGINE_ASSET_PATH
    "/editor";
    std::ifstream file(editorDir + "/editor_settings.json");
    if (!file.is_open()) return;

    json data;
    try
    {
        file >> data;

        if (data.contains("general"))
        {
            m_AutoSaveEnabled = data["general"].value("autoSaveEnabled", m_AutoSaveEnabled);
            m_AutoSaveIntervalSeconds = data["general"].value("autoSaveIntervalSeconds", m_AutoSaveIntervalSeconds);
            m_AutoSavePopupEnabled = data["general"].value("autoSavePopupEnabled", m_AutoSavePopupEnabled);
            m_AutoSavePopupDuration = data["general"].value("autoSavePopupDuration", m_AutoSavePopupDuration);
        }

        if (data.contains("editor"))
        {
            if (data["editor"].contains("gridColor"))
            {
                m_GridColor.r = data["editor"]["gridColor"][0];
                m_GridColor.g = data["editor"]["gridColor"][1];
                m_GridColor.b = data["editor"]["gridColor"][2];
            }
            m_GridOpacity = data["editor"].value("gridOpacity", m_GridOpacity);
            if (data["editor"].contains("editorBgColor"))
            {
                m_EditorBgColor.r = data["editor"]["editorBgColor"][0];
                m_EditorBgColor.g = data["editor"]["editorBgColor"][1];
                m_EditorBgColor.b = data["editor"]["editorBgColor"][2];
            }
            m_DefaultObjectSize = data["editor"].value("defaultObjectSize", m_DefaultObjectSize);
            if (data["editor"].contains("selectionOutlineColor"))
            {
                m_SelectionOutlineColor.r = data["editor"]["selectionOutlineColor"][0];
                m_SelectionOutlineColor.g = data["editor"]["selectionOutlineColor"][1];
                m_SelectionOutlineColor.b = data["editor"]["selectionOutlineColor"][2];
            }
            m_SelectionOutlineThickness = data["editor"].
                    value("selectionOutlineThickness", m_SelectionOutlineThickness);
            m_PreferredIDE = data["editor"].value("preferredIDE", m_PreferredIDE);
        }

        if (data.contains("rendering"))
        {
            m_ShowFPS = data["rendering"].value("showFPS", m_ShowFPS);
            m_FPSCapIndex = data["rendering"].value("fpsCapIndex", m_FPSCapIndex);
            m_ZoomSensitivity = data["rendering"].value("zoomSensitivity", m_ZoomSensitivity);
            m_ZoomMin = data["rendering"].value("zoomMin", m_ZoomMin);
            m_ZoomMax = data["rendering"].value("zoomMax", m_ZoomMax);
        }

        if (data.contains("input"))
        {
            m_PanOnMiddleButton = data["input"].value("panOnMiddleButton", m_PanOnMiddleButton);
            m_InvertPan = data["input"].value("invertPan", m_InvertPan);
            m_ScrollSensitivity = data["input"].value("scrollSensitivity", m_ScrollSensitivity);
        }

        if (data.contains("debug"))
        {
            m_ShowEntityIDs = data["debug"].value("showEntityIDs", m_ShowEntityIDs);
            m_ShowColliderOutlines = data["debug"].value("showColliderOutlines", m_ShowColliderOutlines);
            m_LogLevel = data["debug"].value("logLevel", m_LogLevel);
            m_ShowAutoSaveInTitle = data["debug"].value("showAutoSaveInTitle", m_ShowAutoSaveInTitle);
        }
    } catch (...) {}
}


void EditorScene::LoadProjectSettings()
{
    std::string path = std::string(ENGINE_ASSET_PATH) + "/project_settings.json";
    if (std::filesystem::exists(path))
    {
        try
        {
            std::ifstream f(path);
            json j;
            f >> j;
            m_ProjectName = j.value("ProjectName", "RayneGame");
            m_ProjectVersion = j.value("Version", "1.0.0");
            m_ProjectAuthor = j.value("Author", "");
            m_ProjectStartScene = j.value("StartScene", "scenes/game.json");
            m_ProjectWindowWidth = j.value("WindowWidth", 1920);
            m_ProjectWindowHeight = j.value("WindowHeight", 1080);
            m_ProjectVSync = j.value("VSync", true);
            m_ProjectTargetFPS = j.value("TargetFPS", 60);
            m_ProjectFullscreen = j.value("Fullscreen", false);
            if (j.contains("ClearColor"))
            {
                m_ProjectClearColor = HexToColor(j["ClearColor"].get<std::string>(), sf::Color(18, 20, 23));
            }
            m_ProjectMasterVolume = j.value("MasterVolume", 100.f);
            m_ProjectMusicVolume = j.value("MusicVolume", 100.f);
            PhysicsSystem::LoadCollisionSettings(j);
        } catch (...) { std::cout << "[ERROR] Failed to load project settings\n"; }
    }

    if (g_App)
    {
        g_App->SetProjectName(m_ProjectName);
        g_App->SetProjectVersion(m_ProjectVersion);
        g_App->SetProjectAuthor(m_ProjectAuthor);
        g_App->SetTargetResolution(m_ProjectWindowWidth, m_ProjectWindowHeight);
        g_App->SetVSync(m_ProjectVSync);
        g_App->SetTargetFPS(m_ProjectTargetFPS);
        g_App->SetClearColor(m_ProjectClearColor);
        g_App->SetMasterVolume(m_ProjectMasterVolume);
        g_App->SetMusicVolume(m_ProjectMusicVolume);
    }
}


void EditorScene::SaveProjectSettings()
{
    std::string path = std::string(ENGINE_ASSET_PATH) + "/project_settings.json";
    try
    {
        json j;
        j["ProjectName"] = m_ProjectName;
        j["Version"] = m_ProjectVersion;
        j["Author"] = m_ProjectAuthor;
        j["StartScene"] = m_ProjectStartScene;
        j["WindowWidth"] = m_ProjectWindowWidth;
        j["WindowHeight"] = m_ProjectWindowHeight;
        j["VSync"] = m_ProjectVSync;
        j["TargetFPS"] = m_ProjectTargetFPS;
        j["Fullscreen"] = m_ProjectFullscreen;
        j["ClearColor"] = ColorToHex(m_ProjectClearColor);
        j["MasterVolume"] = m_ProjectMasterVolume;
        j["MusicVolume"] = m_ProjectMusicVolume;
        PhysicsSystem::SaveCollisionSettings(j);

        std::ofstream f(path);
        f << j.dump(4);
    } catch (...) { std::cout << "[ERROR] Failed to save project settings\n"; }

    if (g_App)
    {
        g_App->SetProjectName(m_ProjectName);
        g_App->SetProjectVersion(m_ProjectVersion);
        g_App->SetProjectAuthor(m_ProjectAuthor);
        g_App->SetTargetResolution(m_ProjectWindowWidth, m_ProjectWindowHeight);
        g_App->SetVSync(m_ProjectVSync);
        g_App->SetTargetFPS(m_ProjectTargetFPS);
        g_App->SetClearColor(m_ProjectClearColor);
        g_App->SetMasterVolume(m_ProjectMasterVolume);
        g_App->SetMusicVolume(m_ProjectMusicVolume);
    }
}


void EditorScene::CommitActiveProjectSettingsField()
{
    if (m_ActiveProjectSettingsField == ProjectSettingsField::None) return;

    if (m_ActiveProjectSettingsField ==
        ProjectSettingsField::ProjectName)
    {
        if (!m_ProjectSettingsInputText.empty()) { m_ProjectName = m_ProjectSettingsInputText; }
    } else if (m_ActiveProjectSettingsField ==
               ProjectSettingsField::Version)
    {
        if (!m_ProjectSettingsInputText.empty()) { m_ProjectVersion = m_ProjectSettingsInputText; }
    } else if (m_ActiveProjectSettingsField ==
               ProjectSettingsField::Author) { m_ProjectAuthor = m_ProjectSettingsInputText; } else if (
        m_ActiveProjectSettingsField == ProjectSettingsField::StartScene)
    {
        if (!m_ProjectSettingsInputText.empty()) { m_ProjectStartScene = m_ProjectSettingsInputText; }
    } else if (m_ActiveProjectSettingsField == ProjectSettingsField::WindowWidth)
    {
        try
        {
            int v = std::stoi(m_ProjectSettingsInputText);
            if (v > 100) m_ProjectWindowWidth = v;
        } catch (...) {}
    } else if (m_ActiveProjectSettingsField == ProjectSettingsField::WindowHeight)
    {
        try
        {
            int v = std::stoi(m_ProjectSettingsInputText);
            if (v > 100) m_ProjectWindowHeight = v;
        } catch (...) {}
    } else if (m_ActiveProjectSettingsField == ProjectSettingsField::TargetFPS)
    {
        try
        {
            int v = std::stoi(m_ProjectSettingsInputText);
            if (v >= 0) m_ProjectTargetFPS = v;
        } catch (...) {}
    } else if (m_ActiveProjectSettingsField == ProjectSettingsField::ClearColorHex)
    {
        m_ProjectClearColor = HexToColor(m_ProjectSettingsInputText, m_ProjectClearColor);
    } else if (m_ActiveProjectSettingsField == ProjectSettingsField::MasterVolume)
    {
        try
        {
            float v = std::stof(m_ProjectSettingsInputText);
            m_ProjectMasterVolume = std::clamp(v, 0.f, 100.f);
        } catch (...) {}
    } else if (m_ActiveProjectSettingsField == ProjectSettingsField::MusicVolume)
    {
        try
        {
            float v = std::stof(m_ProjectSettingsInputText);
            m_ProjectMusicVolume = std::clamp(v, 0.f, 100.f);
        } catch (...) {}
    } else if (m_ActiveProjectSettingsField >= ProjectSettingsField::ChannelName0 &&
               m_ActiveProjectSettingsField <= ProjectSettingsField::ChannelName7)
    {
        int chIdx = (int) m_ActiveProjectSettingsField - (int) ProjectSettingsField::ChannelName0;
        if (!m_ProjectSettingsInputText.empty())
        {
            PhysicsSystem::SetChannelName(chIdx, m_ProjectSettingsInputText);
        }
    }

    m_ActiveProjectSettingsField = ProjectSettingsField::None;
    m_ProjectSettingsInputText.clear();
    SaveProjectSettings();
}


float EditorScene::DrawProjectSettingsToggle(sf::RenderWindow &window, const std::string &label,
                                             bool value, const std::string &action,
                                             float x, float y, float winW)
{
    sf::Text tLabel(label, *m_Font, 11);
    tLabel.setPosition(x, y + 4.f);
    tLabel.setFillColor(C_TEXT_SECONDARY);
    window.draw(tLabel);

    float inX = x + 160.f;
    sf::RectangleShape togBtn(sf::Vector2f(44.f, 22.f));
    togBtn.setPosition(inX, y);
    togBtn.setFillColor(value ? C_ACCENT : C_BG_INPUT);
    togBtn.setOutlineColor(C_BORDER);
    togBtn.setOutlineThickness(1.f);
    window.draw(togBtn);

    sf::CircleShape knob(8.f);
    knob.setFillColor(C_TEXT_PRIMARY);
    knob.setPosition(value ? (inX + 24.f) : (inX + 4.f), y + 3.f);
    window.draw(knob);

    sf::Text statusText(value ? "ON" : "OFF", *m_Font, 10);
    statusText.setPosition(inX + 52.f, y + 4.f);
    statusText.setFillColor(value ? C_SUCCESS : C_TEXT_MUTED);
    window.draw(statusText);

    m_ProjectSettingsButtons.push_back({togBtn.getGlobalBounds(), action});
    return 32.f;
}


float EditorScene::DrawProjectSettingsInputField(sf::RenderWindow &window, const std::string &label,
                                                 const std::string &currentVal, ProjectSettingsField field,
                                                 float x, float y, float winW)
{
    sf::Text tLabel(label, *m_Font, 11);
    tLabel.setPosition(x, y + 4.f);
    tLabel.setFillColor(C_TEXT_SECONDARY);
    window.draw(tLabel);

    float inX = x + 160.f;
    float inW = winW - inX - 20.f;

    sf::RectangleShape box(sf::Vector2f(inW, 22.f));
    box.setPosition(inX, y);
    bool active = (m_ActiveProjectSettingsField == field);
    bool hovered = box.getGlobalBounds().contains(m_MouseScreenPos);
    box.setFillColor(active ? C_BG_INPUT : (hovered ? sf::Color(28, 32, 40) : C_BG_INPUT));
    box.setOutlineColor(active ? C_ACCENT : (hovered ? C_BORDER_LIGHT : C_BORDER));
    box.setOutlineThickness(1.f);
    window.draw(box);

    if (active)
    {
        m_ProjectSettingsInputBounds = box.getGlobalBounds();
        if (HasTextSelection())
        {
            sf::Text t(m_ProjectSettingsInputText, *m_Font, 11);
            int sMin = std::clamp(GetSelectionMin(), 0, (int) m_ProjectSettingsInputText.size());
            int sMax = std::clamp(GetSelectionMax(), 0, (int) m_ProjectSettingsInputText.size());
            float x1 = t.findCharacterPos(sMin).x;
            float x2 = t.findCharacterPos(sMax).x;
            sf::RectangleShape selBox({x2 - x1, 14.f});
            selBox.setPosition(inX + 6.f + x1, y + 4.f);
            selBox.setFillColor(sf::Color(60, 120, 240, 140));
            window.draw(selBox);
        }
    }

    std::string display = active ? m_ProjectSettingsInputText : currentVal;
    if (active && (int) (m_FPSClock.getElapsedTime().asSeconds() * 2) % 2 == 0) display += "|";

    sf::Text tVal(display, *m_Font, 11);
    tVal.setPosition(inX + 6.f, y + 4.f);
    tVal.setFillColor(C_TEXT_PRIMARY);
    window.draw(tVal);

    if (field == ProjectSettingsField::ClearColorHex)
    {
        sf::Color previewCol = HexToColor(active ? m_ProjectSettingsInputText : currentVal, m_ProjectClearColor);
        sf::RectangleShape colorPreview(sf::Vector2f(16.f, 16.f));
        colorPreview.setPosition(inX + inW - 20.f, y + 3.f);
        colorPreview.setFillColor(previewCol);
        colorPreview.setOutlineThickness(1.f);
        colorPreview.setOutlineColor(C_BORDER_LIGHT);
        window.draw(colorPreview);
    }

    m_ProjectSettingsButtons.push_back({{inX, y, inW, 22.f}, "edit_proj_" + std::to_string((int) field)});
    return 32.f;
}


void EditorScene::DrawProjectSettingsWindow(sf::RenderWindow &window)
{
    if (!m_ShowProjectSettings) return;

    const float w = 700.f;
    const float h = 520.f;
    const float x = (window.getSize().x - w) / 2.f;
    const float y = (window.getSize().y - h) / 2.f;

    sf::RectangleShape backdrop(sf::Vector2f(window.getSize().x, window.getSize().y));
    backdrop.setFillColor(sf::Color(0, 0, 0, 140));
    window.draw(backdrop);

    sf::RectangleShape panel(sf::Vector2f(w, h));
    panel.setPosition(x, y);
    panel.setFillColor(C_BG_PANEL);
    panel.setOutlineColor(C_BORDER_LIGHT);
    panel.setOutlineThickness(1.f);
    window.draw(panel);

    m_ProjectSettingsButtons.clear();

    sf::Text title("Project Settings", *m_Font, 16);
    title.setPosition(x + 20.f, y + 16.f);
    title.setFillColor(C_TEXT_PRIMARY);
    window.draw(title);

    sf::RectangleShape closeBtn(sf::Vector2f(22.f, 22.f));
    closeBtn.setPosition(x + w - 32.f, y + 16.f);
    closeBtn.setFillColor(C_BG_INPUT);
    closeBtn.setOutlineColor(C_BORDER);
    closeBtn.setOutlineThickness(1.f);
    window.draw(closeBtn);

    sf::Text closeTxt("X", *m_Font, 11);
    closeTxt.setPosition(x + w - 26.f, y + 19.f);
    closeTxt.setFillColor(C_TEXT_MUTED);
    window.draw(closeTxt);
    m_ProjectSettingsButtons.push_back({closeBtn.getGlobalBounds(), "close_proj_settings"});

    float tabY = y + 50.f;
    std::vector<std::string> tabs = {"General", "Display & Graphics", "Audio", "Collision Matrix"};
    float tabX = x + 20.f;
    for (int i = 0; i < (int) tabs.size(); ++i)
    {
        sf::Text tabText(tabs[i], *m_Font, 11);
        float tw = tabText.getLocalBounds().width + 24.f;
        sf::RectangleShape tabBox(sf::Vector2f(tw, 26.f));
        tabBox.setPosition(tabX, tabY);
        bool isCurrentTab = (m_ProjectSettingsTab == i);
        tabBox.setFillColor(isCurrentTab ? C_ACCENT : C_BG_ELEVATED);
        tabBox.setOutlineColor(isCurrentTab ? C_ACCENT_BRIGHT : C_BORDER);
        tabBox.setOutlineThickness(1.f);
        window.draw(tabBox);

        tabText.setPosition(tabX + 12.f, tabY + 6.f);
        tabText.setFillColor(isCurrentTab ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
        window.draw(tabText);

        m_ProjectSettingsButtons.push_back({tabBox.getGlobalBounds(), "tab_proj_" + std::to_string(i)});
        tabX += tw + 6.f;
    }

    sf::RectangleShape divLine(sf::Vector2f(w - 40.f, 1.f));
    divLine.setPosition(x + 20.f, tabY + 34.f);
    divLine.setFillColor(C_BORDER);
    window.draw(divLine);

    float currY = tabY + 44.f;

    if (m_ProjectSettingsTab == 0)
    {
        currY += DrawProjectSettingsInputField(window, "Project Name", m_ProjectName, ProjectSettingsField::ProjectName,
                                               x + 20.f, currY, x + w);
        currY += DrawProjectSettingsInputField(window, "Version", m_ProjectVersion, ProjectSettingsField::Version,
                                               x + 20.f, currY, x + w);
        currY += DrawProjectSettingsInputField(window, "Author / Studio", m_ProjectAuthor, ProjectSettingsField::Author,
                                               x + 20.f, currY, x + w);
        currY += DrawProjectSettingsInputField(window, "Start Scene", m_ProjectStartScene,
                                               ProjectSettingsField::StartScene, x + 20.f, currY, x + w);
    } else if (m_ProjectSettingsTab == 1)
    {
        currY += DrawProjectSettingsInputField(window, "Window Width", std::to_string(m_ProjectWindowWidth),
                                               ProjectSettingsField::WindowWidth, x + 20.f, currY, x + w);
        currY += DrawProjectSettingsInputField(window, "Window Height", std::to_string(m_ProjectWindowHeight),
                                               ProjectSettingsField::WindowHeight, x + 20.f, currY, x + w);
        currY += DrawProjectSettingsToggle(window, "Fullscreen (Standalone)", m_ProjectFullscreen, "toggle_fullscreen",
                                           x + 20.f, currY, x + w);
        currY += DrawProjectSettingsInputField(window, "Clear Color (Hex)", ColorToHex(m_ProjectClearColor),
                                               ProjectSettingsField::ClearColorHex, x + 20.f, currY, x + w);
    } else if (m_ProjectSettingsTab == 2)
    {
        currY += DrawProjectSettingsInputField(window, "Master Volume (0-100)",
                                               std::to_string((int) m_ProjectMasterVolume),
                                               ProjectSettingsField::MasterVolume, x + 20.f, currY, x + w);
        currY += DrawProjectSettingsInputField(window, "Music Volume (0-100)",
                                               std::to_string((int) m_ProjectMusicVolume),
                                               ProjectSettingsField::MusicVolume, x + 20.f, currY, x + w);
    } else if (m_ProjectSettingsTab == 3)
    {
        sf::Text sub("Click cells to toggle layer collisions. Matrix is symmetric. Rename layers on the right:", *m_Font, 11);
        sub.setPosition(x + 20.f, currY);
        sub.setFillColor(C_TEXT_MUTED);
        window.draw(sub);
        currY += 22.f;

        const float matX = x + 130.f;
        const float matY = currY + 22.f;
        const float cellW = 28.f;
        const float cellH = 26.f;
        const float boxSz = 20.f;

        // Column headers (0..7)
        for (int c = 0; c < PhysicsSystem::MAX_CHANNELS; ++c)
        {
            sf::Text chNum(std::to_string(c), *m_Font, 10);
            chNum.setFillColor(C_TEXT_MUTED);
            chNum.setPosition(matX + c * cellW + 6.f, matY - 18.f);
            window.draw(chNum);
        }

        // Rows and cells
        for (int r = 0; r < PhysicsSystem::MAX_CHANNELS; ++r)
        {
            std::string rLabel = std::to_string(r) + ": " + PhysicsSystem::GetChannelName(r);
            if (rLabel.length() > 14) rLabel = rLabel.substr(0, 13) + ".";
            sf::Text rowTxt(rLabel, *m_Font, 11);
            rowTxt.setPosition(x + 20.f, matY + r * cellH + 3.f);
            rowTxt.setFillColor(C_TEXT_SECONDARY);
            window.draw(rowTxt);

            for (int c = 0; c < PhysicsSystem::MAX_CHANNELS; ++c)
            {
                float cx = matX + c * cellW;
                float cy = matY + r * cellH;
                sf::FloatRect cellBounds(cx, cy, boxSz, boxSz);
                bool canCol = PhysicsSystem::CanCollide(r, c);
                bool hov = cellBounds.contains(m_MouseScreenPos);

                sf::RectangleShape cellBox(sf::Vector2f(boxSz, boxSz));
                cellBox.setPosition(cx, cy);
                cellBox.setFillColor(canCol ? (r == c ? C_ACCENT : C_ACCENT_DIM) : C_BG_INPUT);
                cellBox.setOutlineColor(hov ? C_ACCENT_BRIGHT : (canCol ? C_ACCENT : C_BORDER));
                cellBox.setOutlineThickness(1.f);
                window.draw(cellBox);

                if (canCol)
                {
                    sf::RectangleShape mark(sf::Vector2f(8.f, 8.f));
                    mark.setPosition(cx + 6.f, cy + 6.f);
                    mark.setFillColor(sf::Color::White);
                    window.draw(mark);
                }

                m_ProjectSettingsButtons.push_back({cellBounds, "col_toggle_" + std::to_string(r) + "_" + std::to_string(c)});
            }
        }

        // Quick buttons below matrix
        float qbY = matY + PhysicsSystem::MAX_CHANNELS * cellH + 12.f;
        auto drawQuickBtn = [&](const std::string &lbl, const std::string &action, float bx) -> float {
            sf::Text bt(lbl, *m_Font, 11);
            float bw = bt.getLocalBounds().width + 16.f;
            sf::RectangleShape bbox(sf::Vector2f(bw, 22.f));
            bbox.setPosition(bx, qbY);
            bbox.setFillColor(C_BG_ELEVATED);
            bbox.setOutlineColor(C_BORDER);
            bbox.setOutlineThickness(1.f);
            window.draw(bbox);
            bt.setPosition(bx + 8.f, qbY + 3.f);
            bt.setFillColor(C_TEXT_SECONDARY);
            window.draw(bt);
            m_ProjectSettingsButtons.push_back({{bx, qbY, bw, 22.f}, action});
            return bw + 6.f;
        };

        float qbX = x + 20.f;
        qbX += drawQuickBtn("All On", "col_all_on", qbX);
        qbX += drawQuickBtn("All Off", "col_all_off", qbX);
        drawQuickBtn("Reset Defaults", "col_reset", qbX);

        // Right column: layer names
        float rightX = x + 380.f;
        float rightY = currY + 6.f;
        sf::Text rt("Layer Names:", *m_Font, 12);
        rt.setPosition(rightX, rightY);
        rt.setFillColor(C_TEXT_PRIMARY);
        window.draw(rt);
        rightY += 20.f;

        for (int i = 0; i < PhysicsSystem::MAX_CHANNELS; ++i)
        {
            rightY += DrawProjectSettingsInputField(window, "Layer " + std::to_string(i), PhysicsSystem::GetChannelName(i),
                                                    static_cast<ProjectSettingsField>((int) ProjectSettingsField::ChannelName0 + i),
                                                    rightX, rightY, x + w - 20.f);
        }
    }

    float btnW = 110.f;
    float btnH = 28.f;
    float btnX = x + w - btnW - 20.f;
    float btnY = y + h - btnH - 16.f;
    sf::RectangleShape saveBtn(sf::Vector2f(btnW, btnH));
    saveBtn.setPosition(btnX, btnY);
    saveBtn.setFillColor(C_ACCENT);
    window.draw(saveBtn);

    sf::Text saveTxt("Save & Close", *m_Font, 11);
    saveTxt.setPosition(btnX + (btnW - saveTxt.getLocalBounds().width) / 2.f, btnY + 7.f);
    saveTxt.setFillColor(C_TEXT_PRIMARY);
    window.draw(saveTxt);
    m_ProjectSettingsButtons.push_back({{btnX, btnY, btnW, btnH}, "close_proj_settings"});
}


void EditorScene::HandleProjectSettingsClick(sf::Vector2f pos)
{
    for (const auto &btn: m_ProjectSettingsButtons)
    {
        if (btn.bounds.contains(pos))
        {
            if (btn.action == "close_proj_settings")
            {
                CommitActiveProjectSettingsField();
                m_ShowProjectSettings = false;
                SaveProjectSettings();
            } else if (btn.action == "toggle_vsync")
            {
                CommitActiveProjectSettingsField();
                m_ProjectVSync = !m_ProjectVSync;
                SaveProjectSettings();
            } else if (btn.action == "toggle_fullscreen")
            {
                CommitActiveProjectSettingsField();
                m_ProjectFullscreen = !m_ProjectFullscreen;
                SaveProjectSettings();
            } else if (btn.action.find("tab_proj_") == 0)
            {
                CommitActiveProjectSettingsField();
                m_ProjectSettingsTab = std::stoi(btn.action.substr(9));
            } else if (btn.action.find("col_toggle_") == 0)
            {
                CommitActiveProjectSettingsField();
                std::string rest = btn.action.substr(11);
                size_t us = rest.find('_');
                if (us != std::string::npos)
                {
                    int r = std::stoi(rest.substr(0, us));
                    int c = std::stoi(rest.substr(us + 1));
                    PhysicsSystem::SetCanCollide(r, c, !PhysicsSystem::CanCollide(r, c));
                    SaveProjectSettings();
                }
            } else if (btn.action == "col_all_on")
            {
                CommitActiveProjectSettingsField();
                for (int r = 0; r < PhysicsSystem::MAX_CHANNELS; ++r)
                    for (int c = 0; c < PhysicsSystem::MAX_CHANNELS; ++c)
                        PhysicsSystem::SetCanCollide(r, c, true);
                SaveProjectSettings();
            } else if (btn.action == "col_all_off")
            {
                CommitActiveProjectSettingsField();
                for (int r = 0; r < PhysicsSystem::MAX_CHANNELS; ++r)
                    for (int c = 0; c < PhysicsSystem::MAX_CHANNELS; ++c)
                        PhysicsSystem::SetCanCollide(r, c, false);
                SaveProjectSettings();
            } else if (btn.action == "col_reset")
            {
                CommitActiveProjectSettingsField();
                PhysicsSystem::ResetCollisionMatrix();
                SaveProjectSettings();
            } else if (btn.action.find("edit_proj_") == 0)
            {
                CommitActiveProjectSettingsField();
                int fieldIdx = std::stoi(btn.action.substr(10));
                m_ActiveProjectSettingsField = static_cast<ProjectSettingsField>(fieldIdx);

                if (m_ActiveProjectSettingsField == ProjectSettingsField::ProjectName)
                    m_ProjectSettingsInputText = m_ProjectName;
                else if (m_ActiveProjectSettingsField == ProjectSettingsField::Version)
                    m_ProjectSettingsInputText = m_ProjectVersion;
                else if (m_ActiveProjectSettingsField == ProjectSettingsField::Author)
                    m_ProjectSettingsInputText = m_ProjectAuthor;
                else if (m_ActiveProjectSettingsField == ProjectSettingsField::StartScene)
                    m_ProjectSettingsInputText = m_ProjectStartScene;
                else if (m_ActiveProjectSettingsField == ProjectSettingsField::WindowWidth)
                    m_ProjectSettingsInputText = std::to_string(m_ProjectWindowWidth);
                else if (m_ActiveProjectSettingsField == ProjectSettingsField::WindowHeight)
                    m_ProjectSettingsInputText = std::to_string(m_ProjectWindowHeight);
                else if (m_ActiveProjectSettingsField == ProjectSettingsField::TargetFPS)
                    m_ProjectSettingsInputText = std::to_string(m_ProjectTargetFPS);
                else if (m_ActiveProjectSettingsField == ProjectSettingsField::ClearColorHex)
                    m_ProjectSettingsInputText = ColorToHex(m_ProjectClearColor);
                else if (m_ActiveProjectSettingsField == ProjectSettingsField::MasterVolume)
                    m_ProjectSettingsInputText = std::to_string((int) m_ProjectMasterVolume);
                else if (m_ActiveProjectSettingsField == ProjectSettingsField::MusicVolume)
                    m_ProjectSettingsInputText = std::to_string((int) m_ProjectMusicVolume);
                else if (m_ActiveProjectSettingsField >= ProjectSettingsField::ChannelName0 &&
                         m_ActiveProjectSettingsField <= ProjectSettingsField::ChannelName7)
                {
                    int chIdx = (int) m_ActiveProjectSettingsField - (int) ProjectSettingsField::ChannelName0;
                    m_ProjectSettingsInputText = PhysicsSystem::GetChannelName(chIdx);
                }
            }
            return;
        }
    }
    CommitActiveProjectSettingsField();
}

