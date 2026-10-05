#include "EditorScene_Common.h"
#include "FileDropManager.h"
#include "../Panels/HierarchyPanel.h"
#include "../Panels/InspectorPanel.h"
#include "../Panels/MenuBarPanel.h"
#include "../Panels/ToolbarPanel.h"
#include "../Panels/SpotlightPalette.h"
#include "../Gizmos/GizmoManager.h"
#include "../Modals/EditorModals.h"
#include "../Data/EditorSerializer.h"

EditorScene::EditorScene(SceneManager &manager, sf::RenderWindow &window, Registry &registry)
    : Scene(manager), m_Window(window), m_Registry(registry)
{
    m_Hierarchy = std::make_unique<HierarchyPanel>(this);
    m_Inspector = std::make_unique<InspectorPanel>(this);
    m_MenuBar = std::make_unique<MenuBarPanel>(this);
    m_Toolbar = std::make_unique<ToolbarPanel>(this);
    m_Spotlight = std::make_unique<SpotlightPalette>(this);
    m_GizmoMgr = std::make_unique<GizmoManager>(this);
    m_EditorModals = std::make_unique<EditorModals>(this);
    m_EditorSerializer = std::make_unique<EditorSerializer>(this);

    m_Font = ResourceManager::Get().GetFont(ENGINE_ASSET_PATH "/fonts/Merriweather.ttf");
    std::filesystem::path projRoot = FindProjectRoot();
    m_ContentBrowser = std::make_unique<ContentBrowser>(*m_Font, (projRoot / "assets").string());
    m_ConsolePanel = std::make_unique<ConsolePanel>(*m_Font);
    m_ProfilerPanel = std::make_unique<ProfilerPanel>(*m_Font);
    m_ContentBrowser->onSceneLoadRequest = [this](const std::string &path) {
        std::error_code ec;
        std::filesystem::path p(path);
        std::filesystem::path root = std::filesystem::absolute(ASSET_PATH, ec);
        std::string relPath = std::filesystem::proximate(p, root, ec).generic_string();
        if (ec) relPath = p.filename().string();

        if (path.find("_ui.json") != std::string::npos || relPath.find("_ui.json") != std::string::npos)
        {
            UIManager::Get().SetCurrentUIPath(relPath);
            m_manager.SwitchSceneTo("ui_editor");
            return;
        }

        this->LoadFromJson(path);
        this->m_SceneSavePath = relPath;
        this->SaveSettings();
        std::cout << "[INFO] [EditorScene] Loaded scene from browser. Set active path to: " << relPath << "\n";
    };
    m_ContentBrowser->onScriptOpenRequest = [this](const std::string &path) { this->OpenScriptInIDE(path); };
    m_ContentBrowser->onTemplateOpenRequest = [this](const std::string &path) { this->EnterTemplateEditMode(path); };
    m_ContentBrowser->onAssetMoved = [this](const std::string &oldPath, const std::string &newPath) { this->HandleAssetMoved(oldPath, newPath); };

    m_camera = window.getDefaultView();

    const float winW = static_cast<float>(window.getSize().x);
    const float winH = static_cast<float>(window.getSize().y);

    sf::View adjustedView = m_camera;
    adjustedView.setViewport({
        HierarchyWidth / winW,
        TopBarHeight / winH,
        1.f - ((InspectorWidth + HierarchyWidth) / winW),
        1.f - (BrowserHeight / winH) - (TopBarHeight / winH)
    });
    m_camera = adjustedView;

    UpdateBounds();

    m_Preview.setSize({m_GridSize, m_GridSize});
    m_Preview.setFillColor(sf::Color(C_ACCENT.r, C_ACCENT.g, C_ACCENT.b, 45));
    m_Preview.setOutlineColor(sf::Color(C_ACCENT_HOV.r, C_ACCENT_HOV.g, C_ACCENT_HOV.b, 160));
    m_Preview.setOutlineThickness(1.f);

    m_CirclePreview.setRadius(m_GridSize / 2.f);
    m_CirclePreview.setFillColor(sf::Color(C_ACCENT.r, C_ACCENT.g, C_ACCENT.b, 45));
    m_CirclePreview.setOutlineColor(sf::Color(C_ACCENT_HOV.r, C_ACCENT_HOV.g, C_ACCENT_HOV.b, 160));
    m_CirclePreview.setOutlineThickness(1.f);

    m_StatusText.setFont(*m_Font);
    m_StatusText.setCharacterSize(11);
    m_StatusText.setFillColor(C_TEXT_MUTED);

    m_InspectorPanel.setFillColor(C_BG_PANEL);
    m_HierarchyPanel.setFillColor(C_BG_PANEL);

    InitMenus();
    InitSpotlightItems();
    UpdateStatusText();

    LoadSettings();
    LoadProjectSettings();

    const std::string scenesDir = ASSET_PATH
    "/scenes";
    const std::string defaultScenePath = std::string(ASSET_PATH) + "/" + m_SceneSavePath;

    if (!std::filesystem::exists(scenesDir)) { std::filesystem::create_directories(scenesDir); }

    if (std::filesystem::exists(defaultScenePath))
    {
        std::cout << "[INFO] [EditorScene] Auto-loading default scene at startup...\n";
        LoadFromJson(defaultScenePath);
    } else
    {
        std::cout << "[INFO] [EditorScene] Default scene not found, creating new empty scene at " << defaultScenePath <<
                "...\n";
        SaveToJson(defaultScenePath);
        std::string defaultUIPath = defaultScenePath.substr(0, defaultScenePath.find_last_of('.')) + "_ui.json";
        if (!std::filesystem::exists(defaultUIPath))
        {
            std::ofstream uiFile(defaultUIPath);
            if (uiFile.is_open())
            {
                uiFile << "{\n    \"ui_elements\": []\n}\n";
                uiFile.close();
            }
        }
    }
}

EditorScene::~EditorScene()
{
    FileDropManager::Get().ClearDropCallback();
}

void EditorScene::OnEnter()
{
    LoadProjectSettings();
    if (!m_PlayModeSnapshot.empty())
    {
        RestoreSnapshot();
        m_PlayModeSnapshot = json{};
    }

    AudioManager::Get().StopAllSounds();
    AudioManager::Get().StopMusic();

    std::cout << "[INFO] [EditorScene] Activated Editor Layout\n";
    UpdateBounds();
    UpdateStatusText();

    FileDropManager::Get().SetDropCallback([this](const std::vector<std::string> &paths, sf::Vector2f pos) {
        HandleExternalFileDrop(paths, pos);
    });
}

void EditorScene::OnExit()
{
    FileDropManager::Get().ClearDropCallback();
    std::cout << "[INFO] [EditorScene] Exited Editor mode.\n";
    SyncToRegistry();
}

void EditorScene::OnShutdown()
{
    std::cout << "[INFO] [EditorScene] Shutting down, saving scene state...\n";
    if (!m_PlayModeSnapshot.empty())
    {
        RestoreSnapshot();
        m_PlayModeSnapshot = json{};
    }

    SaveToJson(std::string(ASSET_PATH) + "/" + m_SceneSavePath);
    SaveSettings();
}

void EditorScene::UpdateBounds()
{
    const float w = static_cast<float>(m_Window.getSize().x);
    const float h = static_cast<float>(m_Window.getSize().y);

    m_HierarchyBounds = {0.f, TopBarHeight, HierarchyWidth, h - TopBarHeight};
    m_BrowserBounds = {HierarchyWidth, h - BrowserHeight, w - InspectorWidth - HierarchyWidth, BrowserHeight};

    m_TabBrowserBounds = {HierarchyWidth, h - BrowserHeight - TabBarHeight, 100.f, TabBarHeight};
    m_TabConsoleBounds = {HierarchyWidth + 100.f, h - BrowserHeight - TabBarHeight, 100.f, TabBarHeight};
    m_TabProfilerBounds = {HierarchyWidth + 200.f, h - BrowserHeight - TabBarHeight, 100.f, TabBarHeight};

    m_InspectorBounds = {w - InspectorWidth, TopBarHeight, InspectorWidth, h - TopBarHeight};
}


