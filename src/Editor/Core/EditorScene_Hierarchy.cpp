#include "EditorScene_Common.h"
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <map>


void EditorScene::DrawHierarchy(sf::RenderWindow &window)
{
    m_HierarchyHitboxes.clear();
    m_HierarchyFoldHitboxes.clear();

    const float panelX = m_HierarchyBounds.left;
    const float panelY = m_HierarchyBounds.top;
    const float panelH = m_HierarchyBounds.height;

    m_HierarchyPanel.setPosition(panelX, panelY);
    m_HierarchyPanel.setSize({HierarchyWidth, panelH});
    window.draw(m_HierarchyPanel);

    sf::RectangleShape rightBorder({1.f, panelH});
    rightBorder.setFillColor(C_BORDER);
    rightBorder.setPosition(panelX + HierarchyWidth - 1.f, panelY);
    window.draw(rightBorder); {
        sf::RectangleShape header({HierarchyWidth, 36.f});
        header.setPosition(panelX, panelY);
        header.setFillColor(C_BG_ELEVATED);
        window.draw(header);

        sf::RectangleShape headerLine({HierarchyWidth, 1.f});
        headerLine.setFillColor(C_BORDER);
        headerLine.setPosition(panelX, panelY + 35.f);
        window.draw(headerLine);

        sf::Text title;
        title.setFont(*m_Font);
        title.setCharacterSize(11);
        title.setFillColor(m_HasUnsavedChanges ? C_WARNING : C_TEXT_SECONDARY);
        title.setStyle(sf::Text::Bold);
        title.setString(m_HasUnsavedChanges ? "SCENE HIERARCHY *" : "SCENE HIERARCHY");
        title.setPosition(panelX + 12.f, panelY + 12.f);
        window.draw(title);
    }

    sf::View prevView = window.getView();

    const float winW = static_cast<float>(window.getSize().x);
    const float winH = static_cast<float>(window.getSize().y);

    sf::View clipView;
    clipView.setSize(HierarchyWidth, panelH - 36.f);
    clipView.setCenter(panelX + HierarchyWidth / 2.f, panelY + 36.f + (panelH - 36.f) / 2.f);
    clipView.setViewport({
        panelX / winW,
        (panelY + 36.f) / winH,
        HierarchyWidth / winW,
        (panelH - 36.f) / winH
    });
    window.setView(clipView);

    float y = panelY + 36.f - m_HierarchyScrollY;
    const float rowHeight = 26.f;

    std::function < void(EditorObject *, int) > drawNode = [&](EditorObject *obj, int depth) {
        if (!obj) return;

        float indent = depth * 16.f;
        sf::FloatRect rowRect(panelX, y, HierarchyWidth, rowHeight);
        bool isSelected = (obj == m_Selected);
        bool isHovered = rowRect.contains(m_MouseScreenPos) && m_MouseScreenPos.y > panelY + 36.f;
        bool isDropTarget = (m_HierarchyDragging && m_HierarchyDragTargetId == obj->id);

        if (y + rowHeight > panelY + 36.f && y < panelY + panelH)
        {
            if (isSelected || isHovered || isDropTarget)
            {
                sf::RectangleShape rowBg({HierarchyWidth, rowHeight});
                rowBg.setPosition(panelX, y);
                if (isDropTarget && m_HierarchyDropMode == HierarchyDropMode::Inside)
                {
                    rowBg.setFillColor(sf::Color(80, 120, 240, 140));
                    rowBg.setOutlineColor(C_ACCENT_BRIGHT);
                    rowBg.setOutlineThickness(1.5f);
                } else
                {
                    rowBg.setFillColor(isSelected ? C_ACCENT_DIM : C_BG_ELEVATED);
                    rowBg.setOutlineColor(sf::Color::Transparent);
                    rowBg.setOutlineThickness(0.f);
                }
                window.draw(rowBg);

                if (isSelected && (!isDropTarget || m_HierarchyDropMode != HierarchyDropMode::Inside))
                {
                    sf::RectangleShape indicator({2.f, rowHeight});
                    indicator.setPosition(panelX, y);
                    indicator.setFillColor(C_ACCENT);
                    window.draw(indicator);
                }
            }

            if (isDropTarget && (m_HierarchyDropMode == HierarchyDropMode::Above || m_HierarchyDropMode == HierarchyDropMode::Below))
            {
                float lineY = (m_HierarchyDropMode == HierarchyDropMode::Above) ? y : (y + rowHeight);
                float lineLeft = panelX + indent + 8.f;
                float lineW = HierarchyWidth - (indent + 12.f);

                sf::RectangleShape insertLine({lineW, 2.f});
                insertLine.setOrigin(0.f, 1.f);
                insertLine.setPosition(lineLeft, lineY);
                insertLine.setFillColor(sf::Color(100, 180, 255));
                window.draw(insertLine);

                sf::CircleShape insertDot(3.5f);
                insertDot.setOrigin(3.5f, 3.5f);
                insertDot.setPosition(lineLeft, lineY);
                insertDot.setFillColor(sf::Color(100, 180, 255));
                window.draw(insertDot);
            }

            auto children = GetChildren(obj->id);
            bool hasChildren = !children.empty();
            bool isCollapsed = m_HierarchyCollapsed.count(obj->id) > 0;

            if (hasChildren)
            {
                sf::FloatRect foldRect(panelX + 4.f + indent, y, 14.f, rowHeight);
                m_HierarchyFoldHitboxes.push_back({foldRect, obj->id});

                sf::Text arrowText;
                arrowText.setFont(*m_Font);
                arrowText.setCharacterSize(10);
                arrowText.setFillColor(C_TEXT_MUTED);
                arrowText.setString(isCollapsed ? ">" : "v");
                arrowText.setPosition(foldRect.left + 3.f, foldRect.top + 7.f);
                window.draw(arrowText);
            }

            sf::CircleShape icon;
            icon.setRadius(5.f);
            icon.setPosition(panelX + (hasChildren ? 20.f : 16.f) + indent, y + 8.f);

            if (IsPolygonType(obj->objectType))
            {
                icon.setPointCount(GetPolygonPointCount(obj->objectType));
                icon.setFillColor(obj->color);
            } else if (obj->objectType == ObjectType::Sprite)
            {
                icon.setPointCount(4);
                icon.setFillColor(sf::Color(150, 150, 255));
            } else
            {
                icon.setPointCount(4);
                icon.setFillColor(obj->color);
            }
            window.draw(icon);

            sf::Text nameText;
            nameText.setFont(*m_Font);
            nameText.setCharacterSize(12);
            if (!obj->templatePath.empty())
                nameText.setFillColor(isSelected ? sf::Color(140, 210, 255) : sf::Color(100, 180, 240));
            else
                nameText.setFillColor(isSelected ? C_TEXT_PRIMARY : C_TEXT_SECONDARY);
            nameText.setString(obj->id);
            nameText.setPosition(panelX + (hasChildren ? 38.f : 34.f) + indent, y + 4.f);
            window.draw(nameText);
        }

        m_HierarchyHitboxes.push_back({sf::FloatRect(panelX, y, HierarchyWidth, rowHeight), obj});
        y += rowHeight;

        auto children = GetChildren(obj->id);
        bool isCollapsed = m_HierarchyCollapsed.count(obj->id) > 0;
        if (!children.empty() && !isCollapsed) { for (auto *child: children) { drawNode(child, depth + 1); } }
    };

    std::vector<EditorObject *> rootObjects;
    for (auto &obj: m_Objects)
    {
        if (obj.parentId.empty() || ObjectById(obj.parentId) == nullptr) { rootObjects.push_back(&obj); }
    }

    for (auto *root: rootObjects) { drawNode(root, 0); }

    if (y < panelY + panelH)
    {
        m_HierarchyRootDropZone = sf::FloatRect(panelX, y, HierarchyWidth, panelY + panelH - y);
        if (m_HierarchyDragging &&m_HierarchyDragTargetId.empty() && m_HierarchyRootDropZone.contains(m_MouseScreenPos)
        ) {
            sf::RectangleShape rootIndicator({HierarchyWidth - 8.f, 20.f});
            rootIndicator.setPosition(panelX + 4.f, y + 4.f);
            rootIndicator.setFillColor(sf::Color(80, 120, 240, 60));
            rootIndicator.setOutlineColor(C_ACCENT);
            rootIndicator.setOutlineThickness(1.f);
            window.draw(rootIndicator);

            sf::Text rText;
            rText.setFont(*m_Font);
            rText.setCharacterSize(10);
            rText.setFillColor(C_TEXT_MUTED);
            rText.setString("Detach to Root");
            rText.setPosition(panelX + 12.f, y + 7.f);
            window.draw(rText);
        }
    } else { m_HierarchyRootDropZone = sf::FloatRect(0, 0, 0, 0); }

    window.setView(prevView);

    if (m_HierarchyDragging && !m_HierarchyDragSourceId.empty())
    {
        std::string actionDesc = m_HierarchyDragSourceId;
        if (m_HierarchyDropMode == HierarchyDropMode::Above && !m_HierarchyDragTargetId.empty())
            actionDesc += " [Above " + m_HierarchyDragTargetId + "]";
        else if (m_HierarchyDropMode == HierarchyDropMode::Below && !m_HierarchyDragTargetId.empty())
            actionDesc += " [Below " + m_HierarchyDragTargetId + "]";
        else if (m_HierarchyDropMode == HierarchyDropMode::Inside && !m_HierarchyDragTargetId.empty())
            actionDesc += " [Inside " + m_HierarchyDragTargetId + "]";
        else if (m_HierarchyDropMode == HierarchyDropMode::Root)
            actionDesc += " [To Root]";

        sf::Text gt;
        gt.setFont(*m_Font);
        gt.setCharacterSize(11);
        gt.setFillColor(sf::Color::White);
        gt.setString(actionDesc);

        float ghostW = std::max(130.f, gt.getLocalBounds().width + 20.f);
        sf::RectangleShape ghost({ghostW, 22.f});
        ghost.setPosition(m_MouseScreenPos.x + 12.f, m_MouseScreenPos.y + 12.f);
        ghost.setFillColor(sf::Color(35, 38, 46, 235));
        ghost.setOutlineColor(C_ACCENT_BRIGHT);
        ghost.setOutlineThickness(1.5f);
        window.draw(ghost);

        gt.setPosition(m_MouseScreenPos.x + 18.f, m_MouseScreenPos.y + 15.f);
        window.draw(gt);
    }

    if (m_HierarchyContextMenuOpen)
    {
        m_ContextHitboxes.clear();
        const float itemH = 28.f;
        const float menuW = 140.f;

        std::vector<std::pair<std::string, std::string> > actions;
        if (m_ContextObject && !m_ContextObject->templatePath.empty())
        {
            actions.push_back({"Edit Template", "edit_template"});
        }
        actions.push_back({"Rename", "rename"});
        actions.push_back({"Duplicate", "duplicate"});
        actions.push_back({"Save as Template", "save_template"});
        actions.push_back({"Delete", "delete"});

        const float menuH = actions.size() * itemH;
        sf::RectangleShape bg({menuW, menuH});
        bg.setPosition(m_ContextMenuPos);
        bg.setFillColor(C_BG_ELEVATED);
        bg.setOutlineColor(C_BORDER_LIGHT);
        bg.setOutlineThickness(1.f);
        window.draw(bg);

        float cy = m_ContextMenuPos.y;
        for (const auto &action: actions)
        {
            sf::FloatRect row(m_ContextMenuPos.x, cy, menuW, itemH);
            bool hov = row.contains(m_MouseScreenPos);
            if (hov)
            {
                sf::RectangleShape hovBg({menuW, itemH});
                hovBg.setPosition(m_ContextMenuPos.x, cy);
                hovBg.setFillColor(sf::Color(C_BG_ELEVATED.r + 10, C_BG_ELEVATED.g + 10, C_BG_ELEVATED.b + 10));
                window.draw(hovBg);
            }

            sf::Text t;
            t.setFont(*m_Font);
            t.setCharacterSize(12);
            t.setString(action.first);
            t.setFillColor(action.second == "delete" ? C_DANGER : (hov ? C_TEXT_PRIMARY : C_TEXT_SECONDARY));
            t.setPosition(m_ContextMenuPos.x + 12.f, cy + 6.f);
            window.draw(t);

            m_ContextHitboxes.push_back({row, action.second});
            cy += itemH;
        }
    }
}


