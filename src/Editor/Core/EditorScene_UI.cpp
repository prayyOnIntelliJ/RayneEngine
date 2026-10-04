#include "EditorScene_Common.h"
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <map>


void EditorScene::DrawToolbar(sf::RenderWindow &window)
{
    const float w = static_cast<float>(window.getSize().x);
    const float ty = MenuBarHeight;

    sf::RectangleShape bg({w, ToolbarHeight});
    bg.setFillColor(C_BG_PANEL);
    bg.setPosition(0.f, ty);
    window.draw(bg);

    sf::RectangleShape border({w, 1.f});
    border.setFillColor(C_BORDER);
    border.setPosition(0.f, ty + ToolbarHeight - 1.f);
    window.draw(border);

    m_ToolbarHitboxes.clear();

    const auto drawSep = [&](float sx) {
        sf::RectangleShape s({1.f, ToolbarHeight - 16.f});
        s.setFillColor(C_BORDER);
        s.setPosition(sx, ty + 8.f);
        window.draw(s);
    };

    const auto drawBtn = [&](sf::FloatRect r, const std::string &label, bool active,
                             sf::Color accentFill = C_ACCENT, sf::Color accentBorder = C_ACCENT) {
        const bool hov = r.contains(m_MouseScreenPos);
        sf::Color fill = active
                             ? C_ACCENT_DIM
                             : hov
                                   ? C_BG_ELEVATED
                                   : sf::Color(0, 0, 0, 0);
        sf::Color bdr = active
                            ? sf::Color(accentBorder.r, accentBorder.g, accentBorder.b, 200)
                            : hov
                                  ? C_BORDER_LIGHT
                                  : sf::Color(0, 0, 0, 0);
        DrawPill(window, r, fill, bdr);

        sf::Text t;
        t.setFont(*m_Font);
        t.setCharacterSize(12);
        t.setFillColor(active
                           ? sf::Color(accentBorder.r, accentBorder.g, accentBorder.b, 255)
                           : hov
                                 ? C_TEXT_PRIMARY
                                 : C_TEXT_SECONDARY);
        t.setString(label);
        t.setPosition(r.left + (r.width - t.getLocalBounds().width) / 2.f,
                      r.top + (r.height - t.getLocalBounds().height) / 2.f - 2.f);
        window.draw(t);
    };

    float cx = 10.f; {
        const sf::FloatRect selRect(cx, ty + 4.f, 72.f, ToolbarHeight - 8.f);
        drawBtn(selRect, "Pointer", !m_PlacementActive, C_ACCENT, C_ACCENT);
        m_ToolbarHitboxes.push_back({selRect, "tool_select"});
        cx += selRect.width + 6.f;
    } {
        std::string addLabel = m_PlacementActive ? "+ " + GetObjectTypeName(m_PlacementType) : "+ Add";
        float addW = m_PlacementActive ? 100.f : 86.f;
        const sf::FloatRect ar(cx, ty + 4.f, addW, ToolbarHeight - 8.f);
        const bool hov = ar.contains(m_MouseScreenPos);
        const bool act = m_AddDropdownOpen || m_PlacementActive;

        sf::Color fill = act ? C_ACCENT_DIM : hov ? C_BG_ELEVATED : C_BG_ELEVATED;
        sf::Color bdr = act ? C_ACCENT : hov ? C_BORDER_LIGHT : C_BORDER;
        DrawPill(window, ar, fill, bdr);

        sf::Text at;
        at.setFont(*m_Font);
        at.setCharacterSize(12);
        at.setFillColor(act ? C_ACCENT_BRIGHT : hov ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
        at.setString(addLabel);
        at.setPosition(ar.left + (ar.width - at.getLocalBounds().width) / 2.f,
                       ar.top + (ar.height - at.getLocalBounds().height) / 2.f - 2.f);
        window.draw(at);

        m_AddBtnBounds = ar;
        m_ToolbarHitboxes.push_back({ar, "add_dropdown"});
        cx += ar.width + 6.f;
    }

    if (m_PlacementActive)
    {
        const sf::FloatRect dropRect(cx, ty + 4.f, 60.f, ToolbarHeight - 8.f);
        drawBtn(dropRect, "Drop", false, C_TEXT_MUTED, C_BORDER_LIGHT);
        m_ToolbarHitboxes.push_back({dropRect, "clear_placement"});
        cx += dropRect.width + 6.f;
    }

    drawSep(cx);
    cx += 12.f; {
        const sf::FloatRect dr(cx, ty + 4.f, 56.f, ToolbarHeight - 8.f);
        drawBtn(dr, "Delete", false, C_DANGER, C_DANGER);
        m_ToolbarHitboxes.push_back({dr, "delete"});
        cx += dr.width + 8.f;
    }

    drawSep(cx);
    cx += 12.f; {
        const sf::FloatRect ur(cx, ty + 4.f, 76.f, ToolbarHeight - 8.f);
        drawBtn(ur, "UI Editor", false, C_ACCENT, C_ACCENT);
        m_ToolbarHitboxes.push_back({ur, "open_ui_editor"});
        cx += ur.width + 8.f;
    }

    drawSep(cx);
    cx += 12.f; {
        const sf::FloatRect gr(cx, ty + 4.f, 70.f, ToolbarHeight - 8.f);
        drawBtn(gr, m_SnapToGrid ? "Grid ON" : "Grid", m_SnapToGrid);
        m_ToolbarHitboxes.push_back({gr, "toggle_grid"});
        cx += gr.width + 8.f;
    }

    if (m_EditingTemplate)
    {
        drawSep(cx);
        cx += 12.f;
        std::string tmplName = std::filesystem::path(m_EditingTemplatePath).filename().string();
        std::string title = "[Template Edit Mode: " + tmplName + "]";
        sf::Text tMode;
        tMode.setFont(*m_Font);
        tMode.setCharacterSize(12);
        tMode.setFillColor(sf::Color(255, 200, 80));
        tMode.setString(title);
        tMode.setPosition(cx, ty + (ToolbarHeight - tMode.getLocalBounds().height) / 2.f - 2.f);
        window.draw(tMode);
        cx += tMode.getLocalBounds().width + 12.f;

        const sf::FloatRect saveBtn(cx, ty + 4.f, 65.f, ToolbarHeight - 8.f);
        drawBtn(saveBtn, "Save", false, sf::Color(35, 90, 60), sf::Color(60, 160, 100));
        m_ToolbarHitboxes.push_back({saveBtn, "save_template_only"});
        cx += saveBtn.width + 6.f;

        const sf::FloatRect exitBtn(cx, ty + 4.f, 120.f, ToolbarHeight - 8.f);
        drawBtn(exitBtn, "< Save & Exit", true, sf::Color(200, 100, 40), sf::Color(255, 140, 50));
        m_ToolbarHitboxes.push_back({exitBtn, "exit_template_mode"});
        m_ExitTemplateModeBtnBounds = exitBtn;
        cx += exitBtn.width + 6.f;

        const sf::FloatRect discardBtn(cx, ty + 4.f, 75.f, ToolbarHeight - 8.f);
        drawBtn(discardBtn, "Discard", false, C_DANGER_DIM, C_DANGER);
        m_ToolbarHitboxes.push_back({discardBtn, "discard_template_mode"});
        cx += discardBtn.width + 6.f;
    } {
        const float runW = 92.f;
        const float runX = w - InspectorWidth - runW - 10.f;
        const sf::FloatRect rr(runX, ty + 4.f, runW, ToolbarHeight - 8.f);
        const bool hov = rr.contains(m_MouseScreenPos);

        sf::Color fill = hov
                             ? sf::Color(C_ACCENT_ACT.r, C_ACCENT_ACT.g, C_ACCENT_ACT.b, 240)
                             : C_ACCENT_DIM;
        sf::Color runBdr = hov ? C_ACCENT_HOV : C_ACCENT;
        DrawPill(window, rr, fill, runBdr);

        sf::Text rt;
        rt.setFont(*m_Font);
        rt.setCharacterSize(12);
        rt.setFillColor(hov ? C_TEXT_PRIMARY : C_ACCENT_BRIGHT);
        rt.setString("Run  F5");
        rt.setPosition(rr.left + (rr.width - rt.getLocalBounds().width) / 2.f,
                       rr.top + (rr.height - rt.getLocalBounds().height) / 2.f - 2.f);
        window.draw(rt);

        drawSep(runX - 8.f);
        m_ToolbarHitboxes.push_back({rr, "run"});
    }
}


void EditorScene::InitSpotlightItems()
{
    m_AllSpotlightItems = {
        {"tool_select", "Pointer / Select", "Tools", "Clear held object and enter selection mode", ObjectType::None},
        {"add_rect", "Rectangle", "Primitives", "2D rectangular shape primitive", ObjectType::Rectangle},
        {"add_circle", "Circle", "Primitives", "2D circular shape primitive", ObjectType::Circle},
        {"add_triangle", "Triangle", "Primitives", "3-sided polygon primitive", ObjectType::Triangle},
        {"add_pentagon", "Pentagon", "Primitives", "5-sided polygon primitive", ObjectType::Pentagon},
        {"add_hexagon", "Hexagon", "Primitives", "6-sided polygon primitive", ObjectType::Hexagon},
        {
            "add_empty", "Empty Entity", "Gameplay", "Empty transform node for organization & parenting",
            ObjectType::Empty
        },
        {
            "add_spawn", "Spawn Point", "Gameplay", "Level player/actor spawn point with beacon gizmo",
            ObjectType::SpawnPoint
        },
        {
            "add_trigger", "Trigger Zone", "Gameplay", "Sensor area with isTrigger=true collision callbacks",
            ObjectType::TriggerZone
        },
        {"add_cam_obj", "Camera", "Gameplay", "In-game camera with live viewport frustum frame", ObjectType::Camera},
        {"add_phys_box", "Physics Box", "Physics", "Dynamic box with Rigidbody2D and collider", ObjectType::PhysicsBox},
        {
            "add_phys_ball", "Physics Ball", "Physics", "Dynamic bouncy ball with Rigidbody2D and collider",
            ObjectType::PhysicsBall
        },
        {
            "add_static_platform", "Static Platform", "Physics", "Solid static barrier/platform with collision",
            ObjectType::StaticPlatform
        },
        {"add_sprite", "Sprite", "Media & FX", "Sprite entity ready for texture drag-and-drop", ObjectType::Sprite},
        {
            "add_world_text", "World Text", "Media & FX", "Formatted text rendered directly in the game world",
            ObjectType::WorldText
        },
        {
            "add_audio_source", "Audio Source", "Media & FX", "Positional or ambient sound emitter component",
            ObjectType::AudioSource
        },
        {
            "add_particle_emitter", "Particle Emitter", "Media & FX", "Real-time 2D particle simulation effect",
            ObjectType::ParticleEmitter
        }
    };
    FilterSpotlightItems();
}


void EditorScene::FilterSpotlightItems()
{
    m_FilteredSpotlightItems.clear();
    std::string q = m_SpotlightQuery;
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);

    const std::vector<std::string> cats = {"All", "Primitives", "Gameplay", "Physics", "Media & FX"};
    std::string activeCat = (m_SpotlightCategory >= 0 && m_SpotlightCategory < (int) cats.size())
                                ? cats[m_SpotlightCategory]
                                : "All";

    for (const auto &item: m_AllSpotlightItems)
    {
        if (activeCat != "All" && item.category != activeCat)
            continue;

        if (!q.empty())
        {
            std::string n = item.name;
            std::string d = item.desc;
            std::string c = item.category;
            std::transform(n.begin(), n.end(), n.begin(), ::tolower);
            std::transform(d.begin(), d.end(), d.begin(), ::tolower);
            std::transform(c.begin(), c.end(), c.begin(), ::tolower);

            if (n.find(q) == std::string::npos && d.find(q) == std::string::npos && c.find(q) == std::string::npos)
                continue;
        }

        m_FilteredSpotlightItems.push_back(item);
    }

    if (m_SpotlightSelectedIndex >= (int) m_FilteredSpotlightItems.size())
        m_SpotlightSelectedIndex = std::max(0, (int) m_FilteredSpotlightItems.size() - 1);
}