void EditorScene::HandleEvent(const sf::Event &event)
{
    if (event.type == sf::Event::MouseMoved)
        m_MouseScreenPos = {(float) event.mouseMove.x, (float) event.mouseMove.y};
    else if (event.type == sf::Event::MouseButtonPressed || event.type == sf::Event::MouseButtonReleased)
        m_MouseScreenPos = {(float) event.mouseButton.x, (float) event.mouseButton.y};
    else if (event.type == sf::Event::MouseWheelScrolled)
        m_MouseScreenPos = {(float) event.mouseWheelScroll.x, (float) event.mouseWheelScroll.y};

    UpdateBounds();

    if (m_ShowBuildPopup)
    {
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
        {
            if (m_BuildFinished || !m_CancelBuildRequested) { m_ShowBuildPopup = false; }
            return;
        }

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
        {
            float pW = 500.f;
            float pH = 180.f;
            float pX = (m_Window.getSize().x - pW) / 2.f;
            float pY = (m_Window.getSize().y - pH) / 2.f;

            float btnW = 100.f;
            float btnH = 30.f;
            float btnX = pX + pW / 2.f - btnW / 2.f;
            float btnY = pY + pH - 20.f - btnH;

            sf::FloatRect btnBounds(btnX, btnY, btnW, btnH);
            if (btnBounds.contains((float) event.mouseButton.x, (float) event.mouseButton.y))
            {
                if (m_BuildFinished) { m_ShowBuildPopup = false; } else
                {
                    std::lock_guard<std::mutex> lock(m_BuildMutex);
                    m_CancelBuildRequested = true;
                }
            }
            return;
        }

        if (event.type == sf::Event::MouseMoved ||
            event.type == sf::Event::MouseButtonPressed ||
            event.type == sf::Event::MouseButtonReleased ||
            event.type == sf::Event::MouseWheelScrolled ||
            event.type == sf::Event::KeyPressed ||
            event.type == sf::Event::KeyReleased) { return; }
    }

    if (m_ShowScriptErrorModal)
    {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
        {
            if (m_ScriptErrorOkBtn.contains(m_MouseScreenPos))
            {
                m_ShowScriptErrorModal = false;
                m_ScriptErrorDetails.clear();
                m_ScriptErrorPath.clear();
                return;
            }
            if (!m_ScriptErrorPath.empty() && m_ScriptErrorOpenIDEBtn.contains(m_MouseScreenPos))
            {
                OpenScriptInIDE(m_ScriptErrorPath);
                m_ShowScriptErrorModal = false;
                m_ScriptErrorDetails.clear();
                m_ScriptErrorPath.clear();
                return;
            }
        } else if (event.type == sf::Event::KeyPressed)
        {
            if (event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::Return || event.key.code ==
                sf::Keyboard::Enter)
            {
                m_ShowScriptErrorModal = false;
                m_ScriptErrorDetails.clear();
                m_ScriptErrorPath.clear();
                return;
            }
        }
        return;
    }

    if (m_ShowDeleteModal)
    {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
        {
            if (m_DeleteModalCascadeBtn.contains(m_MouseScreenPos)) { ConfirmDeleteCascade(); } else if (
                m_DeleteModalUnparentBtn.contains(m_MouseScreenPos)) { ConfirmDeleteUnparent(); } else if (
                m_DeleteModalCancelBtn.contains(m_MouseScreenPos))
            {
                m_ShowDeleteModal = false;
                m_DeleteModalTargetId.clear();
                m_DeleteModalDescendantIds.clear();
            }
        } else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
        {
            m_ShowDeleteModal = false;
            m_DeleteModalTargetId.clear();
            m_DeleteModalDescendantIds.clear();
        }
        return;
    }

    if (m_ShowSaveTemplatePrompt)
    {
        if (event.type == sf::Event::KeyPressed)
        {
            if (event.key.code == sf::Keyboard::Escape)
            {
                m_ShowSaveTemplatePrompt = false;
                return;
            }
            if (event.key.code == sf::Keyboard::Enter)
            {
                if (!m_SaveTemplateInputName.empty())
                {
                    SaveAsTemplate(ObjectById(m_SaveTemplateTargetId), m_SaveTemplateInputName);
                }
                m_ShowSaveTemplatePrompt = false;
                return;
            }
        } else if (event.type == sf::Event::TextEntered)
        {
            if (event.text.unicode == 8)
            {
                if (!m_SaveTemplateInputName.empty())
                    m_SaveTemplateInputName.pop_back();
            } else if (event.text.unicode >= 32 && event.text.unicode < 127)
            {
                char c = static_cast<char>(event.text.unicode);
                if (std::isalnum(c) || c == '_' || c == '-') { m_SaveTemplateInputName += c; }
            }
            return;
        } else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
        {
            float pW = 400.f;
            float pH = 180.f;
            float pX = (m_Window.getSize().x - pW) / 2.f;
            float pY = (m_Window.getSize().y - pH) / 2.f;

            float btnW = 90.f;
            float btnH = 28.f;
            float btnY = pY + pH - 45.f;
            float btnSaveX = pX + pW - 20.f - btnW * 2.f - 10.f;
            float btnCancelX = pX + pW - 20.f - btnW;

            sf::FloatRect saveBounds(btnSaveX, btnY, btnW, btnH);
            sf::FloatRect cancelBounds(btnCancelX, btnY, btnW, btnH);
            sf::Vector2f mPos((float) event.mouseButton.x, (float) event.mouseButton.y);

            if (saveBounds.contains(mPos))
            {
                if (!m_SaveTemplateInputName.empty())
                {
                    SaveAsTemplate(ObjectById(m_SaveTemplateTargetId), m_SaveTemplateInputName);
                }
                m_ShowSaveTemplatePrompt = false;
            } else if (cancelBounds.contains(mPos)) { m_ShowSaveTemplatePrompt = false; } else
            {
                sf::FloatRect panelBounds(pX, pY, pW, pH);
                if (!panelBounds.contains(mPos)) { m_ShowSaveTemplatePrompt = false; }
            }
            return;
        }
        return;
    }

    if (m_SpotlightOpen)
    {
        const float cardH = 58.f;
        const float cardGap = 8.f;
        int totalRows = (static_cast<int>(m_FilteredSpotlightItems.size()) + 1) / 2;
        float totalContentH = totalRows > 0 ? (totalRows * (cardH + cardGap) - cardGap) : 0.0f;
        float maxScroll = std::max(0.0f, totalContentH - m_SpotlightItemsViewportBounds.height + 16.0f);

        auto AutoScrollToSelected = [&]() {
            if (m_SpotlightSelectedIndex < 0 || m_SpotlightSelectedIndex >= static_cast<int>(m_FilteredSpotlightItems.
                    size()))
                return;
            int selRow = m_SpotlightSelectedIndex / 2;
            float topY = selRow * (cardH + cardGap);
            float bottomY = topY + cardH;
            if (topY < m_SpotlightScrollY) { m_SpotlightScrollY = topY; } else if (
                bottomY > m_SpotlightScrollY + m_SpotlightItemsViewportBounds.height)
            {
                m_SpotlightScrollY = bottomY - m_SpotlightItemsViewportBounds.height;
            }
            m_SpotlightScrollY = std::max(0.0f, std::min(m_SpotlightScrollY, maxScroll));
        };

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
        {
            if (m_SpotlightCloseBtnBounds.contains(m_MouseScreenPos))
            {
                CloseSpotlight();
                return;
            }

            if (m_SpotlightClearSearchBtnBounds.contains(m_MouseScreenPos))
            {
                m_SpotlightQuery.clear();
                m_SpotlightSearchFocused = true;
                FilterSpotlightItems();
                m_SpotlightSelectedIndex = 0;
                m_SpotlightScrollY = 0.0f;
                return;
            }

            if (m_SpotlightSearchBoxBounds.contains(m_MouseScreenPos))
            {
                m_SpotlightSearchFocused = true;
                return;
            } else { m_SpotlightSearchFocused = false; }

            if (m_SpotlightScrollbarThumbBounds.contains(m_MouseScreenPos))
            {
                m_SpotlightDraggingScrollbar = true;
                m_SpotlightDragScrollStartMouseY = m_MouseScreenPos.y;
                m_SpotlightDragScrollStartScrollY = m_SpotlightScrollY;
                return;
            } else if (m_SpotlightScrollbarTrackBounds.contains(m_MouseScreenPos))
            {
                float relY = (m_MouseScreenPos.y - m_SpotlightScrollbarTrackBounds.top) /
                             m_SpotlightScrollbarTrackBounds.height;
                m_SpotlightScrollY = std::max(0.0f, std::min(maxScroll, relY * maxScroll));
                return;
            }

            for (const auto &[rect, catIdx]: m_SpotlightCategoryHitboxes)
            {
                if (rect.contains(m_MouseScreenPos))
                {
                    m_SpotlightCategory = catIdx;
                    m_SpotlightSelectedIndex = 0;
                    m_SpotlightScrollY = 0.0f;
                    FilterSpotlightItems();
                    return;
                }
            }

            if (m_SpotlightItemsViewportBounds.contains(m_MouseScreenPos))
            {
                for (const auto &[rect, itemIdx]: m_SpotlightItemHitboxes)
                {
                    if (rect.contains(m_MouseScreenPos))
                    {
                        if (itemIdx >= 0 && itemIdx < (int) m_FilteredSpotlightItems.size())
                        {
                            SelectSpotlightItem(m_FilteredSpotlightItems[itemIdx]);
                        }
                        return;
                    }
                }
            }

            if (!m_SpotlightModalBounds.contains(m_MouseScreenPos)) { CloseSpotlight(); }
            return;
        } else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
        {
            m_SpotlightDraggingScrollbar = false;
            return;
        } else if (event.type == sf::Event::MouseMoved)
        {
            if (m_SpotlightDraggingScrollbar &&maxScroll > 0.0f && m_SpotlightScrollbarTrackBounds.height > 0.0f) {
                float deltaY = m_MouseScreenPos.y - m_SpotlightDragScrollStartMouseY;
                float usableTrackH = m_SpotlightScrollbarTrackBounds.height - m_SpotlightScrollbarThumbBounds.height;
                if (usableTrackH > 0.0f)
                {
                    float scrollDelta = (deltaY / usableTrackH) * maxScroll;
                    m_SpotlightScrollY = std::max(
                        0.0f, std::min(maxScroll, m_SpotlightDragScrollStartScrollY + scrollDelta));
                }
                return;
            }
        } else if (event.type == sf::Event::MouseWheelScrolled)
        {
            if (m_SpotlightModalBounds.contains(m_MouseScreenPos))
            {
                m_SpotlightScrollY = std::max(
                    0.0f, std::min(maxScroll, m_SpotlightScrollY - event.mouseWheelScroll.delta * 40.0f));
            }
            return;
        } else if (event.type == sf::Event::KeyPressed)
        {
            if (event.key.code == sf::Keyboard::Escape)
            {
                if (m_SpotlightSearchFocused) { m_SpotlightSearchFocused = false; } else { CloseSpotlight(); }
                return;
            }
            if (event.key.code == sf::Keyboard::Return || event.key.code == sf::Keyboard::Enter)
            {
                if (!m_FilteredSpotlightItems.empty() && m_SpotlightSelectedIndex >= 0 && m_SpotlightSelectedIndex < (
                        int) m_FilteredSpotlightItems.size())
                {
                    SelectSpotlightItem(m_FilteredSpotlightItems[m_SpotlightSelectedIndex]);
                }
                return;
            }
            if (event.key.code == sf::Keyboard::Up)
            {
                if (!m_FilteredSpotlightItems.empty())
                {
                    if (m_SpotlightSelectedIndex >= 2) { m_SpotlightSelectedIndex -= 2; } else
                    {
                        m_SpotlightSelectedIndex = 0;
                    }
                    AutoScrollToSelected();
                }
                return;
            }
            if (event.key.code == sf::Keyboard::Down)
            {
                if (!m_FilteredSpotlightItems.empty())
                {
                    if (m_SpotlightSelectedIndex + 2 < (int) m_FilteredSpotlightItems.size())
                    {
                        m_SpotlightSelectedIndex += 2;
                    } else { m_SpotlightSelectedIndex = (int) m_FilteredSpotlightItems.size() - 1; }
                    AutoScrollToSelected();
                }
                return;
            }
            if (event.key.code == sf::Keyboard::Left)
            {
                if (m_SpotlightSelectedIndex % 2 == 1)
                {
                    m_SpotlightSelectedIndex--;
                    AutoScrollToSelected();
                }
                return;
            }
            if (event.key.code == sf::Keyboard::Right)
            {
                if (m_SpotlightSelectedIndex % 2 == 0 && m_SpotlightSelectedIndex + 1 < (int) m_FilteredSpotlightItems.
                    size())
                {
                    m_SpotlightSelectedIndex++;
                    AutoScrollToSelected();
                }
                return;
            }
            if (!m_SpotlightSearchFocused)
            {
                if (event.key.code == sf::Keyboard::D)
                {
                    m_SpotlightCategory = (m_SpotlightCategory + 1) % 5;
                    m_SpotlightSelectedIndex = 0;
                    m_SpotlightScrollY = 0.0f;
                    FilterSpotlightItems();
                    return;
                }
                if (event.key.code == sf::Keyboard::A)
                {
                    m_SpotlightCategory = (m_SpotlightCategory - 1 + 5) % 5;
                    m_SpotlightSelectedIndex = 0;
                    m_SpotlightScrollY = 0.0f;
                    FilterSpotlightItems();
                    return;
                }
            }
            if (event.key.code == sf::Keyboard::Tab)
            {
                if (event.key.shift)
                    m_SpotlightCategory = (m_SpotlightCategory - 1 + 5) % 5;
                else
                    m_SpotlightCategory = (m_SpotlightCategory + 1) % 5;
                m_SpotlightSelectedIndex = 0;
                m_SpotlightScrollY = 0.0f;
                FilterSpotlightItems();
                return;
            }
        } else if (event.type == sf::Event::TextEntered)
        {
            if (event.text.unicode == 8 || event.text.unicode == 127)
            {
                if (!m_SpotlightQuery.empty())
                {
                    m_SpotlightQuery.pop_back();
                    FilterSpotlightItems();
                    m_SpotlightSelectedIndex = 0;
                    m_SpotlightScrollY = 0.0f;
                }
                return;
            }
            if (event.text.unicode >= 32 && event.text.unicode < 127)
            {
                char c = static_cast<char>(event.text.unicode);
                if (!m_SpotlightSearchFocused)
                {
                    if (c == 'a' || c == 'A' || c == 'd' || c == 'D') { return; }
                    m_SpotlightSearchFocused = true;
                }
                m_SpotlightQuery += c;
                FilterSpotlightItems();
                m_SpotlightSelectedIndex = 0;
                m_SpotlightScrollY = 0.0f;
                return;
            }
            return;
        }
        return;
    }

    if (m_ShowProjectSettings)
    {
        if (event.type == sf::Event::TextEntered && m_ActiveProjectSettingsField != ProjectSettingsField::None)
        {
            if (event.text.unicode == '\b')
            {
                if (HasTextSelection()) DeleteActiveSelection();
                else if (!m_ProjectSettingsInputText.empty()) m_ProjectSettingsInputText.pop_back();
            } else if (event.text.unicode == '\r' || event.text.unicode ==
                       '\n') { CommitActiveProjectSettingsField(); } else if (event.text.unicode == 27)
            {
                m_ActiveProjectSettingsField = ProjectSettingsField::None;
                m_ProjectSettingsInputText.clear();
                m_InputSelectionStart = -1;
                m_InputSelectionEnd = -1;
            } else if (event.text.unicode >= 32 && event.text.unicode < 128)
            {
                if (HasTextSelection()) DeleteActiveSelection();
                m_ProjectSettingsInputText += static_cast<char>(event.text.unicode);
            }
            return;
        }

        if (event.type == sf::Event::KeyPressed && m_ActiveProjectSettingsField != ProjectSettingsField::None)
        {
            const bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(
                                  sf::Keyboard::RControl);
            if (ctrl && event.key.code == sf::Keyboard::A)
            {
                m_InputSelectionStart = 0;
                m_InputSelectionEnd = (int) m_ProjectSettingsInputText.size();
                return;
            }
            if (ctrl && event.key.code == sf::Keyboard::C && HasTextSelection())
            {
                int sMin = std::clamp(GetSelectionMin(), 0, (int) m_ProjectSettingsInputText.size());
                int sMax = std::clamp(GetSelectionMax(), 0, (int) m_ProjectSettingsInputText.size());
                sf::Clipboard::setString(m_ProjectSettingsInputText.substr(sMin, sMax - sMin));
                return;
            }
            if (ctrl && event.key.code == sf::Keyboard::V)
            {
                if (HasTextSelection()) DeleteActiveSelection();
                std::string pasteStr = sf::Clipboard::getString().toAnsiString();
                for (char c: pasteStr) { if (c >= 32 && c < 127) m_ProjectSettingsInputText += c; }
                return;
            }
            if (event.key.code == sf::Keyboard::Delete && HasTextSelection())
            {
                DeleteActiveSelection();
                return;
            }
        }

        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
        {
            if (m_ActiveProjectSettingsField != ProjectSettingsField::None)
            {
                m_ActiveProjectSettingsField = ProjectSettingsField::None;
                m_ProjectSettingsInputText.clear();
                m_InputSelectionStart = -1;
                m_InputSelectionEnd = -1;
            } else
            {
                CommitActiveProjectSettingsField();
                m_ShowProjectSettings = false;
            }
            return;
        }

        if (event.type == sf::Event::MouseMoved)
        {
            m_MouseScreenPos = {(float) event.mouseMove.x, (float) event.mouseMove.y};
            if (m_IsSelectingText &&m_ActiveProjectSettingsField != ProjectSettingsField::None) {
                float textStartX = m_ProjectSettingsInputBounds.left + 6.f;
                float localX = m_MouseScreenPos.x - textStartX;
                sf::Text t(m_ProjectSettingsInputText, *m_Font, 11);
                int bestIdx = 0;
                float bestDist = 1e9f;
                for (int i = 0; i <= (int) m_ProjectSettingsInputText.size(); ++i)
                {
                    float cx = t.findCharacterPos(i).x;
                    float d = std::abs(localX - cx);
                    if (d < bestDist)
                    {
                        bestDist = d;
                        bestIdx = i;
                    }
                }
                m_InputSelectionEnd = bestIdx;
            }
            return;
        }

        if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
        {
            m_IsSelectingText = false;
            if (m_InputSelectionStart == m_InputSelectionEnd)
            {
                m_InputSelectionStart = -1;
                m_InputSelectionEnd = -1;
            }
            return;
        }

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
        {
            sf::Vector2f mPos{(float) event.mouseButton.x, (float) event.mouseButton.y};
            if (m_ActiveProjectSettingsField != ProjectSettingsField::None && m_ProjectSettingsInputBounds.
                contains(mPos))
            {
                float textStartX = m_ProjectSettingsInputBounds.left + 6.f;
                float localX = mPos.x - textStartX;
                sf::Text t(m_ProjectSettingsInputText, *m_Font, 11);
                int bestIdx = 0;
                float bestDist = 1e9f;
                for (int i = 0; i <= (int) m_ProjectSettingsInputText.size(); ++i)
                {
                    float cx = t.findCharacterPos(i).x;
                    float d = std::abs(localX - cx);
                    if (d < bestDist)
                    {
                        bestDist = d;
                        bestIdx = i;
                    }
                }
                m_InputSelectionStart = bestIdx;
                m_InputSelectionEnd = bestIdx;
                m_IsSelectingText = true;
                return;
            }
            HandleProjectSettingsClick(mPos);
            return;
        }

        return;
    }

    if (m_ShowSettings)
    {
        if (event.type == sf::Event::TextEntered && m_ActiveSettingsField != SettingsField::None)
        {
            if (event.text.unicode ==
                '\b') { if (!m_SettingsInputText.empty()) m_SettingsInputText.pop_back(); } else if (
                event.text.unicode == '\r' || event.text.unicode == '\n')
            {
                try
                {
                    float v = std::stof(m_SettingsInputText);
                    if (m_ActiveSettingsField == SettingsField::AutoSaveInterval)
                        m_AutoSaveIntervalSeconds = std::max(10.f, v);
                    else if (m_ActiveSettingsField == SettingsField::AutoSavePopupDuration)
                        m_AutoSavePopupDuration = std::clamp(v, 1.f, 60.f);
                    else if (m_ActiveSettingsField == SettingsField::GridSize)
                        m_GridSize = m_DefaultObjectSize = std::clamp(v, 8.f, 256.f);
                    else if (m_ActiveSettingsField == SettingsField::DefaultObjectSize)
                        m_DefaultObjectSize = std::clamp(v, 8.f, 512.f);
                    else if (m_ActiveSettingsField == SettingsField::SelectionThickness)
                        m_SelectionOutlineThickness = std::clamp(v, 0.f, 8.f);
                    else if (m_ActiveSettingsField == SettingsField::GridOpacityVal)
                        m_GridOpacity = std::clamp(static_cast<int>(v), 0, 255);
                    else if (m_ActiveSettingsField == SettingsField::ZoomSensitivity)
                        m_ZoomSensitivity = std::clamp(v, 0.01f, 0.5f);
                    else if (m_ActiveSettingsField == SettingsField::ZoomMin)
                        m_ZoomMin = std::clamp(v, 0.05f, 1.f);
                    else if (m_ActiveSettingsField == SettingsField::ZoomMax)
                        m_ZoomMax = std::clamp(v, 1.f, 20.f);
                    else if (m_ActiveSettingsField == SettingsField::ScrollSensitivity)
                        m_ScrollSensitivity = std::clamp(v, 1.f, 100.f);
                } catch (...) {}

                if (m_ActiveSettingsField == SettingsField::SceneSavePath)
                    m_SceneSavePath = m_SettingsInputText;

                m_ActiveSettingsField = SettingsField::None;
                m_SettingsInputText.clear();
                SaveSettings();
            } else if (event.text.unicode == 27)
            {
                m_ActiveSettingsField = SettingsField::None;
                m_SettingsInputText.clear();
            } else if (event.text.unicode < 128) { m_SettingsInputText += static_cast<char>(event.text.unicode); }
            return;
        }

        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
        {
            if (m_ActiveSettingsField != SettingsField::None)
            {
                m_ActiveSettingsField = SettingsField::None;
                m_SettingsInputText.clear();
            } else { m_ShowSettings = false; }
            return;
        }

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
        {
            HandleSettingsClick({(float) event.mouseButton.x, (float) event.mouseButton.y});
            return;
        }

        return;
    }

    if (event.type == sf::Event::MouseMoved)
    {
        m_MouseScreenPos = {(float) event.mouseMove.x, (float) event.mouseMove.y};

        if (m_IsSelectingText &&m_ActiveField != EditField::None) {
            float textStartX = m_ActiveInputBounds.left + 5.f;
            float localX = m_MouseScreenPos.x - textStartX;
            sf::Text t(m_ActiveInputText, *m_Font, 12);
            int bestIdx = 0;
            float bestDist = 1e9f;
            for (int i = 0; i <= (int) m_ActiveInputText.size(); ++i)
            {
                float cx = t.findCharacterPos(i).x;
                float d = std::abs(localX - cx);
                if (d < bestDist)
                {
                    bestDist = d;
                    bestIdx = i;
                }
            }
            m_InputSelectionEnd = bestIdx;
        }

        if (m_panning)
        {
            int dx = event.mouseMove.x - m_PanMouseStartPos.x;
            int dy = event.mouseMove.y - m_PanMouseStartPos.y;
            if (dx * dx + dy * dy > 16)
            {
                m_HasPanned = true;
            }
            sf::Vector2f current = m_Window.mapPixelToCoords(
                {event.mouseMove.x, event.mouseMove.y}, m_camera);
            m_camera.move(m_panStart - current);
        }

        if (m_MouseScreenPos.y > TopBarHeight && m_PlacementActive)
        {
            sf::Vector2f pos = MouseWorldPos();
            if (IsPolygonType(m_PlacementType))
            {
                m_CirclePreview.setPointCount(GetPolygonPointCount(m_PlacementType));
                m_CirclePreview.setRadius(m_GridSize * 0.5f);
                m_CirclePreview.setFillColor(sf::Color(255, 255, 255, 70));
                m_CirclePreview.setPosition(SnapToGrid(pos));
            } else if (m_PlacementType == ObjectType::PhysicsBall)
            {
                m_CirclePreview.setPointCount(30);
                m_CirclePreview.setRadius(32.f);
                m_CirclePreview.setFillColor(sf::Color(220, 80, 80, 90));
                m_CirclePreview.setPosition(SnapToGrid(pos));
            } else if (m_PlacementType == ObjectType::Camera)
            {
                m_Preview.setSize({64.f, 48.f});
                m_Preview.setFillColor(sf::Color(64, 160, 216, 100));
                m_Preview.setPosition(SnapToGrid(pos));
            } else if (m_PlacementType == ObjectType::Empty)
            {
                m_Preview.setSize({32.f, 32.f});
                m_Preview.setFillColor(sf::Color(180, 180, 180, 70));
                m_Preview.setPosition(SnapToGrid(pos));
            } else if (m_PlacementType == ObjectType::SpawnPoint)
            {
                m_Preview.setSize({48.f, 48.f});
                m_Preview.setFillColor(sf::Color(255, 200, 60, 90));
                m_Preview.setPosition(SnapToGrid(pos));
            } else if (m_PlacementType == ObjectType::TriggerZone)
            {
                m_Preview.setSize({120.f, 80.f});
                m_Preview.setFillColor(sf::Color(40, 200, 80, 70));
                m_Preview.setPosition(SnapToGrid(pos));
            } else if (m_PlacementType == ObjectType::PhysicsBox)
            {
                m_Preview.setSize({64.f, 64.f});
                m_Preview.setFillColor(sf::Color(210, 140, 70, 90));
                m_Preview.setPosition(SnapToGrid(pos));
            } else if (m_PlacementType == ObjectType::StaticPlatform)
            {
                m_Preview.setSize({240.f, 32.f});
                m_Preview.setFillColor(sf::Color(100, 110, 125, 110));
                m_Preview.setPosition(SnapToGrid(pos));
            } else if (m_PlacementType == ObjectType::WorldText)
            {
                m_Preview.setSize({160.f, 36.f});
                m_Preview.setFillColor(sf::Color(100, 220, 255, 70));
                m_Preview.setPosition(SnapToGrid(pos));
            } else if (m_PlacementType == ObjectType::AudioSource)
            {
                m_Preview.setSize({48.f, 48.f});
                m_Preview.setFillColor(sf::Color(160, 100, 240, 90));
                m_Preview.setPosition(SnapToGrid(pos));
            } else if (m_PlacementType == ObjectType::ParticleEmitter)
            {
                m_Preview.setSize({48.f, 48.f});
                m_Preview.setFillColor(sf::Color(255, 150, 40, 90));
                m_Preview.setPosition(SnapToGrid(pos));
            } else
            {
                m_Preview.setSize({m_GridSize, m_GridSize});
                m_Preview.setFillColor(sf::Color(255, 255, 255, 50));
                m_Preview.setPosition(SnapToGrid(pos));
            }
        }

        if (m_HierarchyPotentialDrag)
        {
            float dist = std::hypot(m_MouseScreenPos.x - m_HierarchyDragStartPos.x,
                                    m_MouseScreenPos.y - m_HierarchyDragStartPos.y);
            if (dist > 5.f)
            {
                m_HierarchyDragging = true;
                m_HierarchyPotentialDrag = false;
            }
        }

        if (m_HierarchyDragging)
        {
            m_HierarchyDragTargetId.clear();
            for (auto &[rect, obj]: m_HierarchyHitboxes)
            {
                if (rect.contains(m_MouseScreenPos))
                {
                    if (obj && obj->id != m_HierarchyDragSourceId && !IsDescendantOf(obj->id, m_HierarchyDragSourceId))
                    {
                        m_HierarchyDragTargetId = obj->id;
                    }
                    break;
                }
            }
        }

        if (m_Dragging)
        {
            sf::Vector2f primaryPos = SnapToGrid(MouseWorldPos() - m_DragOffset);
            if (m_Selected)
            {
                sf::Vector2f delta = primaryPos - m_Selected->worldPosition;
                for (auto *obj: m_SelectedObjects)
                {
                    bool parentSelected = false;
                    for (auto *other: m_SelectedObjects)
                    {
                        if (other != obj && IsDescendantOf(obj->id, other->id))
                        {
                            parentSelected = true;
                            break;
                        }
                    }
                    if (!parentSelected)
                    {
                        if (obj->parentId.empty() || ObjectById(obj->parentId) == nullptr)
                        {
                            obj->localPosition += delta;
                        } else
                        {
                            auto *p = ObjectById(obj->parentId);
                            sf::Transform pTr;
                            pTr.translate(p->worldPosition);
                            pTr.rotate(p->worldRotation);
                            pTr.scale(p->worldScaleX, p->worldScaleY);
                            obj->localPosition = pTr.getInverse().transformPoint(obj->worldPosition + delta);
                        }
                    }
                }
                UpdateWorldTransforms();
            }
        }

        if (m_Resizing && m_Selected)
        {
            sf::Vector2f mouseWorld = MouseWorldPos();
            sf::Vector2f deltaWorld = mouseWorld - m_ResizeMouseStart;
            sf::Vector2f deltaLocal = RotatePoint(deltaWorld, {0.f, 0.f}, -m_Selected->worldRotation);

            if (std::abs(m_Selected->worldScaleX) > 0.001f) deltaLocal.x /= m_Selected->worldScaleX;
            if (std::abs(m_Selected->worldScaleY) > 0.001f) deltaLocal.y /= m_Selected->worldScaleY;

            sf::Vector2f newSize = m_ResizeObjSize;
            sf::Vector2f localPosOffset = {0.f, 0.f};
            const float minSize = 4.f;

            switch (m_ResizeHandle)
            {
                case 0:
                    localPosOffset.x = std::min(deltaLocal.x, m_ResizeObjSize.x - minSize);
                    localPosOffset.y = std::min(deltaLocal.y, m_ResizeObjSize.y - minSize);
                    newSize.x = m_ResizeObjSize.x - localPosOffset.x;
                    newSize.y = m_ResizeObjSize.y - localPosOffset.y;
                    break;
                case 1:
                    localPosOffset.y = std::min(deltaLocal.y, m_ResizeObjSize.y - minSize);
                    newSize.y = m_ResizeObjSize.y - localPosOffset.y;
                    break;
                case 2:
                    localPosOffset.y = std::min(deltaLocal.y, m_ResizeObjSize.y - minSize);
                    newSize.x = std::max(m_ResizeObjSize.x + deltaLocal.x, minSize);
                    newSize.y = m_ResizeObjSize.y - localPosOffset.y;
                    break;
                case 3:
                    localPosOffset.x = std::min(deltaLocal.x, m_ResizeObjSize.x - minSize);
                    newSize.x = m_ResizeObjSize.x - localPosOffset.x;
                    break;
                case 4:
                    newSize.x = std::max(m_ResizeObjSize.x + deltaLocal.x, minSize);
                    break;
                case 5:
                    localPosOffset.x = std::min(deltaLocal.x, m_ResizeObjSize.x - minSize);
                    newSize.x = m_ResizeObjSize.x - localPosOffset.x;
                    newSize.y = std::max(m_ResizeObjSize.y + deltaLocal.y, minSize);
                    break;
                case 6:
                    newSize.y = std::max(m_ResizeObjSize.y + deltaLocal.y, minSize);
                    break;
                case 7:
                    newSize.x = std::max(m_ResizeObjSize.x + deltaLocal.x, minSize);
                    newSize.y = std::max(m_ResizeObjSize.y + deltaLocal.y, minSize);
                    break;
                default: break;
            }

            sf::Vector2f scaledOffset = {
                localPosOffset.x * m_Selected->worldScaleX, localPosOffset.y * m_Selected->worldScaleY
            };
            sf::Vector2f worldPosOffset = RotatePoint(scaledOffset, {0.f, 0.f}, m_Selected->worldRotation);
            sf::Vector2f newPos = m_ResizeObjOrigin + worldPosOffset;

            if (m_Selected->parentId.empty() || ObjectById(m_Selected->parentId) == nullptr)
            {
                m_Selected->localPosition = newPos;
            } else
            {
                auto *p = ObjectById(m_Selected->parentId);
                sf::Transform pTr;
                pTr.translate(p->worldPosition);
                pTr.rotate(p->worldRotation);
                pTr.scale(p->worldScaleX, p->worldScaleY);
                m_Selected->localPosition = pTr.getInverse().transformPoint(newPos);
            }
            m_Selected->shape.setSize(newSize);
            UpdateWorldTransforms();
        }

        if (m_Rotating && m_Selected)
        {
            sf::Vector2f pivot = m_Selected->shape.getPosition();
            sf::Vector2f pos = MouseWorldPos();
            float currentAngle = std::atan2(pos.y - pivot.y, pos.x - pivot.x) * 180.f / 3.14159265f;

            float stepDelta = currentAngle - m_RotateMouseAngleStart;
            while (stepDelta > 180.f) stepDelta -= 360.f;
            while (stepDelta < -180.f) stepDelta += 360.f;

            m_RotateMouseAngleStart = currentAngle;
            m_RotateObjAngleStart += stepDelta;

            float newRotation = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)
                                    ? (std::round(m_RotateObjAngleStart / 15.f) * 15.f)
                                    : m_RotateObjAngleStart;

            if (m_Selected->parentId.empty() || ObjectById(m_Selected->parentId) == nullptr)
            {
                m_Selected->rotation = newRotation;
            } else
            {
                auto *p = ObjectById(m_Selected->parentId);
                m_Selected->rotation = newRotation - p->worldRotation;
            }
            UpdateWorldTransforms();
        }

        if (m_ContentBrowser->HasDraggedAsset())
            m_ContentBrowser->HandleEvent(event, m_MouseScreenPos);

        return;
    }

    if (event.type == sf::Event::MouseButtonReleased)
    {
        m_MouseScreenPos = {(float) event.mouseButton.x, (float) event.mouseButton.y};
        if (event.mouseButton.button == sf::Mouse::Left)
        {
            m_IsSelectingText = false;
            if (m_InputSelectionStart == m_InputSelectionEnd)
            {
                m_InputSelectionStart = -1;
                m_InputSelectionEnd = -1;
            }
        }
    }

    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F3)
    {
        Profiler::Get().ToggleHud();
    }

    const bool inTopBars = m_MouseScreenPos.y < TopBarHeight;
    const bool inMenuBar = m_MouseScreenPos.y < MenuBarHeight;
    const bool inBrowser = m_BrowserBounds.contains(m_MouseScreenPos);
    const bool inInspector = m_InspectorBounds.contains(m_MouseScreenPos);
    const bool inTabs = m_TabBrowserBounds.contains(m_MouseScreenPos) ||
                        m_TabConsoleBounds.contains(m_MouseScreenPos) ||
                        m_TabProfilerBounds.contains(m_MouseScreenPos);

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
    {
        if (m_ActiveField != EditField::None && m_ActiveInputBounds.contains(m_MouseScreenPos))
        {
            float textStartX = m_ActiveInputBounds.left + 5.f;
            float localX = m_MouseScreenPos.x - textStartX;
            sf::Text t(m_ActiveInputText, *m_Font, 12);
            int bestIdx = 0;
            float bestDist = 1e9f;
            for (int i = 0; i <= (int) m_ActiveInputText.size(); ++i)
            {
                float cx = t.findCharacterPos(i).x;
                float d = std::abs(localX - cx);
                if (d < bestDist)
                {
                    bestDist = d;
                    bestIdx = i;
                }
            }
            m_InputSelectionStart = bestIdx;
            m_InputSelectionEnd = bestIdx;
            m_IsSelectingText = true;
            return;
        }

        if (m_TabBrowserBounds.contains(m_MouseScreenPos))
        {
            m_ActiveBottomPanelTab = BottomPanelTab::ContentBrowser;
            return;
        } else if (m_TabConsoleBounds.contains(m_MouseScreenPos))
        {
            m_ActiveBottomPanelTab = BottomPanelTab::Console;
            return;
        } else if (m_TabProfilerBounds.contains(m_MouseScreenPos))
        {
            m_ActiveBottomPanelTab = BottomPanelTab::Profiler;
            return;
        }
    }

    if (event.type == sf::Event::MouseButtonReleased &&
        event.mouseButton.button == sf::Mouse::Left &&
        m_ContentBrowser->HasDraggedAsset())
    {
        DraggedAsset drag = m_ContentBrowser->GetDraggedAsset();

        m_ContentBrowser->ClearDrag();

        if (inBrowser)
        {
            std::string targetFolder = m_ContentBrowser->GetDropTargetFolder(m_MouseScreenPos);
            if (!targetFolder.empty())
            {
                m_ContentBrowser->MoveAsset(drag.path, targetFolder);
            }
            return;
        }

        const bool inHierarchy = m_HierarchyBounds.contains(m_MouseScreenPos);

        if (drag.type == AssetType::Image)
        {
            if (inInspector || inHierarchy)
            {
                EditorObject *dropTarget = inInspector ? GetInspectedObject() : m_Selected;
                if (dropTarget)
                {
                    bool droppedOnImageProp = false;
                    if (inInspector)
                    {
                        for (const auto &btn: m_InspectorButtons)
                        {
                            if (btn.bounds.contains(m_MouseScreenPos) && btn.action.rfind("edit_script_prop_", 0) == 0)
                            {
                                std::string propName = btn.action.substr(17);
                                auto it = dropTarget->scriptProperties.find(propName);
                                if (it != dropTarget->scriptProperties.end() && it->second.type ==
                                    ScriptComponent::PropertyType::Image)
                                {
                                    it->second.stringVal = drag.path;
                                    if (dropTarget->entity != 0 && m_Registry.HasComponent<ScriptComponent>(
                                            dropTarget->entity))
                                        m_Registry.GetComponent<ScriptComponent>(dropTarget->entity).
                                                SetExportedProperty(it->second);
                                    SetDirty(true);
                                    droppedOnImageProp = true;
                                    break;
                                }
                            }
                        }

                        if (!droppedOnImageProp)
                        {
                            for (auto &pair: dropTarget->scriptProperties)
                            {
                                if (pair.second.type == ScriptComponent::PropertyType::Image)
                                {
                                    pair.second.stringVal = drag.path;
                                    if (dropTarget->entity != 0 && m_Registry.HasComponent<ScriptComponent>(
                                            dropTarget->entity))
                                        m_Registry.GetComponent<ScriptComponent>(dropTarget->entity).
                                                SetExportedProperty(pair.second);
                                    SetDirty(true);
                                    droppedOnImageProp = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (!droppedOnImageProp)
                        ApplySpriteToObject(*dropTarget, drag.path);
                }
            } else if (!inBrowser && !inTopBars && !inTabs)
            {
                EditorObject *hit = ObjectAt(MouseWorldPos());
                if (hit)
                    ApplySpriteToObject(*hit, drag.path);
                else
                {
                    m_Selected = nullptr;
                    AddObjectWithSprite(MouseWorldPos(), drag.path);
                }
            }
        } else if (drag.type == AssetType::Script)
        {
            if (!inBrowser && !inTopBars && !inTabs)
            {
                EditorObject *target = nullptr;
                if (inInspector)
                    target = GetInspectedObject();
                else if (inHierarchy)
                    target = m_Selected;
                else
                    target = ObjectAt(MouseWorldPos());

                if (target && target->entity != 0 && !m_Registry.HasComponent<ScriptComponent>(target->entity))
                {
                    auto &sc = m_Registry.AddComponent(target->entity, ScriptComponent(LuaState::GetLua(), drag.path, target->entity));
                    sc.SetEntity(target->entity);
                    target->scriptPath = drag.path;

                    for (const auto &prop: sc.GetExportedProperties()) { target->scriptProperties[prop.name] = prop; }
                }
            }
        } else if (drag.type == AssetType::Template)
        {
            EditorObject *dropTarget = inInspector ? GetInspectedObject() : m_Selected;
            if (inInspector && dropTarget)
            {
                bool assigned = false;
                for (const auto &btn: m_InspectorButtons)
                {
                    if (btn.bounds.contains(m_MouseScreenPos) && btn.action.rfind("edit_script_prop_", 0) == 0)
                    {
                        std::string propName = btn.action.substr(17);
                        auto it = dropTarget->scriptProperties.find(propName);
                        if (it != dropTarget->scriptProperties.end() && it->second.type ==
                            ScriptComponent::PropertyType::Template)
                        {
                            it->second.stringVal = drag.path;
                            if (dropTarget->entity != 0 && m_Registry.HasComponent<ScriptComponent>(dropTarget->entity))
                            {
                                m_Registry.GetComponent<ScriptComponent>(dropTarget->entity).SetExportedProperty(
                                    it->second);
                            }
                            SetDirty(true);
                            assigned = true;
                            std::cout << "[INFO] [ContentBrowser] Assigned template to property " << propName << "\n";
                            break;
                        }
                    }
                }

                if (!assigned)
                {
                    for (auto &pair: dropTarget->scriptProperties)
                    {
                        if (pair.second.type == ScriptComponent::PropertyType::Template)
                        {
                            pair.second.stringVal = drag.path;
                            if (dropTarget->entity != 0 && m_Registry.HasComponent<ScriptComponent>(dropTarget->entity))
                            {
                                m_Registry.GetComponent<ScriptComponent>(dropTarget->entity).SetExportedProperty(
                                    pair.second);
                            }
                            SetDirty(true);
                            std::cout << "[INFO] [ContentBrowser] Assigned template to property " << pair.first << "\n";
                            break;
                        }
                    }
                }
            } else if (!inBrowser && !inTopBars && !inTabs)
            {
                sf::Vector2f dropPos = MouseWorldPos();
                InstantiateTemplateOnCanvas(drag.path, dropPos);
            }
        } else if (drag.type == AssetType::Audio)
        {
            if (inInspector)
            {
                EditorObject *dropTarget = GetInspectedObject();
                if (dropTarget)
                {
                    dropTarget->audioClipPath = drag.path;
                    if (dropTarget->entity != 0)
                    {
                        if (m_Registry.HasComponent<AudioSourceComponent>(dropTarget->entity))
                            m_Registry.GetComponent<AudioSourceComponent>(dropTarget->entity).soundPath = drag.path;
                        else
                        {
                            AudioSourceComponent ac;
                            ac.soundPath = drag.path;
                            m_Registry.AddComponent(dropTarget->entity, ac);
                        }
                    }
                    SetDirty(true);
                    std::cout << "[INFO] [ContentBrowser] Assigned audio " << drag.path << " to " << dropTarget->id <<
                            "\n";
                }
            } else if (!inBrowser && !inTopBars && !inTabs)
            {
                EditorObject *hit = ObjectAt(MouseWorldPos());
                if (hit)
                {
                    hit->audioClipPath = drag.path;
                    if (hit->entity != 0)
                    {
                        if (m_Registry.HasComponent<AudioSourceComponent>(hit->entity))
                            m_Registry.GetComponent<AudioSourceComponent>(hit->entity).soundPath = drag.path;
                        else
                        {
                            AudioSourceComponent ac;
                            ac.soundPath = drag.path;
                            m_Registry.AddComponent(hit->entity, ac);
                        }
                    }
                    SetDirty(true);
                } else
                {
                    m_Selected = nullptr;
                    AddObject(MouseWorldPos(), ObjectType::AudioSource);
                    if (!m_Objects.empty())
                    {
                        m_Objects.back().audioClipPath = drag.path;
                        if (m_Objects.back().entity != 0 && m_Registry.HasComponent<AudioSourceComponent>(
                                m_Objects.back().entity))
                            m_Registry.GetComponent<AudioSourceComponent>(m_Objects.back().entity).soundPath = drag.
                                    path;
                    }
                }
            }
        }

        return;
    }

    if (m_ActiveBottomPanelTab == BottomPanelTab::Console)
    {
        if (inBrowser || m_ConsolePanel->IsInputActive()) { m_ConsolePanel->HandleEvent(event, m_MouseScreenPos); }
    } else if (m_ActiveBottomPanelTab == BottomPanelTab::Profiler)
    {
        if (inBrowser) { m_ProfilerPanel->HandleEvent(event, m_MouseScreenPos); }
    } else
    {
        if (inBrowser || m_ContentBrowser->HasDraggedAsset() ||
            m_ContentBrowser->IsInputActive() || m_ContentBrowser->IsContextMenuOpen())
        {
            std::string prevSel = m_ContentBrowser->GetSelectedPath();
            m_ContentBrowser->HandleEvent(event, m_MouseScreenPos);

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
            {
                if (m_ContentBrowser->GetSelectedPath() != prevSel && !m_ContentBrowser->GetSelectedPath().empty())
                {
                    if (m_Selected) m_Selected->selected = false;
                    m_Selected = nullptr;
                    UpdateStatusText();
                }
            }
        }
    }

    if ((m_ContentBrowser->IsInputActive() || m_ConsolePanel->IsInputActive()) &&
        (event.type == sf::Event::TextEntered || event.type == sf::Event::KeyPressed))
        return;

    if (event.type == sf::Event::TextEntered && m_ActiveField != EditField::None)
    {
        if (event.text.unicode == '\b')
        {
            if (HasTextSelection()) DeleteActiveSelection();
            else if (!m_ActiveInputText.empty()) m_ActiveInputText.pop_back();
        } else if (
            event.text.unicode == '\r' || event.text.unicode == '\n') { CommitActiveField(); } else if (
            event.text.unicode >= 32 && event.text.unicode < 128)
        {
            char c = static_cast<char>(event.text.unicode);
            if (HasTextSelection()) DeleteActiveSelection();

            if (m_ActiveField == EditField::Name || m_ActiveField == EditField::Tag ||
                m_ActiveField == EditField::Script || m_ActiveField ==
                EditField::UIText) { m_ActiveInputText += c; } else if (m_ActiveField == EditField::ZIndex)
            {
                if (std::isdigit(c) || (c == '-' && m_ActiveInputText.empty()))
                    m_ActiveInputText += c;
            } else if (m_ActiveField == EditField::ScriptProperty)
            {
                ScriptComponent::PropertyType pType = ScriptComponent::PropertyType::String;
                if (m_Selected)
                {
                    auto it = m_Selected->scriptProperties.find(m_ActiveScriptProperty);
                    if (it != m_Selected->scriptProperties.end())
                        pType = it->second.type;
                }

                if (pType == ScriptComponent::PropertyType::Int)
                {
                    if (std::isdigit(c) || (c == '-' && m_ActiveInputText.empty()))
                        m_ActiveInputText += c;
                } else if (pType == ScriptComponent::PropertyType::Float)
                {
                    if (std::isdigit(c) || (c == '-' && m_ActiveInputText.empty()) || (
                            c == '.' && m_ActiveInputText.find('.') == std::string::npos))
                        m_ActiveInputText += c;
                } else { m_ActiveInputText += c; }
            } else if (m_ActiveField == EditField::ColorR || m_ActiveField == EditField::ColorG ||
                       m_ActiveField == EditField::ColorB || m_ActiveField == EditField::CollisionChannel)
            {
                if (std::isdigit(c))
                    m_ActiveInputText += c;
            } else if (std::isdigit(c) || (c == '-' && m_ActiveInputText.empty())) { m_ActiveInputText += c; } else if (
                c == '.' && m_ActiveInputText.find('.') == std::string::npos) { m_ActiveInputText += c; }
        }
        return;
    }

    if (event.type == sf::Event::Resized)
    {
        const float nW = static_cast<float>(event.size.width);
        const float nH = static_cast<float>(event.size.height);
        sf::FloatRect vp = m_camera.getViewport();
        vp.left = HierarchyWidth / nW;
        vp.top = TopBarHeight / nH;
        vp.width = 1.f - ((InspectorWidth + HierarchyWidth) / nW);
        vp.height = 1.f - (BrowserHeight / nH) - (TopBarHeight / nH);
        m_camera.setViewport(vp);
        UpdateBounds();
    }

    const bool inHierarchy = m_HierarchyBounds.contains(m_MouseScreenPos);

    if (event.type == sf::Event::MouseWheelScrolled)
    {
        if (inHierarchy)
        {
            m_HierarchyScrollY -= event.mouseWheelScroll.delta * m_ScrollSensitivity;
            if (m_HierarchyScrollY < 0.f) m_HierarchyScrollY = 0.f;
        } else if (m_InspectorBounds.contains(m_MouseScreenPos))
        {
            m_InspectorScrollY -= event.mouseWheelScroll.delta * 30.f;
            float maxScroll = std::max(0.f, m_InspectorContentHeight - (m_InspectorBounds.height - 42.f));
            m_InspectorScrollY = std::max(0.f, std::min(m_InspectorScrollY, maxScroll));
        } else if (!inTopBars && !inInspector && !inBrowser)
        {
            float delta = m_InvertPan ? -event.mouseWheelScroll.delta : event.mouseWheelScroll.delta;
            const float factor = delta > 0 ? (1.f - m_ZoomSensitivity) : (1.f + m_ZoomSensitivity);

            const float currentW = m_camera.getSize().x;
            const float newW = currentW * factor;
            constexpr float MinZoomSize = 80.f;
            constexpr float MaxZoomSize = 12000.f;

            if (newW >= MinZoomSize && newW <= MaxZoomSize)
                m_camera.zoom(factor);
        }
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        (event.mouseButton.button == sf::Mouse::Middle || event.mouseButton.button == sf::Mouse::Right))
    {
        if (event.mouseButton.button == sf::Mouse::Right && inHierarchy)
        {
            m_HierarchyContextMenuOpen = false;
            for (auto &[rect, obj]: m_HierarchyHitboxes)
            {
                if (rect.contains(m_MouseScreenPos))
                {
                    m_ContextObject = obj;
                    m_HierarchyContextMenuOpen = true;
                    m_ContextMenuPos = m_MouseScreenPos;
                    if (m_Selected) m_Selected->selected = false;
                    m_Selected = obj;
                    m_Selected->selected = true;
                    UpdateStatusText();
                    break;
                }
            }
        } else if (!inHierarchy && !inInspector && !inBrowser && !inTopBars)
        {
            m_panning = true;
            m_HasPanned = false;
            m_PanMouseStartPos = sf::Mouse::getPosition(m_Window);
            m_panStart = m_Window.mapPixelToCoords(m_PanMouseStartPos, m_camera);
        }
    }

    if (event.type == sf::Event::MouseButtonReleased &&
        (event.mouseButton.button == sf::Mouse::Middle || event.mouseButton.button == sf::Mouse::Right))
    {
        if (event.mouseButton.button == sf::Mouse::Right)
        {
            int dx = event.mouseButton.x - m_PanMouseStartPos.x;
            int dy = event.mouseButton.y - m_PanMouseStartPos.y;
            bool moved = m_HasPanned || (dx * dx + dy * dy > 16);
            if (!moved && m_PlacementActive && !inHierarchy && !inInspector && !inBrowser && !inTopBars)
            {
                m_PlacementActive = false;
                UpdateStatusText();
                std::cout << "[INFO] [EditorScene] Cancelled placement via stationary right click (Select Mode)\n";
            }
        }
        m_panning = false;
        m_HasPanned = false;
    }

    if (event.type == sf::Event::MouseButtonReleased &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        if (m_HierarchyDragging)
        {
            bool droppedOnEntityProp = false;
            if (m_InspectorBounds.contains(m_MouseScreenPos))
            {
                EditorObject *inspTarget = GetInspectedObject();
                if (inspTarget)
                {
                    for (const auto &btn: m_InspectorButtons)
                    {
                        if (btn.bounds.contains(m_MouseScreenPos) && btn.action.rfind("edit_script_prop_", 0) == 0)
                        {
                            std::string propName = btn.action.substr(17);
                            auto it = inspTarget->scriptProperties.find(propName);
                            if (it != inspTarget->scriptProperties.end() && it->second.type ==
                                ScriptComponent::PropertyType::Entity)
                            {
                                it->second.stringVal = m_HierarchyDragSourceId;
                                if (inspTarget->entity != 0 && m_Registry.HasComponent<
                                        ScriptComponent>(inspTarget->entity))
                                {
                                    m_Registry.GetComponent<ScriptComponent>(inspTarget->entity).SetExportedProperty(
                                        it->second);
                                }
                                SetDirty(true);
                                droppedOnEntityProp = true;
                                std::cout << "[INFO] [Hierarchy] Assigned entity " << m_HierarchyDragSourceId <<
                                        " to property " << propName << "\n";
                                break;
                            }
                        }
                    }
                }
            }
            if (!droppedOnEntityProp)
            {
                if (!m_HierarchyDragTargetId.empty())
                {
                    SetParent(m_HierarchyDragSourceId, m_HierarchyDragTargetId, true);
                } else if (m_HierarchyRootDropZone.contains(m_MouseScreenPos))
                {
                    SetParent(m_HierarchyDragSourceId, "", true);
                }
            }
            m_HierarchyDragging = false;
            m_HierarchyDragSourceId.clear();
            m_HierarchyDragTargetId.clear();
        }
        m_HierarchyPotentialDrag = false;

        if (m_ShowSettings) { SaveSettings(); }

        if (m_Dragging && !m_SelectedObjects.empty())
        {
            auto macroCmd = std::make_shared<MacroCommand>();
            bool movedAny = false;

            for (auto *obj: m_SelectedObjects)
            {
                if (obj->entity != 0 && m_Registry.HasComponent<TransformComponent>(obj->entity))
                {
                    auto &t = m_Registry.GetComponent<TransformComponent>(obj->entity);
                    t.x = obj->localPosition.x;
                    t.y = obj->localPosition.y;
                    t.worldX = obj->worldPosition.x;
                    t.worldY = obj->worldPosition.y;

                    json after = SerializeObject(*obj);
                    json before = m_DragBeforeStates[obj->id];
                    if (before != after)
                    {
                        macroCmd->commands.push_back(std::make_shared<ObjectStateCommand>(obj->id, before, after));
                        movedAny = true;
                    }
                }
            }
            if (movedAny)
            {
                m_UndoStack.push_back(macroCmd);
                m_RedoStack.clear();
                SetDirty(true);
            }
        }
        m_Dragging = false;

        if (m_BoxSelecting)
        {
            sf::Vector2f pos = MouseWorldPos();
            sf::Vector2f diff = pos - m_BoxSelectStart;
            if (std::abs(diff.x) < 2.f && std::abs(diff.y) < 2.f)
            {
                if (m_PlacementActive)
                {
                    AddObject(pos, m_PlacementType);
                }
                else
                {
                    ClearSelection();
                }
            } else
            {
                sf::FloatRect selectRect(
                    std::min(m_BoxSelectStart.x, pos.x),
                    std::min(m_BoxSelectStart.y, pos.y),
                    std::abs(diff.x),
                    std::abs(diff.y)
                );

                bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl);
                bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift);
                if (!ctrl && !shift) ClearSelection();

                for (auto &obj: m_Objects)
                {
                    if (obj.shape.getGlobalBounds().intersects(selectRect)) { SelectObject(&obj, true); }
                }
            }
            m_BoxSelecting = false;
        }

        if (m_Resizing && m_Selected &&m_Selected->entity != 0) {
            const sf::Vector2f newSize = m_Selected->shape.getSize();

            if (m_Registry.HasComponent<TransformComponent>(m_Selected->entity))
            {
                auto &t = m_Registry.GetComponent<TransformComponent>(m_Selected->entity);
                t.x = m_Selected->localPosition.x;
                t.y = m_Selected->localPosition.y;
                t.worldX = m_Selected->worldPosition.x;
                t.worldY = m_Selected->worldPosition.y;
            }

            if (m_Registry.HasComponent<RenderComponent>(m_Selected->entity))
            {
                auto &rc = m_Registry.GetComponent<RenderComponent>(m_Selected->entity);
                rc.size = newSize;
            }

            if (m_Selected->previewTexture && m_Registry.HasComponent<SpriteComponent>(m_Selected->entity))
            {
                m_Registry.GetComponent<SpriteComponent>(m_Selected->entity) =
                        SpriteComponent(m_Selected->spritePath, newSize);
            }

            json after = SerializeObject(*m_Selected);
            json before = m_DragBeforeStates[m_Selected->id];
            if (before != after)
            {
                auto cmd = std::make_shared<ObjectStateCommand>(m_Selected->id, before, after);
                m_UndoStack.push_back(cmd);
                m_RedoStack.clear();
                SetDirty(true);
            }
        }
        m_Resizing = false;
        m_ResizeHandle = -1;

        if (m_Rotating && m_Selected &&m_Selected->entity != 0) {
            if (m_Registry.HasComponent<TransformComponent>(m_Selected->entity))
            {
                auto &t = m_Registry.GetComponent<TransformComponent>(m_Selected->entity);
                t.rotation = m_Selected->rotation;
                t.worldRotation = m_Selected->worldRotation;
            }

            json after = SerializeObject(*m_Selected);
            json before = m_DragBeforeStates[m_Selected->id];
            if (before != after)
            {
                auto cmd = std::make_shared<ObjectStateCommand>(m_Selected->id, before, after);
                m_UndoStack.push_back(cmd);
                m_RedoStack.clear();
                SetDirty(true);
            }
        }
        m_Rotating = false;
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        if (m_HierarchyContextMenuOpen)
        {
            m_HierarchyContextMenuOpen = false;
            bool hitContext = false;
            for (auto &[rect, action]: m_ContextHitboxes)
            {
                if (rect.contains(m_MouseScreenPos))
                {
                    hitContext = true;
                    if (action == "rename" && m_ContextObject)
                    {
                        SelectObject(m_ContextObject, false);
                        m_ActiveField = EditField::Name;
                        m_ActiveInputText = m_ContextObject->id;
                    } else if (action == "delete" && m_ContextObject)
                    {
                        SelectObject(m_ContextObject, false);
                        DeleteSelected();
                    } else if (action == "duplicate" && m_ContextObject)
                    {
                        json j = SerializeObject(*m_ContextObject);
                        std::string newId = NextId();
                        j["id"] = newId;
                        j.erase("entity");
                        j["x"] = j.value("x", 0.f) + 20.f;
                        j["y"] = j.value("y", 0.f) + 20.f;
                        DeserializeObject(j);
                        EditorObject *newObj = ObjectById(newId);
                        if (newObj)
                        {
                            SelectObject(newObj, false);
                            auto cmd = std::make_shared<ObjectStateCommand>(
                                newId, json(nullptr), SerializeObject(*newObj));
                            m_UndoStack.push_back(cmd);
                            m_RedoStack.clear();
                            SetDirty(true);
                        }
                    } else if (action == "edit_template" && m_ContextObject && !m_ContextObject->templatePath.empty())
                    {
                        EnterTemplateEditMode(m_ContextObject->templatePath);
                    } else if (action == "save_template" && m_ContextObject) { SaveAsTemplate(m_ContextObject, ""); }
                    break;
                }
            }
            if (hitContext) return;
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
            bool onHeader = false;
            for (int i = 0; i < (int) m_Menus.size(); i++)
            {
                if (m_Menus[i].bounds.contains(m_MouseScreenPos))
                {
                    m_OpenMenuIndex = (m_OpenMenuIndex == i) ? -1 : i;
                    onHeader = true;
                    break;
                }
            }
            if (!onHeader) m_OpenMenuIndex = -1;
            return;
        }

        if (m_AddDropdownOpen)
        {
            for (auto &[r, a]: m_AddDropdownHitboxes)
            {
                if (r.contains(m_MouseScreenPos))
                {
                    if (a == "tool_select" || a == "clear_placement")
                    {
                        m_PlacementActive = false;
                    }
                    else if (a == "add_rect")
                    {
                        if (m_PlacementActive && m_PlacementType == ObjectType::Rectangle) m_PlacementActive = false;
                        else { m_PlacementActive = true; m_PlacementType = ObjectType::Rectangle; }
                    }
                    else if (a == "add_circle")
                    {
                        if (m_PlacementActive && m_PlacementType == ObjectType::Circle) m_PlacementActive = false;
                        else { m_PlacementActive = true; m_PlacementType = ObjectType::Circle; }
                    }
                    else if (a == "add_triangle")
                    {
                        if (m_PlacementActive && m_PlacementType == ObjectType::Triangle) m_PlacementActive = false;
                        else { m_PlacementActive = true; m_PlacementType = ObjectType::Triangle; }
                    }
                    else if (a == "add_pentagon")
                    {
                        if (m_PlacementActive && m_PlacementType == ObjectType::Pentagon) m_PlacementActive = false;
                        else { m_PlacementActive = true; m_PlacementType = ObjectType::Pentagon; }
                    }
                    else if (a == "add_hexagon")
                    {
                        if (m_PlacementActive && m_PlacementType == ObjectType::Hexagon) m_PlacementActive = false;
                        else { m_PlacementActive = true; m_PlacementType = ObjectType::Hexagon; }
                    }
                    else if (a == "add_cam_obj")
                    {
                        if (m_PlacementActive && m_PlacementType == ObjectType::Camera) m_PlacementActive = false;
                        else { m_PlacementActive = true; m_PlacementType = ObjectType::Camera; }
                    }
                    UpdateStatusText();
                    m_AddDropdownOpen = false;
                    return;
                }
            }
            if (!m_AddBtnBounds.contains(m_MouseScreenPos))
                m_AddDropdownOpen = false;
        }

        if (inMenuBar)
        {
            bool found = false;
            for (int i = 0; i < (int) m_Menus.size(); i++)
            {
                if (m_Menus[i].bounds.contains(m_MouseScreenPos))
                {
                    m_OpenMenuIndex = (m_OpenMenuIndex == i) ? -1 : i;
                    m_AddDropdownOpen = false;
                    found = true;
                    break;
                }
            }
            if (!found) m_OpenMenuIndex = -1;
            return;
        }

        if (inTopBars)
        {
            for (auto &[r, a]: m_ToolbarHitboxes)
            {
                if (r.contains(m_MouseScreenPos))
                {
                    if (a == "add_dropdown")
                    {
                        OpenSpotlight();
                        return;
                    } else
                    {
                        HandleMenuAction(a);
                        m_AddDropdownOpen = false;
                    }
                    return;
                }
            }
            return;
        }

        if (inInspector)
        {
            HandleInspectorClick(m_MouseScreenPos);
            return;
        }

        if (inHierarchy)
        {
            for (const auto &fold: m_HierarchyFoldHitboxes)
            {
                if (fold.first.contains(m_MouseScreenPos))
                {
                    if (m_HierarchyCollapsed.count(fold.second)) { m_HierarchyCollapsed.erase(fold.second); } else
                    {
                        m_HierarchyCollapsed.insert(fold.second);
                    }
                    return;
                }
            }
            for (auto &[rect, obj]: m_HierarchyHitboxes)
            {
                if (rect.contains(m_MouseScreenPos))
                {
                    if (m_Selected) m_Selected->selected = false;
                    m_Selected = obj;
                    m_Selected->selected = true;
                    m_SelectedObjects = {obj};
                    m_PlacementActive = false;
                    UpdateStatusText();

                    m_HierarchyPotentialDrag = true;
                    m_HierarchyDragStartPos = m_MouseScreenPos;
                    m_HierarchyDragSourceId = obj->id;
                    return;
                }
            }
            return;
        }

        if (inBrowser) return;

        CommitActiveField();

        sf::Vector2f pos = MouseWorldPos();


        if (m_Selected)
        {
            const int handle = GetResizeHandle(pos);
            if (handle >= 0)
            {
                m_Resizing = true;
                m_DragBeforeStates.clear();
                m_DragBeforeStates[m_Selected->id] = SerializeObject(*m_Selected);
                m_ResizeHandle = handle;
                m_ResizeMouseStart = pos;
                m_ResizeObjOrigin = m_Selected->shape.getPosition();
                m_ResizeObjSize = m_Selected->shape.getSize();
                UpdateStatusText();
                return;
            }
            if (GetRotateHandle(pos))
            {
                m_Rotating = true;
                m_DragBeforeStates.clear();
                m_DragBeforeStates[m_Selected->id] = SerializeObject(*m_Selected);
                sf::Vector2f pivot = m_Selected->shape.getPosition();
                m_RotateMouseAngleStart = std::atan2(pos.y - pivot.y, pos.x - pivot.x) * 180.f / 3.14159265f;
                m_RotateObjAngleStart = m_Selected->rotation;
                return;
            }
        }

        EditorObject *hit = ObjectAt(pos);

        bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl);
        bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift);

        if (hit)
        {
            m_PlacementActive = false;
            if (ctrl || shift)
            {
                if (IsSelected(hit))
                {
                    hit->selected = false;
                    std::erase(m_SelectedObjects, hit);
                    m_Selected = m_SelectedObjects.empty() ? nullptr : m_SelectedObjects.back();
                } else { SelectObject(hit, true); }
            } else { if (!IsSelected(hit)) { SelectObject(hit, false); } }

            m_Dragging = true;
            m_DragBeforeStates.clear();
            for (auto *obj: m_SelectedObjects) { m_DragBeforeStates[obj->id] = SerializeObject(*obj); }
            m_DragOffset = pos - hit->worldPosition;
        } else
        {
            SelectObject(nullptr, false);
            if (!ctrl && !shift)
            {
                m_BoxSelecting = true;
                m_BoxSelectStart = pos;
            }
        }


        UpdateStatusText();
    }

    if (event.type == sf::Event::KeyPressed)
    {
        const bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(
                              sf::Keyboard::RControl);

        if (m_ActiveField != EditField::None)
        {
            if (ctrl && event.key.code == sf::Keyboard::A)
            {
                m_InputSelectionStart = 0;
                m_InputSelectionEnd = (int) m_ActiveInputText.size();
                return;
            }
            if (ctrl && event.key.code == sf::Keyboard::C && HasTextSelection())
            {
                int sMin = std::clamp(GetSelectionMin(), 0, (int) m_ActiveInputText.size());
                int sMax = std::clamp(GetSelectionMax(), 0, (int) m_ActiveInputText.size());
                sf::Clipboard::setString(m_ActiveInputText.substr(sMin, sMax - sMin));
                return;
            }
            if (ctrl && event.key.code == sf::Keyboard::V)
            {
                if (HasTextSelection()) DeleteActiveSelection();
                std::string pasteStr = sf::Clipboard::getString().toAnsiString();
                for (char c: pasteStr) { if (c >= 32 && c < 127) m_ActiveInputText += c; }
                return;
            }
            if (event.key.code == sf::Keyboard::Delete && HasTextSelection())
            {
                DeleteActiveSelection();
                return;
            }
        }

        const bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
        if (ctrl && shift && event.key.code == sf::Keyboard::T) HandleMenuAction("open_template_dialog");
        if (ctrl && event.key.code == sf::Keyboard::S) HandleMenuAction("save");
        if (ctrl && event.key.code == sf::Keyboard::L) HandleMenuAction("load");
        if (ctrl && event.key.code == sf::Keyboard::D) HandleMenuAction("duplicate");
        if (ctrl && event.key.code == sf::Keyboard::C) HandleMenuAction("copy");
        if (ctrl && event.key.code == sf::Keyboard::V) HandleMenuAction("paste");
        if (ctrl && event.key.code == sf::Keyboard::Z) UndoCommand();
        if (ctrl && event.key.code == sf::Keyboard::Y) RedoCommand();
        if (ctrl && event.key.code == sf::Keyboard::Comma) HandleMenuAction("settings");
        if (ctrl && event.key.code == sf::Keyboard::Space)
        {
            OpenSpotlight();
            return;
        }
        if (event.key.code == sf::Keyboard::G) HandleMenuAction("toggle_grid");

        if (event.key.code == sf::Keyboard::Delete)
        {
            DeleteSelected();
            UpdateStatusText();
        }

        if (event.key.code == sf::Keyboard::Escape)
        {
            if (m_OpenMenuIndex >= 0 || m_AddDropdownOpen)
            {
                m_OpenMenuIndex = -1;
                m_AddDropdownOpen = false;
                return;
            }
            if (m_ActiveField != EditField::None)
            {
                m_ActiveField = EditField::None;
                m_ActiveInputText.clear();
                m_InputSelectionStart = -1;
                m_InputSelectionEnd = -1;
                return;
            }
            if (m_PlacementActive)
            {
                m_PlacementActive = false;
                UpdateStatusText();
                std::cout << "[INFO] [EditorScene] Cleared held object (Select Mode)\n";
                return;
            }
            if (m_Selected) m_Selected->selected = false;
            m_Selected = nullptr;
            m_SelectedObjects.clear();

            UpdateStatusText();
        }

        if ((event.key.code == sf::Keyboard::Q || event.key.code == sf::Keyboard::V) && m_ActiveField == EditField::None)
        {
            if (m_PlacementActive)
            {
                m_PlacementActive = false;
                UpdateStatusText();
                std::cout << "[INFO] [EditorScene] Switched to Select / Pointer mode (no object in hand)\n";
            }
        }

        if (event.key.code == sf::Keyboard::F5) { TryLaunchPlayMode(); }
    }
}

