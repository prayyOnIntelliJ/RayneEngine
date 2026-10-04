#include "EditorScene_Common.h"
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <map>


void EditorScene::InitMenus()
{
    MenuEntry datei;
    datei.label = "File";
    datei.items = {
        {"Save", "save", false, "Ctrl+S"},
        {"Load", "load", false, "Ctrl+L"},
        {"", "", true, ""},
        {"Open in CLion", "open_clion", false, ""},
        {"Open in Rider", "open_rider", false, ""},
        {"Open in VS Code", "open_vscode", false, ""},
        {"", "", true, ""},
        {"Project Settings", "project_settings", false, ""},
        {"Quit", "quit", false, ""}
    };

    MenuEntry buildMenu;
    buildMenu.label = "Build";
    buildMenu.items = {
        {"Build Game (Standalone)", "build_game", false, ""},
        {"Package Engine (ZIP)", "package_engine", false, ""}
    };

    MenuEntry edit;
    edit.label = "Edit";
    edit.items = {
        {"Settings", "settings", false, "Ctrl+,"},
        {"", "", true, ""},
        {"Copy", "copy", false, "Ctrl+C"},
        {"Paste", "paste", false, "Ctrl+V"},
        {"Duplicate", "duplicate", false, "Ctrl+D"},
        {"Delete", "delete", false, "Del"},
        {"", "", true, ""},
        {"Deselect everything", "deselect", false, "Esc"},
        {"", "", true, ""},
        {"Clear Scene", "reset_scene", false, ""}
    };

    MenuEntry ansicht;
    ansicht.label = "View";
    ansicht.items = {
        {"Toggle Grid", "toggle_grid", false, "G"},
        {"Center Camera", "center_camera", false, ""}
    };

    MenuEntry tools;
    tools.label = "Tools";
    tools.items = {
        {"Run Scene", "run", false, "F5"},
        {"", "", true, ""},
        {"Save", "save", false, "Ctrl+S"}
    };

    m_Menus = {datei, buildMenu, edit, ansicht, tools};
}


void EditorScene::HandleMenuAction(const std::string &action)
{
    if (action == "settings")
    {
        m_ShowSettings = !m_ShowSettings;
        m_ActiveSettingsField = SettingsField::None;
        m_SettingsInputText.clear();
    } else if (action == "save")
    {
        SyncToRegistry();
        SaveToJson(std::string(ASSET_PATH) + "/" + m_SceneSavePath);
        std::cout << "[INFO] [EditorScene] Scene saved successfully to " << m_SceneSavePath << "\n";
        m_SaveFeedbackTimer = 2.0f;
    } else if (action == "load")
    {
        LoadFromJson(std::string(ASSET_PATH) + "/" + m_SceneSavePath);
        std::cout << "[INFO] [EditorScene] Scene loaded successfully from " << m_SceneSavePath << "\n";
    } else if (action == "build_game") { ExportStandaloneGame(); } else if (
        action == "package_engine") { PackageEngineZip(); } else if (action == "project_settings")
    {
        m_ShowProjectSettings = !m_ShowProjectSettings;
        m_ActiveProjectSettingsField = ProjectSettingsField::None;
        m_ProjectSettingsInputText.clear();
    } else if (action == "open_clion" || action == "open_rider" || action == "open_vscode")
    {
        std::filesystem::path assetsPath = FindProjectRoot() / "assets";
        std::string exeName = action == "open_clion" ? "clion" : (action == "open_rider" ? "rider" : "code");
        m_PreferredIDE = exeName;
        SaveSettings();
        std::string cmd = exeName + " \"" + assetsPath.string() + "\"";
        LaunchProcessDetached(cmd);
    } else if (action == "quit") { m_Window.close(); } else if (action == "delete")
    {
        DeleteSelected();
        UpdateStatusText();
    } else if (action == "deselect")
    {
        ClearSelection();
        m_ActiveField = EditField::None;
        m_ActiveInputText.clear();
        UpdateStatusText();
    } else if (action == "toggle_grid")
    {
        m_SnapToGrid = !m_SnapToGrid;
        UpdateStatusText();
    } else if (action == "center_camera") { m_camera.setCenter(0.f, 0.f); } else if (action == "run")
    {
        TryLaunchPlayMode();
    } else if (action == "reset_scene")
    {
        for (auto &obj: m_Objects)
            if (obj.entity != 0) m_Registry.DestroyEntity(obj.entity);
        m_Objects.clear();
        m_Selected = nullptr;
        UpdateStatusText();
    } else if (action == "duplicate")
    {
        if (m_Selected)
        {
            json j = SerializeObject(*m_Selected);
            std::string newId = NextId();
            j["id"] = newId;
            j["x"] = j.value("x", 0.f) + m_GridSize;
            j["y"] = j.value("y", 0.f) + m_GridSize;
            DeserializeObject(j);
            EditorObject *newObj = ObjectById(newId);
            if (newObj)
            {
                SelectObject(newObj, false);
                auto cmd = std::make_shared<ObjectStateCommand>(newId, json(nullptr), SerializeObject(*newObj));
                m_UndoStack.push_back(cmd);
                m_RedoStack.clear();
                SetDirty(true);
            }
        }
    } else if (action == "copy") { CopySelection(); } else if (action == "paste") { PasteClipboard(); } else if (
        action == "open_ui_editor") { m_manager.SwitchSceneTo("ui_editor"); } else if (
        action == "exit_template_mode") { ExitTemplateEditMode(true); }
}


void EditorScene::DrawMenuBar(sf::RenderWindow &window)
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

    std::string sceneFile = m_SceneSavePath;
    const size_t slash = sceneFile.find_last_of("/\\");
    if (slash != std::string::npos) sceneFile = sceneFile.substr(slash + 1);

    sf::Text watermark;
    watermark.setFont(*m_Font);
    watermark.setCharacterSize(11);
    watermark.setFillColor(m_HasUnsavedChanges ? C_WARNING : C_TEXT_MUTED);
    watermark.setString(sceneFile + (m_HasUnsavedChanges ? " *" : "") + "  |  RayneEngine");
    watermark.setPosition(w - watermark.getLocalBounds().width - 12.f, 9.f);
    window.draw(watermark);
}