void EditorScene::OpenSpotlight()
{
    m_SpotlightOpen = true;
    m_SpotlightQuery.clear();
    m_SpotlightSelectedIndex = 0;
    m_SpotlightScrollY = 0.0f;
    m_SpotlightSearchFocused = false;
    m_SpotlightDraggingScrollbar = false;
    m_AddDropdownOpen = false;
    m_OpenMenuIndex = -1;
    FilterSpotlightItems();
}


void EditorScene::CloseSpotlight()
{
    m_SpotlightOpen = false;
    m_SpotlightQuery.clear();
    m_SpotlightSearchFocused = false;
    m_SpotlightDraggingScrollbar = false;
}


void EditorScene::SelectSpotlightItem(const SpotlightItem &item)
{
    if (item.type == ObjectType::None || item.id == "tool_select")
    {
        m_PlacementActive = false;
        std::cout << "[INFO] [EditorScene] Switched to Select / Pointer mode (no object in hand)\n";
    }
    else
    {
        m_PlacementActive = true;
        m_PlacementType = item.type;
        std::cout << "[INFO] [EditorScene] Placement mode activated for: " << item.name << "\n";
    }
    CloseSpotlight();
    UpdateStatusText();
}


void EditorScene::DrawSpotlightItemIcon(sf::RenderWindow &window, ObjectType type, sf::Vector2f center, float size)
{
    float half = size * 0.5f;
    switch (type)
    {
        case ObjectType::None: {
            sf::ConvexShape arrow(4);
            arrow.setPoint(0, {center.x - half * 0.4f, center.y - half * 0.8f});
            arrow.setPoint(1, {center.x - half * 0.4f, center.y + half * 0.6f});
            arrow.setPoint(2, {center.x + half * 0.1f, center.y + half * 0.1f});
            arrow.setPoint(3, {center.x + half * 0.6f, center.y + half * 0.1f});
            arrow.setFillColor(sf::Color(200, 210, 225));
            window.draw(arrow);
            break;
        }
        case ObjectType::Rectangle: {
            sf::RectangleShape r({size, size * 0.75f});
            r.setOrigin(half, half * 0.75f);
            r.setPosition(center);
            r.setFillColor(sf::Color(100, 149, 237));
            window.draw(r);
            break;
        }
        case ObjectType::Circle: {
            sf::CircleShape c(half);
            c.setOrigin(half, half);
            c.setPosition(center);
            c.setFillColor(sf::Color(237, 149, 100));
            window.draw(c);
            break;
        }
        case ObjectType::Triangle: {
            sf::CircleShape t(half, 3);
            t.setOrigin(half, half);
            t.setPosition(center);
            t.setFillColor(sf::Color(149, 237, 100));
            window.draw(t);
            break;
        }
        case ObjectType::Pentagon: {
            sf::CircleShape p(half, 5);
            p.setOrigin(half, half);
            p.setPosition(center);
            p.setFillColor(sf::Color(237, 100, 237));
            window.draw(p);
            break;
        }
        case ObjectType::Hexagon: {
            sf::CircleShape h(half, 6);
            h.setOrigin(half, half);
            h.setPosition(center);
            h.setFillColor(sf::Color(237, 237, 100));
            window.draw(h);
            break;
        }
        case ObjectType::Empty: {
            sf::CircleShape d(half * 0.8f, 4);
            d.setOrigin(half * 0.8f, half * 0.8f);
            d.setPosition(center);
            d.setRotation(45.f);
            d.setFillColor(sf::Color::Transparent);
            d.setOutlineColor(sf::Color(180, 180, 180));
            d.setOutlineThickness(1.5f);
            window.draw(d);
            break;
        }
        case ObjectType::SpawnPoint: {
            sf::CircleShape ring(half * 0.85f);
            ring.setOrigin(half * 0.85f, half * 0.85f);
            ring.setPosition(center);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineColor(sf::Color(255, 200, 60));
            ring.setOutlineThickness(1.5f);
            window.draw(ring);
            sf::CircleShape dot(3.f);
            dot.setOrigin(3.f, 3.f);
            dot.setPosition(center);
            dot.setFillColor(sf::Color(255, 200, 60));
            window.draw(dot);
            break;
        }
        case ObjectType::TriggerZone: {
            sf::RectangleShape r({size, size * 0.65f});
            r.setOrigin(half, half * 0.65f);
            r.setPosition(center);
            r.setFillColor(sf::Color(40, 200, 80, 120));
            r.setOutlineColor(sf::Color(60, 240, 100));
            r.setOutlineThickness(1.f);
            window.draw(r);
            break;
        }
        case ObjectType::Camera: {
            sf::RectangleShape b({size, size * 0.7f});
            b.setOrigin(half, half * 0.7f);
            b.setPosition(center);
            b.setFillColor(sf::Color(50, 70, 95));
            b.setOutlineColor(sf::Color(64, 180, 240));
            b.setOutlineThickness(1.f);
            window.draw(b);
            sf::CircleShape lens(4.f);
            lens.setOrigin(4.f, 4.f);
            lens.setPosition(center);
            lens.setFillColor(sf::Color(64, 180, 240));
            window.draw(lens);
            break;
        }
        case ObjectType::PhysicsBox: {
            sf::RectangleShape b({size * 0.85f, size * 0.85f});
            b.setOrigin(half * 0.85f, half * 0.85f);
            b.setPosition(center);
            b.setFillColor(sf::Color(210, 140, 70));
            window.draw(b);
            break;
        }
        case ObjectType::PhysicsBall: {
            sf::CircleShape b(half * 0.85f);
            b.setOrigin(half * 0.85f, half * 0.85f);
            b.setPosition(center);
            b.setFillColor(sf::Color(220, 80, 80));
            window.draw(b);
            break;
        }
        case ObjectType::StaticPlatform: {
            sf::RectangleShape p({size, size * 0.35f});
            p.setOrigin(half, half * 0.35f);
            p.setPosition(center);
            p.setFillColor(sf::Color(100, 110, 125));
            p.setOutlineColor(sf::Color(140, 150, 165));
            p.setOutlineThickness(1.f);
            window.draw(p);
            break;
        }
        case ObjectType::Sprite: {
            sf::RectangleShape sp({size * 0.85f, size * 0.85f});
            sp.setOrigin(half * 0.85f, half * 0.85f);
            sp.setPosition(center);
            sp.setFillColor(sf::Color(60, 120, 180));
            window.draw(sp);
            break;
        }
        case ObjectType::WorldText: {
            sf::Text t;
            t.setFont(*m_Font);
            t.setCharacterSize(16);
            t.setStyle(sf::Text::Bold);
            t.setFillColor(sf::Color::White);
            t.setString("T");
            t.setOrigin(t.getLocalBounds().width * 0.5f, t.getLocalBounds().height * 0.5f);
            t.setPosition(center - sf::Vector2f(0.f, 3.f));
            window.draw(t);
            break;
        }
        case ObjectType::AudioSource: {
            sf::CircleShape s(half * 0.85f);
            s.setOrigin(half * 0.85f, half * 0.85f);
            s.setPosition(center);
            s.setFillColor(sf::Color(160, 100, 240));
            window.draw(s);
            break;
        }
        case ObjectType::ParticleEmitter: {
            sf::CircleShape p(half * 0.85f, 4);
            p.setOrigin(half * 0.85f, half * 0.85f);
            p.setPosition(center);
            p.setRotation(45.f);
            p.setFillColor(sf::Color(255, 160, 40));
            window.draw(p);
            break;
        }
        default:
            break;
    }
}