void EditorScene::Update(float deltaTime)
{
    Profiler::Get().SetEntityCount(static_cast<int>(m_Objects.size()));
    lua_State *L = LuaState::GetLua().lua_state();
    if (L)
    {
        int kb = lua_gc(L, LUA_GCCOUNT, 0);
        Profiler::Get().SetLuaMemory(static_cast<float>(kb));
    }

    if (m_SaveFeedbackTimer > 0.f)
        m_SaveFeedbackTimer -= deltaTime;

    if (m_ImportNotificationTimer > 0.f)
    {
        m_ImportNotificationTimer -= deltaTime;
        if (m_ImportNotificationTimer < 0.f) m_ImportNotificationTimer = 0.f;
    }

    m_FrameCount++;
    float elapsed = m_FPSClock.getElapsedTime().asSeconds();
    if (elapsed >= 0.5f)
    {
        m_FPS = static_cast<float>(m_FrameCount) / elapsed;
        m_FrameCount = 0;
        m_FPSClock.restart();

        std::string sceneName = m_SceneSavePath;
        const size_t slash = sceneName.find_last_of("/\\");
        if (slash != std::string::npos) sceneName = sceneName.substr(slash + 1);
        const size_t dot = sceneName.rfind('.');
        if (dot != std::string::npos) sceneName = sceneName.substr(0, dot);

        std::string title =
                (g_App ? g_App->GetProjectName() : std::string(Rayne::DEFAULT_PROJECT_NAME))
                + ": " + sceneName + (m_HasUnsavedChanges ? "*" : "")
                + " (" + Rayne::PlatformString() + ")"
                + " - RayneEngine " + Rayne::VersionString();
        if (m_ShowAutoSaveInTitle)
        {
            int remaining = static_cast<int>(m_AutoSaveIntervalSeconds - m_AutoSaveTimer);
            title += "  |  AutoSave in " + std::to_string(remaining) + "s";
        }
        m_Window.setTitle(title);
    }

    if (m_AutoSaveEnabled)
    {
        if (m_ShowAutoSavePopup)
        {
            m_AutoSavePopupTimer -= deltaTime;
            if (m_AutoSavePopupTimer <= 0.f)
            {
                SaveToJson(std::string(ASSET_PATH) + "/" + m_SceneSavePath);
                std::cout << "[INFO] [EditorScene] AutoSaved scene\n";
                m_ShowAutoSavePopup = false;
            }
        } else
        {
            m_AutoSaveTimer += deltaTime;
            if (m_AutoSaveTimer >= m_AutoSaveIntervalSeconds)
            {
                if (m_AutoSavePopupEnabled)
                {
                    m_ShowAutoSavePopup = true;
                    m_AutoSavePopupTimer = m_AutoSavePopupDuration;
                } else
                {
                    SaveToJson(std::string(ASSET_PATH) + "/" + m_SceneSavePath);
                    std::cout << "[INFO] [EditorScene] AutoSaved scene (silent)\n";
                }
                m_AutoSaveTimer = 0.f;
            }
        }
    } else
    {
        m_ShowAutoSavePopup = false;
        m_AutoSaveTimer = 0.f;
    }

    m_Registry.ForEach<ScriptComponent>([this](Entity entity, ScriptComponent &sc) {
        if (sc.ReloadIfNeeded())
        {
            for (auto &obj: m_Objects)
            {
                if (obj.entity == entity)
                {
                    SyncExportedScriptProperties(obj, sc);

                    if (m_Selected && m_Selected->entity == entity) {
                        if (m_ActiveField == EditField::ScriptProperty)
                        {
                            m_ActiveField = EditField::None;
                            m_ActiveInputText.clear();
                        }
                    }
                    std::cout << "[INFO] [EditorScene] Preserved and refreshed exported script variables for Entity " << obj.id <<
                            "\n";
                    break;
                }
            }
        }
    });

    for (auto &obj: m_Objects)
    {
        if ((obj.objectType == ObjectType::ParticleEmitter ||
             (obj.entity != 0 && m_Registry.HasComponent<ParticleEmitterComponent>(obj.entity))) &&
            obj.particleEmitting)
        {
            obj.particleTimer += deltaTime;
            float spawnInterval = 1.0f / std::max(1.0f, obj.particleRate);
            sf::Vector2f emitterPos = obj.worldPosition + sf::Vector2f(obj.shape.getSize().x * 0.5f,
                                                                       obj.shape.getSize().y * 0.5f);

            while (obj.particleTimer >= spawnInterval)
            {
                obj.particleTimer -= spawnInterval;
                if ((int) obj.editorParticles.size() < obj.particleMaxParticles)
                {
                    Particle p;
                    p.position = emitterPos;
                    float angleRad = (obj.particleAngle + (static_cast<float>(rand() % 1000) / 1000.0f - 0.5f) * obj.
                                      particleSpread) * 3.14159265f / 180.0f;
                    float speed = obj.particleSpeed + (static_cast<float>(rand() % 1000) / 1000.0f - 0.5f) * 40.0f;
                    p.velocity = sf::Vector2f(std::cos(angleRad) * speed, std::sin(angleRad) * speed);
                    p.lifetime = 0.0f;
                    p.maxLifetime = std::max(0.1f, obj.particleLifetime);
                    p.size = obj.particleStartSize;
                    p.color = obj.particleStartColor;
                    obj.editorParticles.push_back(p);
                }
            }

            for (auto it = obj.editorParticles.begin(); it != obj.editorParticles.end();)
            {
                it->lifetime += deltaTime;
                if (it->lifetime >= it->maxLifetime) { it = obj.editorParticles.erase(it); } else
                {
                    it->velocity.x += obj.particleGravityX * deltaTime;
                    it->velocity.y += obj.particleGravityY * deltaTime;
                    it->position += it->velocity * deltaTime;
                    float t = std::min(1.0f, it->lifetime / it->maxLifetime);
                    it->size = obj.particleStartSize + (obj.particleEndSize - obj.particleStartSize) * t;
                    auto lerpC = [](sf::Uint8 a, sf::Uint8 b, float factor) -> sf::Uint8 {
                        return static_cast<sf::Uint8>(a + (b - a) * factor);
                    };
                    it->color.r = lerpC(obj.particleStartColor.r, obj.particleEndColor.r, t);
                    it->color.g = lerpC(obj.particleStartColor.g, obj.particleEndColor.g, t);
                    it->color.b = lerpC(obj.particleStartColor.b, obj.particleEndColor.b, t);
                    it->color.a = lerpC(obj.particleStartColor.a, obj.particleEndColor.a, t);
                    ++it;
                }
            }
        } else if (!obj.editorParticles.empty()) { obj.editorParticles.clear(); }
    }
}