void EditorScene::DeleteSelected()
{
    if (m_SelectedObjects.empty()) return;

    if (m_SelectedObjects.size() == 1)
    {
        auto children = GetChildren(m_SelectedObjects[0]->id);
        if (!children.empty())
        {
            DeleteObjectWithPrompt(m_SelectedObjects[0]);
            return;
        }
    }

    size_t count = m_SelectedObjects.size();
    auto macroCmd = std::make_shared<MacroCommand>();
    for (auto *obj: m_SelectedObjects)
    {
        json before = SerializeObject(*obj);
        macroCmd->commands.push_back(std::make_shared<ObjectStateCommand>(obj->id, before, json()));
    }
    ExecuteCommand(macroCmd);
    ClearSelection();
    std::cout << "[INFO] [EditorScene] Deleted " << count << " object(s).\n";
}


void EditorScene::DeleteObjectWithPrompt(EditorObject *obj)
{
    if (!obj) return;
    auto children = GetChildren(obj->id);
    if (children.empty())
    {
        auto macroCmd = std::make_shared<MacroCommand>();
        json before = SerializeObject(*obj);
        macroCmd->commands.push_back(std::make_shared<ObjectStateCommand>(obj->id, before, json()));
        ExecuteCommand(macroCmd);
        ClearSelection();
        return;
    }

    m_DeleteModalTargetId = obj->id;
    m_DeleteModalDescendantIds.clear();
    for (auto *ch: children) { m_DeleteModalDescendantIds.push_back(ch->id); }
    m_ShowDeleteModal = true;
}