void EditorScene::DrawSpotlightPalette(sf::RenderWindow &window)
{
    m_SpotlightItemHitboxes.clear();
    m_SpotlightCategoryHitboxes.clear();

    const float winW = static_cast<float>(window.getSize().x);
    const float winH = static_cast<float>(window.getSize().y);
    sf::RectangleShape overlay({winW, winH});
    overlay.setFillColor(sf::Color(0, 0, 0, 175));
    window.draw(overlay);

    const float modalW = 680.f;
    const float modalH = 520.f;
    const float modalX = (winW - modalW) * 0.5f;
    const float modalY = std::max(20.f, (winH - modalH) * 0.45f);
    m_SpotlightModalBounds = sf::FloatRect(modalX, modalY, modalW, modalH);
    sf::RectangleShape modalBg({modalW, modalH});
    modalBg.setPosition(modalX, modalY);
    modalBg.setFillColor(C_BG_ELEVATED);
    modalBg.setOutlineColor(C_BORDER_LIGHT);
    modalBg.setOutlineThickness(1.5f);
    window.draw(modalBg);
    sf::RectangleShape headerBg({modalW, 40.f});
    headerBg.setPosition(modalX, modalY);
    headerBg.setFillColor(C_BG_PANEL);
    window.draw(headerBg);

    sf::Text titleText;
    titleText.setFont(*m_Font);
    titleText.setCharacterSize(13);
    titleText.setStyle(sf::Text::Bold);
    titleText.setFillColor(C_TEXT_PRIMARY);
    titleText.setString("ADD OBJECT  |  SPOTLIGHT PALETTE");
    titleText.setPosition(modalX + 16.f, modalY + 11.f);
    window.draw(titleText);
    const float closeBtnSize = 24.f;
    const float closeBtnX = modalX + modalW - closeBtnSize - 12.f;
    const float closeBtnY = modalY + 8.f;
    m_SpotlightCloseBtnBounds = sf::FloatRect(closeBtnX, closeBtnY, closeBtnSize, closeBtnSize);
    bool isCloseHov = m_SpotlightCloseBtnBounds.contains(m_MouseScreenPos);

    sf::RectangleShape closeBtnBg({closeBtnSize, closeBtnSize});
    closeBtnBg.setPosition(closeBtnX, closeBtnY);
    closeBtnBg.setFillColor(isCloseHov ? sf::Color(180, 50, 50) : C_BG_ELEVATED);
    closeBtnBg.setOutlineColor(isCloseHov ? sf::Color(220, 80, 80) : C_BORDER_LIGHT);
    closeBtnBg.setOutlineThickness(1.f);
    window.draw(closeBtnBg);

    sf::Text closeBtnText;
    closeBtnText.setFont(*m_Font);
    closeBtnText.setCharacterSize(14);
    closeBtnText.setStyle(sf::Text::Bold);
    closeBtnText.setFillColor(isCloseHov ? sf::Color::White : C_TEXT_MUTED);
    closeBtnText.setString("x");
    closeBtnText.setPosition(closeBtnX + 7.f, closeBtnY + 2.f);
    window.draw(closeBtnText);
    sf::Text scBadge;
    scBadge.setFont(*m_Font);
    scBadge.setCharacterSize(11);
    scBadge.setFillColor(C_TEXT_MUTED);
    scBadge.setString("Shortcut: Ctrl + Space");
    scBadge.setPosition(closeBtnX - scBadge.getLocalBounds().width - 14.f, modalY + 13.f);
    window.draw(scBadge);
    const float searchY = modalY + 52.f;
    const float searchH = 38.f;
    const float searchPad = 16.f;
    const float searchW = modalW - searchPad * 2.f;
    m_SpotlightSearchBoxBounds = sf::FloatRect(modalX + searchPad, searchY, searchW, searchH);

    sf::RectangleShape searchBg({searchW, searchH});
    searchBg.setPosition(modalX + searchPad, searchY);
    searchBg.setFillColor(C_BG_INPUT);
    searchBg.setOutlineColor(m_SpotlightSearchFocused ? C_ACCENT_HOV : C_ACCENT);
    searchBg.setOutlineThickness(m_SpotlightSearchFocused ? 1.5f : 1.f);
    window.draw(searchBg);

    sf::Text searchIcon;
    searchIcon.setFont(*m_Font);
    searchIcon.setCharacterSize(13);
    searchIcon.setFillColor(C_ACCENT);
    searchIcon.setString("[>]");
    searchIcon.setPosition(modalX + searchPad + 10.f, searchY + 9.f);
    window.draw(searchIcon);

    sf::Text queryText;
    queryText.setFont(*m_Font);
    queryText.setCharacterSize(13);
    if (m_SpotlightQuery.empty())
    {
        queryText.setFillColor(C_TEXT_MUTED);
        queryText.setString(m_SpotlightSearchFocused
                                ? "Type to search objects..."
                                : "Type or click here to search (e.g. Camera, Physics, Light)...");
    } else
    {
        static sf::Clock cursorClock;
        bool cursorBlink = static_cast<int>(cursorClock.getElapsedTime().asSeconds() * 2.f) % 2 == 0;
        queryText.setFillColor(C_TEXT_PRIMARY);
        queryText.setString(m_SpotlightQuery + ((m_SpotlightSearchFocused && cursorBlink) ? "|" : ""));
    }
    queryText.setPosition(modalX + searchPad + 38.f, searchY + 9.f);
    window.draw(queryText);
    if (!m_SpotlightQuery.empty())
    {
        const float clearSize = 22.f;
        const float clearX = modalX + searchPad + searchW - clearSize - 8.f;
        const float clearY = searchY + (searchH - clearSize) * 0.5f;
        m_SpotlightClearSearchBtnBounds = sf::FloatRect(clearX, clearY, clearSize, clearSize);
        bool isClearHov = m_SpotlightClearSearchBtnBounds.contains(m_MouseScreenPos);

        sf::RectangleShape clearBg({clearSize, clearSize});
        clearBg.setPosition(clearX, clearY);
        clearBg.setFillColor(isClearHov ? C_BG_ELEVATED : C_BG_PANEL);
        clearBg.setOutlineColor(C_BORDER_LIGHT);
        clearBg.setOutlineThickness(1.f);
        window.draw(clearBg);

        sf::Text clearTxt;
        clearTxt.setFont(*m_Font);
        clearTxt.setCharacterSize(12);
        clearTxt.setFillColor(isClearHov ? sf::Color::White : C_TEXT_MUTED);
        clearTxt.setString("x");
        clearTxt.setPosition(clearX + 7.f, clearY + 2.f);
        window.draw(clearTxt);
    } else { m_SpotlightClearSearchBtnBounds = sf::FloatRect(); }
    const float catY = searchY + searchH + 10.f;
    const std::vector<std::string> cats = {"All", "Primitives", "Gameplay", "Physics", "Media & FX"};
    float chipX = modalX + searchPad;
    for (int i = 0; i < (int) cats.size(); ++i)
    {
        sf::Text ct;
        ct.setFont(*m_Font);
        ct.setCharacterSize(11);
        ct.setString(cats[i]);
        float ctw = ct.getLocalBounds().width;
        float chipW = ctw + 20.f;
        float chipH = 24.f;

        const sf::FloatRect chipRect(chipX, catY, chipW, chipH);
        m_SpotlightCategoryHitboxes.push_back({chipRect, i});

        bool isAct = (m_SpotlightCategory == i);
        bool isHov = chipRect.contains(m_MouseScreenPos);

        sf::RectangleShape chipBg({chipW, chipH});
        chipBg.setPosition(chipX, catY);
        chipBg.setFillColor(isAct ? C_ACCENT : (isHov ? C_BG_ELEVATED : C_BG_PANEL));
        chipBg.setOutlineColor(isAct ? C_ACCENT_HOV : C_BORDER_LIGHT);
        chipBg.setOutlineThickness(1.f);
        window.draw(chipBg);

        ct.setFillColor(isAct ? sf::Color::White : (isHov ? C_TEXT_PRIMARY : C_TEXT_SECONDARY));
        ct.setPosition(chipX + 10.f, catY + 4.f);
        window.draw(ct);

        chipX += chipW + 8.f;
    }
    const float itemsY = catY + 36.f;
    const float itemsH = modalH - (itemsY - modalY) - 36.f;
    m_SpotlightItemsViewportBounds = sf::FloatRect(modalX + searchPad, itemsY, searchW, itemsH);

    const float cardGap = 8.f;
    const float scrollbarW = 8.f;
    const float cardContainerW = searchW - scrollbarW - 4.f;
    const float cardW = (cardContainerW - cardGap) / 2.f;
    const float cardH = 58.f;

    int totalRows = (static_cast<int>(m_FilteredSpotlightItems.size()) + 1) / 2;
    float totalContentH = totalRows > 0 ? (totalRows * (cardH + cardGap) - cardGap) : 0.0f;
    float maxScroll = std::max(0.0f, totalContentH - itemsH + 16.0f);
    m_SpotlightScrollY = std::max(0.0f, std::min(m_SpotlightScrollY, maxScroll));

    sf::View origView = window.getView();
    sf::View clipView;
    clipView.setSize(modalW, itemsH);
    clipView.setCenter(modalX + modalW * 0.5f, itemsY + itemsH * 0.5f);
    clipView.setViewport({
        modalX / winW,
        itemsY / winH,
        modalW / winW,
        itemsH / winH
    });
    window.setView(clipView);

    for (int i = 0; i < (int) m_FilteredSpotlightItems.size(); ++i)
    {
        const auto &item = m_FilteredSpotlightItems[i];
        int col = i % 2;
        int row = i / 2;

        float cx = modalX + searchPad + col * (cardW + cardGap);
        float cy = itemsY + row * (cardH + cardGap) - m_SpotlightScrollY;

        const sf::FloatRect cardRect(cx, cy, cardW, cardH);
        m_SpotlightItemHitboxes.push_back({cardRect, i});

        bool isSel = (m_SpotlightSelectedIndex == i);
        bool isHov = m_SpotlightItemsViewportBounds.contains(m_MouseScreenPos) && cardRect.contains(m_MouseScreenPos);

        sf::RectangleShape cardBg({cardW, cardH});
        cardBg.setPosition(cx, cy);
        cardBg.setFillColor(isSel ? sf::Color(45, 60, 85) : (isHov ? sf::Color(38, 43, 52) : C_BG_PANEL));
        cardBg.setOutlineColor(isSel ? C_ACCENT : (isHov ? C_BORDER_LIGHT : C_BORDER));
        cardBg.setOutlineThickness(isSel ? 1.5f : 1.f);
        window.draw(cardBg);

        sf::RectangleShape iconBox({42.f, 42.f});
        iconBox.setPosition(cx + 8.f, cy + 8.f);
        iconBox.setFillColor(C_BG_INPUT);
        iconBox.setOutlineColor(isSel ? C_ACCENT : C_BORDER);
        iconBox.setOutlineThickness(1.f);
        window.draw(iconBox);

        DrawSpotlightItemIcon(window, item.type, sf::Vector2f(cx + 29.f, cy + 29.f), 24.f);

        sf::Text title;
        title.setFont(*m_Font);
        title.setCharacterSize(13);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(isSel ? sf::Color::White : (isHov ? C_TEXT_PRIMARY : sf::Color(220, 225, 235)));
        title.setString(item.name);
        title.setPosition(cx + 58.f, cy + 8.f);
        window.draw(title);

        sf::Text catBadge;
        catBadge.setFont(*m_Font);
        catBadge.setCharacterSize(10);
        catBadge.setString(item.category);
        float bw = catBadge.getLocalBounds().width + 10.f;
        sf::RectangleShape badgeBg({bw, 16.f});
        badgeBg.setPosition(cx + cardW - bw - 8.f, cy + 8.f);
        badgeBg.setFillColor(sf::Color(25, 30, 38));
        badgeBg.setOutlineColor(C_BORDER);
        badgeBg.setOutlineThickness(1.f);
        window.draw(badgeBg);
        catBadge.setFillColor(C_TEXT_MUTED);
        catBadge.setPosition(cx + cardW - bw + 5.f, cy + 8.f);
        window.draw(catBadge);

        sf::Text desc;
        desc.setFont(*m_Font);
        desc.setCharacterSize(10);
        desc.setFillColor(C_TEXT_MUTED);
        desc.setString(item.desc);
        desc.setPosition(cx + 58.f, cy + 28.f);
        window.draw(desc);
    }

    window.setView(origView);
    const float trackX = modalX + searchPad + searchW - scrollbarW;
    const float trackY = itemsY;
    const float trackH = itemsH;
    m_SpotlightScrollbarTrackBounds = sf::FloatRect(trackX, trackY, scrollbarW, trackH);

    if (maxScroll > 0.0f)
    {
        sf::RectangleShape trackBg({scrollbarW, trackH});
        trackBg.setPosition(trackX, trackY);
        trackBg.setFillColor(C_BG_INPUT);
        window.draw(trackBg);

        float thumbRatio = std::max(0.15f, std::min(1.0f, itemsH / totalContentH));
        float thumbH = std::max(20.f, trackH * thumbRatio);
        float thumbY = trackY + (m_SpotlightScrollY / maxScroll) * (trackH - thumbH);
        m_SpotlightScrollbarThumbBounds = sf::FloatRect(trackX, thumbY, scrollbarW, thumbH);

        bool isThumbHov = m_SpotlightScrollbarThumbBounds.contains(m_MouseScreenPos) || m_SpotlightDraggingScrollbar;
        sf::RectangleShape thumbBg({scrollbarW, thumbH});
        thumbBg.setPosition(trackX, thumbY);
        thumbBg.setFillColor(isThumbHov ? C_ACCENT : C_BORDER_LIGHT);
        window.draw(thumbBg);
    } else { m_SpotlightScrollbarThumbBounds = sf::FloatRect(); }
    sf::RectangleShape footerBg({modalW, 32.f});
    footerBg.setPosition(modalX, modalY + modalH - 32.f);
    footerBg.setFillColor(C_BG_PANEL);
    window.draw(footerBg);

    sf::Text footerText;
    footerText.setFont(*m_Font);
    footerText.setCharacterSize(10);
    footerText.setFillColor(C_TEXT_MUTED);
    footerText.setString(
        "[A / D] Kategorie    [Pfeiltasten] Navigieren    [Klick / Enter] Platzieren    [Esc] Schliessen");
    footerText.setPosition(modalX + 16.f, modalY + modalH - 22.f);
    window.draw(footerText);

    std::string countStr = std::to_string(m_FilteredSpotlightItems.size()) + " Objekte";
    sf::Text countText;
    countText.setFont(*m_Font);
    countText.setCharacterSize(10);
    countText.setFillColor(C_TEXT_MUTED);
    countText.setString(countStr);
    countText.setPosition(modalX + modalW - countText.getLocalBounds().width - 16.f, modalY + modalH - 22.f);
    window.draw(countText);
}