void EditorScene::Render(sf::RenderWindow &window)
{
    window.setView(m_camera);
    DrawGrid();
    DrawWorldAxes(window);

    if (m_EditingTemplate)
    {
        sf::VertexArray crosshair(sf::Lines, 4);
        crosshair[0] = sf::Vertex(sf::Vector2f(-30.f, 0.f), sf::Color(255, 200, 80, 220));
        crosshair[1] = sf::Vertex(sf::Vector2f(30.f, 0.f), sf::Color(255, 200, 80, 220));
        crosshair[2] = sf::Vertex(sf::Vector2f(0.f, -30.f), sf::Color(255, 200, 80, 220));
        crosshair[3] = sf::Vertex(sf::Vector2f(0.f, 30.f), sf::Color(255, 200, 80, 220));
        window.draw(crosshair);

        sf::CircleShape originPoint(3.f);
        originPoint.setOrigin(3.f, 3.f);
        originPoint.setPosition(0.f, 0.f);
        originPoint.setFillColor(sf::Color(255, 200, 80));
        window.draw(originPoint);

        sf::Text originText;
        originText.setFont(*m_Font);
        originText.setCharacterSize(11);
        originText.setFillColor(sf::Color(255, 200, 80, 200));
        originText.setString("Template Origin (0,0)");
        originText.setPosition(8.f, 6.f);
        window.draw(originText);
    }

    std::vector<EditorObject *> sortedObjects;
    sortedObjects.reserve(m_Objects.size());
    for (auto &obj: m_Objects) sortedObjects.push_back(&obj);
    std::stable_sort(sortedObjects.begin(), sortedObjects.end(), [](const EditorObject *a, const EditorObject *b) {
        return a->zIndex < b->zIndex;
    });

    for (auto *pObj: sortedObjects)
    {
        auto &obj = *pObj;
        if (IsPolygonType(obj.objectType))
        {
            obj.circleShape.setPointCount(GetPolygonPointCount(obj.objectType));
            obj.circleShape.setPosition(obj.shape.getPosition());
            float rx = obj.shape.getSize().x * 0.5f;
            float ry = obj.shape.getSize().y * 0.5f;
            if (rx > 0.001f && ry > 0.001f)
            {
                obj.circleShape.setRadius(rx);
                obj.circleShape.setScale(obj.scaleX, obj.scaleY * (ry / rx));
            }
            obj.circleShape.setRotation(obj.rotation);
            obj.circleShape.setFillColor(obj.color);
            obj.circleShape.setOutlineColor(obj.selected ? m_SelectionOutlineColor : sf::Color::Transparent);
            obj.circleShape.setOutlineThickness(obj.selected ? m_SelectionOutlineThickness : 0.f);
            window.draw(obj.circleShape);
        } else if (obj.objectType == ObjectType::Camera)
        {
            obj.shape.setScale(obj.scaleX, obj.scaleY);
            obj.shape.setRotation(obj.rotation);
            obj.shape.setFillColor(sf::Color(35, 44, 58));
            obj.shape.setOutlineColor(obj.selected ? m_SelectionOutlineColor : sf::Color(64, 180, 240, 200));
            obj.shape.setOutlineThickness(obj.selected ? m_SelectionOutlineThickness : 1.5f);
            window.draw(obj.shape);

            sf::Vector2f center = obj.shape.getPosition() + sf::Vector2f(
                                      obj.shape.getSize().x * 0.5f, obj.shape.getSize().y * 0.5f);
            sf::CircleShape lens(12.f);
            lens.setOrigin(12.f, 12.f);
            lens.setPosition(center);
            lens.setFillColor(sf::Color(20, 28, 38));
            lens.setOutlineColor(sf::Color(80, 190, 250));
            lens.setOutlineThickness(1.5f);
            window.draw(lens);

            sf::CircleShape pupil(6.f);
            pupil.setOrigin(6.f, 6.f);
            pupil.setPosition(center);
            pupil.setFillColor(sf::Color(50, 150, 220, 200));
            window.draw(pupil);

            sf::RectangleShape notch({16.f, 5.f});
            notch.setPosition(obj.shape.getPosition() + sf::Vector2f(obj.shape.getSize().x * 0.5f - 8.f, -4.f));
            notch.setFillColor(sf::Color(35, 44, 58));
            notch.setOutlineColor(sf::Color(64, 180, 240, 180));
            notch.setOutlineThickness(1.f);
            window.draw(notch);

            sf::CircleShape recDot(3.f);
            recDot.setPosition(obj.shape.getPosition() + sf::Vector2f(obj.shape.getSize().x - 9.f, 4.f));
            recDot.setFillColor(sf::Color(255, 60, 60));
            window.draw(recDot);
        } else if (obj.objectType == ObjectType::Empty)
        {
            sf::Vector2f center = obj.shape.getPosition() + sf::Vector2f(
                                      obj.shape.getSize().x * 0.5f, obj.shape.getSize().y * 0.5f);
            sf::CircleShape d(12.f, 4);
            d.setOrigin(12.f, 12.f);
            d.setPosition(center);
            d.setRotation(45.f);
            d.setFillColor(sf::Color(180, 180, 180, 40));
            d.setOutlineColor(obj.selected ? m_SelectionOutlineColor : sf::Color(180, 180, 180, 180));
            d.setOutlineThickness(obj.selected ? m_SelectionOutlineThickness : 1.5f);
            window.draw(d);
            obj.shape.setScale(obj.scaleX, obj.scaleY);
            obj.shape.setOutlineColor(sf::Color::Transparent);
            obj.shape.setOutlineThickness(0.f);
        } else if (obj.objectType == ObjectType::SpawnPoint)
        {
            sf::Vector2f center = obj.shape.getPosition() + sf::Vector2f(
                                      obj.shape.getSize().x * 0.5f, obj.shape.getSize().y * 0.5f);
            sf::CircleShape beacon(22.f);
            beacon.setOrigin(22.f, 22.f);
            beacon.setPosition(center);
            beacon.setFillColor(sf::Color(255, 200, 60, 35));
            beacon.setOutlineColor(obj.selected ? m_SelectionOutlineColor : sf::Color(255, 200, 60, 200));
            beacon.setOutlineThickness(obj.selected ? m_SelectionOutlineThickness : 1.5f);
            window.draw(beacon);

            sf::CircleShape core(5.f);
            core.setOrigin(5.f, 5.f);
            core.setPosition(center);
            core.setFillColor(sf::Color(255, 200, 60));
            window.draw(core);

            sf::Text spLabel("SPAWN", *m_Font, 10);
            spLabel.setFillColor(sf::Color(255, 200, 60, 220));
            spLabel.setPosition(center.x - spLabel.getLocalBounds().width * 0.5f, center.y + 24.f);
            window.draw(spLabel);
            obj.shape.setScale(obj.scaleX, obj.scaleY);
        } else if (obj.objectType == ObjectType::TriggerZone)
        {
            obj.shape.setScale(obj.scaleX, obj.scaleY);
            obj.shape.setRotation(obj.rotation);
            obj.shape.setFillColor(sf::Color(40, 200, 80, 45));
            obj.shape.setOutlineColor(obj.selected ? m_SelectionOutlineColor : sf::Color(60, 240, 100, 180));
            obj.shape.setOutlineThickness(obj.selected ? m_SelectionOutlineThickness : 1.5f);
            window.draw(obj.shape);

            sf::Text trigText("[TRIGGER]", *m_Font, 10);
            trigText.setFillColor(sf::Color(80, 255, 120, 200));
            sf::Vector2f center = obj.shape.getPosition() + sf::Vector2f(
                                      obj.shape.getSize().x * 0.5f * obj.scaleX,
                                      obj.shape.getSize().y * 0.5f * obj.scaleY);
            trigText.setPosition(center.x - trigText.getLocalBounds().width * 0.5f, center.y - 7.f);
            window.draw(trigText);
        } else if (obj.objectType == ObjectType::WorldText || (
                       obj.entity != 0 && m_Registry.HasComponent<TextComponent>(obj.entity)))
        {
            sf::Text worldTxt(obj.textString, *m_Font, obj.textFontSize);
            worldTxt.setFillColor(obj.textColor);
            worldTxt.setPosition(obj.shape.getPosition());
            worldTxt.setRotation(obj.rotation);
            worldTxt.setScale(obj.scaleX, obj.scaleY);
            window.draw(worldTxt);

            sf::FloatRect tb = worldTxt.getGlobalBounds();
            sf::RectangleShape tBounds({std::max(40.f, tb.width + 12.f), std::max(20.f, tb.height + 8.f)});
            tBounds.setPosition(tb.left - 6.f, tb.top - 4.f);
            tBounds.setFillColor(sf::Color::Transparent);
            tBounds.setOutlineColor(obj.selected ? m_SelectionOutlineColor : sf::Color(100, 200, 255, 70));
            tBounds.setOutlineThickness(obj.selected ? m_SelectionOutlineThickness : 1.f);
            window.draw(tBounds);
            obj.shape.setSize({tBounds.getSize().x, tBounds.getSize().y});
        } else if (obj.objectType == ObjectType::AudioSource)
        {
            obj.shape.setScale(obj.scaleX, obj.scaleY);
            obj.shape.setRotation(obj.rotation);
            obj.shape.setFillColor(sf::Color(45, 30, 65, 180));
            obj.shape.setOutlineColor(obj.selected ? m_SelectionOutlineColor : sf::Color(160, 100, 240, 200));
            obj.shape.setOutlineThickness(obj.selected ? m_SelectionOutlineThickness : 1.5f);
            window.draw(obj.shape);

            sf::Vector2f center = obj.shape.getPosition() + sf::Vector2f(
                                      obj.shape.getSize().x * 0.5f, obj.shape.getSize().y * 0.5f);
            sf::ConvexShape cone(3);
            cone.setPoint(0, {-6.f, -8.f});
            cone.setPoint(1, {4.f, -12.f});
            cone.setPoint(2, {4.f, 12.f});
            cone.setPosition(center);
            cone.setFillColor(sf::Color(180, 130, 250));
            window.draw(cone);

            sf::CircleShape wave(12.f);
            wave.setOrigin(12.f, 12.f);
            wave.setPosition(center);
            wave.setFillColor(sf::Color::Transparent);
            wave.setOutlineColor(sf::Color(200, 150, 255, 180));
            wave.setOutlineThickness(1.5f);
            window.draw(wave);

            std::string aName = obj.audioClipPath.empty()
                                    ? "Audio"
                                    : std::filesystem::path(obj.audioClipPath).stem().string();
            sf::Text aLabel(aName, *m_Font, 9);
            aLabel.setFillColor(sf::Color(200, 160, 255, 200));
            aLabel.setPosition(center.x - aLabel.getLocalBounds().width * 0.5f, center.y + 18.f);
            window.draw(aLabel);
        } else if (obj.objectType == ObjectType::ParticleEmitter)
        {
            obj.shape.setScale(obj.scaleX, obj.scaleY);
            obj.shape.setRotation(obj.rotation);
            obj.shape.setFillColor(sf::Color(50, 25, 40, 160));
            obj.shape.setOutlineColor(obj.selected ? m_SelectionOutlineColor : sf::Color(255, 120, 180, 200));
            obj.shape.setOutlineThickness(obj.selected ? m_SelectionOutlineThickness : 1.5f);
            window.draw(obj.shape);

            sf::Vector2f center = obj.shape.getPosition() + sf::Vector2f(
                                      obj.shape.getSize().x * 0.5f, obj.shape.getSize().y * 0.5f);
            sf::CircleShape emitterCore(7.f);
            emitterCore.setOrigin(7.f, 7.f);
            emitterCore.setPosition(center);
            emitterCore.setFillColor(sf::Color(255, 150, 50));
            window.draw(emitterCore);

            for (const auto &p: obj.editorParticles)
            {
                sf::CircleShape pc(p.size * 0.5f);
                pc.setOrigin(p.size * 0.5f, p.size * 0.5f);
                pc.setPosition(p.position);
                pc.setFillColor(p.color);
                window.draw(pc);
            }
        } else
        {
            obj.shape.setScale(obj.scaleX, obj.scaleY);
            obj.shape.setRotation(obj.rotation);
            obj.shape.setOutlineColor(obj.selected ? m_SelectionOutlineColor : sf::Color::Transparent);
            obj.shape.setOutlineThickness(obj.selected ? m_SelectionOutlineThickness : 0.f);
            window.draw(obj.shape);
        }

        if (obj.previewTexture)
        {
            obj.previewSprite.setTexture(*obj.previewTexture);
            obj.previewSprite.setPosition(obj.shape.getPosition());
            obj.previewSprite.setRotation(obj.rotation);
            auto texSize = obj.previewTexture->getSize();
            if (texSize.x > 0 && texSize.y > 0)
            {
                obj.previewSprite.setScale(
                    (obj.shape.getSize().x / static_cast<float>(texSize.x)) * obj.scaleX,
                    (obj.shape.getSize().y / static_cast<float>(texSize.y)) * obj.scaleY);
            }
            window.draw(obj.previewSprite);
        }

        if (m_ShowColliderOutlines &&obj.entity != 0 && m_Registry.HasComponent<CollisionComponent>(obj.entity)) {
            auto &col = m_Registry.GetComponent<CollisionComponent>(obj.entity);
            sf::Color colOutline = col.isTrigger ? sf::Color(255, 215, 0, 200) : sf::Color(40, 220, 100, 200);
            if (col.shape == ColliderShape::Circle)
            {
                float rx = obj.shape.getSize().x * 0.5f;
                float ry = obj.shape.getSize().y * 0.5f;
                sf::CircleShape circ(rx);
                circ.setPosition(obj.shape.getPosition());
                circ.setScale(obj.scaleX, obj.scaleY * (ry / std::max(0.001f, rx)));
                circ.setRotation(obj.rotation);
                circ.setFillColor(sf::Color::Transparent);
                circ.setOutlineColor(colOutline);
                circ.setOutlineThickness(1.5f);
                window.draw(circ);
            } else
            {
                sf::RectangleShape colBox(obj.shape.getSize());
                colBox.setPosition(obj.shape.getPosition());
                colBox.setScale(obj.scaleX, obj.scaleY);
                colBox.setRotation(obj.rotation);
                colBox.setFillColor(sf::Color::Transparent);
                colBox.setOutlineColor(colOutline);
                colBox.setOutlineThickness(1.5f);
                window.draw(colBox);
            }
        }

        if (m_ShowEntityIDs &&obj.entity != 0) {
            sf::Text idText;
            idText.setFont(*m_Font);
            idText.setCharacterSize(9);
            idText.setFillColor(sf::Color(255, 200, 60, 200));
            idText.setString("#" + std::to_string(obj.entity));
            idText.setPosition(obj.shape.getPosition() + sf::Vector2f(2.f, 2.f));
            window.draw(idText);
        }

        if (obj.entity != 0 && m_Registry.HasComponent<CameraComponent>(obj.entity))
        {
            auto &cam = m_Registry.GetComponent<CameraComponent>(obj.entity);
            float zoom = cam.zoom > 0.001f ? cam.zoom : 1.0f;
            float baseW = (m_ProjectWindowWidth > 0 ? static_cast<float>(m_ProjectWindowWidth) : 1920.f);
            float baseH = (m_ProjectWindowHeight > 0 ? static_cast<float>(m_ProjectWindowHeight) : 1080.f);
            float viewW = baseW / zoom;
            float viewH = baseH / zoom;
            sf::Transform tf;
            tf.translate(obj.worldPosition);
            tf.rotate(obj.worldRotation);
            tf.scale(obj.worldScaleX, obj.worldScaleY);
            sf::Vector2f objCenter = tf.transformPoint(obj.shape.getSize().x * 0.5f,
                                                      obj.shape.getSize().y * 0.5f);
            sf::Vector2f camCenter = objCenter + sf::Vector2f(cam.offsetX, cam.offsetY);

            if (std::abs(cam.offsetX) > 0.1f || std::abs(cam.offsetY) > 0.1f)
            {
                sf::Vertex line[] = {
                    sf::Vertex(objCenter, sf::Color(64, 180, 240, 160)),
                    sf::Vertex(camCenter, sf::Color(64, 180, 240, 160))
                };
                window.draw(line, 2, sf::Lines);
            }

            sf::RectangleShape frustum({viewW, viewH});
            frustum.setOrigin(viewW * 0.5f, viewH * 0.5f);
            frustum.setPosition(camCenter);
            frustum.setRotation(obj.worldRotation);
            frustum.setFillColor(sf::Color(64, 180, 240, obj.selected ? 16 : 5));
            frustum.setOutlineColor(sf::Color(64, 180, 240, obj.selected ? 220 : 90));
            frustum.setOutlineThickness(obj.selected ? 2.0f : 1.0f);
            window.draw(frustum);

            sf::RectangleShape crossH({24.f, 1.5f});
            crossH.setOrigin(12.f, 0.75f);
            crossH.setPosition(camCenter);
            crossH.setFillColor(sf::Color(64, 180, 240, obj.selected ? 200 : 100));
            window.draw(crossH);

            sf::RectangleShape crossV({1.5f, 24.f});
            crossV.setOrigin(0.75f, 12.f);
            crossV.setPosition(camCenter);
            crossV.setFillColor(sf::Color(64, 180, 240, obj.selected ? 200 : 100));
            window.draw(crossV);

            sf::Text vLabel;
            vLabel.setFont(*m_Font);
            vLabel.setCharacterSize(10);
            vLabel.setFillColor(sf::Color(64, 180, 240, obj.selected ? 240 : 130));
            std::string labelStr = "CAMERA VIEW [" + std::to_string(static_cast<int>(baseW)) + "x" + std::to_string(static_cast<int>(baseH)) + "]";
            if (std::abs(zoom - 1.0f) > 0.01f) labelStr += " Zoom: " + FormatFloat(zoom, 2) + "x";
            vLabel.setString(labelStr);
            sf::Vector2f topPos = camCenter - sf::Vector2f(vLabel.getLocalBounds().width * 0.5f, viewH * 0.5f + 16.f);
            vLabel.setPosition(topPos);
            window.draw(vLabel);
        }
    }
    int activeCams = 0;
    std::vector<sf::Vector2f> camPositions;
    CameraMultiFollowMode sharedMode = CameraMultiFollowMode::Average;

    for (const auto &obj: m_Objects)
    {
        if (obj.entity != 0 && m_Registry.HasComponent<CameraComponent>(obj.entity))
        {
            auto &c = m_Registry.GetComponent<CameraComponent>(obj.entity);
            if (c.active)
            {
                activeCams++;
                sharedMode = c.multiFollowMode;
                sf::Transform tf;
                tf.translate(obj.worldPosition);
                tf.rotate(obj.worldRotation);
                tf.scale(obj.worldScaleX, obj.worldScaleY);
                sf::Vector2f p = tf.transformPoint(obj.shape.getSize().x * 0.5f, obj.shape.getSize().y * 0.5f)
                                 + sf::Vector2f(c.offsetX, c.offsetY);
                camPositions.push_back(p);
            }
        }
    }

    if (activeCams >= 2)
    {
        for (size_t i = 0; i < camPositions.size() - 1; ++i)
        {
            sf::Vertex link[] = {
                sf::Vertex(camPositions[i], sf::Color(255, 200, 60, 140)),
                sf::Vertex(camPositions[i + 1], sf::Color(255, 200, 60, 140))
            };
            window.draw(link, 2, sf::Lines);
        }

        if (sharedMode == CameraMultiFollowMode::Average || sharedMode == CameraMultiFollowMode::AutoFrame)
        {
            sf::Vector2f sumPos(0.f, 0.f);
            for (const auto &p: camPositions) sumPos += p;
            sf::Vector2f mid = sumPos / static_cast<float>(camPositions.size());

            sf::CircleShape midMarker(6.f);
            midMarker.setOrigin(6.f, 6.f);
            midMarker.setPosition(mid);
            midMarker.setFillColor(sf::Color(255, 200, 60, 200));
            midMarker.setOutlineColor(sf::Color::White);
            midMarker.setOutlineThickness(1.5f);
            window.draw(midMarker);

            sf::Text midText;
            midText.setFont(*m_Font);
            midText.setCharacterSize(10);
            midText.setFillColor(sf::Color(255, 200, 60, 220));
            midText.setString(sharedMode == CameraMultiFollowMode::AutoFrame ? "AUTO-FRAME FOCUS" : "CAMERA MIDPOINT");
            midText.setPosition(mid + sf::Vector2f(-midText.getLocalBounds().width * 0.5f, 10.f));
            window.draw(midText);
        }
    }

    bool canPlace = m_PlacementActive;
    if (m_Dragging || m_Resizing || m_Rotating || m_BoxSelecting || m_panning) canPlace = false;

    if (canPlace)
    {
        if (m_MouseScreenPos.y < TopBarHeight) canPlace = false;
        else if (m_BrowserBounds.contains(m_MouseScreenPos)) canPlace = false;
        else if (m_InspectorBounds.contains(m_MouseScreenPos)) canPlace = false;
        else if (m_HierarchyBounds.contains(m_MouseScreenPos)) canPlace = false;
        else if (m_TabBrowserBounds.contains(m_MouseScreenPos)) canPlace = false;
        else if (m_TabConsoleBounds.contains(m_MouseScreenPos)) canPlace = false;
        else if (m_TabProfilerBounds.contains(m_MouseScreenPos)) canPlace = false;
    }

    if (canPlace)
    {
        sf::Vector2f wPos = MouseWorldPos();
        if (m_Selected &&GetResizeHandle(wPos) >= 0) canPlace = false;
        else if (m_Selected &&GetRotateHandle(wPos)) canPlace = false;
        else if (ObjectAt(wPos) != nullptr) canPlace = false;
    }

    if (canPlace)
    {
        if (IsPolygonType(m_PlacementType) || m_PlacementType == ObjectType::PhysicsBall)
        {
            window.draw(m_CirclePreview);
        } else if (m_PlacementType == ObjectType::Camera)
        {
            float baseW = (m_ProjectWindowWidth > 0 ? static_cast<float>(m_ProjectWindowWidth) : 1920.f);
            float baseH = (m_ProjectWindowHeight > 0 ? static_cast<float>(m_ProjectWindowHeight) : 1080.f);
            sf::Vector2f pPos = SnapToGrid(MouseWorldPos());
            sf::Vector2f pCenter = pPos + sf::Vector2f(32.f, 24.f);
            sf::RectangleShape previewFrustum({baseW, baseH});
            previewFrustum.setOrigin(baseW * 0.5f, baseH * 0.5f);
            previewFrustum.setPosition(pCenter);
            previewFrustum.setFillColor(sf::Color(64, 180, 240, 12));
            previewFrustum.setOutlineColor(sf::Color(64, 180, 240, 140));
            previewFrustum.setOutlineThickness(1.5f);
            window.draw(previewFrustum);

            sf::Text vLabel;
            vLabel.setFont(*m_Font);
            vLabel.setCharacterSize(10);
            vLabel.setFillColor(sf::Color(64, 180, 240, 180));
            vLabel.setString("CAMERA VIEW [" + std::to_string(static_cast<int>(baseW)) + "x" + std::to_string(static_cast<int>(baseH)) + "]");
            vLabel.setPosition(pCenter - sf::Vector2f(vLabel.getLocalBounds().width * 0.5f, baseH * 0.5f + 16.f));
            window.draw(vLabel);

            window.draw(m_Preview);
        } else if (m_PlacementType == ObjectType::WorldText)
        {
            window.draw(m_Preview);
            sf::Text gText("World Text", *m_Font, 24);
            gText.setFillColor(sf::Color(255, 255, 255, 140));
            gText.setPosition(m_Preview.getPosition() + sf::Vector2f(6.f, 2.f));
            window.draw(gText);
        } else if (m_PlacementType == ObjectType::TriggerZone)
        {
            window.draw(m_Preview);
            sf::Text gText("[TRIGGER ZONE]", *m_Font, 10);
            gText.setFillColor(sf::Color(80, 255, 120, 180));
            sf::Vector2f center = m_Preview.getPosition() + sf::Vector2f(
                                      m_Preview.getSize().x * 0.5f, m_Preview.getSize().y * 0.5f);
            gText.setPosition(center.x - gText.getLocalBounds().width * 0.5f, center.y - 6.f);
            window.draw(gText);
        } else if (m_PlacementType == ObjectType::Empty)
        {
            sf::Vector2f center = m_Preview.getPosition() + sf::Vector2f(
                                      m_Preview.getSize().x * 0.5f, m_Preview.getSize().y * 0.5f);
            sf::CircleShape d(12.f, 4);
            d.setOrigin(12.f, 12.f);
            d.setPosition(center);
            d.setRotation(45.f);
            d.setFillColor(sf::Color(180, 180, 180, 50));
            d.setOutlineColor(sf::Color(200, 200, 200, 180));
            d.setOutlineThickness(1.5f);
            window.draw(d);
        } else if (m_PlacementType == ObjectType::SpawnPoint)
        {
            sf::Vector2f center = m_Preview.getPosition() + sf::Vector2f(
                                      m_Preview.getSize().x * 0.5f, m_Preview.getSize().y * 0.5f);
            sf::CircleShape beacon(22.f);
            beacon.setOrigin(22.f, 22.f);
            beacon.setPosition(center);
            beacon.setFillColor(sf::Color(255, 200, 60, 40));
            beacon.setOutlineColor(sf::Color(255, 200, 60, 200));
            beacon.setOutlineThickness(1.5f);
            window.draw(beacon);

            sf::CircleShape core(5.f);
            core.setOrigin(5.f, 5.f);
            core.setPosition(center);
            core.setFillColor(sf::Color(255, 200, 60));
            window.draw(core);
        } else { window.draw(m_Preview); }
    }

    if (m_BoxSelecting)
    {
        sf::Vector2f pos = MouseWorldPos();
        sf::RectangleShape box;
        box.setPosition(std::min(m_BoxSelectStart.x, pos.x), std::min(m_BoxSelectStart.y, pos.y));
        box.setSize({std::abs(pos.x - m_BoxSelectStart.x), std::abs(pos.y - m_BoxSelectStart.y)});
        box.setFillColor(sf::Color(m_SelectionOutlineColor.r, m_SelectionOutlineColor.g, m_SelectionOutlineColor.b,
                                   50));
        box.setOutlineColor(m_SelectionOutlineColor);
        box.setOutlineThickness(1.f);
        window.draw(box);
    }

    DrawGizmos(window);

    const sf::View uiView(sf::FloatRect(
        0.f, 0.f,
        static_cast<float>(m_Window.getSize().x),
        static_cast<float>(m_Window.getSize().y)));
    window.setView(uiView);
    UpdateBounds();

    auto drawTab = [&](const std::string &label, const sf::FloatRect &bounds, bool active) {
        sf::RectangleShape tabRect({bounds.width, bounds.height});
        tabRect.setPosition(bounds.left, bounds.top);
        tabRect.setFillColor(active ? C_BG_PANEL : C_BG_ELEVATED);
        window.draw(tabRect);

        sf::Text tabText;
        tabText.setFont(*m_Font);
        tabText.setCharacterSize(12);
        tabText.setFillColor(active ? C_TEXT_PRIMARY : C_TEXT_MUTED);
        tabText.setString(label);
        tabText.setPosition(bounds.left + 10.f, bounds.top + 4.f);
        window.draw(tabText);
    };

    drawTab("Files", m_TabBrowserBounds, m_ActiveBottomPanelTab == BottomPanelTab::ContentBrowser);
    drawTab("Console", m_TabConsoleBounds, m_ActiveBottomPanelTab == BottomPanelTab::Console);
    drawTab("Profiler", m_TabProfilerBounds, m_ActiveBottomPanelTab == BottomPanelTab::Profiler);

    if (m_ActiveBottomPanelTab == BottomPanelTab::ContentBrowser)
    {
        m_ContentBrowser->Render(window,
                                 m_BrowserBounds.left, m_BrowserBounds.top,
                                 m_BrowserBounds.width, m_BrowserBounds.height);
    } else if (m_ActiveBottomPanelTab == BottomPanelTab::Console)
    {
        m_ConsolePanel->Render(window,
                               m_BrowserBounds.left, m_BrowserBounds.top,
                               m_BrowserBounds.width, m_BrowserBounds.height);
    } else if (m_ActiveBottomPanelTab == BottomPanelTab::Profiler)
    {
        m_ProfilerPanel->Render(window,
                                m_BrowserBounds.left, m_BrowserBounds.top,
                                m_BrowserBounds.width, m_BrowserBounds.height);
    }

    if (m_Inspector) m_Inspector->Draw(window);
    else DrawInspector(window);
    if (m_Hierarchy) m_Hierarchy->Draw(window);
    else DrawHierarchy(window); {
        sf::Text info;
        info.setFont(*m_Font);
        info.setCharacterSize(10);
        info.setFillColor(C_TEXT_MUTED);
        const sf::Vector2f wp = m_Window.mapPixelToCoords(sf::Mouse::getPosition(m_Window), m_camera);
        info.setString("x:" + std::to_string((int) wp.x) + " y:" + std::to_string((int) wp.y));
        info.setPosition(8.f, m_BrowserBounds.top - 16.f);
        window.draw(info);
    }

    m_StatusText.setPosition(8.f, m_BrowserBounds.top - 30.f);
    window.draw(m_StatusText);

    if (m_Toolbar) m_Toolbar->Draw(window);
    else DrawToolbar(window);
    if (m_MenuBar) m_MenuBar->Draw(window);
    else DrawMenuBar(window);

    if (m_ShowFPS)
    {
        sf::Text fpsText;
        fpsText.setFont(*m_Font);
        fpsText.setCharacterSize(11);
        fpsText.setFillColor(sf::Color(80, 255, 120, 220));
        fpsText.setString("FPS: " + std::to_string(static_cast<int>(m_FPS)));
        fpsText.setPosition(m_Window.getSize().x - InspectorWidth - 60.f, TopBarHeight + 6.f);
        window.draw(fpsText);
    }

    if (m_ShowAutoSavePopup && m_AutoSavePopupEnabled)
    {
        sf::Text asText;
        asText.setFont(*m_Font);
        asText.setCharacterSize(14);
        asText.setFillColor(C_TEXT_PRIMARY);
        int secondsLeft = static_cast<int>(m_AutoSavePopupTimer + 0.99f);
        asText.setString("AutoSave in " + std::to_string(secondsLeft) + " seconds...");

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

        sf::RectangleShape warningBar({4.f, pH});
        warningBar.setFillColor(C_WARNING);
        warningBar.setPosition(pX, pY);

        asText.setPosition(pX + 20.f, pY + (pH - th) / 2.f - 4.f);

        window.draw(asBg);
        window.draw(warningBar);
        window.draw(asText);
    }

    if (m_SaveFeedbackTimer > 0.f)
    {
        sf::Text asText;
        asText.setFont(*m_Font);
        asText.setCharacterSize(14);
        asText.setFillColor(C_TEXT_PRIMARY);
        asText.setString("Scene Saved successfully!");

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

        float alpha = std::clamp(m_SaveFeedbackTimer / 0.5f, 0.f, 1.f) * 255.f;
        asBg.setFillColor(sf::Color(C_BG_ELEVATED.r, C_BG_ELEVATED.g, C_BG_ELEVATED.b, alpha));
        asBg.setOutlineColor(sf::Color(C_BORDER_LIGHT.r, C_BORDER_LIGHT.g, C_BORDER_LIGHT.b, alpha));
        successBar.setFillColor(sf::Color(C_SUCCESS.r, C_SUCCESS.g, C_SUCCESS.b, alpha));
        asText.setFillColor(sf::Color(C_TEXT_PRIMARY.r, C_TEXT_PRIMARY.g, C_TEXT_PRIMARY.b, alpha));

        window.draw(asBg);
        window.draw(successBar);
        window.draw(asText);
    }

    if (m_AddDropdownOpen) DrawAddDropdown(window);

    if (m_ShowSettings) DrawSettingsWindow(window);
    if (m_ShowProjectSettings) DrawProjectSettingsWindow(window);
    if (m_ShowBuildPopup) DrawBuildPopup(window);
    if (m_ShowSaveTemplatePrompt) DrawSaveTemplateModal(window);
    if (m_SpotlightOpen) DrawSpotlightPalette(window);

    if (m_ContentBrowser->HasDraggedAsset() &&
        m_ContentBrowser->GetDraggedAsset().type == AssetType::Image)
    {
        const bool overInspector = m_InspectorBounds.contains(m_MouseScreenPos);
        const bool overHierarchy = m_HierarchyBounds.contains(m_MouseScreenPos);

        if (overInspector && m_Selected)
        {
            sf::RectangleShape glow(sf::Vector2f(m_InspectorBounds.width, m_InspectorBounds.height));
            glow.setPosition(m_InspectorBounds.left, m_InspectorBounds.top);
            glow.setFillColor(sf::Color(C_ACCENT.r, C_ACCENT.g, C_ACCENT.b, 18));
            glow.setOutlineColor(sf::Color(C_ACCENT.r, C_ACCENT.g, C_ACCENT.b, 160));
            glow.setOutlineThickness(2.f);
            window.draw(glow);

            sf::Text dropHint;
            dropHint.setFont(*m_Font);
            dropHint.setCharacterSize(11);
            dropHint.setFillColor(sf::Color(C_ACCENT_BRIGHT.r, C_ACCENT_BRIGHT.g, C_ACCENT_BRIGHT.b, 230));
            dropHint.setString("Drop to set Sprite");
            const float hw = dropHint.getLocalBounds().width;
            dropHint.setPosition(
                m_InspectorBounds.left + (m_InspectorBounds.width - hw) / 2.f,
                m_InspectorBounds.top + 6.f);
            window.draw(dropHint);
        } else if (overHierarchy && m_Selected)
        {
            sf::RectangleShape glow(sf::Vector2f(m_HierarchyBounds.width, m_HierarchyBounds.height));
            glow.setPosition(m_HierarchyBounds.left, m_HierarchyBounds.top);
            glow.setFillColor(sf::Color(C_ACCENT.r, C_ACCENT.g, C_ACCENT.b, 18));
            glow.setOutlineColor(sf::Color(C_ACCENT.r, C_ACCENT.g, C_ACCENT.b, 140));
            glow.setOutlineThickness(2.f);
            window.draw(glow);
        }
    }

    m_ContentBrowser->RenderDragGhost(window);
    DrawTooltip(window);
    DrawDeleteModal(window);
    if (m_ShowScriptErrorModal) DrawScriptErrorModal(window);
    DrawImportNotification(window);
}