void EditorScene::ConfirmDeleteCascade()
{
    if (m_DeleteModalTargetId.empty()) return;

    std::vector<std::string> toDelete;
    auto collect = [&](auto &self, const std::string &parentId) -> void {
        for (auto *ch: GetChildren(parentId))
        {
            self(self, ch->id);
            toDelete.push_back(ch->id);
        }
    };
    collect(collect, m_DeleteModalTargetId);
    toDelete.push_back(m_DeleteModalTargetId);

    ClearSelection();

    auto macroCmd = std::make_shared<MacroCommand>();
    for (const auto &id: toDelete)
    {
        auto *obj = ObjectById(id);
        if (obj)
        {
            json before = SerializeObject(*obj);
            macroCmd->commands.push_back(std::make_shared<ObjectStateCommand>(obj->id, before, json()));
        }
    }
    ExecuteCommand(macroCmd);

    m_ShowDeleteModal = false;
    m_DeleteModalTargetId.clear();
    m_DeleteModalDescendantIds.clear();
}


void EditorScene::ConfirmDeleteUnparent()
{
    if (m_DeleteModalTargetId.empty()) return;
    auto *target = ObjectById(m_DeleteModalTargetId);
    std::string grandParent = target ? target->parentId : "";

    for (auto *ch: GetChildren(m_DeleteModalTargetId)) { SetParent(ch->id, grandParent, true); }

    ClearSelection();

    if (target)
    {
        auto macroCmd = std::make_shared<MacroCommand>();
        json before = SerializeObject(*target);
        macroCmd->commands.push_back(std::make_shared<ObjectStateCommand>(target->id, before, json()));
        ExecuteCommand(macroCmd);
    }

    m_ShowDeleteModal = false;
    m_DeleteModalTargetId.clear();
    m_DeleteModalDescendantIds.clear();
}


void EditorScene::DeleteActiveSelection()
{
    if (!HasTextSelection()) return;
    int sMin = GetSelectionMin();
    int sMax = GetSelectionMax();

    if (m_ActiveProjectSettingsField != ProjectSettingsField::None)
    {
        sMin = std::clamp(sMin, 0, (int) m_ProjectSettingsInputText.size());
        sMax = std::clamp(sMax, 0, (int) m_ProjectSettingsInputText.size());
        m_ProjectSettingsInputText.erase(sMin, sMax - sMin);
    } else if (m_ActiveField != EditField::None)
    {
        sMin = std::clamp(sMin, 0, (int) m_ActiveInputText.size());
        sMax = std::clamp(sMax, 0, (int) m_ActiveInputText.size());
        m_ActiveInputText.erase(sMin, sMax - sMin);
    }
    m_InputSelectionStart = -1;
    m_InputSelectionEnd = -1;
}