void EditorScene::DrawDeleteModal(sf::RenderWindow &window)
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
    title.setString("Delete Object with Children");
    title.setPosition(mx + 20.f, my + 18.f);
    window.draw(title);

    sf::Text msg;
    msg.setFont(*m_Font);
    msg.setCharacterSize(12);
    msg.setFillColor(C_TEXT_SECONDARY);
    msg.setString("Object '" + m_DeleteModalTargetId + "' has " + std::to_string(m_DeleteModalDescendantIds.size()) +
                  " direct child object(s).\nHow would you like to proceed?");
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


void EditorScene::UpdateStatusText()
{
    std::string s = "Objects: " + std::to_string(m_Objects.size());
    s += "  Grid: " + std::string(m_SnapToGrid ? "ON" : "OFF");
    if (m_PlacementActive)
        s += "  |  [Holding: " + GetObjectTypeName(m_PlacementType) + " (Esc/R-Click to drop)]";
    else
        s += "  |  [Pointer]";
    if (m_Selected)
        s += "  |  " + m_Selected->id
                + "  (" + std::to_string((int) m_Selected->shape.getPosition().x)
                + ", " + std::to_string((int) m_Selected->shape.getPosition().y) + ")";
    if (m_HasUnsavedChanges)
        s += "  |  * Unsaved";
    m_StatusText.setString(s);
}