void DrawPill(sf::RenderWindow &window, sf::FloatRect r, sf::Color fill, sf::Color outline)
{
    const float radius = 4.f;
    const int cornerPoints = 8;
    sf::ConvexShape shape;
    shape.setPointCount(cornerPoints * 4);

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

    if (fill.a > 0)
    {
        sf::ConvexShape shadow = shape;
        shadow.setFillColor(sf::Color(0, 0, 0, 60));
        shadow.move(0.f, 2.f);
        window.draw(shadow);
    }

    shape.setFillColor(fill);
    shape.setOutlineColor(outline);
    shape.setOutlineThickness(1.f);
    window.draw(shape);

    if (fill.a > 0)
    {
        sf::ConvexShape highlight = shape;
        highlight.setFillColor(sf::Color::Transparent);
        highlight.setOutlineColor(sf::Color(255, 255, 255, 25));
        highlight.setOutlineThickness(1.f);
        highlight.move(0.f, -1.f);
        window.draw(highlight);
    }
}


std::string GetInspectorTooltip(const std::string &key)
{
    // Identity & Hierarchy
    if (key == "Name") return "Unique identifier of the object in scene hierarchy";
    if (key == "Tag") return "Category tag for scripts and collision queries (e.g. 'Player', 'Enemy', 'Ground')";
    if (key == "Entity") return "Internal numeric ECS entity ID";
    if (key == "Type") return "Geometric shape or sprite render type";
    if (key == "Parent") return "Parent entity: moving or rotating parent transforms this child";

    // Transform
    if (key == "Position X" || key == "X") return "Horizontal position in world units";
    if (key == "Position Y" || key == "Y") return "Vertical position in world units";
    if (key == "Rotation") return "Rotation angle in degrees (clockwise)";
    if (key == "Scale X") return "Horizontal scale factor (1.0 = normal size)";
    if (key == "Scale Y") return "Vertical scale factor (1.0 = normal size)";
    if (key == "Width" || key == "W") return "Width of the shape / collider in world units";
    if (key == "Height" || key == "H") return "Height of the shape / collider in world units";
    if (key == "Radius" || key == "R") return "Radius of the circle shape / collider";
    if (key == "Z-Index" || key == "Z") return "Render sorting layer: higher values render in front";

    // Appearance & Color
    if (key == "Color Red" || key == "R") return "Red color component (0-255)";
    if (key == "Color Green" || key == "G") return "Green color component (0-255)";
    if (key == "Color Blue" || key == "B") return "Blue color component (0-255)";
    if (key == "Alpha" || key == "A" || key == "Opacity") return "Transparency opacity (0 = invisible, 255 = fully opaque)";
    if (key == "File" || key == "Sprite") return "Assigned texture file path. Drag an image from Content Browser to replace";
    if (key == "Flip X") return "Mirror sprite horizontally across vertical axis";
    if (key == "Flip Y") return "Mirror sprite vertically across horizontal axis";

    // Velocity
    if (key == "Velocity X" || key == "dX" || key == "Dx") return "Linear velocity along X axis (units per second)";
    if (key == "Velocity Y" || key == "dY" || key == "Dy") return "Linear velocity along Y axis (units per second)";

    // Collision Component
    if (key == "Collision Channel" || key == "Channel") return "Collision layer filter (0-31): entities only interact if channels match or are permitted";
    if (key == "Is Trigger" || key == "Trigger") return "Trigger sensor: passes through objects without physical resistance, firing OnTriggerEnter / OnCollision in Lua";
    if (key == "Collider Shape") return "Shape used for collision calculations: Circle (radial check) or Box (oriented bounding box)";
    if (key == "Contact Type") return "Solid: movable dynamic collision response; Static: immovable physical world obstacle";
    if (key == "Solid") return "Whether physical collisions stop and block movement";
    if (key == "Offset X" || key == "Collider Offset X") return "Horizontal offset of collider relative to entity origin";
    if (key == "Offset Y" || key == "Collider Offset Y") return "Vertical offset of collider relative to entity origin";

    // Rigidbody 2D Component
    if (key == "Body Type") return "Dynamic: affected by forces & gravity; Kinematic: moved by script velocity; Static: immovable terrain";
    if (key == "Mass") return "Physical mass in kg: affects momentum, inertia, and how hard it is to push or stop";
    if (key == "Gravity Scale") return "Gravity multiplier: 1.0 = normal gravity, 0.0 = zero-G / top-down, -1.0 = inverted gravity";
    if (key == "Bounciness" || key == "Restitution") return "Impact elasticity: 0.0 = no bounce (thud), 1.0 = perfect bounce (100% kinetic energy conserved)";
    if (key == "Linear Drag" || key == "Drag") return "Air / fluid resistance slowing linear velocity over time (0.0 = frictionless)";
    if (key == "Angular Drag") return "Rotational friction slowing spin speed over time";
    if (key == "Freeze Rotation") return "Locks rotation angle: keeps entity upright even after off-center collisions";

    // Camera Component
    if (key == "Active" || key == "Active Camera") return "Designates whether this camera currently renders the game view";
    if (key == "Zoom" || key == "Camera Zoom") return "Camera zoom multiplier: 1.0 = normal, <1.0 = zoom in, >1.0 = zoom out";
    if (key == "View Width" || key == "View Height") return "Viewport dimensions in world units";
    if (key == "Clear Color") return "Background color rendered when clearing screen before drawing entities";

    // Audio Source Component
    if (key == "Audio Clip" || key == "Audio Path" || key == "Sound") return "Audio sound effect or music file path (WAV, OGG, MP3)";
    if (key == "Volume") return "Audio playback volume level (0 = silent, 100 = full volume)";
    if (key == "Pitch") return "Audio playback speed and pitch multiplier (1.0 = normal pitch)";
    if (key == "Loop" || key == "Looping") return "Automatically repeat playback when finished";
    if (key == "Play on Start" || key == "Play On Start") return "Begin audio playback automatically as soon as scene starts";

    // Particle Emitter Component
    if (key == "Emitting" || key == "Particle Emitting") return "Toggle continuous generation of particles on or off";
    if (key == "Emission Rate" || key == "Rate") return "Number of particles spawned per second";
    if (key == "Lifetime (s)" || key == "Lifetime") return "Duration in seconds before an individual particle expires";
    if (key == "Speed") return "Initial ejection velocity speed for new particles";
    if (key == "Angle (deg)" || key == "Angle") return "Base direction angle for emitted particles in degrees";
    if (key == "Spread (deg)" || key == "Spread") return "Cone angle dispersion around base emission direction";
    if (key == "Start Size") return "Particle scale when newly spawned";
    if (key == "End Size") return "Particle scale at the end of its lifetime";
    if (key == "Gravity X") return "Horizontal drift acceleration applied to particles";
    if (key == "Gravity Y") return "Vertical acceleration applied to particles (positive = downwards)";

    // Script Component
    if (key == "Script" || key == "Script Path") return "Attached Lua script controlling entity logic and behaviors";
    if (key == "OnCreate") return "Lua lifecycle function called once when entity spawns or scene starts";
    if (key == "OnUpdate") return "Lua lifecycle function called every frame with delta time (dt)";
    if (key == "OnCollision") return "Lua callback called when physical collision occurs";
    if (key == "OnTriggerEnter") return "Lua callback called when trigger zone overlap occurs";
    if (key == "OnInputReceived") return "Lua callback called on keyboard, mouse, and game controller input";

    // Text Component
    if (key == "Text" || key == "Text Content") return "String displayed by in-game world text";
    if (key == "Font Size" || key == "Character Size") return "Size of the font glyphs in points / pixels";
    if (key == "Font" || key == "Font Path") return "TrueType font (.ttf) file used for text rendering";
    if (key == "Line Spacing") return "Vertical distance factor between text lines";

    return "";
}


float EditorScene::DrawTemplatePreview(sf::RenderWindow &window, const std::string &templatePath, float x, float y,
                                       float w, float h, const std::string &action)
{
    const float previewW = (w > 0.f) ? w : (InspectorWidth - InspectorPad * 2.f);
    const float previewH = h;
    const sf::FloatRect boxRect(x, y, previewW, previewH);
    const bool hovered = boxRect.contains(m_MouseScreenPos);

    if (!action.empty()) { m_InspectorButtons.push_back({boxRect, action}); }
    sf::RectangleShape card({previewW, previewH});
    card.setPosition(x, y);
    card.setFillColor(hovered ? sf::Color(26, 36, 48) : sf::Color(20, 24, 30));
    card.setOutlineColor(hovered ? sf::Color(70, 140, 240) : sf::Color(45, 60, 80));
    card.setOutlineThickness(1.f);
    window.draw(card);

    std::filesystem::path rootDir = m_ContentBrowser
                                        ? std::filesystem::path(m_ContentBrowser->GetRootPath())
                                        : (FindProjectRoot() / "assets");
    std::filesystem::path tp(templatePath);
    if (!tp.is_absolute())
    {
        if (templatePath.rfind("assets/", 0) == 0) tp = rootDir.parent_path() / templatePath;
        else tp = rootDir / tp;
    }

    const float thumbSize = previewH - 10.f;
    const float thumbX = x + 5.f;
    const float thumbY = y + 5.f;

    sf::RectangleShape thumbBg({thumbSize, thumbSize});
    thumbBg.setPosition(thumbX, thumbY);
    thumbBg.setFillColor(sf::Color(14, 18, 22));
    thumbBg.setOutlineColor(sf::Color(50, 70, 95));
    thumbBg.setOutlineThickness(1.f);
    window.draw(thumbBg);

    std::error_code ec;
    bool valid = false;
    json data;
    if (!templatePath.empty() && std::filesystem::exists(tp, ec))
    {
        std::ifstream f(tp);
        if (f.is_open())
        {
            try
            {
                data = json::parse(f);
                valid = true;
            } catch (...) {}
        }
    }

    if (valid && data.contains("objects") && data["objects"].is_array() && !data["objects"].empty())
    {
        const auto &rootObj = data["objects"][0];
        std::string tmplName = data.value("name", tp.stem().string());
        int objCount = static_cast<int>(data["objects"].size());
        std::string rootType = rootObj.value("type", "rectangle");
        std::string spritePath = rootObj.value("sprite", "");

        bool drewVisual = false;
        if (!spritePath.empty())
        {
            std::shared_ptr<sf::Texture> tex = ResourceManager::Get().GetTexture(spritePath);
            if (!tex)
            {
                std::filesystem::path spPath(spritePath);
                std::filesystem::path fullSp = spPath.is_absolute() ? spPath : (rootDir.parent_path() / spPath);
                tex = ResourceManager::Get().GetTexture(fullSp.string());
                if (!tex) tex = ResourceManager::Get().GetTexture((rootDir / spPath).string());
            }

            if (tex && tex->getSize().x > 0 && tex->getSize().y > 0)
            {
                sf::Sprite sprite(*tex);
                const sf::Vector2u ts = tex->getSize();
                const float scaleX = (thumbSize - 4.f) / static_cast<float>(ts.x);
                const float scaleY = (thumbSize - 4.f) / static_cast<float>(ts.y);
                const float scale = std::min(scaleX, scaleY);
                sprite.setScale(scale, scale);
                sprite.setPosition(
                    thumbX + 2.f + ((thumbSize - 4.f) - ts.x * scale) / 2.f,
                    thumbY + 2.f + ((thumbSize - 4.f) - ts.y * scale) / 2.f);
                window.draw(sprite);
                drewVisual = true;
            }
        }

        if (!drewVisual)
        {
            sf::Color objColor(100, 149, 237);
            if (rootObj.contains("color") && rootObj["color"].is_array() && rootObj["color"].size() >= 3)
            {
                objColor = sf::Color(rootObj["color"][0], rootObj["color"][1], rootObj["color"][2]);
            }

            if (rootType == "circle")
            {
                sf::CircleShape circ((thumbSize - 12.f) / 2.f);
                circ.setPosition(thumbX + 6.f, thumbY + 6.f);
                circ.setFillColor(objColor);
                circ.setOutlineColor(sf::Color(255, 255, 255, 60));
                circ.setOutlineThickness(1.f);
                window.draw(circ);
            } else if (rootType == "triangle")
            {
                sf::CircleShape tri((thumbSize - 12.f) / 2.f, 3);
                tri.setPosition(thumbX + 6.f, thumbY + 6.f);
                tri.setFillColor(objColor);
                tri.setOutlineColor(sf::Color(255, 255, 255, 60));
                tri.setOutlineThickness(1.f);
                window.draw(tri);
            } else
            {
                sf::RectangleShape rect({thumbSize - 14.f, thumbSize - 14.f});
                rect.setPosition(thumbX + 7.f, thumbY + 7.f);
                rect.setFillColor(objColor);
                rect.setOutlineColor(sf::Color(255, 255, 255, 60));
                rect.setOutlineThickness(1.f);
                window.draw(rect);
            }
        }

        float textX = thumbX + thumbSize + 8.f;

        std::string dispName = tmplName + ".template";
        if (dispName.length() > 20) dispName = dispName.substr(0, 17) + "...";
        sf::Text nameText;
        nameText.setFont(*m_Font);
        nameText.setCharacterSize(11);
        nameText.setFillColor(sf::Color(120, 190, 255));
        nameText.setStyle(sf::Text::Bold);
        nameText.setString(dispName);
        nameText.setPosition(textX, y + 6.f);
        window.draw(nameText);

        sf::Text infoText;
        infoText.setFont(*m_Font);
        infoText.setCharacterSize(10);
        infoText.setFillColor(C_TEXT_SECONDARY);
        infoText.setString(std::to_string(objCount) + (objCount == 1 ? " Object (" : " Objects (") + rootType + ")");
        infoText.setPosition(textX, y + 21.f);
        window.draw(infoText);

        sf::Text badgeText;
        badgeText.setFont(*m_Font);
        badgeText.setCharacterSize(9);
        badgeText.setFillColor(sf::Color(80, 160, 240));
        badgeText.setString("[Prefab Template]");
        badgeText.setPosition(textX, y + 36.f);
        window.draw(badgeText);
    } else
    {
        sf::Text emptyIcon;
        emptyIcon.setFont(*m_Font);
        emptyIcon.setCharacterSize(12);
        emptyIcon.setFillColor(C_TEXT_MUTED);
        emptyIcon.setString("TMPL");
        emptyIcon.setPosition(thumbX + (thumbSize - emptyIcon.getLocalBounds().width) / 2.f, thumbY + 14.f);
        window.draw(emptyIcon);

        float textX = thumbX + thumbSize + 8.f;
        sf::Text msgText;
        msgText.setFont(*m_Font);
        msgText.setCharacterSize(11);
        msgText.setFillColor(templatePath.empty() ? C_TEXT_MUTED : C_DANGER);
        msgText.setString(templatePath.empty() ? "(Drag .template here)" : "Template not found");
        msgText.setPosition(textX, y + 10.f);
        window.draw(msgText);

        if (!templatePath.empty())
        {
            std::string filename = tp.filename().string();
            if (filename.length() > 20) filename = filename.substr(0, 17) + "...";
            sf::Text subText;
            subText.setFont(*m_Font);
            subText.setCharacterSize(9);
            subText.setFillColor(C_TEXT_MUTED);
            subText.setString(filename);
            subText.setPosition(textX, y + 26.f);
            window.draw(subText);
        }
    }

    return y + previewH + 4.f;
}


