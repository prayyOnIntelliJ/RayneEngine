#include "EditorScene_Common.h"
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <map>


void EditorScene::DrawAddDropdown(sf::RenderWindow &window)
{
    m_AddDropdownHitboxes.clear();

    const float dropX = m_AddBtnBounds.left;
    const float dropY = TopBarHeight;
    const float dropW = 180.f;
    const float itemH = 38.f;
    const float headerH = 24.f;

    struct DropItem
    {
        std::string label;
        std::string action;
        std::string desc;
        bool isHeader = false;
    };

    std::vector<DropItem> items = {
        {"Primitives", "", "", true},
        {"Rectangle", "add_rect", "Rectangle primitive", false},
        {"Circle", "add_circle", "Circle primitive", false},
        {"Triangle", "add_triangle", "Triangle primitive", false},
        {"Pentagon", "add_pentagon", "Pentagon primitive", false},
        {"Hexagon", "add_hexagon", "Hexagon primitive", false},
        {"Objects", "", "", true},
        {"Camera", "add_cam_obj", "In-game camera object", false}
    };

    float dropH = 12.f;
    for (const auto &item: items) { dropH += item.isHeader ? headerH : itemH; }

    sf::RectangleShape bg({dropW, dropH});
    bg.setFillColor(C_BG_ELEVATED);
    bg.setOutlineColor(C_BORDER_LIGHT);
    bg.setOutlineThickness(1.f);
    bg.setPosition(dropX, dropY);
    window.draw(bg);

    float iy = dropY + 6.f;
    for (auto &item: items)
    {
        if (item.isHeader)
        {
            sf::Text ht;
            ht.setFont(*m_Font);
            ht.setCharacterSize(11);
            ht.setFillColor(C_TEXT_SECONDARY);
            ht.setString(item.label);
            ht.setPosition(dropX + 8.f, iy + 6.f);
            window.draw(ht);
            iy += headerH;
            continue;
        }

        const sf::FloatRect ir(dropX, iy, dropW, itemH);
        const bool hov = ir.contains(m_MouseScreenPos);
        const bool cur = (item.action == "add_rect" && m_PlacementType == ObjectType::Rectangle) ||
                         (item.action == "add_circle" && m_PlacementType == ObjectType::Circle) ||
                         (item.action == "add_triangle" && m_PlacementType == ObjectType::Triangle) ||
                         (item.action == "add_pentagon" && m_PlacementType == ObjectType::Pentagon) ||
                         (item.action == "add_hexagon" && m_PlacementType == ObjectType::Hexagon) ||
                         (item.action == "add_cam_obj" && m_PlacementType == ObjectType::Camera);

        if (hov)
        {
            sf::RectangleShape ibg({dropW - 8.f, itemH - 4.f});
            ibg.setFillColor(C_ACCENT_DIM);
            ibg.setPosition(dropX + 4.f, iy + 2.f);
            window.draw(ibg);
        }

        if (cur)
        {
            sf::RectangleShape accent({3.f, itemH - 14.f});
            accent.setFillColor(C_ACCENT);
            accent.setPosition(dropX + 6.f, iy + 7.f);
            window.draw(accent);
        }

        sf::Text lt;
        lt.setFont(*m_Font);
        lt.setCharacterSize(12);
        lt.setFillColor((hov || cur) ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
        lt.setString(item.label);
        lt.setPosition(dropX + 16.f, iy + 6.f);
        window.draw(lt);

        sf::Text dt;
        dt.setFont(*m_Font);
        dt.setCharacterSize(10);
        dt.setFillColor(C_TEXT_MUTED);
        dt.setString(item.desc);
        dt.setPosition(dropX + 16.f, iy + 21.f);
        window.draw(dt);

        m_AddDropdownHitboxes.push_back({ir, item.action});
        iy += itemH;
    }
}


EditorObject *EditorScene::GetInspectedObject()
{
    if (m_InspectorLocked && !m_LockedObjectId.empty())
    {
        EditorObject *locked = ObjectById(m_LockedObjectId);
        if (locked) return locked;
        m_InspectorLocked = false;
        m_LockedObjectId.clear();
    }
    return m_Selected;
}


void EditorScene::DrawInspectorHeader(sf::RenderWindow &window, float panelX, float panelY)
{
    sf::RectangleShape header({InspectorWidth, 36.f});
    header.setPosition(panelX, panelY);
    header.setFillColor(C_BG_ELEVATED);
    window.draw(header);

    sf::RectangleShape headerLine({InspectorWidth, 1.f});
    headerLine.setFillColor(C_BORDER);
    headerLine.setPosition(panelX, panelY + 35.f);
    window.draw(headerLine);

    sf::Text title;
    title.setFont(*m_Font);
    title.setCharacterSize(11);
    title.setFillColor(m_InspectorLocked ? sf::Color(255, 205, 75) : C_TEXT_SECONDARY);
    title.setStyle(sf::Text::Bold);
    if (m_InspectorLocked && !m_LockedObjectId.empty()) { title.setString("INSPECTOR [LOCKED]"); } else
    {
        title.setString("INSPECTOR");
    }
    title.setPosition(panelX + InspectorPad + 2.f, panelY + 12.f);
    window.draw(title);
    const float btnSize = 22.f;
    const sf::FloatRect lockBtnRect(panelX + InspectorWidth - btnSize - 8.f, panelY + 7.f, btnSize, btnSize);
    const bool lockHov = lockBtnRect.contains(m_MouseScreenPos);

    if (lockHov)
    {
        if (m_InspectorLocked)
        {
            m_ActiveTooltip = "Unlock Inspector (currently locked to " + m_LockedObjectId + ")";
        } else
        {
            EditorObject *target = m_Selected;
            m_ActiveTooltip = target ? ("Lock Inspector to " + target->id) : "Select an object to lock Inspector";
        }
    }
    sf::RectangleShape lockBg({btnSize, btnSize});
    lockBg.setPosition(lockBtnRect.left, lockBtnRect.top);
    if (m_InspectorLocked)
    {
        lockBg.setFillColor(lockHov ? sf::Color(70, 60, 20) : sf::Color(50, 42, 14));
        lockBg.setOutlineColor(sf::Color(255, 205, 75));
        lockBg.setOutlineThickness(1.5f);
    } else
    {
        lockBg.setFillColor(lockHov ? C_BG_ELEVATED : C_BG_INPUT);
        lockBg.setOutlineColor(lockHov ? C_TEXT_SECONDARY : C_BORDER);
        lockBg.setOutlineThickness(1.f);
    }
    window.draw(lockBg);
    const sf::Color lockColor = m_InspectorLocked
                                    ? sf::Color(255, 215, 80)
                                    : (lockHov ? C_TEXT_PRIMARY : sf::Color(140, 145, 160));
    sf::RectangleShape lockBody({10.f, 8.f});
    lockBody.setPosition(lockBtnRect.left + 6.f, lockBtnRect.top + 10.f);
    lockBody.setFillColor(lockColor);
    window.draw(lockBody);
    sf::RectangleShape shackle({6.f, 6.f});
    if (m_InspectorLocked) { shackle.setPosition(lockBtnRect.left + 8.f, lockBtnRect.top + 5.f); } else
    {
        shackle.setPosition(lockBtnRect.left + 9.5f, lockBtnRect.top + 3.5f);
    }
    shackle.setFillColor(sf::Color::Transparent);
    shackle.setOutlineColor(lockColor);
    shackle.setOutlineThickness(1.5f);
    window.draw(shackle);
    sf::RectangleShape keyhole({2.f, 3.f});
    keyhole.setPosition(lockBtnRect.left + 10.f, lockBtnRect.top + 12.f);
    keyhole.setFillColor(sf::Color(20, 22, 28));
    window.draw(keyhole);

    m_InspectorButtons.push_back({lockBtnRect, "toggle_inspector_lock"});
}


void EditorScene::DrawInspector(sf::RenderWindow &window)
{
    m_InspectorButtons.clear();
    m_ActiveTooltip.clear();

    const float panelX = m_InspectorBounds.left;
    const float panelY = m_InspectorBounds.top;
    const float panelH = m_InspectorBounds.height;
    float maxScroll = std::max(0.f, m_InspectorContentHeight - (panelH - 42.f));
    m_InspectorScrollY = std::max(0.f, std::min(m_InspectorScrollY, maxScroll));

    m_InspectorPanel.setPosition(panelX, panelY);
    m_InspectorPanel.setSize({InspectorWidth, panelH});
    window.draw(m_InspectorPanel);

    sf::RectangleShape leftBorder({1.f, panelH});
    leftBorder.setFillColor(C_BORDER);
    leftBorder.setPosition(panelX, panelY);
    window.draw(leftBorder);
    sf::View origView = window.getView();
    window.setView(window.getDefaultView());

    const float winW = static_cast<float>(window.getSize().x);
    const float winH = static_cast<float>(window.getSize().y);

    sf::View clipView;
    clipView.setSize(InspectorWidth, panelH - 36.f);
    clipView.setCenter(panelX + InspectorWidth / 2.f, panelY + 36.f + (panelH - 36.f) / 2.f);
    clipView.setViewport({
        panelX / winW,
        (panelY + 36.f) / winH,
        InspectorWidth / winW,
        (panelH - 36.f) / winH
    });
    window.setView(clipView);

    EditorObject *target = GetInspectedObject();

    if (!target)
    {
        std::string selPath = m_ContentBrowser->GetSelectedPath();
        if (!selPath.empty())
        {
            float y = panelY + 42.f - m_InspectorScrollY;
            float contentStartY = y;
            y = DrawSectionHeader(window, "FILE PROPERTIES", sf::Color(180, 180, 220), panelX, y);

            std::error_code ec;
            std::filesystem::path p(selPath);

            if (std::filesystem::exists(p, ec))
            {
                std::string filename = p.filename().string();
                std::string ext = p.extension().string();
                std::string typeName = "Unknown File";

                std::string lowerExt = ext;
                std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::tolower);

                if (lowerExt == ".lua") typeName = "Lua Script";
                else if (lowerExt == ".json") typeName = "JSON Data";
                else if (lowerExt == ".template") typeName = "Template";
                else if (lowerExt == ".png" || lowerExt == ".jpg" || lowerExt == ".jpeg" || lowerExt == ".jfif")
                    typeName = "Image Asset";
                else if (lowerExt == ".wav" || lowerExt == ".ogg") typeName = "Audio Asset";
                else if (lowerExt == ".ttf") typeName = "Font Asset";
                else if (std::filesystem::is_directory(p, ec)) typeName = "Directory";

                uintmax_t size = std::filesystem::file_size(p, ec);
                std::string sizeStr = "-";
                if (!ec)
                {
                    if (size < 1024) sizeStr = std::to_string(size) + " B";
                    else if (size < 1024 * 1024) sizeStr = std::to_string(size / 1024) + " KB";
                    else sizeStr = std::to_string(size / (1024 * 1024)) + " MB";
                }

                auto ftime = std::filesystem::last_write_time(p, ec);
                std::string timeStr = "Unknown";
                if (!ec)
                {
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
                    std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
                    char timeBuf[64];
                    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&cftime));
                    timeStr = timeBuf;
                }

                y = DrawRow(window, "Name", filename, panelX, y);
                y = DrawRow(window, "Type", typeName, panelX, y);
                if (typeName != "Directory") { y = DrawRow(window, "Size", sizeStr, panelX, y); }
                y = DrawRow(window, "Modified", timeStr, panelX, y);

                if (typeName == "Image Asset")
                {
                    y += 10.f;
                    y = DrawSectionHeader(window, "PREVIEW", sf::Color(220, 150, 220), panelX, y);

                    auto tex = ResourceManager::Get().GetTexture(selPath);
                    if (tex)
                    {
                        const sf::Vector2u ts = tex->getSize();
                        y = DrawRow(window, "Dimensions", std::to_string(ts.x) + " x " + std::to_string(ts.y), panelX,
                                    y);

                        float previewW = InspectorWidth - 20.f;
                        float previewH = previewW * ((float) ts.y / (float) ts.x);
                        if (previewH > 200.f)
                        {
                            previewH = 200.f;
                            previewW = previewH * ((float) ts.x / (float) ts.y);
                        }

                        sf::Sprite sprite(*tex);
                        sprite.setScale(previewW / ts.x, previewH / ts.y);
                        sprite.setPosition(panelX + 10.f + (InspectorWidth - 20.f - previewW) / 2.f, y + 10.f);
                        window.draw(sprite);

                        y += previewH + 20.f;
                    }
                } else if (typeName == "Template")
                {
                    y += 10.f;
                    y = DrawSectionHeader(window, "TEMPLATE PREVIEW", sf::Color(100, 180, 255), panelX, y);
                    y = DrawTemplatePreview(window, selPath, panelX + InspectorPad, y,
                                            InspectorWidth - InspectorPad * 2.f, 56.f);

                    y += 6.f;
                    y = DrawSectionHeader(window, "FILE CONTENT", sf::Color(150, 220, 150), panelX, y);

                    std::ifstream ifs(selPath);
                    if (ifs.is_open())
                    {
                        int lineCount = 0;
                        std::string line;
                        std::string previewContent;
                        while (std::getline(ifs, line) && lineCount < 15)
                        {
                            previewContent += line + "\n";
                            lineCount++;
                        }
                        if (std::getline(ifs, line)) { previewContent += "..."; }
                        ifs.close();

                        sf::Text contentText;
                        contentText.setFont(*m_Font);
                        contentText.setCharacterSize(9);
                        contentText.setFillColor(sf::Color(180, 190, 200));
                        contentText.setString(previewContent);
                        contentText.setPosition(panelX + 10.f, y + 5.f);
                        window.draw(contentText);

                        y += contentText.getLocalBounds().height + 15.f;
                    }
                } else if (typeName == "Lua Script" || typeName == "JSON Data")
                {
                    y += 10.f;
                    y = DrawSectionHeader(window, "FILE CONTENT", sf::Color(150, 220, 150), panelX, y);

                    std::ifstream ifs(selPath);
                    if (ifs.is_open())
                    {
                        int lineCount = 0;
                        std::string line;
                        std::string previewContent;
                        while (std::getline(ifs, line) && lineCount < 20)
                        {
                            previewContent += line + "\n";
                            lineCount++;
                        }
                        if (std::getline(ifs, line)) { previewContent += "..."; }
                        ifs.close();

                        sf::Text contentText;
                        contentText.setFont(*m_Font);
                        contentText.setCharacterSize(9);
                        contentText.setFillColor(sf::Color(180, 190, 200));
                        contentText.setString(previewContent);
                        contentText.setPosition(panelX + 10.f, y + 5.f);
                        window.draw(contentText);

                        y += contentText.getLocalBounds().height + 15.f;
                    }
                }
                m_InspectorContentHeight = (y - contentStartY);
            }
        } else
        {
            sf::Text empty;
            empty.setFont(*m_Font);
            empty.setCharacterSize(12);
            empty.setFillColor(C_TEXT_MUTED);
            empty.setString("No selection");
            empty.setPosition(panelX + InspectorPad + 2.f, panelY + 52.f - m_InspectorScrollY);
            window.draw(empty);
            m_InspectorContentHeight = 0.f;
        }
        window.setView(origView);
        DrawInspectorHeader(window, panelX, panelY);
        return;
    }

    float y = panelY + 42.f - m_InspectorScrollY;
    float contentStartY = panelY + 42.f - m_InspectorScrollY;

    y = DrawSectionHeader(window, "OBJECT", C_TEXT_SECONDARY, panelX, y);

    std::string nameDisplay = (m_ActiveField == EditField::Name && !m_ActiveInputText.empty())
                                  ? m_ActiveInputText + "|"
                                  : (m_ActiveField == EditField::Name ? "|" : target->id);
    y = DrawEditableRow(window, "Name", nameDisplay, "edit_name", panelX, y);

    std::string tagDisplay = (m_ActiveField == EditField::Tag && !m_ActiveInputText.empty())
                                 ? m_ActiveInputText + "|"
                                 : (m_ActiveField == EditField::Tag ? "|" : target->tag);
    y = DrawEditableRow(window, "Tag", tagDisplay, "edit_tag", panelX, y);
    y = DrawRow(window, "Entity", std::to_string(target->entity), panelX, y); {
        std::string typeLabel = "rectangle";
        if (target->objectType == ObjectType::Circle) typeLabel = "circle";
        else if (target->objectType == ObjectType::Triangle) typeLabel = "triangle";
        else if (target->objectType == ObjectType::Pentagon) typeLabel = "pentagon";
        else if (target->objectType == ObjectType::Hexagon) typeLabel = "hexagon";
        else if (target->objectType == ObjectType::Sprite) typeLabel = "sprite";
        else if (target->objectType == ObjectType::Camera) typeLabel = "camera";
        y = DrawRow(window, "Type", typeLabel, panelX, y);
    }
    std::string parentDisplay = target->parentId.empty() ? "(None)" : target->parentId;
    y = DrawRow(window, "Parent", parentDisplay, panelX, y);
    if (!target->parentId.empty())
    {
        y = DrawActionButton(window, "Detach Parent", "detach_parent", panelX, y, C_DANGER_DIM, C_DANGER);
    }

    if (!target->templatePath.empty())
    {
        std::filesystem::path tp(target->templatePath);
        y = DrawRow(window, "Template", tp.filename().string(), panelX, y);
        y = DrawTemplatePreview(window, target->templatePath, panelX + InspectorPad, y,
                                InspectorWidth - InspectorPad * 2.f, 52.f);
        y = DrawActionButton(window, "Edit Template", "edit_template", panelX, y, sf::Color(30, 100, 70),
                             sf::Color(60, 180, 120));
        y = DrawActionButton(window, "Apply to Template", "apply_template", panelX, y, sf::Color(30, 80, 140),
                             sf::Color(70, 140, 240));
        y = DrawActionButton(window, "Unlink Template", "unlink_template", panelX, y, C_DANGER_DIM, C_DANGER);
    } else
    {
        y = DrawActionButton(window, "Save as Template", "save_as_template", panelX, y, sf::Color(40, 60, 90),
                             sf::Color(80, 120, 180));
    }

    y += 8.f;
    y = DrawSectionHeader(window, "TRANSFORM", C_TEXT_SECONDARY, panelX, y);

    std::string txDisplay = (m_ActiveField == EditField::TransformX && !m_ActiveInputText.empty())
                                ? m_ActiveInputText + "|"
                                : (m_ActiveField == EditField::TransformX
                                       ? "|"
                                       : std::to_string((int) target->localPosition.x));
    y = DrawEditableRow(window, "Position X", txDisplay, "edit_x", panelX, y);
    std::string tyDisplay = (m_ActiveField == EditField::TransformY && !m_ActiveInputText.empty())
                                ? m_ActiveInputText + "|"
                                : (m_ActiveField == EditField::TransformY
                                       ? "|"
                                       : std::to_string((int) target->localPosition.y));
    y = DrawEditableRow(window, "Position Y", tyDisplay, "edit_y", panelX, y);

    std::string rotDisplay = (m_ActiveField == EditField::Rotation && !m_ActiveInputText.empty())
                                 ? m_ActiveInputText + "|"
                                 : (m_ActiveField == EditField::Rotation
                                        ? "|"
                                        : FormatFloat(target->rotation, 2));
    y = DrawEditableRow(window, "Rotation", rotDisplay, "edit_rot", panelX, y);

    std::string sxDisplay = (m_ActiveField == EditField::ScaleX && !m_ActiveInputText.empty())
                                ? m_ActiveInputText + "|"
                                : (m_ActiveField == EditField::ScaleX
                                       ? "|"
                                       : FormatFloat(target->scaleX, 2));
    y = DrawEditableRow(window, "Scale X", sxDisplay, "edit_scalex", panelX, y);

    std::string syDisplay = (m_ActiveField == EditField::ScaleY && !m_ActiveInputText.empty())
                                ? m_ActiveInputText + "|"
                                : (m_ActiveField == EditField::ScaleY
                                       ? "|"
                                       : FormatFloat(target->scaleY, 2));
    y = DrawEditableRow(window, "Scale Y", syDisplay, "edit_scaley", panelX, y);


    y += 8.f;
    y = DrawSectionHeader(window, "RENDER", C_TEXT_SECONDARY, panelX, y);

    std::string wDisplay = (m_ActiveField == EditField::SizeW && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::SizeW
                                      ? "|"
                                      : std::to_string((int) target->shape.getSize().x));
    y = DrawEditableRow(window, "Width", wDisplay, "edit_w", panelX, y);
    std::string hDisplay = (m_ActiveField == EditField::SizeH && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::SizeH
                                      ? "|"
                                      : std::to_string((int) target->shape.getSize().y));
    y = DrawEditableRow(window, "Height", hDisplay, "edit_h", panelX, y);

    y += 4.f;

    if (!target->previewTexture)
    {
        const float totalW = InspectorWidth - InspectorPad * 2;
        const float pickBtnW = 75.f;
        const float swatchW = totalW - pickBtnW - 6.f;

        const sf::FloatRect swatchRect(panelX + InspectorPad, y, swatchW, 22.f);
        const bool swatchHov = swatchRect.contains(m_MouseScreenPos);
        sf::RectangleShape colorSwatch({swatchW, 22.f});
        colorSwatch.setFillColor(target->color);
        colorSwatch.setOutlineColor(swatchHov ? C_ACCENT : C_BORDER_LIGHT);
        colorSwatch.setOutlineThickness(swatchHov ? 2.f : 1.f);
        colorSwatch.setPosition(swatchRect.left, swatchRect.top);
        window.draw(colorSwatch);
        m_InspectorButtons.push_back({swatchRect, "pick_color"});

        const sf::FloatRect btnRect(panelX + InspectorPad + swatchW + 6.f, y, pickBtnW, 22.f);
        const bool btnHov = btnRect.contains(m_MouseScreenPos);
        DrawPill(window, btnRect, btnHov ? C_BG_ELEVATED : C_BG_INPUT, btnHov ? C_ACCENT : C_BORDER_LIGHT);
        sf::Text btnText;
        btnText.setFont(*m_Font);
        btnText.setCharacterSize(11);
        btnText.setFillColor(btnHov ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
        btnText.setString("Pick Color");
        btnText.setPosition(btnRect.left + (btnRect.width - btnText.getLocalBounds().width) / 2.f, btnRect.top + 4.f);
        window.draw(btnText);
        m_InspectorButtons.push_back({btnRect, "pick_color"});

        y += 28.f;
    }

    std::string rDisplay = (m_ActiveField == EditField::ColorR && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::ColorR ? "|" : std::to_string(target->color.r));
    y = DrawEditableRow(window, "Color Red", rDisplay, "edit_r", panelX, y);
    std::string gDisplay = (m_ActiveField == EditField::ColorG && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::ColorG ? "|" : std::to_string(target->color.g));
    y = DrawEditableRow(window, "Color Green", gDisplay, "edit_g", panelX, y);
    std::string bDisplay = (m_ActiveField == EditField::ColorB && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::ColorB ? "|" : std::to_string(target->color.b));
    y = DrawEditableRow(window, "Color Blue", bDisplay, "edit_b", panelX, y);

    std::string zDisplay = (m_ActiveField == EditField::ZIndex && !m_ActiveInputText.empty())
                               ? m_ActiveInputText + "|"
                               : (m_ActiveField == EditField::ZIndex ? "|" : std::to_string(target->zIndex));
    y = DrawEditableRow(window, "Z-Index", zDisplay, "edit_z", panelX, y); {
        const float btnW = (InspectorWidth - InspectorPad * 2.f - 6.f) / 2.f;
        const sf::FloatRect fwdRect(panelX + InspectorPad, y, btnW, 20.f);
        const sf::FloatRect bwdRect(panelX + InspectorPad + btnW + 6.f, y, btnW, 20.f);
        const bool fwdHov = fwdRect.contains(m_MouseScreenPos);
        const bool bwdHov = bwdRect.contains(m_MouseScreenPos);

        DrawPill(window, fwdRect, fwdHov ? C_BG_ELEVATED : C_BG_INPUT, fwdHov ? C_ACCENT : C_BORDER);
        DrawPill(window, bwdRect, bwdHov ? C_BG_ELEVATED : C_BG_INPUT, bwdHov ? C_ACCENT : C_BORDER);

        sf::Text fwdTxt("+ Forward", *m_Font, 10);
        fwdTxt.setFillColor(fwdHov ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
        fwdTxt.setPosition(fwdRect.left + (btnW - fwdTxt.getLocalBounds().width) / 2.f, fwdRect.top + 3.f);
        window.draw(fwdTxt);

        sf::Text bwdTxt("- Backward", *m_Font, 10);
        bwdTxt.setFillColor(bwdHov ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
        bwdTxt.setPosition(bwdRect.left + (btnW - bwdTxt.getLocalBounds().width) / 2.f, bwdRect.top + 3.f);
        window.draw(bwdTxt);

        m_InspectorButtons.push_back({fwdRect, "layer_forward"});
        m_InspectorButtons.push_back({bwdRect, "layer_backward"});

        if (fwdHov) m_ActiveTooltip = "Bring object forward (+1 layer)";
        if (bwdHov) m_ActiveTooltip = "Send object backward (-1 layer)";

        y += 24.f;
    }

    bool isNeverVisibleType = (target->objectType == ObjectType::SpawnPoint ||
                               target->objectType == ObjectType::AudioSource ||
                               target->objectType == ObjectType::ParticleEmitter ||
                               target->objectType == ObjectType::Camera ||
                               target->objectType == ObjectType::Empty ||
                               target->objectType == ObjectType::TriggerZone);

    if (isNeverVisibleType) { y = DrawRow(window, "Visible in Game", "Hidden (Helper)", panelX, y); } else
    {
        y = DrawCheckboxRow(window, "Visible in Game", target->visibleInGame, "toggle_visible_in_game", panelX, y);
    }

    y += 8.f;


    if (target->previewTexture)
    {
        y = DrawSectionHeader(window, "SPRITE COMPONENT", C_TEXT_SECONDARY, panelX, y);

        std::string spriteName = target->spritePath;
        const size_t sl = spriteName.find_last_of("/\\");
        if (sl != std::string::npos) spriteName = spriteName.substr(sl + 1);
        y = DrawRow(window, "File", spriteName.empty() ? "none" : spriteName, panelX, y);

        if (target->previewTexture)
        {
            const float thumbH = 48.f;
            const float thumbW = InspectorWidth - InspectorPad * 2;
            sf::RectangleShape thumb({thumbW, thumbH});
            thumb.setPosition(panelX + InspectorPad, y);
            thumb.setFillColor(C_BG_INPUT);
            thumb.setOutlineColor(C_BORDER);
            thumb.setOutlineThickness(1.f);
            window.draw(thumb);

            sf::Sprite preview;
            preview.setTexture(*target->previewTexture);
            const sf::Vector2u ts = target->previewTexture->getSize();
            if (ts.x > 0 && ts.y > 0)
            {
                const float scaleX = thumbW / static_cast<float>(ts.x);
                const float scaleY = thumbH / static_cast<float>(ts.y);
                const float scale = std::min(scaleX, scaleY);
                preview.setScale(scale, scale);
                preview.setPosition(
                    panelX + InspectorPad + (thumbW - ts.x * scale) / 2.f,
                    y + (thumbH - ts.y * scale) / 2.f);
            }
            window.draw(preview);
            y += thumbH + 4.f;
        }

        y = DrawActionButton(window, "Remove Sprite", "remove_sprite", panelX, y, C_DANGER_DIM, C_DANGER);
    } else
    {
        y = DrawActionButton(window, "+ Sprite Component", "change_sprite", panelX, y, C_BG_ELEVATED, C_BORDER_LIGHT);
        sf::Text hint;
        hint.setFont(*m_Font);
        hint.setCharacterSize(10);
        hint.setFillColor(C_TEXT_MUTED);
        hint.setString("(drag image from browser)");
        hint.setPosition(panelX + InspectorPad + 4.f, y);
        window.draw(hint);
        y += 16.f;
    }

    y += 8.f;

    if (target->entity != 0 && m_Registry.HasComponent<VelocityComponent>(target->entity))
    {
        auto &vel = m_Registry.GetComponent<VelocityComponent>(target->entity);
        y = DrawSectionHeader(window, "VELOCITY", sf::Color(100, 220, 160), panelX, y);

        std::string vxDisplay = (m_ActiveField == EditField::VelocityDX && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::VelocityDX
                                           ? "|"
                                           : FormatFloat(vel.dx, 2));
        y = DrawEditableRow(window, "Velocity X", vxDisplay, "edit_vel_dx", panelX, y);

        std::string vyDisplay = (m_ActiveField == EditField::VelocityDY && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::VelocityDY
                                           ? "|"
                                           : FormatFloat(vel.dy, 2));
        y = DrawEditableRow(window, "Velocity Y", vyDisplay, "edit_vel_dy", panelX, y);

        y += 4.f;
        y = DrawActionButton(window, "Remove Velocity", "remove_velocity", panelX, y, C_DANGER_DIM, C_DANGER);
        y += 8.f;
    } else if (target->entity != 0)
    {
        y = DrawActionButton(window, "+ Velocity", "add_velocity", panelX, y, C_BG_ELEVATED, C_BORDER_LIGHT);
        y += 8.f;
    }

    if (target->entity != 0 && m_Registry.HasComponent<CameraComponent>(target->entity))
    {
        auto &cam = m_Registry.GetComponent<CameraComponent>(target->entity);
        y = DrawSectionHeader(window, "CAMERA", sf::Color(130, 200, 255), panelX, y);

        int activeCamCount = 0;
        m_Registry.ForEach<CameraComponent>([&activeCamCount](Entity, CameraComponent &c) {
            if (c.active) activeCamCount++;
        });

        if (activeCamCount > 1)
        {
            y = DrawRow(window, "Status", "Multi-Target (" + std::to_string(activeCamCount) + " Active)", panelX, y);
            std::string modeStr = "Average (Midpoint)";
            if (cam.multiFollowMode == CameraMultiFollowMode::AutoFrame) modeStr = "Auto-Frame (Both in View)";
            else if (cam.multiFollowMode == CameraMultiFollowMode::Priority) modeStr = "Priority (Leader)";

            y = DrawActionButton(window, "Mode: " + modeStr, "cycle_cam_mode", panelX, y, C_BG_ELEVATED,
                                 C_BORDER_LIGHT);
        } else { y = DrawRow(window, "Status", "Single Camera", panelX, y); }

        y = DrawCheckboxRow(window, "Active", cam.active, "toggle_cam_active", panelX, y);

        if (activeCamCount > 1 && cam.multiFollowMode == CameraMultiFollowMode::Priority)
        {
            std::string prioDisplay = (m_ActiveField == EditField::CameraPriority && !m_ActiveInputText.empty())
                                          ? m_ActiveInputText + "|"
                                          : (m_ActiveField == EditField::CameraPriority
                                                 ? "|"
                                                 : std::to_string(cam.priority));
            y = DrawEditableRow(window, "Priority", prioDisplay, "edit_cam_prio", panelX, y);
        }

        std::string speedDisplay = (m_ActiveField == EditField::CameraSmoothSpeed && !m_ActiveInputText.empty())
                                       ? m_ActiveInputText + "|"
                                       : (m_ActiveField == EditField::CameraSmoothSpeed
                                              ? "|"
                                              : FormatFloat(cam.smoothSpeed, 2));
        y = DrawEditableRow(window, "Smooth Speed", speedDisplay, "edit_cam_speed", panelX, y);

        std::string oxDisplay = (m_ActiveField == EditField::CameraOffsetX && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::CameraOffsetX
                                           ? "|"
                                           : FormatFloat(cam.offsetX, 2));
        y = DrawEditableRow(window, "Offset X", oxDisplay, "edit_cam_ox", panelX, y);

        std::string oyDisplay = (m_ActiveField == EditField::CameraOffsetY && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::CameraOffsetY
                                           ? "|"
                                           : FormatFloat(cam.offsetY, 2));
        y = DrawEditableRow(window, "Offset Y", oyDisplay, "edit_cam_oy", panelX, y);

        std::string zoomDisplay = (m_ActiveField == EditField::CameraZoom && !m_ActiveInputText.empty())
                                      ? m_ActiveInputText + "|"
                                      : (m_ActiveField == EditField::CameraZoom
                                             ? "|"
                                             : FormatFloat(cam.zoom, 2));
        y = DrawEditableRow(window, "Zoom", zoomDisplay, "edit_cam_zoom", panelX, y);

        if (cam.multiFollowMode == CameraMultiFollowMode::AutoFrame)
        {
            std::string padDisplay = (m_ActiveField == EditField::CameraAutoFramePadding && !m_ActiveInputText.empty())
                                         ? m_ActiveInputText + "|"
                                         : (m_ActiveField == EditField::CameraAutoFramePadding
                                                ? "|"
                                                : FormatFloat(cam.autoFramePadding, 1));
            y = DrawEditableRow(window, "Frame Padding", padDisplay, "edit_cam_pad", panelX, y);

            std::string minZDisplay = (m_ActiveField == EditField::CameraMinZoom && !m_ActiveInputText.empty())
                                          ? m_ActiveInputText + "|"
                                          : (m_ActiveField == EditField::CameraMinZoom
                                                 ? "|"
                                                 : FormatFloat(cam.minZoom, 2));
            y = DrawEditableRow(window, "Min Zoom", minZDisplay, "edit_cam_minz", panelX, y);

            std::string maxZDisplay = (m_ActiveField == EditField::CameraMaxZoom && !m_ActiveInputText.empty())
                                          ? m_ActiveInputText + "|"
                                          : (m_ActiveField == EditField::CameraMaxZoom
                                                 ? "|"
                                                 : FormatFloat(cam.maxZoom, 2));
            y = DrawEditableRow(window, "Max Zoom", maxZDisplay, "edit_cam_maxz", panelX, y);
        }

        y += 4.f;
        y = DrawActionButton(window, "Remove Camera", "remove_camera", panelX, y, C_DANGER_DIM, C_DANGER);
        y += 8.f;
    } else if (target->entity != 0)
    {
        y = DrawActionButton(window, "+ Camera Follow", "add_camera", panelX, y, C_BG_ELEVATED, C_BORDER_LIGHT);
        y += 8.f;
    }

    if (target->entity != 0 && m_Registry.HasComponent<CollisionComponent>(target->entity))
    {
        auto &col = m_Registry.GetComponent<CollisionComponent>(target->entity);
        y = DrawSectionHeader(window, "COLLISION", C_TEXT_SECONDARY, panelX, y);
        std::string chanDisplay = (m_ActiveField == EditField::CollisionChannel && !m_ActiveInputText.empty())
                                      ? m_ActiveInputText + "|"
                                      : (m_ActiveField == EditField::CollisionChannel
                                             ? "|"
                                             : std::to_string(col.channel));
        y = DrawEditableRow(window, "Collision Channel", chanDisplay, "edit_collision_channel", panelX, y);
        y = DrawCheckboxRow(window, "Is Trigger", col.isTrigger, "toggle_collision_trigger", panelX, y);
        std::string shapeLabel = "Collider Shape: " +
                                 std::string(col.shape == ColliderShape::Circle ? "Circle" : "Box");
        y = DrawActionButton(window, shapeLabel, "toggle_collision_shape", panelX, y, C_BG_ELEVATED, C_BORDER_LIGHT);
        std::string typeLabel = "Contact Type: " + std::string(col.type == CollisionType::Solid ? "Solid" : "Static");
        y = DrawActionButton(window, typeLabel, "toggle_collision_type", panelX, y, C_BG_ELEVATED, C_BORDER_LIGHT);
        y += 4.f;
        y = DrawActionButton(window, "Remove Collision", "remove_collision", panelX, y, C_DANGER_DIM, C_DANGER);
        y += 8.f;
    } else if (target->entity != 0)
    {
        y = DrawActionButton(window, "+ Collision", "add_collision", panelX, y, C_BG_ELEVATED, C_BORDER_LIGHT);
        y += 8.f;
    }

    if (target->entity != 0 && m_Registry.HasComponent<Rigidbody2DComponent>(target->entity))
    {
        auto &rb = m_Registry.GetComponent<Rigidbody2DComponent>(target->entity);
        y = DrawSectionHeader(window, "RIGIDBODY 2D", sf::Color(100, 190, 255), panelX, y);

        std::string bodyTypeStr = "Dynamic";
        if (rb.bodyType == BodyType::Kinematic) bodyTypeStr = "Kinematic";
        else if (rb.bodyType == BodyType::Static) bodyTypeStr = "Static";
        y = DrawActionButton(window, "Body Type: " + bodyTypeStr, "toggle_rigidbody_type", panelX, y, C_BG_ELEVATED,
                             C_BORDER_LIGHT);

        std::string massDisplay = (m_ActiveField == EditField::RigidbodyMass && !m_ActiveInputText.empty())
                                      ? m_ActiveInputText + "|"
                                      : (m_ActiveField == EditField::RigidbodyMass
                                             ? "|"
                                             : FormatFloat(rb.mass, 2));
        y = DrawEditableRow(window, "Mass", massDisplay, "edit_rb_mass", panelX, y);

        std::string gravDisplay = (m_ActiveField == EditField::RigidbodyGravity && !m_ActiveInputText.empty())
                                      ? m_ActiveInputText + "|"
                                      : (m_ActiveField == EditField::RigidbodyGravity
                                             ? "|"
                                             : FormatFloat(rb.gravityScale, 2));
        y = DrawEditableRow(window, "Gravity Scale", gravDisplay, "edit_rb_gravity", panelX, y);

        std::string restDisplay = (m_ActiveField == EditField::RigidbodyRestitution && !m_ActiveInputText.empty())
                                      ? m_ActiveInputText + "|"
                                      : (m_ActiveField == EditField::RigidbodyRestitution
                                             ? "|"
                                             : FormatFloat(rb.restitution, 2));
        y = DrawEditableRow(window, "Bounciness", restDisplay, "edit_rb_restitution", panelX, y);

        std::string dragDisplay = (m_ActiveField == EditField::RigidbodyDrag && !m_ActiveInputText.empty())
                                      ? m_ActiveInputText + "|"
                                      : (m_ActiveField == EditField::RigidbodyDrag
                                             ? "|"
                                             : FormatFloat(rb.drag, 2));
        y = DrawEditableRow(window, "Linear Drag", dragDisplay, "edit_rb_drag", panelX, y);

        y = DrawCheckboxRow(window, "Freeze Rotation", rb.freezeRotation, "toggle_rb_freeze_rot", panelX, y);

        y += 4.f;
        y = DrawActionButton(window, "Remove Rigidbody", "remove_rigidbody", panelX, y, C_DANGER_DIM, C_DANGER);
        y += 8.f;
    } else if (target->entity != 0)
    {
        y = DrawActionButton(window, "+ Rigidbody 2D", "add_rigidbody", panelX, y, C_BG_ELEVATED, C_BORDER_LIGHT);
        y += 8.f;
    }

    if (target->entity != 0 && (m_Registry.HasComponent<TextComponent>(target->entity) || target->objectType ==
                                ObjectType::WorldText))
    {
        y = DrawSectionHeader(window, "TEXT COMPONENT", sf::Color(100, 220, 255), panelX, y);
        std::string txtDisplay = (m_ActiveField == EditField::TextContent && !m_ActiveInputText.empty())
                                     ? m_ActiveInputText + "|"
                                     : (m_ActiveField == EditField::TextContent ? "|" : target->textString);
        y = DrawEditableRow(window, "Text", txtDisplay, "edit_text_content", panelX, y);

        std::string sizeDisplay = (m_ActiveField == EditField::TextFontSize && !m_ActiveInputText.empty())
                                      ? m_ActiveInputText + "|"
                                      : (m_ActiveField == EditField::TextFontSize
                                             ? "|"
                                             : std::to_string(target->textFontSize));
        y = DrawEditableRow(window, "Font Size", sizeDisplay, "edit_text_fontsize", panelX, y);

        std::string alignStr = (target->textAlignment == 1)
                                   ? "Center"
                                   : ((target->textAlignment == 2) ? "Right" : "Left");
        y = DrawActionButton(window, "Alignment: " + alignStr, "cycle_text_align", panelX, y, C_BG_ELEVATED,
                             C_BORDER_LIGHT);

        float rowY = y;
        sf::Text lbl;
        lbl.setFont(*m_Font);
        lbl.setCharacterSize(11);
        lbl.setFillColor(C_TEXT_MUTED);
        lbl.setString("Text Color");
        lbl.setPosition(panelX + InspectorPad, rowY + 4.f);
        window.draw(lbl);
        sf::RectangleShape swatch({InspectorWidth - InspectorPad * 2.f, 18.f});
        swatch.setFillColor(target->textColor);
        swatch.setPosition(panelX + InspectorPad, rowY + 20.f);
        window.draw(swatch);
        y = DrawActionButton(window, "Pick Text Color", "pick_text_color", panelX, rowY + 40.f, C_BG_ELEVATED,
                             C_BORDER_LIGHT);

        y += 4.f;
        y = DrawActionButton(window, "Remove Text", "remove_text", panelX, y, C_DANGER_DIM, C_DANGER);
        y += 8.f;
    } else if (target->entity != 0)
    {
        y = DrawActionButton(window, "+ Text Component", "add_text", panelX, y, C_BG_ELEVATED, C_BORDER_LIGHT);
        y += 8.f;
    }

    if (target->entity != 0 && (m_Registry.HasComponent<AudioSourceComponent>(target->entity) || target->objectType ==
                                ObjectType::AudioSource))
    {
        y = DrawSectionHeader(window, "AUDIO SOURCE", sf::Color(255, 180, 80), panelX, y);
        std::string pathDisplay = (m_ActiveField == EditField::AudioPath && !m_ActiveInputText.empty())
                                      ? m_ActiveInputText + "|"
                                      : (m_ActiveField == EditField::AudioPath
                                             ? "|"
                                             : (target->audioClipPath.empty()
                                                    ? "(drag .wav/.ogg or click)"
                                                    : std::filesystem::path(
                                                        target->audioClipPath).filename().string()));
        y = DrawEditableRow(window, "Sound Clip", pathDisplay, "edit_audio_path", panelX, y);

        std::string volDisplay = (m_ActiveField == EditField::AudioVolume && !m_ActiveInputText.empty())
                                     ? m_ActiveInputText + "|"
                                     : (m_ActiveField == EditField::AudioVolume
                                            ? "|"
                                            : FormatFloat(target->audioVolume, 1));
        y = DrawEditableRow(window, "Volume (0-100)", volDisplay, "edit_audio_volume", panelX, y);

        std::string pitchDisplay = (m_ActiveField == EditField::AudioPitch && !m_ActiveInputText.empty())
                                       ? m_ActiveInputText + "|"
                                       : (m_ActiveField == EditField::AudioPitch
                                              ? "|"
                                              : FormatFloat(target->audioPitch, 2));
        y = DrawEditableRow(window, "Pitch", pitchDisplay, "edit_audio_pitch", panelX, y);

        y = DrawCheckboxRow(window, "Loop", target->audioLoop, "toggle_audio_loop", panelX, y);
        y = DrawCheckboxRow(window, "Play On Start", target->audioPlayOnStart, "toggle_audio_playonstart", panelX, y);
        y = DrawCheckboxRow(window, "Spatial Audio", target->audioIsSpatial, "toggle_audio_spatial", panelX, y);

        y += 4.f;
        y = DrawActionButton(window, "Test Play Sound", "play_audio_test", panelX, y, C_BG_ELEVATED, C_BORDER_LIGHT);
        y = DrawActionButton(window, "Remove Audio Source", "remove_audio", panelX, y, C_DANGER_DIM, C_DANGER);
        y += 8.f;
    } else if (target->entity != 0)
    {
        y = DrawActionButton(window, "+ Audio Source", "add_audio", panelX, y, C_BG_ELEVATED, C_BORDER_LIGHT);
        y += 8.f;
    }

    if (target->entity != 0 && (m_Registry.HasComponent<ParticleEmitterComponent>(target->entity) || target->objectType
                                == ObjectType::ParticleEmitter))
    {
        y = DrawSectionHeader(window, "PARTICLE EMITTER", sf::Color(255, 120, 190), panelX, y);
        y = DrawCheckboxRow(window, "Emitting", target->particleEmitting, "toggle_particle_emitting", panelX, y);

        std::string rateDisplay = (m_ActiveField == EditField::ParticleRate && !m_ActiveInputText.empty())
                                      ? m_ActiveInputText + "|"
                                      : (m_ActiveField == EditField::ParticleRate
                                             ? "|"
                                             : FormatFloat(target->particleRate, 1));
        y = DrawEditableRow(window, "Emission Rate", rateDisplay, "edit_particle_rate", panelX, y);

        std::string lifeDisplay = (m_ActiveField == EditField::ParticleLifetime && !m_ActiveInputText.empty())
                                      ? m_ActiveInputText + "|"
                                      : (m_ActiveField == EditField::ParticleLifetime
                                             ? "|"
                                             : FormatFloat(target->particleLifetime, 2));
        y = DrawEditableRow(window, "Lifetime (s)", lifeDisplay, "edit_particle_lifetime", panelX, y);

        std::string spdDisplay = (m_ActiveField == EditField::ParticleSpeed && !m_ActiveInputText.empty())
                                     ? m_ActiveInputText + "|"
                                     : (m_ActiveField == EditField::ParticleSpeed
                                            ? "|"
                                            : FormatFloat(target->particleSpeed, 1));
        y = DrawEditableRow(window, "Speed", spdDisplay, "edit_particle_speed", panelX, y);

        std::string angDisplay = (m_ActiveField == EditField::ParticleAngle && !m_ActiveInputText.empty())
                                     ? m_ActiveInputText + "|"
                                     : (m_ActiveField == EditField::ParticleAngle
                                            ? "|"
                                            : FormatFloat(target->particleAngle, 1));
        y = DrawEditableRow(window, "Angle (deg)", angDisplay, "edit_particle_angle", panelX, y);

        std::string sprdDisplay = (m_ActiveField == EditField::ParticleSpread && !m_ActiveInputText.empty())
                                      ? m_ActiveInputText + "|"
                                      : (m_ActiveField == EditField::ParticleSpread
                                             ? "|"
                                             : FormatFloat(target->particleSpread, 1));
        y = DrawEditableRow(window, "Spread (deg)", sprdDisplay, "edit_particle_spread", panelX, y);

        std::string sz1Display = (m_ActiveField == EditField::ParticleStartSize && !m_ActiveInputText.empty())
                                     ? m_ActiveInputText + "|"
                                     : (m_ActiveField == EditField::ParticleStartSize
                                            ? "|"
                                            : FormatFloat(target->particleStartSize, 1));
        y = DrawEditableRow(window, "Start Size", sz1Display, "edit_particle_startsize", panelX, y);

        std::string sz2Display = (m_ActiveField == EditField::ParticleEndSize && !m_ActiveInputText.empty())
                                     ? m_ActiveInputText + "|"
                                     : (m_ActiveField == EditField::ParticleEndSize
                                            ? "|"
                                            : FormatFloat(target->particleEndSize, 1));
        y = DrawEditableRow(window, "End Size", sz2Display, "edit_particle_endsize", panelX, y);

        std::string gxDisplay = (m_ActiveField == EditField::ParticleGravityX && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::ParticleGravityX
                                           ? "|"
                                           : FormatFloat(target->particleGravityX, 1));
        y = DrawEditableRow(window, "Gravity X", gxDisplay, "edit_particle_gravx", panelX, y);

        std::string gyDisplay = (m_ActiveField == EditField::ParticleGravityY && !m_ActiveInputText.empty())
                                    ? m_ActiveInputText + "|"
                                    : (m_ActiveField == EditField::ParticleGravityY
                                           ? "|"
                                           : FormatFloat(target->particleGravityY, 1));
        y = DrawEditableRow(window, "Gravity Y", gyDisplay, "edit_particle_gravy", panelX, y);

        float rowY1 = y;
        sf::Text lbl1;
        lbl1.setFont(*m_Font);
        lbl1.setCharacterSize(11);
        lbl1.setFillColor(C_TEXT_MUTED);
        lbl1.setString("Start Color");
        lbl1.setPosition(panelX + InspectorPad, rowY1 + 4.f);
        window.draw(lbl1);
        sf::RectangleShape swatch1({InspectorWidth - InspectorPad * 2.f, 18.f});
        swatch1.setFillColor(target->particleStartColor);
        swatch1.setPosition(panelX + InspectorPad, rowY1 + 20.f);
        window.draw(swatch1);
        y = DrawActionButton(window, "Pick Start Color", "pick_part_start_color", panelX, rowY1 + 40.f, C_BG_ELEVATED,
                             C_BORDER_LIGHT);

        float rowY2 = y;
        sf::Text lbl2;
        lbl2.setFont(*m_Font);
        lbl2.setCharacterSize(11);
        lbl2.setFillColor(C_TEXT_MUTED);
        lbl2.setString("End Color");
        lbl2.setPosition(panelX + InspectorPad, rowY2 + 4.f);
        window.draw(lbl2);
        sf::RectangleShape swatch2({InspectorWidth - InspectorPad * 2.f, 18.f});
        swatch2.setFillColor(target->particleEndColor);
        swatch2.setPosition(panelX + InspectorPad, rowY2 + 20.f);
        window.draw(swatch2);
        y = DrawActionButton(window, "Pick End Color", "pick_part_end_color", panelX, rowY2 + 40.f, C_BG_ELEVATED,
                             C_BORDER_LIGHT);

        y += 4.f;
        y = DrawActionButton(window, "Remove Emitter", "remove_particle", panelX, y, C_DANGER_DIM, C_DANGER);
        y += 8.f;
    } else if (target->entity != 0)
    {
        y = DrawActionButton(window, "+ Particle Emitter", "add_particle", panelX, y, C_BG_ELEVATED, C_BORDER_LIGHT);
        y += 8.f;
    }

    if (target->entity != 0 && m_Registry.HasComponent<ScriptComponent>(target->entity))
    {
        auto &sc = m_Registry.GetComponent<ScriptComponent>(target->entity);
        if (sc.ReloadIfNeeded())
        {
            auto freshProps = sc.GetExportedProperties();
            target->scriptProperties.clear();
            for (const auto &prop: freshProps) { target->scriptProperties[prop.name] = prop; }
            if (m_ActiveField == EditField::ScriptProperty)
            {
                m_ActiveField = EditField::None;
                m_ActiveInputText.clear();
            }
        } else
        {
            for (const auto &prop: sc.GetExportedProperties())
            {
                if (target->scriptProperties.find(prop.name) == target->scriptProperties.end())
                {
                    target->scriptProperties[prop.name] = prop;
                }
            }
        }

        y = DrawSectionHeader(window, m_HasUnsavedChanges ? "SCRIPT *" : "SCRIPT",
                              m_HasUnsavedChanges ? C_WARNING : C_TEXT_SECONDARY, panelX, y);
        std::string scriptName = target->scriptPath;
        const size_t slash = scriptName.find_last_of("/\\");
        if (slash != std::string::npos) scriptName = scriptName.substr(slash + 1);
        if (m_HasUnsavedChanges) scriptName += " *";
        y = DrawRow(window, "File", scriptName, panelX, y);
        y = DrawRow(window, "OnCreate", "bound", panelX, y);
        y = DrawRow(window, "OnUpdate", "bound", panelX, y);
        y = DrawRow(window, "OnInputReceived", "bound", panelX, y);
        y += 8.f;
        if (!target->scriptProperties.empty())
        {
            y = DrawSectionHeader(window, "EXPORTED VARIABLES", C_TEXT_SECONDARY, panelX, y);
            for (auto &pair: target->scriptProperties)
            {
                const auto &prop = pair.second;
                if (prop.type == ScriptComponent::PropertyType::Bool)
                {
                    y = DrawCheckboxRow(window, prop.name, prop.boolVal, "toggle_script_bool_" + prop.name, panelX, y);
                } else
                {
                    std::string valStr;
                    if (prop.type == ScriptComponent::PropertyType::Int) valStr = std::to_string(prop.intVal);
                    else if (prop.type == ScriptComponent::PropertyType::Float)
                    {
                        char buf[32];
                        snprintf(buf, sizeof(buf), "%.2f", prop.floatVal);
                        valStr = buf;
                    } else if (prop.type == ScriptComponent::PropertyType::String) valStr = prop.stringVal;
                    else if (prop.type == ScriptComponent::PropertyType::Entity) valStr = prop.stringVal.empty()
                        ? "(drag entity)"
                        : prop.stringVal;
                    else if (prop.type == ScriptComponent::PropertyType::Image)
                    {
                        std::string valStr = prop.stringVal.empty()
                                                 ? "(drag image or click)"
                                                 : std::filesystem::path(prop.stringVal).filename().string();
                        if (m_ActiveField == EditField::ScriptProperty && m_ActiveScriptProperty == prop.name)
                            y = DrawEditableRow(window, prop.name, m_ActiveInputText + "_",
                                                "edit_script_prop_" + prop.name, panelX, y);
                        else
                            y = DrawEditableRow(window, prop.name, valStr, "edit_script_prop_" + prop.name, panelX, y);

                        y = DrawImagePreview(window, prop.stringVal, panelX + InspectorPad, y,
                                             InspectorWidth - InspectorPad * 2.f, 52.f,
                                             "edit_script_prop_" + prop.name);
                        continue;
                    } else if (prop.type == ScriptComponent::PropertyType::Template)
                    {
                        std::string valStr = prop.stringVal.empty()
                                                 ? "(drag .template or click)"
                                                 : std::filesystem::path(prop.stringVal).filename().string();
                        if (m_ActiveField == EditField::ScriptProperty && m_ActiveScriptProperty == prop.name)
                            y = DrawEditableRow(window, prop.name, m_ActiveInputText + "_",
                                                "edit_script_prop_" + prop.name, panelX, y);
                        else
                            y = DrawEditableRow(window, prop.name, valStr, "edit_script_prop_" + prop.name, panelX, y);

                        y = DrawTemplatePreview(window, prop.stringVal, panelX + InspectorPad, y,
                                                InspectorWidth - InspectorPad * 2.f, 52.f,
                                                "edit_script_prop_" + prop.name);
                        continue;
                    } else if (prop.type == ScriptComponent::PropertyType::Vec2)
                    {
                        std::string xPropKey = prop.name + "_x";
                        std::string yPropKey = prop.name + "_y";
                        std::string xValStr = (m_ActiveField == EditField::ScriptProperty && m_ActiveScriptProperty ==
                                               xPropKey)
                                                  ? m_ActiveInputText + "_"
                                                  : [&] {
                                                      char b[32];
                                                      snprintf(b, 32, "%.2f", prop.floatVal);
                                                      return std::string(b);
                                                  }();
                        std::string yValStr = (m_ActiveField == EditField::ScriptProperty && m_ActiveScriptProperty ==
                                               yPropKey)
                                                  ? m_ActiveInputText + "_"
                                                  : [&] {
                                                      char b[32];
                                                      snprintf(b, 32, "%.2f", prop.vec2Y);
                                                      return std::string(b);
                                                  }();
                        y = DrawEditableRow(window, prop.name + ".x", xValStr, "edit_script_prop_" + xPropKey, panelX,
                                            y);
                        y = DrawEditableRow(window, prop.name + ".y", yValStr, "edit_script_prop_" + yPropKey, panelX,
                                            y);
                        continue;
                    } else if (prop.type == ScriptComponent::PropertyType::Color)
                    {
                        float rowY = y;
                        sf::Text lbl;
                        lbl.setFont(*m_Font);
                        lbl.setCharacterSize(11);
                        lbl.setFillColor(C_TEXT_MUTED);
                        lbl.setString(prop.name);
                        lbl.setPosition(panelX + InspectorPad, rowY + 4.f);
                        window.draw(lbl);
                        sf::RectangleShape swatch({InspectorWidth - InspectorPad * 2.f, 18.f});
                        swatch.setFillColor(sf::Color(
                            static_cast<sf::Uint8>(prop.colorR),
                            static_cast<sf::Uint8>(prop.colorG),
                            static_cast<sf::Uint8>(prop.colorB)));
                        swatch.setPosition(panelX + InspectorPad, rowY + 20.f);
                        window.draw(swatch);
                        y = DrawActionButton(window, "Pick Color", "pick_color_prop_" + prop.name, panelX,
                                             rowY + 20.f + 20.f, C_BG_ELEVATED, C_BORDER_LIGHT);
                        continue;
                    }

                    if (m_ActiveField == EditField::ScriptProperty && m_ActiveScriptProperty == prop.name)
                        y = DrawEditableRow(window, prop.name, m_ActiveInputText + "_", "edit_script_prop_" + prop.name,
                                            panelX, y);
                    else
                        y = DrawEditableRow(window, prop.name, valStr, "edit_script_prop_" + prop.name, panelX, y);
                }
            }
        }
        y += 4.f;
        y = DrawActionButton(window, "Open Script", "open_script", panelX, y, C_BG_ELEVATED, C_BORDER_LIGHT);
        y = DrawActionButton(window, "Remove Script", "remove_script", panelX, y, C_DANGER_DIM, C_DANGER);
    } else if (target->entity != 0)
    {
        y = DrawActionButton(window, "+ Script", "add_script", panelX, y, C_BG_ELEVATED, C_BORDER_LIGHT);
        if (m_ActiveField == EditField::Script)
            DrawScriptInput(window, panelX, y);
    }
    m_InspectorContentHeight = (y - contentStartY) + m_InspectorScrollY;
    window.setView(origView);
    DrawInspectorHeader(window, panelX, panelY);
}


float EditorScene::DrawSectionHeader(sf::RenderWindow &window, const std::string &title,
                                     sf::Color accent, float x, float y)
{
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
    text.setPosition(x + InspectorPad + 2.f, y + 4.f);
    window.draw(text);

    return y + 20.f;
}


float EditorScene::DrawRow(sf::RenderWindow &window, const std::string &key,
                           const std::string &val, float x, float y)
{
    sf::Text keyText;
    keyText.setFont(*m_Font);
    keyText.setCharacterSize(12);
    keyText.setFillColor(C_TEXT_SECONDARY);
    keyText.setString(key);

    const float labelWidth = keyText.getLocalBounds().width;
    const bool wrapField = (labelWidth + InspectorPad + 12.f > InspectorWidth * 0.48f);
    const float rowH = wrapField ? 38.f : 20.f;

    const sf::FloatRect rowRect(x, y, InspectorWidth, rowH);
    if (rowRect.contains(m_MouseScreenPos))
    {
        std::string tip = GetInspectorTooltip(key);
        if (!tip.empty()) m_ActiveTooltip = tip;
    }

    if (wrapField) { keyText.setPosition(x + InspectorPad + 4.f, y + 2.f); } else
    {
        keyText.setPosition(x + InspectorPad + 4.f, y + 2.f);
    }
    window.draw(keyText);

    sf::Text valText;
    valText.setFont(*m_Font);
    valText.setCharacterSize(12);
    valText.setFillColor(C_TEXT_PRIMARY);
    valText.setString(val);

    if (wrapField) { valText.setPosition(x + InspectorPad + 4.f, y + 18.f); } else
    {
        valText.setPosition(x + InspectorWidth * 0.48f, y + 2.f);
    }
    window.draw(valText);

    sf::RectangleShape line({InspectorWidth - InspectorPad * 2, 1.f});
    line.setFillColor(sf::Color(C_BORDER.r, C_BORDER.g, C_BORDER.b, 80));
    line.setPosition(x + InspectorPad, y + rowH - 2.f);
    window.draw(line);

    return y + rowH;
}


float EditorScene::DrawEditableRow(sf::RenderWindow &window, const std::string &key, const std::string &val,
                                   const std::string &action, float x, float y)
{
    sf::Text keyText;
    keyText.setFont(*m_Font);
    keyText.setCharacterSize(12);
    keyText.setFillColor(C_TEXT_SECONDARY);
    keyText.setString(key);

    const float labelWidth = keyText.getLocalBounds().width;
    const bool wrapField = (labelWidth + InspectorPad + 12.f > InspectorWidth * 0.44f);
    const float rowH = wrapField ? 44.f : 24.f;

    const sf::FloatRect rowRect(x, y, InspectorWidth, rowH);
    if (rowRect.contains(m_MouseScreenPos))
    {
        std::string tip = GetInspectorTooltip(key);
        if (!tip.empty()) m_ActiveTooltip = tip;
    }

    float valX, valY, valW;
    if (wrapField)
    {
        keyText.setPosition(x + InspectorPad + 4.f, y + 2.f);
        valX = x + InspectorPad + 4.f;
        valY = y + 18.f;
        valW = InspectorWidth - (InspectorPad + 4.f) * 2.f;
    } else
    {
        keyText.setPosition(x + InspectorPad + 4.f, y + 3.f);
        valX = x + InspectorWidth * 0.44f;
        valY = y;
        valW = InspectorWidth - InspectorWidth * 0.44f - InspectorPad;
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
            int sMin = std::clamp(GetSelectionMin(), 0, (int) m_ActiveInputText.size());
            int sMax = std::clamp(GetSelectionMax(), 0, (int) m_ActiveInputText.size());
            float x1 = t.findCharacterPos(sMin).x;
            float x2 = t.findCharacterPos(sMax).x;
            sf::RectangleShape selBox({x2 - x1, 14.f});
            selBox.setPosition(valX + 5.f + x1, valY + 3.f);
            selBox.setFillColor(sf::Color(60, 120, 240, 140));
            window.draw(selBox);
        }
    }

    std::string displayVal = val;
    if (!displayVal.empty() && displayVal.back() == '|')
    {
        const bool blink = ((int) (m_FPSClock.getElapsedTime().asSeconds() * 2) % 2 == 0);
        if (!blink)
            displayVal.pop_back();
    }

    sf::Text valText;
    valText.setFont(*m_Font);
    valText.setCharacterSize(12);
    valText.setFillColor(hovered ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
    valText.setString(displayVal);
    valText.setPosition(valX + 5.f, valY + 3.f);
    window.draw(valText);

    m_InspectorButtons.push_back({fieldRect, action});

    sf::RectangleShape line({InspectorWidth - InspectorPad * 2, 1.f});
    line.setFillColor(sf::Color(C_BORDER.r, C_BORDER.g, C_BORDER.b, 60));
    line.setPosition(x + InspectorPad, y + rowH - 1.f);
    window.draw(line);

    return y + rowH;
}


float EditorScene::DrawCheckboxRow(sf::RenderWindow &window, const std::string &key, bool value,
                                   const std::string &action, float x, float y)
{
    const sf::FloatRect rowRect(x, y, InspectorWidth, 22.f);
    if (rowRect.contains(m_MouseScreenPos))
    {
        std::string tip = GetInspectorTooltip(key);
        if (!tip.empty()) m_ActiveTooltip = tip;
    }

    sf::Text keyText;
    keyText.setFont(*m_Font);
    keyText.setCharacterSize(12);
    keyText.setFillColor(C_TEXT_SECONDARY);
    keyText.setString(key);
    keyText.setPosition(x + InspectorPad + 4.f, y + 3.f);
    window.draw(keyText);

    const float boxSize = 18.f;
    const float boxX = x + InspectorWidth * 0.44f;
    const float boxY = y + 1.f;
    const sf::FloatRect boxRect(boxX, boxY, boxSize, boxSize);
    const bool hovered = boxRect.contains(m_MouseScreenPos);

    sf::RectangleShape box({boxSize, boxSize});
    box.setPosition(boxX, boxY);
    if (value)
    {
        box.setFillColor(hovered ? C_ACCENT_HOV : C_ACCENT);
        box.setOutlineColor(hovered ? sf::Color(140, 240, 160) : sf::Color(80, 200, 120));
    } else
    {
        box.setFillColor(hovered ? C_BG_ELEVATED : C_BG_INPUT);
        box.setOutlineColor(hovered ? C_ACCENT : C_BORDER);
    }
    box.setOutlineThickness(1.f);
    window.draw(box);

    if (value)
    {
        sf::RectangleShape stem1({6.f, 2.5f});
        stem1.setOrigin(0.f, 1.25f);
        stem1.setPosition(boxX + 3.5f, boxY + 9.5f);
        stem1.setRotation(45.f);
        stem1.setFillColor(sf::Color::White);
        window.draw(stem1);

        sf::RectangleShape stem2({10.5f, 2.5f});
        stem2.setOrigin(0.f, 1.25f);
        stem2.setPosition(boxX + 7.f, boxY + 13.5f);
        stem2.setRotation(-52.f);
        stem2.setFillColor(sf::Color::White);
        window.draw(stem2);
    }

    sf::Text stateText;
    stateText.setFont(*m_Font);
    stateText.setCharacterSize(11);
    stateText.setFillColor(value ? sf::Color(100, 220, 140) : C_TEXT_MUTED);
    stateText.setString(value ? "true" : "false");
    stateText.setPosition(boxX + boxSize + 8.f, y + 3.f);
    window.draw(stateText);

    const sf::FloatRect clickRect(boxX, y, boxSize + 48.f, 20.f);
    m_InspectorButtons.push_back({clickRect, action});

    sf::RectangleShape line({InspectorWidth - InspectorPad * 2, 1.f});
    line.setFillColor(sf::Color(C_BORDER.r, C_BORDER.g, C_BORDER.b, 60));
    line.setPosition(x + InspectorPad, y + 21.f);
    window.draw(line);

    return y + 22.f;
}


float EditorScene::DrawAddButton(sf::RenderWindow &window, const std::string &label,
                                 const std::string &action, float x, float y)
{
    return DrawActionButton(window, label, action, x, y, C_BG_ELEVATED, C_BORDER_LIGHT);
}


float EditorScene::DrawRemoveButton(sf::RenderWindow &window, const std::string &label,
                                    const std::string &action, float x, float y)
{
    return DrawActionButton(window, label, action, x, y, C_DANGER_DIM, C_DANGER);
}


float EditorScene::DrawActionButton(sf::RenderWindow &window, const std::string &label,
                                    const std::string &action, float x, float y,
                                    sf::Color fillColor, sf::Color borderColor)
{
    const sf::FloatRect btnRect(x + InspectorPad, y + 2.f, InspectorWidth - InspectorPad * 2, 24.f);
    const bool hovered = btnRect.contains(m_MouseScreenPos);

    if (hovered)
    {
        if (label.find("Add") != std::string::npos) m_ActiveTooltip = "Add new component to this object";
        else if (label.find("Remove") != std::string::npos) m_ActiveTooltip = "Remove component from this object";
        else if (label == "Open Script") m_ActiveTooltip = "Open script in code editor";
    }

    sf::Color fill = hovered
                         ? sf::Color(std::min(255, fillColor.r + 16),
                                     std::min(255, fillColor.g + 16),
                                     std::min(255, fillColor.b + 16), 255)
                         : fillColor;
    sf::Color border = hovered
                           ? sf::Color(std::min(255, borderColor.r + 30),
                                       std::min(255, borderColor.g + 30),
                                       std::min(255, borderColor.b + 30), 255)
                           : borderColor;

    DrawPill(window, btnRect, fill, border);

    sf::Text text;
    text.setFont(*m_Font);
    text.setCharacterSize(11);
    text.setFillColor(hovered ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
    text.setString(label);
    text.setPosition(btnRect.left + (btnRect.width - text.getLocalBounds().width) / 2.f,
                     btnRect.top + 6.f);
    window.draw(text);

    m_InspectorButtons.push_back({btnRect, action});

    return y + 30.f;
}


float EditorScene::DrawImagePreview(sf::RenderWindow &window, const std::string &path, float x, float y,
                                    float w, float h, const std::string &action)
{
    const float previewW = (w > 0.f) ? w : (InspectorWidth - InspectorPad * 2.f);
    const float previewH = h;
    const sf::FloatRect boxRect(x, y, previewW, previewH);
    const bool hovered = boxRect.contains(m_MouseScreenPos);

    if (!action.empty()) { m_InspectorButtons.push_back({boxRect, action}); }
    sf::RectangleShape card({previewW, previewH});
    card.setPosition(x, y);
    card.setFillColor(hovered ? sf::Color(32, 38, 48) : C_BG_INPUT);
    card.setOutlineColor(hovered ? C_ACCENT : C_BORDER);
    card.setOutlineThickness(1.f);
    window.draw(card);

    std::filesystem::path p(path);
    std::shared_ptr<sf::Texture> tex = nullptr;
    if (!path.empty())
    {
        tex = ResourceManager::Get().GetTexture(path);
        if (!tex)
        {
            std::filesystem::path rootDir = m_ContentBrowser
                                                ? std::filesystem::path(m_ContentBrowser->GetRootPath())
                                                : (FindProjectRoot() / "assets");
            std::filesystem::path fullP = p.is_absolute() ? p : (rootDir.parent_path() / p);
            tex = ResourceManager::Get().GetTexture(fullP.string());
            if (!tex)
            {
                fullP = rootDir / p;
                tex = ResourceManager::Get().GetTexture(fullP.string());
            }
        }
    }

    const float thumbSize = previewH - 10.f;
    const float thumbX = x + 5.f;
    const float thumbY = y + 5.f;
    sf::RectangleShape thumbBg({thumbSize, thumbSize});
    thumbBg.setPosition(thumbX, thumbY);
    thumbBg.setFillColor(sf::Color(16, 18, 22));
    thumbBg.setOutlineColor(C_BORDER_LIGHT);
    thumbBg.setOutlineThickness(1.f);
    window.draw(thumbBg);

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

        float textX = thumbX + thumbSize + 8.f;
        std::string filename = p.filename().string();
        if (filename.length() > 20) filename = filename.substr(0, 17) + "...";
        sf::Text nameText;
        nameText.setFont(*m_Font);
        nameText.setCharacterSize(11);
        nameText.setFillColor(C_TEXT_PRIMARY);
        nameText.setStyle(sf::Text::Bold);
        nameText.setString(filename);
        nameText.setPosition(textX, y + 6.f);
        window.draw(nameText);
        sf::Text resText;
        resText.setFont(*m_Font);
        resText.setCharacterSize(10);
        resText.setFillColor(C_TEXT_SECONDARY);
        resText.setString(std::to_string(ts.x) + " x " + std::to_string(ts.y) + " px");
        resText.setPosition(textX, y + 21.f);
        window.draw(resText);
        sf::Text tagText;
        tagText.setFont(*m_Font);
        tagText.setCharacterSize(9);
        tagText.setFillColor(sf::Color(180, 150, 220));
        tagText.setString("Image Asset");
        tagText.setPosition(textX, y + 36.f);
        window.draw(tagText);
    } else
    {
        sf::Text emptyIcon;
        emptyIcon.setFont(*m_Font);
        emptyIcon.setCharacterSize(13);
        emptyIcon.setFillColor(C_TEXT_MUTED);
        emptyIcon.setString("IMG");
        emptyIcon.setPosition(thumbX + (thumbSize - emptyIcon.getLocalBounds().width) / 2.f, thumbY + 12.f);
        window.draw(emptyIcon);

        float textX = thumbX + thumbSize + 8.f;
        sf::Text msgText;
        msgText.setFont(*m_Font);
        msgText.setCharacterSize(11);
        msgText.setFillColor(path.empty() ? C_TEXT_MUTED : C_DANGER);
        msgText.setString(path.empty() ? "(Drag image here)" : "Image not found");
        msgText.setPosition(textX, y + 10.f);
        window.draw(msgText);

        if (!path.empty())
        {
            std::string filename = p.filename().string();
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


float EditorScene::DrawScriptInput(sf::RenderWindow &window, float x, float y)
{
    const sf::FloatRect inputRect(x + InspectorPad, y + 2.f, InspectorWidth - InspectorPad * 2, 24.f);

    sf::RectangleShape bg({inputRect.width, inputRect.height});
    bg.setFillColor(C_BG_INPUT);
    bg.setOutlineColor(C_ACCENT);
    bg.setOutlineThickness(1.f);
    bg.setPosition(inputRect.left, inputRect.top);
    window.draw(bg);

    std::string display = (m_ActiveField == EditField::Script && !m_ActiveInputText.empty())
                              ? m_ActiveInputText
                              : (m_ActiveField != EditField::Script)
                                    ? "name"
                                    : "";
    sf::Color textColor = display == "name" ? C_TEXT_MUTED : C_TEXT_PRIMARY;

    sf::Text inputText;
    inputText.setFont(*m_Font);
    inputText.setCharacterSize(11);
    inputText.setFillColor(textColor);
    const bool blink = ((int) (m_FPSClock.getElapsedTime().asSeconds() * 2) % 2 == 0);
    inputText.setString(display + (m_ActiveField == EditField::Script && blink ? "|" : ""));
    inputText.setPosition(inputRect.left + 6.f, inputRect.top + 5.f);
    window.draw(inputText);

    m_ActiveInputBounds = inputRect;
    m_InspectorButtons.push_back({inputRect, "edit_script"});

    sf::Text hint;
    hint.setFont(*m_Font);
    hint.setCharacterSize(10);
    hint.setFillColor(C_TEXT_MUTED);
    hint.setString("Enter to confirm");
    hint.setPosition(x + InspectorPad, y + 30.f);
    window.draw(hint);

    return y + 44.f;
}


void EditorScene::HandleInspectorClick(sf::Vector2f pos)
{
    m_ActiveField = EditField::None;
    m_InputSelectionStart = -1;
    m_InputSelectionEnd = -1;
    m_IsSelectingText = false;

    EditorObject *target = GetInspectedObject();

    for (auto &btn: m_InspectorButtons)
    {
        if (!btn.bounds.contains(pos)) continue;

        if (btn.action == "toggle_inspector_lock")
        {
            if (m_InspectorLocked)
            {
                m_InspectorLocked = false;
                m_LockedObjectId.clear();
                std::cout << "[INFO] [Inspector] Inspector unlocked\n";
            } else if (m_Selected)
            {
                m_InspectorLocked = true;
                m_LockedObjectId = m_Selected->id;
                std::cout << "[INFO] [Inspector] Inspector locked to " << m_LockedObjectId << "\n";
            }
            return;
        }
        float clipTop = m_InspectorBounds.top + 36.f;
        float clipBot = m_InspectorBounds.top + m_InspectorBounds.height;
        if (btn.bounds.top + btn.bounds.height < clipTop || btn.bounds.top > clipBot) continue;

        if (btn.action == "detach_parent" && target)
        {
            SetParent(target->id, "", true);
            return;
        }

        if (btn.action == "add_velocity" && target)
        {
            m_Registry.AddComponent(target->entity, VelocityComponent{0.f, 0.f});
            std::cout << "[INFO] [Inspector] VelocityComponent added to " << target->id << "\n";
            SetDirty(true);
        } else if (btn.action == "remove_velocity" && target)
        {
            m_Registry.RemoveComponent<VelocityComponent>(target->entity);
            std::cout << "[INFO] [Inspector] VelocityComponent removed from " << target->id << "\n";
            SetDirty(true);
        } else if (btn.action == "edit_vel_dx" && target)
        {
            m_ActiveField = EditField::VelocityDX;
            if (m_Registry.HasComponent<VelocityComponent>(target->entity))
                m_ActiveInputText = FormatFloat(m_Registry.GetComponent<VelocityComponent>(target->entity).dx, 2);
            else
                m_ActiveInputText = "0.00";
        } else if (btn.action == "edit_vel_dy" && target)
        {
            m_ActiveField = EditField::VelocityDY;
            if (m_Registry.HasComponent<VelocityComponent>(target->entity))
                m_ActiveInputText = FormatFloat(m_Registry.GetComponent<VelocityComponent>(target->entity).dy, 2);
            else
                m_ActiveInputText = "0.00";
        } else if (btn.action == "add_camera" && target)
        {
            if (!m_Registry.HasComponent<CameraComponent>(target->entity))
                m_Registry.AddComponent(target->entity, CameraComponent{true});
            std::cout << "[INFO] [Inspector] CameraComponent added to " << target->id << "\n";
            SetDirty(true);
        } else if (btn.action == "cycle_cam_mode" && target)
        {
            if (m_Registry.HasComponent<CameraComponent>(target->entity))
            {
                auto &c = m_Registry.GetComponent<CameraComponent>(target->entity);
                int nextMode = (static_cast<int>(c.multiFollowMode) + 1) % 3;
                c.multiFollowMode = static_cast<CameraMultiFollowMode>(nextMode);
                SetDirty(true);
            }
        } else if (btn.action == "toggle_cam_active" && target)
        {
            if (m_Registry.HasComponent<CameraComponent>(target->entity))
            {
                auto &c = m_Registry.GetComponent<CameraComponent>(target->entity);
                c.active = !c.active;
                SetDirty(true);
            }
        } else if (btn.action == "edit_cam_prio" && target)
        {
            m_ActiveField = EditField::CameraPriority;
            if (m_Registry.HasComponent<CameraComponent>(target->entity))
                m_ActiveInputText = std::to_string(m_Registry.GetComponent<CameraComponent>(target->entity).priority);
            else
                m_ActiveInputText = "0";
        } else if (btn.action == "edit_cam_pad" && target)
        {
            m_ActiveField = EditField::CameraAutoFramePadding;
            if (m_Registry.HasComponent<CameraComponent>(target->entity))
                m_ActiveInputText = FormatFloat(
                    m_Registry.GetComponent<CameraComponent>(target->entity).autoFramePadding, 1);
            else
                m_ActiveInputText = "200.0";
        } else if (btn.action == "edit_cam_minz" && target)
        {
            m_ActiveField = EditField::CameraMinZoom;
            if (m_Registry.HasComponent<CameraComponent>(target->entity))
                m_ActiveInputText = FormatFloat(m_Registry.GetComponent<CameraComponent>(target->entity).minZoom, 2);
            else
                m_ActiveInputText = "0.30";
        } else if (btn.action == "edit_cam_maxz" && target)
        {
            m_ActiveField = EditField::CameraMaxZoom;
            if (m_Registry.HasComponent<CameraComponent>(target->entity))
                m_ActiveInputText = FormatFloat(m_Registry.GetComponent<CameraComponent>(target->entity).maxZoom, 2);
            else
                m_ActiveInputText = "3.00";
        } else if (btn.action == "remove_camera" && target)
        {
            m_Registry.RemoveComponent<CameraComponent>(target->entity);
            std::cout << "[INFO] [Inspector] CameraComponent removed from " << target->id << "\n";
            SetDirty(true);
        } else if (btn.action == "edit_cam_speed" && target)
        {
            m_ActiveField = EditField::CameraSmoothSpeed;
            if (m_Registry.HasComponent<CameraComponent>(target->entity))
                m_ActiveInputText = FormatFloat(m_Registry.GetComponent<CameraComponent>(target->entity).smoothSpeed,
                                                2);
            else
                m_ActiveInputText = "0.00";
        } else if (btn.action == "edit_cam_ox" && target)
        {
            m_ActiveField = EditField::CameraOffsetX;
            if (m_Registry.HasComponent<CameraComponent>(target->entity))
                m_ActiveInputText = FormatFloat(m_Registry.GetComponent<CameraComponent>(target->entity).offsetX, 2);
            else
                m_ActiveInputText = "0.00";
        } else if (btn.action == "edit_cam_oy" && target)
        {
            m_ActiveField = EditField::CameraOffsetY;
            if (m_Registry.HasComponent<CameraComponent>(target->entity))
                m_ActiveInputText = FormatFloat(m_Registry.GetComponent<CameraComponent>(target->entity).offsetY, 2);
            else
                m_ActiveInputText = "0.00";
        } else if (btn.action == "edit_cam_zoom" && target)
        {
            m_ActiveField = EditField::CameraZoom;
            if (m_Registry.HasComponent<CameraComponent>(target->entity))
                m_ActiveInputText = FormatFloat(m_Registry.GetComponent<CameraComponent>(target->entity).zoom, 2);
            else
                m_ActiveInputText = "1.00";
        } else if (btn.action == "add_collision" && target)
        {
            m_Registry.AddComponent(target->entity, CollisionComponent{0});
            std::cout << "[INFO] [Inspector] CollisionComponent added to " << target->id << "\n";
            SetDirty(true);
        } else if (btn.action == "remove_collision" && target)
        {
            m_Registry.RemoveComponent<CollisionComponent>(target->entity);
            std::cout << "[INFO] [Inspector] CollisionComponent removed from " << target->id << "\n";
            SetDirty(true);
        } else if (btn.action == "toggle_collision_type" && target)
        {
            if (m_Registry.HasComponent<CollisionComponent>(target->entity))
            {
                auto &col = m_Registry.GetComponent<CollisionComponent>(target->entity);
                col.type = (col.type == CollisionType::Static) ? CollisionType::Solid : CollisionType::Static;
                SetDirty(true);
            }
        } else if (btn.action == "edit_collision_channel" && target)
        {
            m_ActiveField = EditField::CollisionChannel;
            if (m_Registry.HasComponent<CollisionComponent>(target->entity))
            {
                m_ActiveInputText = std::to_string(
                    m_Registry.GetComponent<CollisionComponent>(target->entity).channel);
            }
        } else if (btn.action == "toggle_collision_trigger" && target)
        {
            if (m_Registry.HasComponent<CollisionComponent>(target->entity))
            {
                auto &col = m_Registry.GetComponent<CollisionComponent>(target->entity);
                col.isTrigger = !col.isTrigger;
                SetDirty(true);
            }
        } else if (btn.action == "toggle_collision_shape" && target)
        {
            if (m_Registry.HasComponent<CollisionComponent>(target->entity))
            {
                auto &col = m_Registry.GetComponent<CollisionComponent>(target->entity);
                col.shape = (col.shape == ColliderShape::Box) ? ColliderShape::Circle : ColliderShape::Box;
                SetDirty(true);
            }
        } else if (btn.action == "add_rigidbody" && target)
        {
            m_Registry.AddComponent(target->entity, Rigidbody2DComponent{});
            if (!m_Registry.HasComponent<VelocityComponent>(target->entity))
                m_Registry.AddComponent(target->entity, VelocityComponent{0.f, 0.f});
            std::cout << "[INFO] [Inspector] Rigidbody2DComponent added to " << target->id << "\n";
            SetDirty(true);
        } else if (btn.action == "remove_rigidbody" && target)
        {
            m_Registry.RemoveComponent<Rigidbody2DComponent>(target->entity);
            std::cout << "[INFO] [Inspector] Rigidbody2DComponent removed from " << target->id << "\n";
            SetDirty(true);
        } else if (btn.action == "toggle_rigidbody_type" && target)
        {
            if (m_Registry.HasComponent<Rigidbody2DComponent>(target->entity))
            {
                auto &rb = m_Registry.GetComponent<Rigidbody2DComponent>(target->entity);
                if (rb.bodyType == BodyType::Dynamic) rb.bodyType = BodyType::Kinematic;
                else if (rb.bodyType == BodyType::Kinematic) rb.bodyType = BodyType::Static;
                else rb.bodyType = BodyType::Dynamic;
                SetDirty(true);
            }
        } else if (btn.action == "toggle_rb_freeze_rot" && target)
        {
            if (m_Registry.HasComponent<Rigidbody2DComponent>(target->entity))
            {
                auto &rb = m_Registry.GetComponent<Rigidbody2DComponent>(target->entity);
                rb.freezeRotation = !rb.freezeRotation;
                SetDirty(true);
            }
        } else if (btn.action == "edit_rb_mass" && target)
        {
            m_ActiveField = EditField::RigidbodyMass;
            if (m_Registry.HasComponent<Rigidbody2DComponent>(target->entity))
                m_ActiveInputText = FormatFloat(m_Registry.GetComponent<Rigidbody2DComponent>(target->entity).mass, 2);
        } else if (btn.action == "edit_rb_gravity" && target)
        {
            m_ActiveField = EditField::RigidbodyGravity;
            if (m_Registry.HasComponent<Rigidbody2DComponent>(target->entity))
                m_ActiveInputText =
                        FormatFloat(m_Registry.GetComponent<Rigidbody2DComponent>(target->entity).gravityScale, 2);
        } else if (btn.action == "edit_rb_restitution" && target)
        {
            m_ActiveField = EditField::RigidbodyRestitution;
            if (m_Registry.HasComponent<Rigidbody2DComponent>(target->entity))
                m_ActiveInputText =
                        FormatFloat(m_Registry.GetComponent<Rigidbody2DComponent>(target->entity).restitution, 2);
        } else if (btn.action == "edit_rb_drag" && target)
        {
            m_ActiveField = EditField::RigidbodyDrag;
            if (m_Registry.HasComponent<Rigidbody2DComponent>(target->entity))
                m_ActiveInputText = FormatFloat(m_Registry.GetComponent<Rigidbody2DComponent>(target->entity).drag, 2);
        } else if (btn.action == "add_text" && target)
        {
            target->textString = "World Text";
            target->textFontSize = 28;
            target->textColor = sf::Color::White;
            TextComponent tc;
            tc.text = target->textString;
            tc.characterSize = target->textFontSize;
            tc.color = target->textColor;
            m_Registry.AddComponent(target->entity, tc);
            SetDirty(true);
        } else if (btn.action == "remove_text" && target)
        {
            m_Registry.RemoveComponent<TextComponent>(target->entity);
            SetDirty(true);
        } else if (btn.action == "edit_text_content" && target)
        {
            m_ActiveField = EditField::TextContent;
            m_ActiveInputText = target->textString;
        } else if (btn.action == "edit_text_fontsize" && target)
        {
            m_ActiveField = EditField::TextFontSize;
            m_ActiveInputText = std::to_string(target->textFontSize);
        } else if (btn.action == "cycle_text_align" && target)
        {
            target->textAlignment = (target->textAlignment + 1) % 3;
            if (target->entity != 0 && m_Registry.HasComponent<TextComponent>(target->entity))
                m_Registry.GetComponent<TextComponent>(target->entity).alignment = target->textAlignment;
            SetDirty(true);
        } else if (btn.action == "pick_text_color" && target)
        {
            HWND hwnd = reinterpret_cast<HWND>(m_Window.getSystemHandle());
            if (OpenColorPickerDialog(target->textColor, hwnd))
            {
                if (target->entity != 0 && m_Registry.HasComponent<TextComponent>(target->entity))
                    m_Registry.GetComponent<TextComponent>(target->entity).color = target->textColor;
                SetDirty(true);
            }
        } else if (btn.action == "add_audio" && target)
        {
            AudioSourceComponent ac;
            m_Registry.AddComponent(target->entity, ac);
            SetDirty(true);
        } else if (btn.action == "remove_audio" && target)
        {
            m_Registry.RemoveComponent<AudioSourceComponent>(target->entity);
            SetDirty(true);
        } else if (btn.action == "edit_audio_path" && target)
        {
            m_ActiveField = EditField::AudioPath;
            m_ActiveInputText = target->audioClipPath;
        } else if (btn.action == "edit_audio_volume" && target)
        {
            m_ActiveField = EditField::AudioVolume;
            m_ActiveInputText = FormatFloat(target->audioVolume, 1);
        } else if (btn.action == "edit_audio_pitch" && target)
        {
            m_ActiveField = EditField::AudioPitch;
            m_ActiveInputText = FormatFloat(target->audioPitch, 2);
        } else if (btn.action == "toggle_audio_loop" && target)
        {
            target->audioLoop = !target->audioLoop;
            if (target->entity != 0 && m_Registry.HasComponent<AudioSourceComponent>(target->entity))
                m_Registry.GetComponent<AudioSourceComponent>(target->entity).loop = target->audioLoop;
            SetDirty(true);
        } else if (btn.action == "toggle_audio_playonstart" && target)
        {
            target->audioPlayOnStart = !target->audioPlayOnStart;
            if (target->entity != 0 && m_Registry.HasComponent<AudioSourceComponent>(target->entity))
                m_Registry.GetComponent<AudioSourceComponent>(target->entity).playOnStart = target->audioPlayOnStart;
            SetDirty(true);
        } else if (btn.action == "toggle_audio_spatial" && target)
        {
            target->audioIsSpatial = !target->audioIsSpatial;
            if (target->entity != 0 && m_Registry.HasComponent<AudioSourceComponent>(target->entity))
                m_Registry.GetComponent<AudioSourceComponent>(target->entity).isSpatial = target->audioIsSpatial;
            SetDirty(true);
        } else if (btn.action == "play_audio_test" && target)
        {
            if (!target->audioClipPath.empty())
                AudioManager::Get().PlaySound(target->audioClipPath, target->audioVolume, target->audioPitch,
                                              target->audioLoop);
        } else if (btn.action == "add_particle" && target)
        {
            ParticleEmitterComponent pec;
            m_Registry.AddComponent(target->entity, pec);
            SetDirty(true);
        } else if (btn.action == "remove_particle" && target)
        {
            m_Registry.RemoveComponent<ParticleEmitterComponent>(target->entity);
            target->editorParticles.clear();
            SetDirty(true);
        } else if (btn.action == "toggle_particle_emitting" && target)
        {
            target->particleEmitting = !target->particleEmitting;
            if (target->entity != 0 && m_Registry.HasComponent<ParticleEmitterComponent>(target->entity))
                m_Registry.GetComponent<ParticleEmitterComponent>(target->entity).emitting = target->particleEmitting;
            SetDirty(true);
        } else if (btn.action == "edit_particle_rate" && target)
        {
            m_ActiveField = EditField::ParticleRate;
            m_ActiveInputText = FormatFloat(target->particleRate, 1);
        } else if (btn.action == "edit_particle_lifetime" && target)
        {
            m_ActiveField = EditField::ParticleLifetime;
            m_ActiveInputText = FormatFloat(target->particleLifetime, 2);
        } else if (btn.action == "edit_particle_speed" && target)
        {
            m_ActiveField = EditField::ParticleSpeed;
            m_ActiveInputText = FormatFloat(target->particleSpeed, 1);
        } else if (btn.action == "edit_particle_angle" && target)
        {
            m_ActiveField = EditField::ParticleAngle;
            m_ActiveInputText = FormatFloat(target->particleAngle, 1);
        } else if (btn.action == "edit_particle_spread" && target)
        {
            m_ActiveField = EditField::ParticleSpread;
            m_ActiveInputText = FormatFloat(target->particleSpread, 1);
        } else if (btn.action == "edit_particle_startsize" && target)
        {
            m_ActiveField = EditField::ParticleStartSize;
            m_ActiveInputText = FormatFloat(target->particleStartSize, 1);
        } else if (btn.action == "edit_particle_endsize" && target)
        {
            m_ActiveField = EditField::ParticleEndSize;
            m_ActiveInputText = FormatFloat(target->particleEndSize, 1);
        } else if (btn.action == "edit_particle_gravx" && target)
        {
            m_ActiveField = EditField::ParticleGravityX;
            m_ActiveInputText = FormatFloat(target->particleGravityX, 1);
        } else if (btn.action == "edit_particle_gravy" && target)
        {
            m_ActiveField = EditField::ParticleGravityY;
            m_ActiveInputText = FormatFloat(target->particleGravityY, 1);
        } else if (btn.action == "pick_part_start_color" && target)
        {
            HWND hwnd = reinterpret_cast<HWND>(m_Window.getSystemHandle());
            if (OpenColorPickerDialog(target->particleStartColor, hwnd))
            {
                if (target->entity != 0 && m_Registry.HasComponent<ParticleEmitterComponent>(target->entity))
                    m_Registry.GetComponent<ParticleEmitterComponent>(target->entity).startColor = target->
                            particleStartColor;
                SetDirty(true);
            }
        } else if (btn.action == "pick_part_end_color" && target)
        {
            HWND hwnd = reinterpret_cast<HWND>(m_Window.getSystemHandle());
            if (OpenColorPickerDialog(target->particleEndColor, hwnd))
            {
                if (target->entity != 0 && m_Registry.HasComponent<ParticleEmitterComponent>(target->entity))
                    m_Registry.GetComponent<ParticleEmitterComponent>(target->entity).endColor = target->
                            particleEndColor;
                SetDirty(true);
            }
        } else if (btn.action == "add_script")
        {
            m_ActiveField = EditField::Script;
            m_ActiveInputText = "";
        } else if (btn.action.find("toggle_script_bool_") == 0 && target)
        {
            m_ActiveField = EditField::None;
            std::string propName = btn.action.substr(19);
            auto it = target->scriptProperties.find(propName);
            if (it != target->scriptProperties.end())
            {
                it->second.boolVal = !it->second.boolVal;
                if (target->entity != 0 && m_Registry.HasComponent<ScriptComponent>(target->entity))
                {
                    m_Registry.GetComponent<ScriptComponent>(target->entity).SetExportedProperty(it->second);
                }
                SetDirty(true);
            }
        } else if (btn.action.find("edit_script_prop_") == 0 && target)
        {
            m_ActiveField = EditField::ScriptProperty;
            m_ActiveScriptProperty = btn.action.substr(17);
            bool isVec2Sub = false;
            std::string vec2BaseName;
            bool isXComp = false;
            if (m_ActiveScriptProperty.size() > 2)
            {
                std::string suffix = m_ActiveScriptProperty.substr(m_ActiveScriptProperty.size() - 2);
                if (suffix == "_x" || suffix == "_y")
                {
                    vec2BaseName = m_ActiveScriptProperty.substr(0, m_ActiveScriptProperty.size() - 2);
                    auto baseIt = target->scriptProperties.find(vec2BaseName);
                    if (baseIt != target->scriptProperties.end() &&
                        baseIt->second.type == ScriptComponent::PropertyType::Vec2)
                    {
                        isVec2Sub = true;
                        isXComp = (suffix == "_x");
                        char buf[32];
                        snprintf(buf, sizeof(buf), "%.2f", isXComp ? baseIt->second.floatVal : baseIt->second.vec2Y);
                        m_ActiveInputText = buf;
                    }
                }
            }

            if (!isVec2Sub)
            {
                auto it = target->scriptProperties.find(m_ActiveScriptProperty);
                if (it != target->scriptProperties.end())
                {
                    const auto &prop = it->second;
                    if (prop.type == ScriptComponent::PropertyType::Int)
                        m_ActiveInputText = std::to_string(prop.intVal);
                    else if (prop.type == ScriptComponent::PropertyType::Float)
                    {
                        char buf[32];
                        snprintf(buf, sizeof(buf), "%.2f", prop.floatVal);
                        m_ActiveInputText = buf;
                    } else if (prop.type == ScriptComponent::PropertyType::Bool)
                        m_ActiveInputText = prop.boolVal ? "true" : "false";
                    else if (prop.type == ScriptComponent::PropertyType::String ||
                             prop.type == ScriptComponent::PropertyType::Template ||
                             prop.type == ScriptComponent::PropertyType::Image ||
                             prop.type == ScriptComponent::PropertyType::Entity)
                        m_ActiveInputText = prop.stringVal;
                }
            }
        } else if (btn.action.find("pick_color_prop_") == 0 && target)
        {
            std::string propName = btn.action.substr(16);
            auto it = target->scriptProperties.find(propName);
            if (it != target->scriptProperties.end() &&
                it->second.type == ScriptComponent::PropertyType::Color)
            {
#ifdef _WIN32
                HWND hwnd = reinterpret_cast<HWND>(m_Window.getSystemHandle());
                static COLORREF customColors[16] = {};
                CHOOSECOLOR cc = {};
                cc.lStructSize = sizeof(cc);
                cc.hwndOwner = hwnd;
                cc.lpCustColors = customColors;
                cc.rgbResult = RGB(it->second.colorR, it->second.colorG, it->second.colorB);
                cc.Flags = CC_FULLOPEN | CC_RGBINIT;
                if (ChooseColor(&cc))
                {
                    it->second.colorR = GetRValue(cc.rgbResult);
                    it->second.colorG = GetGValue(cc.rgbResult);
                    it->second.colorB = GetBValue(cc.rgbResult);
                    if (target->entity != 0 && m_Registry.HasComponent<ScriptComponent>(target->entity))
                        m_Registry.GetComponent<ScriptComponent>(target->entity).SetExportedProperty(it->second);
                    SetDirty(true);
                }
#endif
            }
        } else if (btn.action == "edit_script")
        {
            m_ActiveField = EditField::Script;
            m_ActiveInputText = "";
        } else if (btn.action == "remove_script" && target)
        {
            m_Registry.RemoveComponent<ScriptComponent>(target->entity);
            target->scriptPath = "";
            target->scriptProperties.clear();
            std::cout << "[INFO] [Inspector] ScriptComponent removed from " << target->id << "\n";
            SetDirty(true);
        } else if (btn.action == "edit_name" && target)
        {
            m_ActiveField = EditField::Name;
            m_ActiveInputText = target->id;
        } else if (btn.action == "edit_tag" && target)
        {
            m_ActiveField = EditField::Tag;
            m_ActiveInputText = target->tag;
        } else if (btn.action == "edit_x" && target)
        {
            CommitActiveField();
            m_ActiveField = EditField::TransformX;
            m_ActiveInputText = std::to_string((int) target->localPosition.x);
        } else if (btn.action == "edit_y" && target)
        {
            CommitActiveField();
            m_ActiveField = EditField::TransformY;
            m_ActiveInputText = std::to_string((int) target->localPosition.y);
        } else if (btn.action == "edit_rot" && target)
        {
            m_ActiveField = EditField::Rotation;
            m_ActiveInputText = FormatFloat(target->rotation, 2);
        } else if (btn.action == "edit_scalex" && target)
        {
            m_ActiveField = EditField::ScaleX;
            m_ActiveInputText = FormatFloat(target->scaleX, 2);
        } else if (btn.action == "edit_scaley" && target)
        {
            m_ActiveField = EditField::ScaleY;
            m_ActiveInputText = FormatFloat(target->scaleY, 2);
        } else if (btn.action == "edit_w" && target)
        {
            m_ActiveField = EditField::SizeW;
            m_ActiveInputText = std::to_string((int) target->shape.getSize().x);
        } else if (btn.action == "edit_h" && target)
        {
            m_ActiveField = EditField::SizeH;
            m_ActiveInputText = std::to_string((int) target->shape.getSize().y);
        } else if (btn.action == "pick_color" && target)
        {
#ifdef _WIN32
            HWND hwnd = reinterpret_cast<HWND>(m_Window.getSystemHandle());
            if (OpenColorPickerDialog(target->color, hwnd))
            {
                target->shape.setFillColor(target->color);
                if (IsPolygonType(target->objectType))
                    target->circleShape.setFillColor(target->color);
                if (target->entity != 0 && m_Registry.HasComponent<RenderComponent>(target->entity))
                    m_Registry.GetComponent<RenderComponent>(target->entity).color = target->color;
            }
#endif
        } else if (btn.action == "edit_r" && target)
        {
            m_ActiveField = EditField::ColorR;
            m_ActiveInputText = std::to_string(target->color.r);
        } else if (btn.action == "edit_g" && target)
        {
            m_ActiveField = EditField::ColorG;
            m_ActiveInputText = std::to_string(target->color.g);
        } else if (btn.action == "edit_b" && target)
        {
            m_ActiveField = EditField::ColorB;
            m_ActiveInputText = std::to_string(target->color.b);
        } else if (btn.action == "edit_z" && target)
        {
            m_ActiveField = EditField::ZIndex;
            m_ActiveInputText = std::to_string(target->zIndex);
        } else if (btn.action == "layer_forward" && target)
        {
            target->zIndex++;
            SyncToRegistry();
        } else if (btn.action == "layer_backward" && target)
        {
            target->zIndex--;
            SyncToRegistry();
        } else if (btn.action == "toggle_visible_in_game" && target)
        {
            bool isNeverVisibleType = (target->objectType == ObjectType::SpawnPoint ||
                                       target->objectType == ObjectType::AudioSource ||
                                       target->objectType == ObjectType::ParticleEmitter ||
                                       target->objectType == ObjectType::Camera ||
                                       target->objectType == ObjectType::Empty ||
                                       target->objectType == ObjectType::TriggerZone);
            if (!isNeverVisibleType)
            {
                target->visibleInGame = !target->visibleInGame;
                SyncToRegistry();
                SetDirty(true);
            }
        } else if (btn.action == "open_script" && target && !target->scriptPath.empty())
        {
            OpenScriptInIDE(target->scriptPath);
        } else if (btn.action == "remove_sprite" && target)
        {
            target->spritePath.clear();
            target->previewTexture.reset();
            target->previewSprite = sf::Sprite{};
            target->shape.setFillColor(target->color);
            if (target->entity != 0 && m_Registry.HasComponent<SpriteComponent>(target->entity))
                m_Registry.RemoveComponent<SpriteComponent>(target->entity);
            std::cout << "[INFO] [Inspector] SpriteComponent removed from " << target->id << "\n";
        } else if (btn.action == "change_sprite" && target)
        {
            std::cout << "[INFO] [Inspector] Drag an image from the Content Browser to change sprite.\n";
        } else if (btn.action == "edit_template" && target && !target->templatePath.empty())
        {
            EnterTemplateEditMode(target->templatePath);
        } else if (btn.action == "save_as_template" && target) { SaveAsTemplate(target, ""); } else if (
            btn.action == "apply_template" && target) { ApplyToTemplate(target); } else if (
            btn.action == "unlink_template" && target)
        {
            target->templatePath.clear();
            SetDirty(true);
            UpdateStatusText();
        }
        break;
    }
}


void EditorScene::CommitActiveField()
{
    if (m_ActiveField == EditField::None) return;

    EditorObject *inputTarget = GetInspectedObject();
    if (inputTarget && !m_ActiveInputText.empty())
    {
        if (m_ActiveField == EditField::Name)
        {
            if (m_InspectorLocked &&m_LockedObjectId == inputTarget->id) {
                m_LockedObjectId = m_ActiveInputText;
            }
            inputTarget->id = m_ActiveInputText;
            UpdateStatusText();
        } else if (m_ActiveField == EditField::Tag)
        {
            inputTarget->tag = m_ActiveInputText;
            if (inputTarget->entity != 0)
            {
                if (m_Registry.HasComponent<TagComponent>(inputTarget->entity))
                    m_Registry.GetComponent<TagComponent>(inputTarget->entity).tag = inputTarget->tag;
                else
                    m_Registry.AddComponent(inputTarget->entity, TagComponent{inputTarget->tag});
            }
            UpdateStatusText();
        } else if (m_ActiveField == EditField::Script)
        {
            std::string fullPath = std::string(ASSET_PATH) + "/" + m_ActiveInputText + ".lua";
            std::ifstream check(fullPath);
            if (!check.is_open())
            {
                std::ofstream newFile(fullPath);
                newFile << "function OnCreate()\n\nend\n\n";
                newFile << "function OnUpdate(dt)\n\nend\n";
                newFile.close();
                std::cout << "[INFO] [Inspector] Created new Lua script file: " << fullPath << "\n";
            }
            check.close();
            auto &sc = m_Registry.AddComponent(inputTarget->entity,
                                               ScriptComponent(LuaState::GetLua(), fullPath));
            sc.SetEntity(inputTarget->entity);
            inputTarget->scriptPath = fullPath;

            for (const auto &prop: sc.GetExportedProperties()) { inputTarget->scriptProperties[prop.name] = prop; }

            std::cout << "[INFO] [Inspector] Script assigned to entity: " << fullPath << "\n";
        } else if (m_ActiveField == EditField::TransformX || m_ActiveField == EditField::TransformY)
        {
            try
            {
                float val = std::stof(m_ActiveInputText);
                sf::Vector2f pos = inputTarget->localPosition;
                if (m_ActiveField == EditField::TransformX) pos.x = val;
                else pos.y = val;
                inputTarget->localPosition = pos;
                UpdateWorldTransforms();
            } catch (...) {}
        } else if (m_ActiveField == EditField::Rotation)
        {
            try
            {
                float val = std::stof(m_ActiveInputText);
                inputTarget->rotation = val;
                UpdateWorldTransforms();
            } catch (...) {}
        } else if (m_ActiveField == EditField::ScaleX || m_ActiveField == EditField::ScaleY)
        {
            try
            {
                float val = std::stof(m_ActiveInputText);
                if (m_ActiveField == EditField::ScaleX) inputTarget->scaleX = val;
                else inputTarget->scaleY = val;
                UpdateWorldTransforms();
            } catch (...) {}
        } else if (m_ActiveField == EditField::SizeW || m_ActiveField == EditField::SizeH)
        {
            try
            {
                float val = std::max(4.f, std::stof(m_ActiveInputText));
                sf::Vector2f size = inputTarget->shape.getSize();
                if (m_ActiveField == EditField::SizeW) size.x = val;
                else size.y = val;
                inputTarget->shape.setSize(size);

                if (IsPolygonType(inputTarget->objectType))
                {
                    float rx = size.x * 0.5f;
                    float ry = size.y * 0.5f;
                    if (rx > 0.001f && ry > 0.001f)
                    {
                        inputTarget->circleShape.setRadius(rx);
                        inputTarget->circleShape.setScale(inputTarget->scaleX, inputTarget->scaleY * (ry / rx));
                    }
                }
                if (inputTarget->objectType == ObjectType::Sprite && inputTarget->previewTexture)
                {
                    const sf::Vector2u ts = inputTarget->previewTexture->getSize();
                    if (ts.x > 0 && ts.y > 0)
                        inputTarget->previewSprite.setScale(size.x / ts.x, size.y / ts.y);
                }
                if (inputTarget->entity != 0 && m_Registry.HasComponent<RenderComponent>(inputTarget->entity))
                    m_Registry.GetComponent<RenderComponent>(inputTarget->entity).size = size;
            } catch (...) {}
        } else if (m_ActiveField == EditField::ColorR || m_ActiveField == EditField::ColorG || m_ActiveField ==
                   EditField::ColorB)
        {
            try
            {
                int val = std::clamp(std::stoi(m_ActiveInputText), 0, 255);
                if (m_ActiveField == EditField::ColorR) inputTarget->color.r = val;
                else if (m_ActiveField == EditField::ColorG) inputTarget->color.g = val;
                else inputTarget->color.b = val;
                inputTarget->shape.setFillColor(inputTarget->color);
            } catch (...) {}
        } else if (m_ActiveField == EditField::ScriptProperty)
        {
            bool handledAsVec2 = false;
            if (m_ActiveScriptProperty.size() > 2)
            {
                std::string suffix = m_ActiveScriptProperty.substr(m_ActiveScriptProperty.size() - 2);
                if (suffix == "_x" || suffix == "_y")
                {
                    std::string baseName = m_ActiveScriptProperty.substr(0, m_ActiveScriptProperty.size() - 2);
                    auto baseIt = inputTarget->scriptProperties.find(baseName);
                    if (baseIt != inputTarget->scriptProperties.end() &&
                        baseIt->second.type == ScriptComponent::PropertyType::Vec2)
                    {
                        handledAsVec2 = true;
                        try
                        {
                            float val = std::stof(m_ActiveInputText);
                            if (suffix == "_x") baseIt->second.floatVal = val;
                            else baseIt->second.vec2Y = val;
                            if (inputTarget->entity != 0 && m_Registry.HasComponent<ScriptComponent>(
                                    inputTarget->entity))
                                m_Registry.GetComponent<ScriptComponent>(inputTarget->entity).SetExportedProperty(
                                    baseIt->second);
                        } catch (...) {}
                    }
                }
            }

            if (!handledAsVec2)
            {
                auto it = inputTarget->scriptProperties.find(m_ActiveScriptProperty);
                if (it != inputTarget->scriptProperties.end())
                {
                    auto &prop = it->second;
                    try
                    {
                        if (prop.type == ScriptComponent::PropertyType::Int) prop.intVal = std::stoi(m_ActiveInputText);
                        else if (prop.type == ScriptComponent::PropertyType::Float)
                            prop.floatVal = std::stof(m_ActiveInputText);
                        else if (prop.type == ScriptComponent::PropertyType::String) prop.stringVal = m_ActiveInputText;
                        if (inputTarget->entity != 0 && m_Registry.HasComponent<ScriptComponent>(inputTarget->entity))
                        {
                            m_Registry.GetComponent<ScriptComponent>(inputTarget->entity).SetExportedProperty(prop);
                        }
                    } catch (...) {}
                }
            }
        } else if (m_ActiveField == EditField::ZIndex)
        {
            try
            {
                inputTarget->zIndex = std::stoi(m_ActiveInputText);
                SyncToRegistry();
            } catch (...) {}
        } else if (m_ActiveField == EditField::CollisionChannel)
        {
            try
            {
                if (inputTarget->entity != 0 && m_Registry.HasComponent<CollisionComponent>(inputTarget->entity))
                    m_Registry.GetComponent<CollisionComponent>(inputTarget->entity).channel =
                            std::stoi(m_ActiveInputText);
            } catch (...) {}
        } else if (m_ActiveField == EditField::RigidbodyMass)
        {
            try
            {
                if (inputTarget->entity != 0 && m_Registry.HasComponent<Rigidbody2DComponent>(inputTarget->entity))
                    m_Registry.GetComponent<Rigidbody2DComponent>(inputTarget->entity).mass =
                            std::max(0.001f, std::stof(m_ActiveInputText));
            } catch (...) {}
        } else if (m_ActiveField == EditField::RigidbodyGravity)
        {
            try
            {
                if (inputTarget->entity != 0 && m_Registry.HasComponent<Rigidbody2DComponent>(inputTarget->entity))
                    m_Registry.GetComponent<Rigidbody2DComponent>(inputTarget->entity).gravityScale =
                            std::stof(m_ActiveInputText);
            } catch (...) {}
        } else if (m_ActiveField == EditField::RigidbodyRestitution)
        {
            try
            {
                if (inputTarget->entity != 0 && m_Registry.HasComponent<Rigidbody2DComponent>(inputTarget->entity))
                    m_Registry.GetComponent<Rigidbody2DComponent>(inputTarget->entity).restitution =
                            std::clamp(std::stof(m_ActiveInputText), 0.0f, 1.0f);
            } catch (...) {}
        } else if (m_ActiveField == EditField::RigidbodyDrag)
        {
            try
            {
                if (inputTarget->entity != 0 && m_Registry.HasComponent<Rigidbody2DComponent>(inputTarget->entity))
                    m_Registry.GetComponent<Rigidbody2DComponent>(inputTarget->entity).drag =
                            std::max(0.0f, std::stof(m_ActiveInputText));
            } catch (...) {}
        } else if (m_ActiveField == EditField::VelocityDX)
        {
            try
            {
                if (inputTarget->entity != 0)
                {
                    float val = std::stof(m_ActiveInputText);
                    if (m_Registry.HasComponent<VelocityComponent>(inputTarget->entity))
                        m_Registry.GetComponent<VelocityComponent>(inputTarget->entity).dx = val;
                    else
                        m_Registry.AddComponent(inputTarget->entity, VelocityComponent{val, 0.f});
                }
            } catch (...) {}
        } else if (m_ActiveField == EditField::VelocityDY)
        {
            try
            {
                if (inputTarget->entity != 0)
                {
                    float val = std::stof(m_ActiveInputText);
                    if (m_Registry.HasComponent<VelocityComponent>(inputTarget->entity))
                        m_Registry.GetComponent<VelocityComponent>(inputTarget->entity).dy = val;
                    else
                        m_Registry.AddComponent(inputTarget->entity, VelocityComponent{0.f, val});
                }
            } catch (...) {}
        } else if (m_ActiveField == EditField::CameraSmoothSpeed)
        {
            try
            {
                if (inputTarget->entity != 0 && m_Registry.HasComponent<CameraComponent>(inputTarget->entity))
                    m_Registry.GetComponent<CameraComponent>(inputTarget->entity).smoothSpeed =
                            std::max(0.0f, std::stof(m_ActiveInputText));
            } catch (...) {}
        } else if (m_ActiveField == EditField::CameraOffsetX)
        {
            try
            {
                if (inputTarget->entity != 0 && m_Registry.HasComponent<CameraComponent>(inputTarget->entity))
                    m_Registry.GetComponent<CameraComponent>(inputTarget->entity).offsetX =
                            std::stof(m_ActiveInputText);
            } catch (...) {}
        } else if (m_ActiveField == EditField::CameraOffsetY)
        {
            try
            {
                if (inputTarget->entity != 0 && m_Registry.HasComponent<CameraComponent>(inputTarget->entity))
                    m_Registry.GetComponent<CameraComponent>(inputTarget->entity).offsetY =
                            std::stof(m_ActiveInputText);
            } catch (...) {}
        } else if (m_ActiveField == EditField::CameraZoom)
        {
            try
            {
                if (inputTarget->entity != 0 && m_Registry.HasComponent<CameraComponent>(inputTarget->entity))
                    m_Registry.GetComponent<CameraComponent>(inputTarget->entity).zoom =
                            std::max(0.01f, std::stof(m_ActiveInputText));
            } catch (...) {}
        } else if (m_ActiveField == EditField::CameraPriority)
        {
            try
            {
                if (inputTarget->entity != 0 && m_Registry.HasComponent<CameraComponent>(inputTarget->entity))
                    m_Registry.GetComponent<CameraComponent>(inputTarget->entity).priority =
                            std::stoi(m_ActiveInputText);
            } catch (...) {}
        } else if (m_ActiveField == EditField::CameraAutoFramePadding)
        {
            try
            {
                if (inputTarget->entity != 0 && m_Registry.HasComponent<CameraComponent>(inputTarget->entity))
                    m_Registry.GetComponent<CameraComponent>(inputTarget->entity).autoFramePadding =
                            std::max(0.0f, std::stof(m_ActiveInputText));
            } catch (...) {}
        } else if (m_ActiveField == EditField::CameraMinZoom)
        {
            try
            {
                if (inputTarget->entity != 0 && m_Registry.HasComponent<CameraComponent>(inputTarget->entity))
                    m_Registry.GetComponent<CameraComponent>(inputTarget->entity).minZoom =
                            std::max(0.01f, std::stof(m_ActiveInputText));
            } catch (...) {}
        } else if (m_ActiveField == EditField::CameraMaxZoom)
        {
            try
            {
                if (inputTarget->entity != 0 && m_Registry.HasComponent<CameraComponent>(inputTarget->entity))
                    m_Registry.GetComponent<CameraComponent>(inputTarget->entity).maxZoom =
                            std::max(0.01f, std::stof(m_ActiveInputText));
            } catch (...) {}
        } else if (m_ActiveField == EditField::TextContent)
        {
            inputTarget->textString = m_ActiveInputText;
            if (inputTarget->entity != 0 && m_Registry.HasComponent<TextComponent>(inputTarget->entity))
                m_Registry.GetComponent<TextComponent>(inputTarget->entity).text = m_ActiveInputText;
        } else if (m_ActiveField == EditField::TextFontSize)
        {
            try
            {
                inputTarget->textFontSize = static_cast<unsigned int>(std::max(1, std::stoi(m_ActiveInputText)));
                if (inputTarget->entity != 0 && m_Registry.HasComponent<TextComponent>(inputTarget->entity))
                    m_Registry.GetComponent<TextComponent>(inputTarget->entity).characterSize = inputTarget->
                            textFontSize;
            } catch (...) {}
        } else if (m_ActiveField == EditField::AudioPath)
        {
            inputTarget->audioClipPath = m_ActiveInputText;
            if (inputTarget->entity != 0 && m_Registry.HasComponent<AudioSourceComponent>(inputTarget->entity))
                m_Registry.GetComponent<AudioSourceComponent>(inputTarget->entity).soundPath = m_ActiveInputText;
        } else if (m_ActiveField == EditField::AudioVolume)
        {
            try
            {
                inputTarget->audioVolume = std::clamp(std::stof(m_ActiveInputText), 0.0f, 100.0f);
                if (inputTarget->entity != 0 && m_Registry.HasComponent<AudioSourceComponent>(inputTarget->entity))
                    m_Registry.GetComponent<AudioSourceComponent>(inputTarget->entity).volume = inputTarget->
                            audioVolume;
            } catch (...) {}
        } else if (m_ActiveField == EditField::AudioPitch)
        {
            try
            {
                inputTarget->audioPitch = std::max(0.05f, std::stof(m_ActiveInputText));
                if (inputTarget->entity != 0 && m_Registry.HasComponent<AudioSourceComponent>(inputTarget->entity))
                    m_Registry.GetComponent<AudioSourceComponent>(inputTarget->entity).pitch = inputTarget->audioPitch;
            } catch (...) {}
        } else if (m_ActiveField == EditField::ParticleRate)
        {
            try
            {
                inputTarget->particleRate = std::max(0.1f, std::stof(m_ActiveInputText));
                if (inputTarget->entity != 0 && m_Registry.HasComponent<ParticleEmitterComponent>(inputTarget->entity))
                    m_Registry.GetComponent<ParticleEmitterComponent>(inputTarget->entity).emissionRate = inputTarget->
                            particleRate;
            } catch (...) {}
        } else if (m_ActiveField == EditField::ParticleLifetime)
        {
            try
            {
                inputTarget->particleLifetime = std::max(0.05f, std::stof(m_ActiveInputText));
                if (inputTarget->entity != 0 && m_Registry.HasComponent<ParticleEmitterComponent>(inputTarget->entity))
                    m_Registry.GetComponent<ParticleEmitterComponent>(inputTarget->entity).lifetime = inputTarget->
                            particleLifetime;
            } catch (...) {}
        } else if (m_ActiveField == EditField::ParticleSpeed)
        {
            try
            {
                inputTarget->particleSpeed = std::stof(m_ActiveInputText);
                if (inputTarget->entity != 0 && m_Registry.HasComponent<ParticleEmitterComponent>(inputTarget->entity))
                    m_Registry.GetComponent<ParticleEmitterComponent>(inputTarget->entity).speed = inputTarget->
                            particleSpeed;
            } catch (...) {}
        } else if (m_ActiveField == EditField::ParticleAngle)
        {
            try
            {
                inputTarget->particleAngle = std::stof(m_ActiveInputText);
                if (inputTarget->entity != 0 && m_Registry.HasComponent<ParticleEmitterComponent>(inputTarget->entity))
                    m_Registry.GetComponent<ParticleEmitterComponent>(inputTarget->entity).angle = inputTarget->
                            particleAngle;
            } catch (...) {}
        } else if (m_ActiveField == EditField::ParticleSpread)
        {
            try
            {
                inputTarget->particleSpread = std::clamp(std::stof(m_ActiveInputText), 0.0f, 360.0f);
                if (inputTarget->entity != 0 && m_Registry.HasComponent<ParticleEmitterComponent>(inputTarget->entity))
                    m_Registry.GetComponent<ParticleEmitterComponent>(inputTarget->entity).spreadAngle = inputTarget->
                            particleSpread;
            } catch (...) {}
        } else if (m_ActiveField == EditField::ParticleStartSize)
        {
            try
            {
                inputTarget->particleStartSize = std::max(0.1f, std::stof(m_ActiveInputText));
                if (inputTarget->entity != 0 && m_Registry.HasComponent<ParticleEmitterComponent>(inputTarget->entity))
                    m_Registry.GetComponent<ParticleEmitterComponent>(inputTarget->entity).startSize = inputTarget->
                            particleStartSize;
            } catch (...) {}
        } else if (m_ActiveField == EditField::ParticleEndSize)
        {
            try
            {
                inputTarget->particleEndSize = std::max(0.0f, std::stof(m_ActiveInputText));
                if (inputTarget->entity != 0 && m_Registry.HasComponent<ParticleEmitterComponent>(inputTarget->entity))
                    m_Registry.GetComponent<ParticleEmitterComponent>(inputTarget->entity).endSize = inputTarget->
                            particleEndSize;
            } catch (...) {}
        } else if (m_ActiveField == EditField::ParticleGravityX)
        {
            try
            {
                inputTarget->particleGravityX = std::stof(m_ActiveInputText);
                if (inputTarget->entity != 0 && m_Registry.HasComponent<ParticleEmitterComponent>(inputTarget->entity))
                    m_Registry.GetComponent<ParticleEmitterComponent>(inputTarget->entity).gravityX = inputTarget->
                            particleGravityX;
            } catch (...) {}
        } else if (m_ActiveField == EditField::ParticleGravityY)
        {
            try
            {
                inputTarget->particleGravityY = std::stof(m_ActiveInputText);
                if (inputTarget->entity != 0 && m_Registry.HasComponent<ParticleEmitterComponent>(inputTarget->entity))
                    m_Registry.GetComponent<ParticleEmitterComponent>(inputTarget->entity).gravityY = inputTarget->
                            particleGravityY;
            } catch (...) {}
        }
        SetDirty(true);
    }

    m_ActiveField = EditField::None;
    m_ActiveInputText.clear();
    m_InputSelectionStart = -1;
    m_InputSelectionEnd = -1;
}