void EditorScene::DrawTooltip(sf::RenderWindow &window)
{
    if (m_ActiveTooltip.empty()) return;

    sf::Text text;
    text.setFont(*m_Font);
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


void EditorScene::DrawBuildPopup(sf::RenderWindow &window)
{
    if (!m_ShowBuildPopup) return;

    sf::RectangleShape bg(sf::Vector2f(window.getSize().x, window.getSize().y));
    bg.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(bg);

    float pW = 500.f;
    float pH = 180.f;
    float pX = (window.getSize().x - pW) / 2.f;
    float pY = (window.getSize().y - pH) / 2.f;

    sf::RectangleShape panel(sf::Vector2f(pW, pH));
    panel.setPosition(pX, pY);
    panel.setFillColor(C_BG_PANEL);
    panel.setOutlineColor(C_BORDER);
    panel.setOutlineThickness(1.f);
    window.draw(panel);

    std::string statusText;
    float progress = 0.0f;
    bool finished = false; {
        std::lock_guard<std::mutex> lock(m_BuildMutex);
        statusText = m_BuildStatusText;
        progress = m_BuildProgress;
        finished = m_BuildFinished;
    }

    std::string headerStr = (statusText.find("Engine") != std::string::npos || statusText.find("RayneEngine") !=
                             std::string::npos)
                                ? "Packaging RayneEngine (.zip)"
                                : "Building Standalone Game";
    sf::Text header(headerStr, *m_Font, 16);
    header.setPosition(pX + 20.f, pY + 20.f);
    header.setFillColor(C_TEXT_PRIMARY);
    window.draw(header);

    sf::Text status(statusText, *m_Font, 12);
    status.setPosition(pX + 20.f, pY + 60.f);
    status.setFillColor(C_TEXT_SECONDARY);
    window.draw(status);

    float barW = pW - 40.f;
    float barH = 20.f;
    sf::RectangleShape barBg(sf::Vector2f(barW, barH));
    barBg.setPosition(pX + 20.f, pY + 90.f);
    barBg.setFillColor(C_BG_INPUT);
    barBg.setOutlineColor(C_BORDER);
    barBg.setOutlineThickness(1.f);
    window.draw(barBg);

    if (progress > 0.0f)
    {
        sf::RectangleShape barFill(sf::Vector2f(barW * (progress / 100.f), barH));
        barFill.setPosition(pX + 20.f, pY + 90.f);
        barFill.setFillColor(C_ACCENT);
        window.draw(barFill);
    }

    float btnW = 100.f;
    float btnH = 30.f;
    float btnX = pX + pW / 2.f - btnW / 2.f;
    float btnY = pY + pH - 20.f - btnH;

    sf::RectangleShape btn(sf::Vector2f(btnW, btnH));
    btn.setPosition(btnX, btnY);

    sf::Vector2f mPos = MouseWorldPos();
    mPos = {(float) sf::Mouse::getPosition(window).x, (float) sf::Mouse::getPosition(window).y};
    bool hoverBtn = btn.getGlobalBounds().contains(mPos);

    btn.setFillColor(hoverBtn ? C_ACCENT_HOV : C_ACCENT);
    window.draw(btn);

    sf::Text btnText(finished ? "Close" : "Cancel", *m_Font, 12);
    btnText.setFillColor(sf::Color::White);
    btnText.setPosition(
        btnX + btnW / 2.f - btnText.getGlobalBounds().width / 2.f,
        btnY + btnH / 2.f - btnText.getGlobalBounds().height / 2.f - 4.f
    );
    window.draw(btnText);
}


void EditorScene::DrawSaveTemplateModal(sf::RenderWindow &window)
{
    if (!m_ShowSaveTemplatePrompt) return;

    sf::RectangleShape bg(sf::Vector2f(window.getSize().x, window.getSize().y));
    bg.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(bg);

    float pW = 400.f;
    float pH = 180.f;
    float pX = (window.getSize().x - pW) / 2.f;
    float pY = (window.getSize().y - pH) / 2.f;

    sf::RectangleShape panel(sf::Vector2f(pW, pH));
    panel.setPosition(pX, pY);
    panel.setFillColor(C_BG_PANEL);
    panel.setOutlineColor(C_BORDER);
    panel.setOutlineThickness(1.f);
    window.draw(panel);

    sf::Text header("Save as Template", *m_Font, 16);
    header.setPosition(pX + 20.f, pY + 20.f);
    header.setFillColor(C_TEXT_PRIMARY);
    window.draw(header);

    sf::Text label("Template Name:", *m_Font, 12);
    label.setPosition(pX + 20.f, pY + 55.f);
    label.setFillColor(C_TEXT_SECONDARY);
    window.draw(label);

    float inputW = pW - 40.f;
    float inputH = 32.f;
    sf::RectangleShape inputBox(sf::Vector2f(inputW, inputH));
    inputBox.setPosition(pX + 20.f, pY + 75.f);
    inputBox.setFillColor(C_BG_INPUT);
    inputBox.setOutlineColor(C_ACCENT);
    inputBox.setOutlineThickness(1.5f);
    window.draw(inputBox);

    sf::Text inputText(m_SaveTemplateInputName + "|", *m_Font, 13);
    inputText.setPosition(pX + 28.f, pY + 82.f);
    inputText.setFillColor(C_TEXT_PRIMARY);
    window.draw(inputText);

    float btnW = 90.f;
    float btnH = 28.f;
    float btnY = pY + pH - 45.f;
    float btnSaveX = pX + pW - 20.f - btnW * 2.f - 10.f;
    float btnCancelX = pX + pW - 20.f - btnW;

    sf::Vector2f mPos = {(float) sf::Mouse::getPosition(window).x, (float) sf::Mouse::getPosition(window).y};
    sf::RectangleShape btnSave(sf::Vector2f(btnW, btnH));
    btnSave.setPosition(btnSaveX, btnY);
    bool hoverSave = btnSave.getGlobalBounds().contains(mPos);
    btnSave.setFillColor(hoverSave ? C_ACCENT_HOV : C_ACCENT);
    window.draw(btnSave);

    sf::Text saveText("Save", *m_Font, 12);
    sf::FloatRect stBounds = saveText.getLocalBounds();
    saveText.setPosition(btnSaveX + (btnW - stBounds.width) / 2.f, btnY + (btnH - stBounds.height) / 2.f - 2.f);
    saveText.setFillColor(sf::Color::White);
    window.draw(saveText);
    sf::RectangleShape btnCancel(sf::Vector2f(btnW, btnH));
    btnCancel.setPosition(btnCancelX, btnY);
    bool hoverCancel = btnCancel.getGlobalBounds().contains(mPos);
    btnCancel.setFillColor(hoverCancel ? C_BG_ELEVATED : C_BG_INPUT);
    btnCancel.setOutlineColor(C_BORDER);
    btnCancel.setOutlineThickness(1.f);
    window.draw(btnCancel);

    sf::Text cancelText("Cancel", *m_Font, 12);
    sf::FloatRect ctBounds = cancelText.getLocalBounds();
    cancelText.setPosition(btnCancelX + (btnW - ctBounds.width) / 2.f, btnY + (btnH - ctBounds.height) / 2.f - 2.f);
    cancelText.setFillColor(C_TEXT_SECONDARY);
    window.draw(cancelText);
}


void EditorScene::DrawScriptErrorModal(sf::RenderWindow &window)
{
    if (!m_ShowScriptErrorModal) return;

    const float winW = static_cast<float>(window.getSize().x);
    const float winH = static_cast<float>(window.getSize().y);
    sf::RectangleShape dim({winW, winH});
    dim.setFillColor(sf::Color(0, 0, 0, 180));
    window.draw(dim);

    const float modalW = 580.f;
    const float modalH = 260.f;
    const float mx = (winW - modalW) * 0.5f;
    const float my = (winH - modalH) * 0.5f;
    sf::RectangleShape box({modalW, modalH});
    box.setPosition(mx, my);
    box.setFillColor(C_BG_PANEL);
    box.setOutlineColor(C_DANGER);
    box.setOutlineThickness(1.5f);
    window.draw(box);
    sf::RectangleShape headerBg({modalW, 38.f});
    headerBg.setPosition(mx, my);
    headerBg.setFillColor(sf::Color(45, 20, 20));
    window.draw(headerBg);
    sf::Text title;
    title.setFont(*m_Font);
    title.setCharacterSize(14);
    title.setStyle(sf::Text::Bold);
    title.setFillColor(C_DANGER);
    title.setString("SCRIPT ERROR (F5)");
    title.setPosition(mx + 16.f, my + 10.f);
    window.draw(title);
    std::string displayPath = m_ScriptErrorPath;
    if (!displayPath.empty())
    {
        std::error_code ec;
        std::filesystem::path fullP = std::filesystem::absolute(m_ScriptErrorPath, ec);
        std::filesystem::path projRoot = FindProjectRoot();
        std::filesystem::path relP = std::filesystem::relative(fullP, projRoot, ec);
        if (!ec && !relP.empty()) { displayPath = relP.generic_string(); } else
        {
            displayPath = std::filesystem::path(m_ScriptErrorPath).generic_string();
        }
    }

    sf::Text fileTxt;
    fileTxt.setFont(*m_Font);
    fileTxt.setCharacterSize(11);
    fileTxt.setFillColor(sf::Color(255, 200, 100));
    fileTxt.setString("File: " + (displayPath.empty() ? std::string("Unknown") : displayPath));
    fileTxt.setPosition(mx + 16.f, my + 48.f);
    window.draw(fileTxt);
    const float errBoxX = mx + 16.f;
    const float errBoxY = my + 70.f;
    const float errBoxW = modalW - 32.f;
    const float errBoxH = 120.f;
    sf::RectangleShape errBox({errBoxW, errBoxH});
    errBox.setPosition(errBoxX, errBoxY);
    errBox.setFillColor(C_BG_INPUT);
    errBox.setOutlineColor(C_BORDER);
    errBox.setOutlineThickness(1.f);
    window.draw(errBox);
    std::string displayErr = m_ScriptErrorDetails;
    if (displayErr.length() > 280) { displayErr = displayErr.substr(0, 277) + "..."; }

    sf::Text errText;
    errText.setFont(*m_Font);
    errText.setCharacterSize(11);
    errText.setFillColor(sf::Color(255, 120, 120));
    errText.setString(displayErr);
    errText.setPosition(errBoxX + 10.f, errBoxY + 8.f);
    window.draw(errText);
    const float btnH = 30.f;
    const float btnY = my + modalH - btnH - 14.f;
    const float okBtnW = 110.f;
    const float okBtnX = mx + modalW - okBtnW - 16.f;
    m_ScriptErrorOkBtn = sf::FloatRect(okBtnX, btnY, okBtnW, btnH);
    bool okHov = m_ScriptErrorOkBtn.contains(m_MouseScreenPos);

    sf::RectangleShape okBtn({okBtnW, btnH});
    okBtn.setPosition(okBtnX, btnY);
    okBtn.setFillColor(okHov ? C_BG_ELEVATED : C_BG_PANEL);
    okBtn.setOutlineColor(C_BORDER_LIGHT);
    okBtn.setOutlineThickness(1.f);
    window.draw(okBtn);

    sf::Text okTxt;
    okTxt.setFont(*m_Font);
    okTxt.setCharacterSize(11);
    okTxt.setFillColor(okHov ? sf::Color::White : C_TEXT_PRIMARY);
    okTxt.setString("Close");
    float okw = okTxt.getLocalBounds().width;
    okTxt.setPosition(okBtnX + (okBtnW - okw) * 0.5f, btnY + 7.f);
    window.draw(okTxt);

    if (!m_ScriptErrorPath.empty())
    {
        const float ideBtnW = 140.f;
        const float ideBtnX = okBtnX - ideBtnW - 10.f;
        m_ScriptErrorOpenIDEBtn = sf::FloatRect(ideBtnX, btnY, ideBtnW, btnH);
        bool ideHov = m_ScriptErrorOpenIDEBtn.contains(m_MouseScreenPos);

        sf::RectangleShape ideBtn({ideBtnW, btnH});
        ideBtn.setPosition(ideBtnX, btnY);
        ideBtn.setFillColor(ideHov ? C_ACCENT_HOV : C_ACCENT);
        ideBtn.setOutlineColor(C_BORDER_LIGHT);
        ideBtn.setOutlineThickness(1.f);
        window.draw(ideBtn);

        sf::Text ideTxt;
        ideTxt.setFont(*m_Font);
        ideTxt.setCharacterSize(11);
        ideTxt.setFillColor(sf::Color::White);
        ideTxt.setString("Open in IDE");
        float idew = ideTxt.getLocalBounds().width;
        ideTxt.setPosition(ideBtnX + (ideBtnW - idew) * 0.5f, btnY + 7.f);
        window.draw(ideTxt);
    } else { m_ScriptErrorOpenIDEBtn = sf::FloatRect(); }
}