void EditorScene::OpenScriptInIDE(const std::string &scriptPath)
{
    if (scriptPath.empty()) return;

    std::filesystem::path rootDir = FindProjectRoot();
    std::filesystem::path assetsPath = std::filesystem::absolute(rootDir / "assets");

    std::string resolved = ResourceManager::ResolveAssetPath(scriptPath);
    std::filesystem::path absScript = std::filesystem::absolute(resolved.empty() ? scriptPath : resolved);

    std::string ide = m_PreferredIDE;
    if (ide.empty() || !IsExecutableInPath(ide))
    {
        if (IsExecutableInPath("code")) ide = "code";
        else if (IsExecutableInPath("rider")) ide = "rider";
        else if (IsExecutableInPath("clion")) ide = "clion";
        else ide = "";
    }

    if (!ide.empty())
    {
        std::string cmd = ide + " \"" + assetsPath.string() + "\" \"" + absScript.string() + "\"";
        LaunchProcessDetached(cmd);
        std::cout << "[INFO] [EditorScene] Opening script with " << ide << ": " << absScript.string() << " in " <<
                assetsPath.string() << "\n";
    } else
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

void EditorScene::AddObject(sf::Vector2f pos, ObjectType type)
{
    std::string id = NextId();
    sf::Color c = sf::Color(100, 149, 237);
    std::string ts = "rectangle";
    if (type == ObjectType::Circle)
    {
        c = sf::Color(237, 149, 100);
        ts = "circle";
    } else if (type == ObjectType::Triangle)
    {
        c = sf::Color(149, 237, 100);
        ts = "triangle";
    } else if (type == ObjectType::Pentagon)
    {
        c = sf::Color(237, 100, 237);
        ts = "pentagon";
    } else if (type == ObjectType::Hexagon)
    {
        c = sf::Color(237, 237, 100);
        ts = "hexagon";
    } else if (type == ObjectType::Camera)
    {
        c = sf::Color(64, 160, 216);
        ts = "camera";
    } else if (type == ObjectType::Empty)
    {
        c = sf::Color(180, 180, 180);
        ts = "empty";
    } else if (type == ObjectType::SpawnPoint)
    {
        c = sf::Color(255, 200, 60);
        ts = "spawn_point";
    } else if (type == ObjectType::TriggerZone)
    {
        c = sf::Color(40, 200, 80);
        ts = "trigger_zone";
    } else if (type == ObjectType::PhysicsBox)
    {
        c = sf::Color(210, 140, 70);
        ts = "physics_box";
    } else if (type == ObjectType::PhysicsBall)
    {
        c = sf::Color(220, 80, 80);
        ts = "physics_ball";
    } else if (type == ObjectType::StaticPlatform)
    {
        c = sf::Color(100, 110, 125);
        ts = "static_platform";
    } else if (type == ObjectType::Sprite)
    {
        c = sf::Color(255, 255, 255);
        ts = "sprite";
    } else if (type == ObjectType::WorldText)
    {
        c = sf::Color(255, 255, 255);
        ts = "world_text";
    } else if (type == ObjectType::AudioSource)
    {
        c = sf::Color(160, 100, 240);
        ts = "audio_source";
    } else if (type == ObjectType::ParticleEmitter)
    {
        c = sf::Color(255, 150, 40);
        ts = "particle_emitter";
    }

    sf::Vector2f p = SnapToGrid(pos);
    json j;
    j["id"] = id;
    j["type"] = ts;
    if (type == ObjectType::Camera)
    {
        j["tag"] = "Camera";
        j["width"] = 64.f;
        j["height"] = 48.f;
        j["camera"] = {
            {"active", true},
            {"smoothSpeed", 0.0f},
            {"offsetX", 0.0f},
            {"offsetY", 0.0f},
            {"zoom", 1.0f},
            {"priority", 0},
            {"multiFollowMode", 1},
            {"minZoom", 0.3f},
            {"maxZoom", 3.0f},
            {"autoFramePadding", 200.0f}
        };
    } else if (type == ObjectType::Empty)
    {
        j["tag"] = "Empty Entity";
        j["width"] = 32.f;
        j["height"] = 32.f;
    } else if (type == ObjectType::SpawnPoint)
    {
        j["tag"] = "SpawnPoint";
        j["width"] = 48.f;
        j["height"] = 48.f;
    } else if (type == ObjectType::TriggerZone)
    {
        j["tag"] = "TriggerZone";
        j["width"] = 120.f;
        j["height"] = 80.f;
        j["collision"] = {
            {"channel", 0},
            {"type", "solid"},
            {"isTrigger", true},
            {"shape", "box"}
        };
    } else if (type == ObjectType::PhysicsBox)
    {
        j["tag"] = "PhysicsBox";
        j["width"] = 64.f;
        j["height"] = 64.f;
        j["collision"] = {
            {"channel", 0},
            {"type", "solid"},
            {"isTrigger", false},
            {"shape", "box"}
        };
        j["rigidbody"] = {
            {"bodyType", "dynamic"},
            {"mass", 1.0f},
            {"gravityScale", 1.0f},
            {"restitution", 0.1f},
            {"drag", 0.05f},
            {"freezeRotation", false}
        };
        j["velocity"] = {{"dx", 0.f}, {"dy", 0.f}};
    } else if (type == ObjectType::PhysicsBall)
    {
        j["tag"] = "PhysicsBall";
        j["width"] = 64.f;
        j["height"] = 64.f;
        j["collision"] = {
            {"channel", 0},
            {"type", "solid"},
            {"isTrigger", false},
            {"shape", "circle"}
        };
        j["rigidbody"] = {
            {"bodyType", "dynamic"},
            {"mass", 1.0f},
            {"gravityScale", 1.0f},
            {"restitution", 0.7f},
            {"drag", 0.02f},
            {"freezeRotation", false}
        };
        j["velocity"] = {{"dx", 0.f}, {"dy", 0.f}};
    } else if (type == ObjectType::StaticPlatform)
    {
        j["tag"] = "Platform";
        j["width"] = 240.f;
        j["height"] = 32.f;
        j["collision"] = {
            {"channel", 0},
            {"type", "static"},
            {"isTrigger", false},
            {"shape", "box"}
        };
    } else if (type == ObjectType::Sprite)
    {
        j["tag"] = "Sprite";
        j["width"] = 96.f;
        j["height"] = 96.f;
    } else if (type == ObjectType::WorldText)
    {
        j["tag"] = "WorldText";
        j["width"] = 160.f;
        j["height"] = 36.f;
        j["text"] = {
            {"text", "World Text"},
            {"size", 28},
            {"color", {255, 255, 255}},
            {"align", 0},
            {"outlineThickness", 0.0f}
        };
    } else if (type == ObjectType::AudioSource)
    {
        j["tag"] = "AudioSource";
        j["width"] = 48.f;
        j["height"] = 48.f;
        j["audioSource"] = {
            {"soundPath", ""},
            {"volume", 100.0f},
            {"pitch", 1.0f},
            {"loop", false},
            {"playOnStart", true},
            {"isSpatial", false},
            {"minDistance", 150.0f},
            {"attenuation", 1.0f}
        };
    } else if (type == ObjectType::ParticleEmitter)
    {
        j["tag"] = "ParticleEmitter";
        j["width"] = 48.f;
        j["height"] = 48.f;
        j["particleEmitter"] = {
            {"emitting", true},
            {"maxParticles", 120},
            {"rate", 25.0f},
            {"lifetime", 1.5f},
            {"speed", 120.0f},
            {"speedVariance", 40.0f},
            {"angle", -90.0f},
            {"spread", 45.0f},
            {"startSize", 8.0f},
            {"endSize", 2.0f},
            {"startColor", {255, 190, 50}},
            {"endColor", {255, 50, 20}},
            {"gravityX", 0.0f},
            {"gravityY", 60.0f}
        };
    } else
    {
        j["width"] = m_GridSize;
        j["height"] = m_GridSize;
    }
    j["x"] = p.x;
    j["y"] = p.y;
    j["color"] = {c.r, c.g, c.b};
    j["rotation"] = 0.f;
    j["scaleX"] = 1.f;
    j["scaleY"] = 1.f;
    bool defVisible = !(type == ObjectType::SpawnPoint || type == ObjectType::AudioSource || type ==
                        ObjectType::ParticleEmitter || type == ObjectType::Camera || type == ObjectType::Empty || type
                        == ObjectType::TriggerZone);
    j["visibleInGame"] = defVisible;

    auto cmd = std::make_shared<ObjectStateCommand>(id, json(), j);
    ExecuteCommand(cmd);

    for (auto &obj: m_Objects)
    {
        if (obj.id == id)
        {
            SelectObject(&obj, false);
            break;
        }
    }
    std::cout << "[INFO] [EditorScene] Added object '" << id << "' (type: " << ts << ") at (" << p.x << ", " << p.y <<
            ").\n";
}

void EditorScene::AddObjectWithSprite(sf::Vector2f pos, const std::string &spritePath)
{
    std::string id = NextId();
    sf::Vector2f p = SnapToGrid(pos);
    json j;
    j["id"] = id;
    j["type"] = "sprite";
    j["x"] = p.x;
    j["y"] = p.y;
    j["width"] = m_GridSize * 2.f;
    j["height"] = m_GridSize * 2.f;
    j["color"] = {255, 255, 255};
    j["rotation"] = 0.f;
    j["scaleX"] = 1.f;
    j["scaleY"] = 1.f;

    std::error_code ec;
    std::filesystem::path pt(spritePath);
    std::filesystem::path root = std::filesystem::absolute(ASSET_PATH, ec);
    std::string rel = std::filesystem::proximate(pt, root, ec).generic_string();
    j["sprite"] = ec ? spritePath : rel;

    auto cmd = std::make_shared<ObjectStateCommand>(id, json(), j);
    ExecuteCommand(cmd);
    std::cout << "[INFO] [EditorScene] Added sprite object '" << id << "' with texture: " << spritePath << "\n";
}

void EditorScene::ApplySpriteToObject(EditorObject &obj, const std::string &spritePath)
{
    obj.spritePath = spritePath;
    obj.previewTexture = ResourceManager::Get().GetTexture(spritePath);

    if (obj.previewTexture)
    {
        obj.previewSprite.setTexture(*obj.previewTexture, true);
        const sf::Vector2u ts = obj.previewTexture->getSize();
        const sf::Vector2f sz = obj.shape.getSize();
        if (ts.x > 0 && ts.y > 0)
            obj.previewSprite.setScale(sz.x / ts.x, sz.y / ts.y);
        obj.shape.setFillColor(sf::Color(255, 255, 255, 40));
    }

    if (obj.entity != 0)
    {
        if (m_Registry.HasComponent<SpriteComponent>(obj.entity))
            m_Registry.GetComponent<SpriteComponent>(obj.entity) = SpriteComponent(spritePath, obj.shape.getSize());
        else
            m_Registry.AddComponent(obj.entity, SpriteComponent(spritePath, obj.shape.getSize()));
    }

    std::cout << "[INFO] [EditorScene] Sprite texture applied: " << spritePath << "\n";
}

void EditorScene::HandleAssetMoved(const std::string &oldPath, const std::string &newPath)
{
    if (oldPath.empty() || newPath.empty() || oldPath == newPath) return;

    std::error_code ec;
    std::filesystem::path oldAbs = std::filesystem::absolute(oldPath, ec);
    if (ec) oldAbs = std::filesystem::path(oldPath);
    ec.clear();
    std::filesystem::path newAbs = std::filesystem::absolute(newPath, ec);
    if (ec) newAbs = std::filesystem::path(newPath);
    ec.clear();

    std::filesystem::path assetRoot = std::filesystem::absolute(ASSET_PATH, ec);
    if (ec) assetRoot = std::filesystem::path(ASSET_PATH);
    ec.clear();

    std::filesystem::path projRoot = assetRoot.parent_path();

    std::filesystem::path oldRelProj = std::filesystem::proximate(oldAbs, projRoot, ec);
    if (ec) oldRelProj = oldAbs;
    ec.clear();
    std::filesystem::path newRelProj = std::filesystem::proximate(newAbs, projRoot, ec);
    if (ec) newRelProj = newAbs;
    ec.clear();

    std::filesystem::path oldRelAssets = std::filesystem::proximate(oldAbs, assetRoot, ec);
    if (ec) oldRelAssets = oldAbs;
    ec.clear();
    std::filesystem::path newRelAssets = std::filesystem::proximate(newAbs, assetRoot, ec);
    if (ec) newRelAssets = newAbs;
    ec.clear();

    bool isDir = std::filesystem::is_directory(newAbs, ec);
    ec.clear();

    std::string oldAbsStr = oldAbs.generic_string();
    std::string newAbsStr = newAbs.generic_string();
    std::string oldP = oldRelProj.generic_string();
    std::string newP = newRelProj.generic_string();
    std::string oldA = oldRelAssets.generic_string();
    std::string newA = newRelAssets.generic_string();

    auto replacePathString = [&](std::string &str) -> bool {
        if (str.empty()) return false;
        std::string s = str;
        std::replace(s.begin(), s.end(), '\\', '/');

        auto replaceExactOrPrefix = [&](const std::string &oldMatch, const std::string &newMatch) -> bool {
            if (oldMatch.empty() || oldMatch == "." || oldMatch == "/") return false;
            if (s == oldMatch) {
                str = newMatch;
                return true;
            }
            if (isDir && s.rfind(oldMatch + "/", 0) == 0) {
                str = newMatch + s.substr(oldMatch.size());
                return true;
            }
            return false;
        };

        if (replaceExactOrPrefix(oldAbsStr, newAbsStr)) return true;
        if (replaceExactOrPrefix(oldP, newP)) return true;
        if (replaceExactOrPrefix(oldA, newA)) return true;
        return false;
    };

    bool anyObjectChanged = false;
    for (auto &obj : m_Objects)
    {
        // 1. Sprite path
        std::string prevSprite = obj.spritePath;
        if (replacePathString(obj.spritePath))
        {
            ApplySpriteToObject(obj, obj.spritePath);
            anyObjectChanged = true;
            std::cout << "[INFO] [EditorScene] Updated sprite path on entity '" << obj.id << "' to '" << obj.spritePath << "'\n";
        }

        // 2. Script path
        std::string prevScript = obj.scriptPath;
        if (replacePathString(obj.scriptPath))
        {
            anyObjectChanged = true;
            std::cout << "[INFO] [EditorScene] Updated script path on entity '" << obj.id << "' to '" << obj.scriptPath << "'\n";
            if (obj.entity != 0 && m_Registry.HasComponent<ScriptComponent>(obj.entity))
            {
                m_Registry.RemoveComponent<ScriptComponent>(obj.entity);
                if (!obj.scriptPath.empty())
                {
                    std::string resolved = ResourceManager::ResolveAssetPath(obj.scriptPath);
                    auto &sc = m_Registry.AddComponent(obj.entity, ScriptComponent(LuaState::GetLua(), resolved, obj.entity));
                    sc.SetEntity(obj.entity);
                    SyncExportedScriptProperties(obj, sc);
                }
            }
        }

        // 3. Template path
        std::string prevTemplate = obj.templatePath;
        if (replacePathString(obj.templatePath))
        {
            anyObjectChanged = true;
            std::cout << "[INFO] [EditorScene] Updated template path on entity '" << obj.id << "' to '" << obj.templatePath << "'\n";
        }

        // 4. Audio clip path
        std::string prevAudio = obj.audioClipPath;
        if (replacePathString(obj.audioClipPath))
        {
            anyObjectChanged = true;
            std::cout << "[INFO] [EditorScene] Updated audio path on entity '" << obj.id << "' to '" << obj.audioClipPath << "'\n";
            if (obj.entity != 0 && m_Registry.HasComponent<AudioSourceComponent>(obj.entity))
            {
                m_Registry.GetComponent<AudioSourceComponent>(obj.entity).soundPath = obj.audioClipPath;
            }
        }

        // 5. Script exported properties of type Image or Template
        for (auto &[pName, prop] : obj.scriptProperties)
        {
            if (prop.type == ScriptComponent::PropertyType::Image ||
                prop.type == ScriptComponent::PropertyType::Template)
            {
                if (replacePathString(prop.stringVal))
                {
                    anyObjectChanged = true;
                    std::cout << "[INFO] [EditorScene] Updated property '" << pName << "' on entity '" << obj.id << "' to '" << prop.stringVal << "'\n";
                    if (obj.entity != 0 && m_Registry.HasComponent<ScriptComponent>(obj.entity))
                    {
                        m_Registry.GetComponent<ScriptComponent>(obj.entity).SetExportedProperty(prop);
                    }
                }
            }
        }
    }

    // Update active scene save path and project settings if moved
    std::string prevSceneSave = m_SceneSavePath;
    if (replacePathString(m_SceneSavePath))
    {
        SaveSettings();
        std::cout << "[INFO] [EditorScene] Updated active scene save path from '" << prevSceneSave << "' to '" << m_SceneSavePath << "'\n";
    }

    std::string prevStartScene = m_ProjectStartScene;
    if (replacePathString(m_ProjectStartScene))
    {
        SaveProjectSettings();
        std::cout << "[INFO] [EditorScene] Updated project start scene from '" << prevStartScene << "' to '" << m_ProjectStartScene << "'\n";
    }

    if (m_EditingTemplate && replacePathString(m_EditingTemplatePath))
    {
        std::cout << "[INFO] [EditorScene] Updated currently edited template path to '" << m_EditingTemplatePath << "'\n";
    }

    // Scan all scripts (*.lua), templates (*.template), and scenes (*.json) on disk to fix references
    auto replaceInTextFile = [&](const std::filesystem::path &filePath) {
        std::ifstream inFile(filePath, std::ios::in | std::ios::binary);
        if (!inFile.is_open()) return;
        std::string content((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
        inFile.close();

        bool modified = false;
        auto replaceAll = [&](const std::string &from, const std::string &to) {
            if (from.empty() || from == "/" || from == ".") return;
            size_t pos = 0;
            while ((pos = content.find(from, pos)) != std::string::npos) {
                content.replace(pos, from.length(), to);
                pos += to.length();
                modified = true;
            }
        };

        std::string oldA_back = oldA; std::replace(oldA_back.begin(), oldA_back.end(), '/', '\\');
        std::string newA_back = newA; std::replace(newA_back.begin(), newA_back.end(), '/', '\\');
        std::string oldP_back = oldP; std::replace(oldP_back.begin(), oldP_back.end(), '/', '\\');
        std::string newP_back = newP; std::replace(newP_back.begin(), newP_back.end(), '/', '\\');

        replaceAll(oldAbsStr, newAbsStr);
        replaceAll(oldP, newP);
        replaceAll(oldP_back, newP_back);
        replaceAll(oldA, newA);
        replaceAll(oldA_back, newA_back);

        if (modified)
        {
            std::ofstream outFile(filePath, std::ios::out | std::ios::binary | std::ios::trunc);
            if (outFile.is_open())
            {
                outFile.write(content.data(), content.size());
                outFile.close();
                std::cout << "[INFO] [EditorScene] Updated asset references in file: " << filePath.string() << "\n";
            }
        }
    };

    if (std::filesystem::exists(assetRoot, ec))
    {
        for (const auto &dirEntry : std::filesystem::recursive_directory_iterator(assetRoot, ec))
        {
            if (ec) break;
            if (!dirEntry.is_regular_file(ec)) continue;
            std::string ext = dirEntry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".lua" || ext == ".json" || ext == ".template")
            {
                replaceInTextFile(dirEntry.path());
            }
        }
    }

    if (anyObjectChanged)
    {
        SetDirty(true);
    }
}


void EditorScene::SyncToRegistry()
{
    UpdateWorldTransforms();
    for (auto &obj: m_Objects)
    {
        if (obj.entity == 0) continue;

        if (m_Registry.HasComponent<TransformComponent>(obj.entity))
        {
            auto &t = m_Registry.GetComponent<TransformComponent>(obj.entity);
            t.x = obj.localPosition.x;
            t.y = obj.localPosition.y;
            t.rotation = obj.rotation;
            t.scaleX = obj.scaleX;
            t.scaleY = obj.scaleY;
            t.worldX = obj.worldPosition.x;
            t.worldY = obj.worldPosition.y;
            t.worldRotation = obj.worldRotation;
            t.worldScaleX = obj.worldScaleX;
            t.worldScaleY = obj.worldScaleY;
        }

        if (m_Registry.HasComponent<HierarchyComponent>(obj.entity))
        {
            auto &hc = m_Registry.GetComponent<HierarchyComponent>(obj.entity);
            EditorObject *parentObj = ObjectById(obj.parentId);
            hc.parent = parentObj ? parentObj->entity : NULL_ENTITY;
            hc.children.clear();
            for (auto *child: GetChildren(obj.id)) { if (child->entity != 0) hc.children.push_back(child->entity); }
        }

        if (m_Registry.HasComponent<RenderComponent>(obj.entity))
        {
            auto &rc = m_Registry.GetComponent<RenderComponent>(obj.entity);
            rc.size = obj.shape.getSize();
            rc.color = obj.color;
            rc.shapeType = MapToShapeType(obj.objectType);
            rc.zIndex = obj.zIndex;
            rc.visibleInGame = obj.visibleInGame;
        }

        if (obj.previewTexture && !obj.spritePath.empty())
        {
            if (m_Registry.HasComponent<SpriteComponent>(obj.entity))
            {
                m_Registry.GetComponent<SpriteComponent>(obj.entity) =
                        SpriteComponent(obj.spritePath, obj.shape.getSize());
            } else { m_Registry.AddComponent(obj.entity, SpriteComponent(obj.spritePath, obj.shape.getSize())); }
        }

        if (m_Registry.HasComponent<ScriptComponent>(obj.entity))
        {
            auto &sc = m_Registry.GetComponent<ScriptComponent>(obj.entity);
            for (const auto &pair: obj.scriptProperties)
            {
                sc.SetExportedProperty(pair.second);
            }
        }
    }
}

void EditorScene::SyncExportedScriptProperties(EditorObject &obj, ScriptComponent &sc)
{
    auto freshProps = sc.GetExportedProperties();
    std::map<std::string, ScriptComponent::Property> mergedProps;

    for (const auto &fresh: freshProps)
    {
        auto it = obj.scriptProperties.find(fresh.name);
        if (it != obj.scriptProperties.end() && it->second.type == fresh.type)
        {
            // Preserve user's configured value from Inspector
            mergedProps[fresh.name] = it->second;
        }
        else if (it != obj.scriptProperties.end() &&
                 ((it->second.type == ScriptComponent::PropertyType::Int && fresh.type == ScriptComponent::PropertyType::Float) ||
                  (it->second.type == ScriptComponent::PropertyType::Float && fresh.type == ScriptComponent::PropertyType::Int)))
        {
            ScriptComponent::Property converted = fresh;
            if (fresh.type == ScriptComponent::PropertyType::Float)
            {
                converted.floatVal = (it->second.type == ScriptComponent::PropertyType::Int)
                                         ? static_cast<float>(it->second.intVal)
                                         : it->second.floatVal;
            }
            else
            {
                converted.intVal = (it->second.type == ScriptComponent::PropertyType::Float)
                                       ? static_cast<int>(it->second.floatVal)
                                       : it->second.intVal;
                converted.floatVal = static_cast<float>(converted.intVal);
            }
            mergedProps[fresh.name] = converted;
        }
        else
        {
            // Brand new property or type changed: use default from script
            mergedProps[fresh.name] = fresh;
        }
    }

    obj.scriptProperties = std::move(mergedProps);

    // Re-apply preserved values into the Lua environment
    for (const auto &pair: obj.scriptProperties)
    {
        sc.SetExportedProperty(pair.second);
    }
}


sf::Vector2f EditorScene::SnapToGrid(sf::Vector2f pos) const
{
    if (!m_SnapToGrid) return pos;
    return {
        std::floor(pos.x / m_GridSize) * m_GridSize,
        std::floor(pos.y / m_GridSize) * m_GridSize
    };
}

sf::Vector2f EditorScene::MouseWorldPos() const
{
    return m_Window.mapPixelToCoords(sf::Mouse::getPosition(m_Window), m_camera);
}

EditorObject *EditorScene::ObjectAt(sf::Vector2f pos)
{
    std::vector<EditorObject *> sortedObjects;
    sortedObjects.reserve(m_Objects.size());
    for (auto &obj: m_Objects) sortedObjects.push_back(&obj);
    std::stable_sort(sortedObjects.begin(), sortedObjects.end(), [](const EditorObject *a, const EditorObject *b) {
        return a->zIndex > b->zIndex;
    });

    sf::Vector2i zeroScreen{0, 0};
    sf::Vector2i tolScreen{5, 0};
    const sf::Vector2f wZero = m_Window.mapPixelToCoords(zeroScreen, m_camera);
    const sf::Vector2f wTol = m_Window.mapPixelToCoords(tolScreen, m_camera);
    const float screenTolWorld = std::abs(wTol.x - wZero.x);

    for (auto *obj: sortedObjects)
    {
        sf::Transform tr;
        tr.translate(obj->shape.getPosition());
        tr.rotate(obj->rotation);
        tr.scale(obj->scaleX, obj->scaleY);

        sf::Vector2f localPos = tr.getInverse().transformPoint(pos);
        sf::Vector2f s = obj->shape.getSize();

        float tolX = screenTolWorld / std::max(std::abs(obj->scaleX), 0.05f);
        float tolY = screenTolWorld / std::max(std::abs(obj->scaleY), 0.05f);

        if (obj->objectType == ObjectType::Circle)
        {
            float rx = std::abs(s.x) * 0.5f;
            float ry = std::abs(s.y) * 0.5f;
            if (rx > 0.001f && ry > 0.001f)
            {
                float effRx = rx + tolX;
                float effRy = ry + tolY;
                float dx = localPos.x - rx;
                float dy = localPos.y - ry;
                if ((dx * dx) / (effRx * effRx) + (dy * dy) / (effRy * effRy) <= 1.0f)
                    return obj;
            }
        } else
        {
            float minX = std::min(0.f, s.x) - tolX;
            float maxX = std::max(0.f, s.x) + tolX;
            float minY = std::min(0.f, s.y) - tolY;
            float maxY = std::max(0.f, s.y) + tolY;
            if (localPos.x >= minX && localPos.x <= maxX &&
                localPos.y >= minY && localPos.y <= maxY) { return obj; }
        }
    }
    return nullptr;
}

void EditorScene::DrawGrid()
{
    if (!m_SnapToGrid) return;

    sf::VertexArray lines(sf::Lines);

    const sf::Vector2f center = m_camera.getCenter();
    const sf::Vector2f camSize = m_camera.getSize();

    const float left = std::floor((center.x - camSize.x / 2) / m_GridSize) * m_GridSize;
    const float top = std::floor((center.y - camSize.y / 2) / m_GridSize) * m_GridSize;
    const float right = center.x + camSize.x / 2 + m_GridSize;
    const float bottom = center.y + camSize.y / 2 + m_GridSize;

    const sf::Uint8 alpha = static_cast<sf::Uint8>(m_GridOpacity);
    const sf::Color gcMinor(C_GRID_MINOR.r, C_GRID_MINOR.g, C_GRID_MINOR.b, alpha);
    const sf::Color gcMajor(C_GRID_MAJOR.r, C_GRID_MAJOR.g, C_GRID_MAJOR.b, alpha);
    const float majorStep = m_GridSize * 4.f;

    for (float x = left; x < right; x += m_GridSize)
    {
        const bool major = (std::fmod(std::abs(x), majorStep) < 0.5f);
        const sf::Color &gc = major ? gcMajor : gcMinor;
        lines.append({{x, top}, gc});
        lines.append({{x, bottom}, gc});
    }
    for (float y = top; y < bottom; y += m_GridSize)
    {
        const bool major = (std::fmod(std::abs(y), majorStep) < 0.5f);
        const sf::Color &gc = major ? gcMajor : gcMinor;
        lines.append({{left, y}, gc});
        lines.append({{right, y}, gc});
    }

    m_Window.draw(lines);
}

void EditorScene::DrawWorldAxes(sf::RenderWindow &window)
{
    const sf::Vector2f center = m_camera.getCenter();
    const sf::Vector2f camSize = m_camera.getSize();

    const float left = center.x - camSize.x * 0.5f - 200.f;
    const float right = center.x + camSize.x * 0.5f + 200.f;
    const float top = center.y - camSize.y * 0.5f - 200.f;
    const float bottom = center.y + camSize.y * 0.5f + 200.f;

    sf::Vector2i zeroScreen{0, 0};
    sf::Vector2i oneScreen{1, 0};
    const sf::Vector2f wZero = m_Window.mapPixelToCoords(zeroScreen, m_camera);
    const sf::Vector2f wOne = m_Window.mapPixelToCoords(oneScreen, m_camera);
    const float pixelWidth = std::max(1.f, std::abs(wOne.x - wZero.x));

    const float axisThickness = std::max(1.5f, pixelWidth * 1.5f);
    sf::RectangleShape xAxis({right - left, axisThickness});
    xAxis.setOrigin(0.f, axisThickness * 0.5f);
    xAxis.setPosition(left, 0.f);
    xAxis.setFillColor(sf::Color(220, 60, 60, 200));
    window.draw(xAxis);
    sf::RectangleShape yAxis({axisThickness, bottom - top});
    yAxis.setOrigin(axisThickness * 0.5f, 0.f);
    yAxis.setPosition(0.f, top);
    yAxis.setFillColor(sf::Color(60, 200, 60, 200));
    window.draw(yAxis);
    const float markerRadius = std::max(3.f, pixelWidth * 3.5f);
    sf::CircleShape originMarker(markerRadius);
    originMarker.setOrigin(markerRadius, markerRadius);
    originMarker.setPosition(0.f, 0.f);
    originMarker.setFillColor(sf::Color(255, 255, 255, 230));
    originMarker.setOutlineColor(sf::Color(30, 30, 30, 220));
    originMarker.setOutlineThickness(std::max(1.f, pixelWidth * 0.8f));
    window.draw(originMarker);
}

sf::Vector2f RotatePoint(sf::Vector2f point, sf::Vector2f center, float angleDegrees)
{
    float angleRad = angleDegrees * 3.14159265f / 180.f;
    float cosA = std::cos(angleRad);
    float sinA = std::sin(angleRad);

    sf::Vector2f translated = point - center;
    sf::Vector2f rotated;
    rotated.x = translated.x * cosA - translated.y * sinA;
    rotated.y = translated.x * sinA + translated.y * cosA;

    return rotated + center;
}

sf::Vector2f HandlePos(const EditorObject *obj, int idx)
{
    sf::Vector2f p = obj->shape.getPosition();
    sf::Vector2f s = {obj->shape.getSize().x * obj->scaleX, obj->shape.getSize().y * obj->scaleY};
    sf::Vector2f c = p + s * 0.5f;

    sf::Vector2f u;
    switch (idx)
    {
        case 0: u = {p.x, p.y};
            break;
        case 1: u = {c.x, p.y};
            break;
        case 2: u = {p.x + s.x, p.y};
            break;
        case 3: u = {p.x, c.y};
            break;
        case 4: u = {p.x + s.x, c.y};
            break;
        case 5: u = {p.x, p.y + s.y};
            break;
        case 6: u = {c.x, p.y + s.y};
            break;
        case 7: u = {p.x + s.x, p.y + s.y};
            break;
        default: u = {0.f, 0.f};
            break;
    }

    return RotatePoint(u, p, obj->rotation);
}

int EditorScene::GetResizeHandle(sf::Vector2f worldPos) const
{
    if (!m_Selected) return -1;

    sf::Vector2i zeroScreen{0, 0};
    sf::Vector2i hitScreen{8, 0};
    const sf::Vector2f wZero = m_Window.mapPixelToCoords(zeroScreen, m_camera);
    const sf::Vector2f wHit = m_Window.mapPixelToCoords(hitScreen, m_camera);
    const float hitRadius = std::abs(wHit.x - wZero.x);

    for (int i = 0; i < 8; ++i)
    {
        const sf::Vector2f hp = HandlePos(m_Selected, i);
        const float dx = worldPos.x - hp.x;
        const float dy = worldPos.y - hp.y;
        if (std::sqrt(dx * dx + dy * dy) <= hitRadius)
            return i;
    }
    return -1;
}

void EditorScene::DrawResizeHandles(sf::RenderWindow &window) { DrawGizmos(window); }

sf::Vector2f EditorScene::GetRotateHandlePos(const EditorObject *obj) const
{
    if (!obj) return {0.f, 0.f};

    sf::Vector2i zeroScreen{0, 0};
    sf::Vector2i fortyScreen{0, 36};
    const sf::Vector2f wZero = m_Window.mapPixelToCoords(zeroScreen, m_camera);
    const sf::Vector2f wForty = m_Window.mapPixelToCoords(fortyScreen, m_camera);
    const float rotOffset = std::abs(wForty.y - wZero.y);

    sf::Vector2f p = obj->shape.getPosition();
    sf::Vector2f s = {obj->shape.getSize().x * obj->scaleX, obj->shape.getSize().y * obj->scaleY};
    sf::Vector2f unrotatedTopMid = {p.x + s.x * 0.5f, p.y};

    sf::Vector2f unrotatedRotateHandle = {unrotatedTopMid.x, unrotatedTopMid.y - rotOffset};
    return RotatePoint(unrotatedRotateHandle, p, obj->rotation);
}

void EditorScene::DrawGizmos(sf::RenderWindow &window)
{
    if (!m_Selected) return;

    sf::Vector2i zeroScreen{0, 0};
    sf::Vector2i handleScreen{5, 0};
    const sf::Vector2f wZero = m_Window.mapPixelToCoords(zeroScreen, m_camera);
    const sf::Vector2f wHandle = m_Window.mapPixelToCoords(handleScreen, m_camera);
    const float hw = std::abs(wHandle.x - wZero.x);

    sf::Vector2f p = m_Selected->shape.getPosition();
    sf::Vector2f s = {
        m_Selected->shape.getSize().x * m_Selected->scaleX, m_Selected->shape.getSize().y * m_Selected->scaleY
    };
    sf::Vector2f unrotatedTopMid = {p.x + s.x * 0.5f, p.y};
    sf::Vector2f topMid = RotatePoint(unrotatedTopMid, p, m_Selected->rotation);
    m_RotateHandlePos = GetRotateHandlePos(m_Selected);

    static const sf::Color C_SCALE_FILL = sf::Color(80, 165, 255, 240);
    static const sf::Color C_SCALE_EDGE = sf::Color(120, 195, 255, 240);
    static const sf::Color C_SCALE_OUTL = sf::Color(25, 30, 45, 255);

    for (int i = 0; i < 8; ++i)
    {
        const sf::Vector2f hp = HandlePos(m_Selected, i);
        const bool isCorner = (i == 0 || i == 2 || i == 5 || i == 7);

        sf::RectangleShape handle({hw * 2.f, hw * 2.f});
        handle.setOrigin(hw, hw);
        handle.setPosition(hp);
        handle.setRotation(m_Selected->rotation);
        handle.setFillColor(isCorner ? C_SCALE_FILL : C_SCALE_EDGE);
        handle.setOutlineColor(C_SCALE_OUTL);
        handle.setOutlineThickness(std::max(1.f, hw * 0.25f));
        window.draw(handle);
    }

    static const sf::Color C_ROT_LINE = sf::Color(80, 220, 140, 180);
    sf::VertexArray line(sf::Lines, 2);
    line[0] = {topMid, C_ROT_LINE};
    line[1] = {m_RotateHandlePos, C_ROT_LINE};
    window.draw(line);

    static const sf::Color C_ROT_FILL = sf::Color(60, 210, 120, 230);
    static const sf::Color C_ROT_OUTL = sf::Color(30, 160, 80, 255);
    const float rotR = hw * 1.4f;

    sf::CircleShape rotHandle(rotR);
    rotHandle.setOrigin(rotR, rotR);
    rotHandle.setPosition(m_RotateHandlePos);
    rotHandle.setFillColor(m_Rotating ? sf::Color(100, 255, 160, 245) : C_ROT_FILL);
    rotHandle.setOutlineColor(C_ROT_OUTL);
    rotHandle.setOutlineThickness(hw * 0.3f);
    window.draw(rotHandle);

    const float a = rotR * 0.55f;
    sf::VertexArray arc(sf::LinesStrip, 5);
    arc[0] = {{m_RotateHandlePos.x - a, m_RotateHandlePos.y}, sf::Color(255, 255, 255, 200)};
    arc[1] = {{m_RotateHandlePos.x - a * 0.5f, m_RotateHandlePos.y - a}, sf::Color(255, 255, 255, 200)};
    arc[2] = {{m_RotateHandlePos.x, m_RotateHandlePos.y - a}, sf::Color(255, 255, 255, 200)};
    arc[3] = {{m_RotateHandlePos.x + a * 0.5f, m_RotateHandlePos.y - a}, sf::Color(255, 255, 255, 200)};
    arc[4] = {{m_RotateHandlePos.x + a, m_RotateHandlePos.y}, sf::Color(255, 255, 255, 200)};
    window.draw(arc);
}

bool EditorScene::GetRotateHandle(sf::Vector2f worldPos) const
{
    if (!m_Selected) return false;

    sf::Vector2i zeroScreen{0, 0};
    sf::Vector2i hitScreen{16, 0};
    const sf::Vector2f wZero = m_Window.mapPixelToCoords(zeroScreen, m_camera);
    const sf::Vector2f wHit = m_Window.mapPixelToCoords(hitScreen, m_camera);
    const float hitRadius = std::abs(wHit.x - wZero.x);

    sf::Vector2f rotHandlePos = GetRotateHandlePos(m_Selected);
    const float dx = worldPos.x - rotHandlePos.x;
    const float dy = worldPos.y - rotHandlePos.y;
    return std::sqrt(dx * dx + dy * dy) <= hitRadius;
}


void EditorScene::SetDirty(bool dirty)
{
    m_HasUnsavedChanges = dirty;
    UpdateStatusText();
}

std::string EditorScene::NextId() { return "obj_" + std::to_string(m_IdCounter++); }


EditorObject *EditorScene::ObjectById(const std::string &id)
{
    for (auto &obj: m_Objects) { if (obj.id == id) return &obj; }
    return nullptr;
}

void EditorScene::RemoveObject(const std::string &id)
{
    if (m_LockedObjectId == id)
    {
        m_InspectorLocked = false;
        m_LockedObjectId.clear();
    }
    for (auto it = m_Objects.begin(); it != m_Objects.end(); ++it)
    {
        if (it->id == id)
        {
            if (m_Selected == &(*it)) m_Selected = nullptr;
            if (it->entity != 0) m_Registry.DestroyEntity(it->entity);
            m_Objects.erase(it);
            return;
        }
    }
}

void EditorScene::UpdateWorldTransforms()
{
    auto updateSubtree = [&](auto &self, EditorObject *obj, const sf::Transform &parentTransform, float parentRot,
                             float parentScaleX, float parentScaleY) -> void {
        if (!obj) return;

        if (obj->parentId.empty() || ObjectById(obj->parentId) == nullptr)
        {
            if (obj->localPosition.x == 0.f && obj->localPosition.y == 0.f &&
                (obj->shape.getPosition().x != 0.f || obj->shape.getPosition().y != 0.f))
            {
                obj->localPosition = obj->shape.getPosition();
            }
            obj->worldPosition = obj->localPosition;
            obj->worldRotation = obj->rotation;
            obj->worldScaleX = obj->scaleX;
            obj->worldScaleY = obj->scaleY;
        } else
        {
            auto *p = ObjectById(obj->parentId);
            if (obj->localPosition.x == 0.f && obj->localPosition.y == 0.f &&
                (obj->shape.getPosition().x != 0.f || obj->shape.getPosition().y != 0.f))
            {
                obj->localPosition = parentTransform.getInverse().transformPoint(obj->shape.getPosition());
            }
            obj->worldPosition = parentTransform.transformPoint(obj->localPosition);
            obj->worldRotation = parentRot + obj->rotation;
            obj->worldScaleX = parentScaleX * obj->scaleX;
            obj->worldScaleY = parentScaleY * obj->scaleY;
        }

        obj->shape.setPosition(obj->worldPosition);
        obj->shape.setRotation(obj->worldRotation);
        obj->shape.setScale(obj->worldScaleX, obj->worldScaleY);

        if (IsPolygonType(obj->objectType))
        {
            float rx = obj->shape.getSize().x * 0.5f;
            float ry = obj->shape.getSize().y * 0.5f;
            if (rx > 0.001f && ry > 0.001f)
            {
                obj->circleShape.setRadius(rx);
                obj->circleShape.setScale(obj->worldScaleX, obj->worldScaleY * (ry / rx));
            }
            obj->circleShape.setPosition(obj->worldPosition);
            obj->circleShape.setRotation(obj->worldRotation);
        }

        if (obj->objectType == ObjectType::Sprite && obj->previewTexture)
        {
            obj->previewSprite.setPosition(obj->worldPosition);
            obj->previewSprite.setRotation(obj->worldRotation);
            obj->previewSprite.setScale(obj->worldScaleX, obj->worldScaleY);
        }

        if (obj->entity != 0 && m_Registry.HasComponent<TransformComponent>(obj->entity))
        {
            auto &tc = m_Registry.GetComponent<TransformComponent>(obj->entity);
            tc.x = obj->localPosition.x;
            tc.y = obj->localPosition.y;
            tc.rotation = obj->rotation;
            tc.scaleX = obj->scaleX;
            tc.scaleY = obj->scaleY;
            tc.worldX = obj->worldPosition.x;
            tc.worldY = obj->worldPosition.y;
            tc.worldRotation = obj->worldRotation;
            tc.worldScaleX = obj->worldScaleX;
            tc.worldScaleY = obj->worldScaleY;
        }

        sf::Transform myTransform;
        myTransform.translate(obj->worldPosition);
        myTransform.rotate(obj->worldRotation);
        myTransform.scale(obj->worldScaleX, obj->worldScaleY);

        for (auto &other: m_Objects)
        {
            if (other.parentId == obj->id && other.id != obj->id)
            {
                self(self, &other, myTransform, obj->worldRotation, obj->worldScaleX, obj->worldScaleY);
            }
        }
    };

    sf::Transform identity;
    for (auto &obj: m_Objects)
    {
        if (obj.parentId.empty() || ObjectById(obj.parentId) == nullptr)
        {
            updateSubtree(updateSubtree, &obj, identity, 0.f, 1.f, 1.f);
        }
    }
}

bool EditorScene::IsDescendantOf(const std::string &childId, const std::string &ancestorId) const
{
    if (childId.empty() || ancestorId.empty()) return false;
    if (childId == ancestorId) return true;
    const EditorObject *cur = const_cast<EditorScene *>(this)->ObjectById(childId);
    while (cur && !cur->parentId.empty())
    {
        if (cur->parentId == ancestorId) return true;
        cur = const_cast<EditorScene *>(this)->ObjectById(cur->parentId);
    }
    return false;
}

std::vector<EditorObject *> EditorScene::GetChildren(const std::string &parentId)
{
    std::vector<EditorObject *> ch;
    for (auto &obj: m_Objects) { if (obj.parentId == parentId && obj.id != parentId) { ch.push_back(&obj); } }
    return ch;
}

void EditorScene::SetParent(const std::string &childId, const std::string &newParentId, bool keepWorldTransform)
{
    if (childId.empty() || childId == newParentId) return;
    auto *child = ObjectById(childId);
    if (!child) return;
    if (child->parentId == newParentId) return;

    if (!newParentId.empty() && IsDescendantOf(newParentId, childId))
    {
        std::cerr << "[WARN] [EditorScene] Cannot parent object to its own descendant!\n";
        return;
    }

    json before = SerializeObject(*child);

    if (keepWorldTransform)
    {
        if (newParentId.empty())
        {
            child->localPosition = child->worldPosition;
            child->rotation = child->worldRotation;
            child->scaleX = child->worldScaleX;
            child->scaleY = child->worldScaleY;
            child->parentId = "";
        } else
        {
            auto *newParent = ObjectById(newParentId);
            if (newParent)
            {
                sf::Transform parentWorldTransform;
                parentWorldTransform.translate(newParent->worldPosition);
                parentWorldTransform.rotate(newParent->worldRotation);
                parentWorldTransform.scale(newParent->worldScaleX, newParent->worldScaleY);

                child->localPosition = parentWorldTransform.getInverse().transformPoint(child->worldPosition);
                child->rotation = child->worldRotation - newParent->worldRotation;
                child->scaleX = (std::abs(newParent->worldScaleX) > 0.0001f)
                                    ? (child->worldScaleX / newParent->worldScaleX)
                                    : child->worldScaleX;
                child->scaleY = (std::abs(newParent->worldScaleY) > 0.0001f)
                                    ? (child->worldScaleY / newParent->worldScaleY)
                                    : child->worldScaleY;
                child->parentId = newParentId;
            }
        }
    } else { child->parentId = newParentId; }

    UpdateWorldTransforms();

    json after = SerializeObject(*child);
    if (before != after)
    {
        m_UndoStack.push_back(std::make_shared<ObjectStateCommand>(child->id, before, after));
        m_RedoStack.clear();
    }
}


void EditorScene::ExecuteCommand(std::shared_ptr<EditorCommand> command)
{
    command->Execute(this);
    m_UndoStack.push_back(command);
    m_RedoStack.clear();
    SetDirty(true);
}

void EditorScene::UndoCommand()
{
    if (!m_UndoStack.empty())
    {
        auto cmd = m_UndoStack.back();
        m_UndoStack.pop_back();
        cmd->Undo(this);
        m_RedoStack.push_back(cmd);
        m_Selected = nullptr;
        SetDirty(true);
        UpdateStatusText();
    }
}

void EditorScene::RedoCommand()
{
    if (!m_RedoStack.empty())
    {
        auto cmd = m_RedoStack.back();
        m_RedoStack.pop_back();
        cmd->Execute(this);
        m_UndoStack.push_back(cmd);
        m_Selected = nullptr;
        SetDirty(true);
        UpdateStatusText();
    }
}

void ObjectStateCommand::Execute(EditorScene *scene)
{
    if (afterState.is_null()) { scene->RemoveObject(objectId); } else
    {
        auto *obj = scene->ObjectById(objectId);
        if (obj) { scene->ApplyState(*obj, afterState); } else { scene->DeserializeObject(afterState); }
    }
}

void ObjectStateCommand::Undo(EditorScene *scene)
{
    if (beforeState.is_null()) { scene->RemoveObject(objectId); } else
    {
        auto *obj = scene->ObjectById(objectId);
        if (obj) { scene->ApplyState(*obj, beforeState); } else { scene->DeserializeObject(beforeState); }
    }
}

void EditorScene::ClearSelection()
{
    for (auto *o: m_SelectedObjects) o->selected = false;
    m_SelectedObjects.clear();
    m_Selected = nullptr;
}

void EditorScene::SelectObject(EditorObject *obj, bool multi)
{
    if (!multi) ClearSelection();
    if (obj)
    {
        if (std::find(m_SelectedObjects.begin(), m_SelectedObjects.end(), obj) == m_SelectedObjects.end())
        {
            m_SelectedObjects.push_back(obj);
            obj->selected = true;
        }
        m_Selected = m_SelectedObjects.size() == 1 ? m_SelectedObjects.back() : nullptr;
        if (!m_InspectorLocked) { m_InspectorScrollY = 0.f; }
    }
}

bool EditorScene::IsSelected(const EditorObject *obj) const
{
    return std::find(m_SelectedObjects.begin(), m_SelectedObjects.end(), obj) != m_SelectedObjects.end();
}

void EditorScene::CopySelection()
{
    m_ClipboardObjects.clear();
    for (auto *obj: m_SelectedObjects) { if (obj) { m_ClipboardObjects.push_back(SerializeObject(*obj)); } }
    if (m_ClipboardObjects.empty() && m_Selected) { m_ClipboardObjects.push_back(SerializeObject(*m_Selected)); }
}

void EditorScene::PasteClipboard()
{
    if (m_ClipboardObjects.empty()) return;

    ClearSelection();
    auto macroCmd = std::make_shared<MacroCommand>();

    for (const auto &item: m_ClipboardObjects)
    {
        json j = item;
        std::string newId = NextId();
        j["id"] = newId;
        j.erase("entity");
        j["x"] = j.value("x", 0.f) + m_GridSize;
        j["y"] = j.value("y", 0.f) + m_GridSize;

        DeserializeObject(j);
        EditorObject *newObj = ObjectById(newId);
        if (newObj)
        {
            SelectObject(newObj, true);
            macroCmd->commands.push_back(
                std::make_shared<ObjectStateCommand>(newId, json(nullptr), SerializeObject(*newObj)));
        }
    }

    if (!macroCmd->commands.empty())
    {
        m_UndoStack.push_back(macroCmd);
        m_RedoStack.clear();
    }
    UpdateStatusText();
}


std::string ResolveCMakeExecutable()
{
    if (system("cmake --version >nul 2>&1") == 0) { return "cmake"; }
#ifdef _WIN32
    std::vector<std::string> candidates = {
        "\"C:\\Program Files\\JetBrains\\CLion 2025.1\\bin\\cmake\\win\\x64\\bin\\cmake.exe\"",
        "\"C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe\"",
        "\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe\""
    };
    for (const auto &c: candidates)
    {
        std::string test = c + " --version >nul 2>&1";
        if (system(test.c_str()) == 0) { return c; }
    }
#else
    if (system("cmake --version >/dev/null 2>&1") == 0) { return "cmake"; }
#endif
    return "";
}

void EditorScene::ExportStandaloneGame()
{
    SyncToRegistry();
    SaveToJson(std::string(ASSET_PATH) + "/" + m_SceneSavePath);
    SaveProjectSettings();
    if (!UIManager::Get().GetElements().empty())
    {
        std::string uiSavePath = std::string(ASSET_PATH) + "/" + m_SceneSavePath.substr(
                                     0, m_SceneSavePath.find_last_of('.')) + "_ui.json";
        UIManager::Get().Save(uiSavePath);
    }
    ConsolePanel::AddLogGlobal("[INFO] Auto-saved scene and project settings before build.", false);
    ConsolePanel::AddLogGlobal("Starting standalone game build and export...", false);

    m_ShowBuildPopup = true;
    m_BuildFinished = false;
    m_BuildProgress = 0.0f;
    m_CancelBuildRequested = false;
    m_BuildStatusText = "Preparing build...";

    std::thread([this]() {
        auto updateStatus = [this](const std::string &text, float progress) {
            std::lock_guard<std::mutex> lock(m_BuildMutex);
            m_BuildStatusText = text;
            if (progress >= 0.0f) m_BuildProgress = progress;
        };

        auto checkCancel = [this]() -> bool {
            std::lock_guard<std::mutex> lock(m_BuildMutex);
            return m_CancelBuildRequested;
        };

        std::filesystem::path rootDir = FindProjectRoot();
        std::filesystem::path appDir = GetAppDir();
        std::string cmakeBin = ResolveCMakeExecutable();

        std::filesystem::path buildDir;
        if (std::filesystem::exists(rootDir / "CMakeLists.txt"))
        {
            if (std::filesystem::exists(rootDir / "cmake-build-debug" / "CMakeCache.txt"))
            {
                buildDir = rootDir / "cmake-build-debug";
            } else if (std::filesystem::exists(rootDir / "cmake-build-release" / "CMakeCache.txt"))
            {
                buildDir = rootDir / "cmake-build-release";
            } else if (std::filesystem::exists(rootDir / "build" / "CMakeCache.txt")) { buildDir = rootDir / "build"; }
        }

        if (!cmakeBin.empty() && !buildDir.empty())
        {
            updateStatus("Building RayneGame target with CMake...", 20.0f);
            ConsolePanel::AddLogGlobal("Building RayneGame in: " + buildDir.string(), false);

            std::string cmdBuild = cmakeBin + " --build \"" + buildDir.string() + "\" --target RayneGame 2>&1";
            FILE *pipe =
#ifdef _WIN32
                    _popen(cmdBuild.c_str(), "r");
#else
                    popen(cmdBuild.c_str(), "r");
#endif
            if (pipe)
            {
                char buffer[256];
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
                {
                    if (checkCancel()) break;
                    std::string line = buffer;
                    if (!line.empty() && line.back() == '\n') line.pop_back();
                    ConsolePanel::AddLogGlobal(line, false);
                    updateStatus(line, -1.f);
                }
#ifdef _WIN32
                _pclose(pipe);
#else
                pclose(pipe);
#endif
            }
        }

        if (checkCancel())
        {
            updateStatus("Export cancelled.", 0.0f);
            std::lock_guard<std::mutex> lock(m_BuildMutex);
            m_BuildFinished = true;
            return;
        }

        std::filesystem::path exePath;
        std::vector<std::filesystem::path> possibleExePaths;
        if (!buildDir.empty())
        {
            possibleExePaths.push_back(buildDir / "RayneGame.exe");
            possibleExePaths.push_back(buildDir / "Release" / "RayneGame.exe");
            possibleExePaths.push_back(buildDir / "Debug" / "RayneGame.exe");
        }
        possibleExePaths.push_back(rootDir / "templates" / "RayneGame.exe");
        possibleExePaths.push_back(appDir / "templates" / "RayneGame.exe");
        possibleExePaths.push_back(rootDir / "RayneGame.exe");
        possibleExePaths.push_back(appDir / "RayneGame.exe");

        for (const auto &p: possibleExePaths)
        {
            if (!p.empty() && std::filesystem::exists(p))
            {
                exePath = p;
                break;
            }
        }

        if (!exePath.empty())
        {
            updateStatus("Packaging game into Export/...", 70.0f);
            try
            {
                std::filesystem::path exportDir = rootDir / "Export";
                std::filesystem::create_directories(exportDir);

                std::string outExeName = m_ProjectName.empty() ? "RayneGame.exe" : m_ProjectName + ".exe";
                std::filesystem::copy_file(exePath, exportDir / outExeName,
                                           std::filesystem::copy_options::overwrite_existing);

                updateStatus("Copying assets...", 80.0f);
                std::error_code ec;
                if (std::filesystem::exists(rootDir / "assets"))
                {
                    std::filesystem::copy(rootDir / "assets", exportDir / "assets",
                                          std::filesystem::copy_options::recursive |
                                          std::filesystem::copy_options::overwrite_existing);
                }
                if (std::filesystem::exists(appDir / "assets") && !
                    std::filesystem::equivalent(appDir / "assets", rootDir / "assets", ec))
                {
                    std::filesystem::copy(appDir / "assets", exportDir / "assets",
                                          std::filesystem::copy_options::recursive |
                                          std::filesystem::copy_options::overwrite_existing);
                    std::filesystem::copy(appDir / "assets", rootDir / "assets",
                                          std::filesystem::copy_options::recursive |
                                          std::filesystem::copy_options::overwrite_existing);
                }
                auto cwdAssets = std::filesystem::current_path() / "assets";
                if (std::filesystem::exists(cwdAssets) &&
                    (!std::filesystem::exists(rootDir / "assets") || !std::filesystem::equivalent(
                         cwdAssets, rootDir / "assets", ec)) &&
                    (!std::filesystem::exists(appDir / "assets") || !
                     std::filesystem::equivalent(cwdAssets, appDir / "assets", ec)))
                {
                    std::filesystem::copy(cwdAssets, exportDir / "assets",
                                          std::filesystem::copy_options::recursive |
                                          std::filesystem::copy_options::overwrite_existing);
                    std::filesystem::copy(cwdAssets, rootDir / "assets",
                                          std::filesystem::copy_options::recursive |
                                          std::filesystem::copy_options::overwrite_existing);
                }

                updateStatus("Copying engine_content...", 90.0f);
                if (std::filesystem::exists(rootDir / "engine_content"))
                {
                    std::filesystem::copy(rootDir / "engine_content", exportDir / "engine_content",
                                          std::filesystem::copy_options::recursive |
                                          std::filesystem::copy_options::overwrite_existing);
                }
                if (std::filesystem::exists(appDir / "engine_content") && !
                    std::filesystem::equivalent(appDir / "engine_content", rootDir / "engine_content", ec))
                {
                    std::filesystem::copy(appDir / "engine_content", exportDir / "engine_content",
                                          std::filesystem::copy_options::recursive |
                                          std::filesystem::copy_options::overwrite_existing);
                }
                auto cwdEngine = std::filesystem::current_path() / "engine_content";
                if (std::filesystem::exists(cwdEngine) &&
                    (!std::filesystem::exists(rootDir / "engine_content") || !std::filesystem::equivalent(
                         cwdEngine, rootDir / "engine_content", ec)) &&
                    (!std::filesystem::exists(appDir / "engine_content") || !
                     std::filesystem::equivalent(cwdEngine, appDir / "engine_content", ec)))
                {
                    std::filesystem::copy(cwdEngine, exportDir / "engine_content",
                                          std::filesystem::copy_options::recursive |
                                          std::filesystem::copy_options::overwrite_existing);
                }

                std::vector<std::filesystem::path> dllSearchDirs = {
                    appDir,
                    exePath.parent_path(),
                    rootDir
                };
                if (!buildDir.empty()) { dllSearchDirs.push_back(buildDir); }
                for (const auto &sDir: dllSearchDirs)
                {
                    if (std::filesystem::exists(sDir))
                    {
                        for (const auto &entry: std::filesystem::directory_iterator(sDir))
                        {
                            if (entry.path().extension() == ".dll")
                            {
                                std::filesystem::copy_file(entry.path(), exportDir / entry.path().filename(),
                                                           std::filesystem::copy_options::overwrite_existing);
                            }
                        }
                    }
                }

#ifdef _WIN32
                SetPathReadOnly(exportDir / "engine_content", true);
                if (std::filesystem::exists(exportDir / "assets" / "scripting"))
                    SetPathReadOnly(exportDir / "assets" / "scripting", true);
#endif

                std::string zipName = (m_ProjectName.empty() ? "RayneGame" : m_ProjectName) + "_Standalone.zip";
                std::filesystem::path zipTarget = rootDir / zipName;
                bool zipCreated = false;
#ifdef _WIN32
                std::string tarCmd = "cd /d \"" + exportDir.string() + "\" && tar.exe -a -cf \"" + zipTarget.string() +
                                     "\" *";
                FILE *pZip = _popen(tarCmd.c_str(), "r");
                if (pZip)
                {
                    int ret = _pclose(pZip);
                    if (ret == 0 && std::filesystem::exists(zipTarget)) zipCreated = true;
                }
#endif
                if (!zipCreated && !cmakeBin.empty())
                {
                    std::string zipCmd = cmakeBin + " -E tar cf \"" + zipTarget.string() + "\" --format=zip .";
#ifdef _WIN32
                    FILE *pZip = _popen(("cd /d \"" + exportDir.string() + "\" && " + zipCmd).c_str(), "r");
                    if (pZip) _pclose(pZip);
#else
                    FILE *pZip = popen(("cd \"" + exportDir.string() + "\" && " + zipCmd).c_str(), "r");
                    if (pZip) pclose(pZip);
#endif
                    if (std::filesystem::exists(zipTarget)) zipCreated = true;
                }
                if (zipCreated)
                {
                    ConsolePanel::AddLogGlobal("Standalone game zip created: " + zipTarget.string(), false);
                }

                updateStatus("Export finished! Exported to Export/ folder.", 100.0f);
                ConsolePanel::AddLogGlobal("Standalone game exported successfully to: " + exportDir.string(), false);

#ifdef _WIN32
                ShellExecuteA(NULL, "explore", exportDir.string().c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif
            } catch (const std::exception &e)
            {
                updateStatus(std::string("Packaging error: ") + e.what(), 100.0f);
                ConsolePanel::AddLogGlobal(std::string("Export error: ") + e.what(), true);
            }
        } else
        {
            updateStatus("Export failed: templates/RayneGame.exe not found.", 0.0f);
            ConsolePanel::AddLogGlobal(
                "Export failed: templates/RayneGame.exe not found. Please ensure templates/RayneGame.exe is present.",
                true);
        } {
            std::lock_guard<std::mutex> lock(m_BuildMutex);
            m_BuildFinished = true;
        }
    }).detach();
}

void EditorScene::PackageEngineZip()
{ {
        std::lock_guard<std::mutex> lock(m_BuildMutex);
        if (m_ShowBuildPopup && !m_BuildFinished)
        {
            ConsolePanel::AddLogGlobal("Another packaging/build task is already in progress.", true);
            return;
        }
        m_ShowBuildPopup = true;
        m_BuildFinished = false;
        m_CancelBuildRequested = false;
        m_BuildProgress = 0.0f;
        m_BuildStatusText = "Starting Engine packaging...";
    }

    std::thread([this]() {
        auto updateStatus = [this](const std::string &status, float progress) {
            std::lock_guard<std::mutex> lock(m_BuildMutex);
            m_BuildStatusText = status;
            if (progress >= 0.0f) m_BuildProgress = progress;
        };

        auto checkCancel = [this]() {
            std::lock_guard<std::mutex> lock(m_BuildMutex);
            return m_CancelBuildRequested;
        };

        try
        {
            updateStatus("Locating project and engine files...", 10.0f);
            std::filesystem::path rootDir = FindProjectRoot();
            std::filesystem::path appDir = GetAppDir();
            std::string cmakeBin = ResolveCMakeExecutable();

            std::filesystem::path engineExeDir = appDir;
            if (!std::filesystem::exists(engineExeDir / "RayneEngine.exe"))
            {
                if (std::filesystem::exists(rootDir / "RayneEngine.exe")) { engineExeDir = rootDir; } else if (
                    std::filesystem::exists(rootDir / "cmake-build-release" / "RayneEngine.exe"))
                {
                    engineExeDir = rootDir / "cmake-build-release";
                } else if (std::filesystem::exists(rootDir / "cmake-build-debug" / "RayneEngine.exe"))
                {
                    engineExeDir = rootDir / "cmake-build-debug";
                }
            }

            updateStatus("Preparing package contents...", 30.0f);
            std::filesystem::path zipTarget = rootDir / "RayneEngine-Editor.zip";

            updateStatus("Packaging RayneEngine into " + zipTarget.filename().string() + "...", 50.0f);
            ConsolePanel::AddLogGlobal("Packaging RayneEngine Editor to: " + zipTarget.string(), false);

            bool packaged = false;
#ifdef _WIN32
            SetPathReadOnly(engineExeDir / "engine_content", true);
            SetPathReadOnly(engineExeDir / "templates", true);
            if (std::filesystem::exists(engineExeDir / "assets" / "scripting"))
                SetPathReadOnly(engineExeDir / "assets" / "scripting", true);

            std::string tarCmd = "cd /d \"" + engineExeDir.string() + "\" && tar.exe -a -cf \"" + zipTarget.string() +
                                 "\" RayneEngine.exe assets engine_content templates *.dll";
            FILE *pPipe = _popen(tarCmd.c_str(), "r");
            if (pPipe)
            {
                int ret = _pclose(pPipe);
                if (ret == 0 && std::filesystem::exists(zipTarget)) packaged = true;
            }
#endif
            if (!packaged && !cmakeBin.empty())
            {
                std::string cmd = cmakeBin + " -E tar cf \"" + zipTarget.string() +
                                  "\" --format=zip \"RayneEngine.exe\" \"assets\" \"engine_content\" \"templates\"";
#ifdef _WIN32
                for (const auto &entry: std::filesystem::directory_iterator(engineExeDir))
                {
                    if (entry.path().extension() == ".dll") { cmd += " \"" + entry.path().filename().string() + "\""; }
                }
                std::string fullCmd = "cd /d \"" + engineExeDir.string() + "\" && " + cmd;
                FILE *pipe = _popen(fullCmd.c_str(), "r");
                if (pipe)
                {
                    char buf[256];
                    while (fgets(buf, sizeof(buf), pipe) != nullptr) {}
                    _pclose(pipe);
                }
#else
                std::string fullCmd = "cd \"" + engineExeDir.string() + "\" && " + cmd;
                FILE *pipe = popen(fullCmd.c_str(), "r");
                if (pipe)
                {
                    char buf[256];
                    while (fgets(buf, sizeof(buf), pipe) != nullptr) {}
                    pclose(pipe);
                }
#endif
                if (std::filesystem::exists(zipTarget)) packaged = true;
            }

            updateStatus("Engine packaged successfully!", 100.0f);
            ConsolePanel::AddLogGlobal("RayneEngine-Editor.zip created successfully in: " + rootDir.string(), false);

#ifdef _WIN32
            ShellExecuteA(NULL, "explore", rootDir.string().c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif
        } catch (const std::exception &e)
        {
            updateStatus(std::string("Packaging error: ") + e.what(), 100.0f);
            ConsolePanel::AddLogGlobal(std::string("Package error: ") + e.what(), true);
        } {
            std::lock_guard<std::mutex> lock(m_BuildMutex);
            m_BuildFinished = true;
        }
    }).detach();
}


std::string ColorToHex(sf::Color c)
{
    char buf[12];
    snprintf(buf, sizeof(buf), "#%02X%02X%02X", c.r, c.g, c.b);
    return std::string(buf);
}

sf::Color HexToColor(const std::string &hex, sf::Color def)
{
    if (hex.empty()) return def;
    std::string s = hex;
    if (s[0] == '#') s = s.substr(1);
    if (s.size() != 6) return def;
    try
    {
        unsigned int val = std::stoul(s, nullptr, 16);
        return sf::Color((val >> 16) & 0xFF, (val >> 8) & 0xFF, val & 0xFF);
    } catch (...) { return def; }
}


void EditorScene::SaveAsTemplate(EditorObject *obj, const std::string &name)
{
    if (!obj) return;

    if (name.empty())
    {
        m_SaveTemplateTargetId = obj->id;
        m_SaveTemplateInputName = obj->id;
        m_ShowSaveTemplatePrompt = true;
        return;
    }

    std::string cleanName = name;
    if (cleanName.size() > 9 && cleanName.substr(cleanName.size() - 9) == ".template")
    {
        cleanName = cleanName.substr(0, cleanName.size() - 9);
    }

    std::filesystem::path rootDir = m_ContentBrowser
                                        ? std::filesystem::path(m_ContentBrowser->GetRootPath())
                                        : (FindProjectRoot() / "assets");
    std::filesystem::path templateDir = rootDir / "templates";
    std::error_code ec;
    std::filesystem::create_directories(templateDir, ec);

    std::filesystem::path targetFile = templateDir / (cleanName + ".template");

    std::vector<EditorObject *> hierarchy;
    std::vector<EditorObject *> queue = {obj};
    while (!queue.empty())
    {
        EditorObject *curr = queue.front();
        queue.erase(queue.begin());
        hierarchy.push_back(curr);
        for (EditorObject *child: GetChildren(curr->id)) { queue.push_back(child); }
    }

    json data;
    data["name"] = cleanName;
    data["objects"] = json::array();

    for (size_t i = 0; i < hierarchy.size(); ++i)
    {
        EditorObject *item = hierarchy[i];
        json j = SerializeObject(*item);
        if (i == 0)
        {
            j["parent"] = "";
            j["x"] = 0.f;
            j["y"] = 0.f;
        }
        data["objects"].push_back(j);
    }

    std::ofstream out(targetFile);
    if (!out.is_open())
    {
        std::cerr << "[ERROR] [EditorScene] Could not open template file for writing: " << targetFile << "\n";
        return;
    }
    out << data.dump(4);
    out.close();
    std::string relPath = std::filesystem::proximate(targetFile, rootDir, ec).generic_string();
    obj->templatePath = ec ? targetFile.generic_string() : ("assets/" + relPath);
    if (m_ContentBrowser) { m_ContentBrowser->Refresh(); }
    SetDirty(true);
    UpdateStatusText();
    std::cout << "[INFO] [EditorScene] Saved template: " << targetFile << "\n";
}

void EditorScene::ApplyToTemplate(EditorObject *obj)
{
    if (!obj || obj->templatePath.empty()) return;

    std::filesystem::path rootDir = m_ContentBrowser
                                        ? std::filesystem::path(m_ContentBrowser->GetRootPath())
                                        : (FindProjectRoot() / "assets");
    std::filesystem::path targetFile(obj->templatePath);
    if (!targetFile.is_absolute())
    {
        if (obj->templatePath.rfind("assets/", 0) == 0) { targetFile = rootDir.parent_path() / obj->templatePath; } else
        {
            targetFile = rootDir / targetFile;
        }
    }

    std::vector<EditorObject *> hierarchy;
    std::vector<EditorObject *> queue = {obj};
    while (!queue.empty())
    {
        EditorObject *curr = queue.front();
        queue.erase(queue.begin());
        hierarchy.push_back(curr);
        for (EditorObject *child: GetChildren(curr->id)) { queue.push_back(child); }
    }

    json data;
    data["name"] = targetFile.stem().string();
    data["objects"] = json::array();

    for (size_t i = 0; i < hierarchy.size(); ++i)
    {
        EditorObject *item = hierarchy[i];
        json j = SerializeObject(*item);
        if (i == 0)
        {
            j["parent"] = "";
            j["x"] = 0.f;
            j["y"] = 0.f;
        }
        data["objects"].push_back(j);
    }

    std::ofstream out(targetFile);
    if (!out.is_open())
    {
        std::cerr << "[ERROR] [EditorScene] Could not open template file for writing: " << targetFile << "\n";
        return;
    }
    out << data.dump(4);
    out.close();

    if (m_ContentBrowser) { m_ContentBrowser->Refresh(); }
    SyncTemplateInstances(obj->templatePath);
    std::cout << "[INFO] [EditorScene] Applied changes to template: " << targetFile << "\n";
}

EditorObject *EditorScene::InstantiateTemplateOnCanvas(const std::string &templatePath, sf::Vector2f pos)
{
    std::filesystem::path rootDir = m_ContentBrowser
                                        ? std::filesystem::path(m_ContentBrowser->GetRootPath())
                                        : (FindProjectRoot() / "assets");
    std::filesystem::path tp(templatePath);
    if (!tp.is_absolute())
    {
        if (templatePath.rfind("assets/", 0) == 0) { tp = rootDir.parent_path() / templatePath; } else
        {
            tp = rootDir / tp;
        }
    }

    std::ifstream file(tp);
    if (!file.is_open())
    {
        std::cerr << "[ERROR] [EditorScene] Failed to open template file: " << tp << "\n";
        return nullptr;
    }

    json data;
    try { data = json::parse(file); } catch (const std::exception &e)
    {
        std::cerr << "[ERROR] [EditorScene] JSON parse error: " << e.what() << "\n";
        return nullptr;
    }

    if (!data.contains("objects") || !data["objects"].is_array() || data["objects"].empty())
    {
        std::cerr << "[ERROR] [EditorScene] Template contains no objects: " << tp << "\n";
        return nullptr;
    }

    std::unordered_map<std::string, std::string> oldToNewId;
    std::vector<std::string> newIds;
    EditorObject *rootObj = nullptr;

    for (const auto &item: data["objects"])
    {
        std::string oldId = item.value("id", "");
        std::string newId = NextId();
        if (!oldId.empty()) { oldToNewId[oldId] = newId; }
        newIds.push_back(newId);
    }

    auto macroCmd = std::make_shared<MacroCommand>();

    for (size_t i = 0; i < data["objects"].size(); ++i)
    {
        json j = data["objects"][i];
        std::string newId = newIds[i];
        j["id"] = newId;
        j.erase("entity");

        std::string oldParent = j.value("parent", "");
        if (i == 0 || oldParent.empty())
        {
            j["parent"] = "";
            j["x"] = pos.x;
            j["y"] = pos.y;
            j["template"] = templatePath;
        } else { if (oldToNewId.find(oldParent) != oldToNewId.end()) { j["parent"] = oldToNewId[oldParent]; } }

        DeserializeObject(j);
        EditorObject *newObj = ObjectById(newId);
        if (newObj)
        {
            if (i == 0) rootObj = newObj;
            macroCmd->commands.push_back(
                std::make_shared<ObjectStateCommand>(newId, json(nullptr), SerializeObject(*newObj)));
        }
    }

    if (!macroCmd->commands.empty())
    {
        m_UndoStack.push_back(macroCmd);
        m_RedoStack.clear();
    }

    ClearSelection();
    if (rootObj) { SelectObject(rootObj, false); }

    UpdateWorldTransforms();
    SetDirty(true);
    UpdateStatusText();
    return rootObj;
}


bool EditorScene::ValidateAllScripts(std::string &outError, std::string &outPath)
{
    std::set<std::string> checkedScripts;
    for (const auto &obj: m_Objects)
    {
        if (obj.scriptPath.empty()) continue;
        if (checkedScripts.count(obj.scriptPath)) continue;
        checkedScripts.insert(obj.scriptPath);

        std::string resolvedPath = ResourceManager::ResolveAssetPath(obj.scriptPath);
        std::error_code ec;
        if (!std::filesystem::exists(resolvedPath, ec))
        {
            outPath = obj.scriptPath;
            outError = "Script file not found: " + obj.scriptPath;
            return false;
        }

        sol::state &lua = LuaState::GetLua();
        sol::load_result lr = lua.load_file(resolvedPath);
        if (!lr.valid())
        {
            sol::error err = lr;
            outPath = resolvedPath;
            outError = err.what();
            return false;
        }
        sol::environment testEnv(lua, sol::create, lua.globals());
        testEnv["self"] = obj.entity != 0 ? obj.entity : 1;
        sol::protected_function pf = lr;
        sol::set_environment(testEnv, pf);
        sol::protected_function_result pr = pf();
        if (!pr.valid())
        {
            sol::error err = pr;
            outPath = resolvedPath;
            outError = err.what();
            return false;
        }
    }
    return true;
}

void EditorScene::TryLaunchPlayMode()
{
    std::string scriptError;
    std::string scriptErrPath;
    if (!ValidateAllScripts(scriptError, scriptErrPath))
    {
        std::cerr << "[ERROR] [EditorScene] Script error prevented game launch: " << scriptError << " in (" <<
                scriptErrPath << ")\n";
        ConsolePanel::AddLogGlobal("[ERROR] [Script Check] Game launch canceled! " + scriptError, true);
        m_ShowScriptErrorModal = true;
        m_ScriptErrorDetails = scriptError;
        m_ScriptErrorPath = scriptErrPath;
        return;
    }

    SyncToRegistry();
    SaveToJson(std::string(ASSET_PATH) + "/" + m_SceneSavePath);
    std::cout << "[INFO] [EditorScene] Auto-saved scene before running.\n";
    SnapshotState();
    m_manager.SwitchSceneTo("game");
}

void EditorScene::SyncTemplateInstances(const std::string &templatePath)
{
    if (templatePath.empty()) return;

    std::filesystem::path rootDir = m_ContentBrowser
                                        ? std::filesystem::path(m_ContentBrowser->GetRootPath())
                                        : (FindProjectRoot() / "assets");
    std::filesystem::path tp(templatePath);
    if (!tp.is_absolute())
    {
        if (templatePath.rfind("assets/", 0) == 0) { tp = rootDir.parent_path() / templatePath; }
        else { tp = rootDir / tp; }
    }

    std::ifstream file(tp);
    if (!file.is_open()) return;

    json data;
    try { data = json::parse(file); }
    catch (...) { return; }

    if (!data.contains("objects") || !data["objects"].is_array() || data["objects"].empty())
        return;

    const auto &tmplRoot = data["objects"][0];

    std::filesystem::path targetP(templatePath);
    std::string targetFilename = targetP.filename().string();

    int updatedCount = 0;
    for (auto &obj : m_Objects)
    {
        if (obj.templatePath.empty()) continue;
        bool matches = (obj.templatePath == templatePath);
        if (!matches)
        {
            std::filesystem::path objP(obj.templatePath);
            if (!targetFilename.empty() && objP.filename() == targetFilename)
            {
                matches = true;
            }
        }
        if (matches)
        {
            if (tmplRoot.contains("tag")) obj.tag = tmplRoot["tag"];
            if (tmplRoot.contains("color") && tmplRoot["color"].is_array() && tmplRoot["color"].size() >= 3)
            {
                obj.color = sf::Color(tmplRoot["color"][0], tmplRoot["color"][1], tmplRoot["color"][2]);
                obj.shape.setFillColor(obj.color);
            }
            if (tmplRoot.contains("width") && tmplRoot.contains("height"))
            {
                obj.shape.setSize({tmplRoot["width"].get<float>(), tmplRoot["height"].get<float>()});
            }
            if (tmplRoot.contains("scaleX")) obj.scaleX = tmplRoot["scaleX"];
            if (tmplRoot.contains("scaleY")) obj.scaleY = tmplRoot["scaleY"];
            obj.shape.setScale(obj.scaleX, obj.scaleY);

            if (tmplRoot.contains("visibleInGame")) obj.visibleInGame = tmplRoot["visibleInGame"];
            if (tmplRoot.contains("script"))
            {
                std::string sPath = tmplRoot["script"].get<std::string>();
                if (sPath != obj.scriptPath)
                {
                    obj.scriptPath = sPath;
                    if (obj.entity != 0 && m_Registry.HasComponent<ScriptComponent>(obj.entity))
                    {
                        m_Registry.RemoveComponent<ScriptComponent>(obj.entity);
                        if (!sPath.empty())
                        {
                            std::string resolved = ResourceManager::ResolveAssetPath(sPath);
                            m_Registry.AddComponent(obj.entity, ScriptComponent(LuaState::GetLua(), resolved, obj.entity));
                        }
                    }
                }
            }
            if (tmplRoot.contains("sprite"))
            {
                std::string sp = tmplRoot["sprite"].get<std::string>();
                if (sp != obj.spritePath)
                {
                    ApplySpriteToObject(obj, sp);
                }
            }
            if (tmplRoot.contains("scriptProperties") && tmplRoot["scriptProperties"].is_object())
            {
                for (auto it = tmplRoot["scriptProperties"].begin(); it != tmplRoot["scriptProperties"].end(); ++it)
                {
                    ScriptComponent::Property prop;
                    prop.name = it.key();
                    prop.type = static_cast<ScriptComponent::PropertyType>(it.value().value("type", 0));
                    if (prop.type == ScriptComponent::PropertyType::Int) prop.intVal = it.value().value("value", 0);
                    else if (prop.type == ScriptComponent::PropertyType::Float) prop.floatVal = it.value().value("value", 0.f);
                    else if (prop.type == ScriptComponent::PropertyType::Bool) prop.boolVal = it.value().value("value", false);
                    else if (prop.type == ScriptComponent::PropertyType::String ||
                             prop.type == ScriptComponent::PropertyType::Template ||
                             prop.type == ScriptComponent::PropertyType::Image ||
                             prop.type == ScriptComponent::PropertyType::Entity)
                        prop.stringVal = it.value().value("value", "");
                    else if (prop.type == ScriptComponent::PropertyType::Vec2 && it.value()["value"].is_object())
                    {
                        prop.floatVal = it.value()["value"].value("x", 0.f);
                        prop.vec2Y = it.value()["value"].value("y", 0.f);
                    }
                    else if (prop.type == ScriptComponent::PropertyType::Color && it.value()["value"].is_object())
                    {
                        prop.colorR = it.value()["value"].value("r", 255);
                        prop.colorG = it.value()["value"].value("g", 255);
                        prop.colorB = it.value()["value"].value("b", 255);
                    }
                    obj.scriptProperties[prop.name] = prop;
                    if (obj.entity != 0 && m_Registry.HasComponent<ScriptComponent>(obj.entity))
                    {
                        m_Registry.GetComponent<ScriptComponent>(obj.entity).SetExportedProperty(prop);
                    }
                }
            }
            updatedCount++;
        }
    }

    UpdateWorldTransforms();
    if (updatedCount > 0)
    {
        SetDirty(true);
        std::cout << "[INFO] [EditorScene] Auto-synchronized " << updatedCount << " instances of template: " << templatePath << "\n";
    }
}

void EditorScene::SaveTemplateFile(const std::string &templatePath)
{
    if (templatePath.empty()) return;

    std::filesystem::path rootDir = m_ContentBrowser
                                        ? std::filesystem::path(m_ContentBrowser->GetRootPath())
                                        : (FindProjectRoot() / "assets");
    std::filesystem::path tp(templatePath);
    if (!tp.is_absolute())
    {
        if (templatePath.rfind("assets/", 0) == 0) { tp = rootDir.parent_path() / templatePath; }
        else { tp = rootDir / tp; }
    }

    json data;
    data["name"] = tp.stem().string();
    data["objects"] = json::array();

    size_t idx = 0;
    for (const auto &obj : m_Objects)
    {
        json j = SerializeObject(obj);
        if (idx == 0)
        {
            j["parent"] = "";
            j["x"] = 0.f;
            j["y"] = 0.f;
        }
        data["objects"].push_back(j);
        idx++;
    }

    std::ofstream out(tp);
    if (out.is_open())
    {
        out << data.dump(4);
        out.close();
        std::cout << "[INFO] [EditorScene] Saved template edits to: " << tp << "\n";
    }
    else
    {
        std::cerr << "[ERROR] [EditorScene] Failed to save template edits to: " << tp << "\n";
    }
}

void EditorScene::EnterTemplateEditMode(const std::string &templatePath)
{
    if (m_EditingTemplate)
    {
        ExitTemplateEditMode(true);
    }

    std::filesystem::path rootDir = m_ContentBrowser
                                        ? std::filesystem::path(m_ContentBrowser->GetRootPath())
                                        : (FindProjectRoot() / "assets");
    std::filesystem::path tp(templatePath);
    if (!tp.is_absolute())
    {
        if (templatePath.rfind("assets/", 0) == 0) { tp = rootDir.parent_path() / templatePath; }
        else { tp = rootDir / tp; }
    }

    std::ifstream file(tp);
    if (!file.is_open())
    {
        std::vector<std::filesystem::path> candidates = {
            rootDir / templatePath,
            rootDir / "templates" / templatePath,
            rootDir.parent_path() / templatePath,
            FindProjectRoot() / "assets" / templatePath,
            FindProjectRoot() / "assets" / "templates" / templatePath,
            FindProjectRoot() / "assets" / "templates" / std::filesystem::path(templatePath).filename()
        };
        for (const auto &cand : candidates)
        {
            if (std::filesystem::exists(cand))
            {
                tp = cand;
                file.open(tp);
                if (file.is_open()) break;
            }
        }
    }

    if (!file.is_open())
    {
        std::cerr << "[ERROR] [EditorScene] Cannot open template file for editing: " << tp << "\n";
        return;
    }

    json data;
    try { data = json::parse(file); }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] [EditorScene] JSON parse error: " << e.what() << "\n";
        return;
    }

    // Snapshot existing scene
    SyncToRegistry();
    m_PreTemplateEditSceneState = json{};
    m_PreTemplateEditSceneState["objects"] = json::array();
    for (auto &obj : m_Objects)
    {
        m_PreTemplateEditSceneState["objects"].push_back(SerializeObject(obj));
    }

    // Clear current scene objects
    for (auto &obj : m_Objects)
    {
        if (obj.entity != 0) m_Registry.DestroyEntity(obj.entity);
    }
    m_Registry.Clear();
    m_Objects.clear();
    ClearSelection();

    m_EditingTemplate = true;
    m_EditingTemplatePath = tp.string();

    // Load template objects
    if (data.contains("objects") && data["objects"].is_array())
    {
        for (const auto &item : data["objects"])
        {
            DeserializeObject(item);
        }
    }

    UpdateWorldTransforms();
    m_camera.setCenter(0.f, 0.f);
    UpdateStatusText();
    std::cout << "[INFO] [EditorScene] Entered Template Edit Mode for: " << m_EditingTemplatePath << "\n";
}

void EditorScene::ExitTemplateEditMode(bool saveChanges)
{
    if (!m_EditingTemplate) return;

    if (saveChanges && !m_EditingTemplatePath.empty())
    {
        SaveTemplateFile(m_EditingTemplatePath);
    }

    std::string savedTemplatePath = m_EditingTemplatePath;

    // Clear template objects from canvas
    for (auto &obj : m_Objects)
    {
        if (obj.entity != 0) m_Registry.DestroyEntity(obj.entity);
    }
    m_Registry.Clear();
    m_Objects.clear();
    ClearSelection();

    // Restore previous scene
    if (m_PreTemplateEditSceneState.contains("objects") && m_PreTemplateEditSceneState["objects"].is_array())
    {
        for (const auto &item : m_PreTemplateEditSceneState["objects"])
        {
            DeserializeObject(item);
        }
    }
    m_PreTemplateEditSceneState = json{};
    m_EditingTemplate = false;
    m_EditingTemplatePath.clear();

    UpdateWorldTransforms();
    UpdateStatusText();

    // Automatically sync instances in the restored scene with updated template!
    if (saveChanges && !savedTemplatePath.empty())
    {
        SyncTemplateInstances(savedTemplatePath);
    }

    if (m_ContentBrowser) { m_ContentBrowser->Refresh(); }
    std::cout << "[INFO] [EditorScene] Exited Template Edit Mode and synchronized instances.\n";
}

void EditorScene::HandleExternalFileDrop(const std::vector<std::string> &paths, sf::Vector2f mousePos)
{
    if (!m_ContentBrowser) return;

    std::string importedFileName;
    if (m_ContentBrowser->ImportExternalFiles(paths, mousePos, importedFileName))
    {
        m_ActiveBottomPanelTab = BottomPanelTab::ContentBrowser;
        ShowImportNotification(importedFileName);
        std::cout << "[INFO] [EditorScene] External file drop imported: " << importedFileName << "\n";
    }
}

void EditorScene::ShowImportNotification(const std::string &filename)
{
    m_ImportNotificationText = "Imported: " + filename;
    m_ImportNotificationTimer = ImportNotificationDuration;
}

void EditorScene::DrawImportNotification(sf::RenderWindow &window)
{
    if (m_ImportNotificationTimer <= 0.f || m_ImportNotificationText.empty()) return;

    const float winW = static_cast<float>(window.getSize().x);
    const float winH = static_cast<float>(window.getSize().y);

    float alpha = 1.f;
    if (m_ImportNotificationTimer < 0.5f)
    {
        alpha = m_ImportNotificationTimer / 0.5f;
    }
    else if (m_ImportNotificationTimer > ImportNotificationDuration - 0.25f)
    {
        alpha = (ImportNotificationDuration - m_ImportNotificationTimer) / 0.25f;
    }
    alpha = std::clamp(alpha, 0.f, 1.f);
    const sf::Uint8 a = static_cast<sf::Uint8>(alpha * 255.f);

    sf::Text text;
    text.setFont(*m_Font);
    text.setCharacterSize(13);
    text.setString(m_ImportNotificationText);

    const float tw = text.getLocalBounds().width;
    const float th = text.getLocalBounds().height;

    const float boxW = std::max(220.f, tw + 56.f);
    const float boxH = 40.f;
    const float margin = 20.f;

    const float boxX = winW - boxW - margin;
    const float boxY = winH - boxH - margin;

    // Shadow
    sf::RectangleShape shadow({boxW, boxH});
    shadow.setPosition(boxX + 2.f, boxY + 2.f);
    shadow.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(70.f * alpha)));
    window.draw(shadow);

    // Card background
    sf::RectangleShape bg({boxW, boxH});
    bg.setPosition(boxX, boxY);
    bg.setFillColor(sf::Color(24, 26, 32, static_cast<sf::Uint8>(245.f * alpha)));
    bg.setOutlineColor(sf::Color(55, 60, 75, static_cast<sf::Uint8>(200.f * alpha)));
    bg.setOutlineThickness(1.f);
    window.draw(bg);

    // Left emerald accent bar
    sf::RectangleShape bar({4.f, boxH});
    bar.setPosition(boxX, boxY);
    bar.setFillColor(sf::Color(46, 204, 113, a));
    window.draw(bar);

    // Status dot
    sf::CircleShape dot(4.f);
    dot.setPosition(boxX + 14.f, boxY + (boxH - 8.f) / 2.f);
    dot.setFillColor(sf::Color(46, 204, 113, a));
    window.draw(dot);

    // Text
    text.setFillColor(sf::Color(240, 242, 245, a));
    text.setPosition(boxX + 28.f, boxY + (boxH - th) / 2.f - 3.f);
    window.draw(text);
}


