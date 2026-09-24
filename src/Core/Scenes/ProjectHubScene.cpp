#include "ProjectHubScene.h"
#include "../UI/UIManager.h"
#include "../Application/Application.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <nlohmann/json.hpp>

ProjectHubScene::ProjectHubScene(SceneManager &manager, sf::RenderWindow &window)
    : Scene(manager), m_Window(window)
{
}

ProjectHubScene::~ProjectHubScene()
{
}

void ProjectHubScene::OnEnter()
{
    std::cout << "[INFO] [ProjectHubScene] Entering Hub Scene\n";
    if (Scene* editor = m_manager.GetScene("editor"))
    {
        editor->OnEnter();
    }
    SetupUI();
    m_UIsCreated = true;
}

void ProjectHubScene::OnExit()
{
    UIManager::Get().GetElements().clear();
    m_UIsCreated = false;
}

void ProjectHubScene::SetupUI()
{
    auto& ui = UIManager::Get();

    std::string curName = "MyProject";
    std::string curAuthor = "Developer";
    std::string curW = "1920";
    std::string curH = "1080";
    std::string curFps = "60";
    bool curVSync = true;

    if (auto* el = ui.GetElement("txt_projName")) curName = el->text;
    if (auto* el = ui.GetElement("txt_author")) curAuthor = el->text;
    if (auto* el = ui.GetElement("txt_width")) curW = el->text;
    if (auto* el = ui.GetElement("txt_height")) curH = el->text;
    if (auto* el = ui.GetElement("txt_fps")) curFps = el->text;
    if (auto* el = ui.GetElement("chk_vsync")) curVSync = el->isChecked;

    ui.GetElements().clear();

    float winW = static_cast<float>(m_Window.getSize().x);
    float winH = static_cast<float>(m_Window.getSize().y);

    float popupW = 540.0f;
    float popupH = 500.0f;
    float popupX = std::max(10.0f, (winW - popupW) / 2.0f);
    float popupY = std::max(10.0f, (winH - popupH) / 2.0f);

    auto* backdrop = ui.CreateElement("hub_backdrop", UIElementType::Panel);
    backdrop->position = {0.f, 0.f};
    backdrop->size = {winW, winH};
    backdrop->color = sf::Color(0, 0, 0, 160);
    backdrop->zIndex = 1;
    backdrop->UpdateDrawables();

    auto* card = ui.CreateElement("hub_card", UIElementType::Panel);
    card->position = {popupX, popupY};
    card->size = {popupW, popupH};
    card->color = sf::Color(26, 28, 34);
    card->outlineColor = sf::Color(62, 68, 84);
    card->outlineThickness = 1.5f;
    card->zIndex = 10;
    card->UpdateDrawables();

    auto* header = ui.CreateElement("hub_header", UIElementType::Panel);
    header->position = {popupX, popupY};
    header->size = {popupW, 46.0f};
    header->color = sf::Color(20, 21, 26);
    header->outlineColor = sf::Color(45, 50, 62);
    header->outlineThickness = 1.0f;
    header->zIndex = 11;
    header->UpdateDrawables();

    auto* title = ui.CreateElement("hub_title", UIElementType::Text);
    title->text = "RayneEngine  |  Project Setup";
    title->characterSize = 15;
    title->size = {400.0f, 20.0f};
    title->textColor = sf::Color(240, 242, 248);
    title->position = {popupX + 20.0f, popupY + 13.0f};
    title->zIndex = 12;
    title->UpdateDrawables();

    auto* desc = ui.CreateElement("hub_desc", UIElementType::Text);
    desc->text = "Configure basic project settings before starting:";
    desc->characterSize = 13;
    desc->size = {490.0f, 18.0f};
    desc->textColor = sf::Color(150, 155, 170);
    desc->position = {popupX + 25.0f, popupY + 58.0f};
    desc->zIndex = 12;
    desc->UpdateDrawables();

    auto* lblName = ui.CreateElement("lbl_projName", UIElementType::Text);
    lblName->text = "Project Name";
    lblName->characterSize = 12;
    lblName->size = {490.0f, 16.0f};
    lblName->textColor = sf::Color(200, 205, 215);
    lblName->position = {popupX + 25.0f, popupY + 90.0f};
    lblName->zIndex = 12;
    lblName->UpdateDrawables();

    auto* txtName = ui.CreateElement("txt_projName", UIElementType::TextInput);
    txtName->text = curName;
    txtName->characterSize = 14;
    txtName->textColor = sf::Color::White;
    txtName->position = {popupX + 25.0f, popupY + 112.0f};
    txtName->size = {490.0f, 34.0f};
    txtName->normalColor = sf::Color(18, 19, 23);
    txtName->borderColor = sf::Color(55, 60, 75);
    txtName->borderThickness = 1.0f;
    txtName->textOffset = {8.0f, 6.0f};
    txtName->zIndex = 12;
    txtName->UpdateDrawables();

    auto* lblAuthor = ui.CreateElement("lbl_author", UIElementType::Text);
    lblAuthor->text = "Author";
    lblAuthor->characterSize = 12;
    lblAuthor->size = {490.0f, 16.0f};
    lblAuthor->textColor = sf::Color(200, 205, 215);
    lblAuthor->position = {popupX + 25.0f, popupY + 158.0f};
    lblAuthor->zIndex = 12;
    lblAuthor->UpdateDrawables();

    auto* txtAuthor = ui.CreateElement("txt_author", UIElementType::TextInput);
    txtAuthor->text = curAuthor;
    txtAuthor->characterSize = 14;
    txtAuthor->textColor = sf::Color::White;
    txtAuthor->position = {popupX + 25.0f, popupY + 180.0f};
    txtAuthor->size = {490.0f, 34.0f};
    txtAuthor->normalColor = sf::Color(18, 19, 23);
    txtAuthor->borderColor = sf::Color(55, 60, 75);
    txtAuthor->borderThickness = 1.0f;
    txtAuthor->textOffset = {8.0f, 6.0f};
    txtAuthor->zIndex = 12;
    txtAuthor->UpdateDrawables();

    auto* lblWidth = ui.CreateElement("lbl_width", UIElementType::Text);
    lblWidth->text = "Window Width (px)";
    lblWidth->characterSize = 12;
    lblWidth->size = {235.0f, 16.0f};
    lblWidth->textColor = sf::Color(200, 205, 215);
    lblWidth->position = {popupX + 25.0f, popupY + 226.0f};
    lblWidth->zIndex = 12;
    lblWidth->UpdateDrawables();

    auto* txtWidth = ui.CreateElement("txt_width", UIElementType::TextInput);
    txtWidth->text = curW;
    txtWidth->characterSize = 14;
    txtWidth->textColor = sf::Color::White;
    txtWidth->position = {popupX + 25.0f, popupY + 248.0f};
    txtWidth->size = {235.0f, 34.0f};
    txtWidth->normalColor = sf::Color(18, 19, 23);
    txtWidth->borderColor = sf::Color(55, 60, 75);
    txtWidth->borderThickness = 1.0f;
    txtWidth->textOffset = {8.0f, 6.0f};
    txtWidth->zIndex = 12;
    txtWidth->UpdateDrawables();

    auto* lblHeight = ui.CreateElement("lbl_height", UIElementType::Text);
    lblHeight->text = "Window Height (px)";
    lblHeight->characterSize = 12;
    lblHeight->size = {235.0f, 16.0f};
    lblHeight->textColor = sf::Color(200, 205, 215);
    lblHeight->position = {popupX + 280.0f, popupY + 226.0f};
    lblHeight->zIndex = 12;
    lblHeight->UpdateDrawables();

    auto* txtHeight = ui.CreateElement("txt_height", UIElementType::TextInput);
    txtHeight->text = curH;
    txtHeight->characterSize = 14;
    txtHeight->textColor = sf::Color::White;
    txtHeight->position = {popupX + 280.0f, popupY + 248.0f};
    txtHeight->size = {235.0f, 34.0f};
    txtHeight->normalColor = sf::Color(18, 19, 23);
    txtHeight->borderColor = sf::Color(55, 60, 75);
    txtHeight->borderThickness = 1.0f;
    txtHeight->textOffset = {8.0f, 6.0f};
    txtHeight->zIndex = 12;
    txtHeight->UpdateDrawables();

    auto* chkVSync = ui.CreateElement("chk_vsync", UIElementType::Checkbox);
    chkVSync->text = ""; 
    chkVSync->isChecked = curVSync;
    chkVSync->position = {popupX + 25.0f, popupY + 304.0f};
    chkVSync->size = {20.0f, 20.0f};
    chkVSync->normalColor = sf::Color(18, 19, 23);
    chkVSync->borderColor = sf::Color(55, 60, 75);
    chkVSync->borderThickness = 1.0f;
    chkVSync->textColor = sf::Color(46, 204, 113);
    chkVSync->zIndex = 12;
    chkVSync->UpdateDrawables();

    auto* lblVSync = ui.CreateElement("lbl_vsync", UIElementType::Text);
    lblVSync->text = "Enable VSync";
    lblVSync->characterSize = 13;
    lblVSync->size = {180.0f, 20.0f};
    lblVSync->textColor = sf::Color(215, 220, 230);
    lblVSync->position = {popupX + 54.0f, popupY + 305.0f};
    lblVSync->zIndex = 12;
    lblVSync->UpdateDrawables();

    auto* lblFPS = ui.CreateElement("lbl_fps", UIElementType::Text);
    lblFPS->text = "Target FPS:";
    lblFPS->characterSize = 13;
    lblFPS->size = {90.0f, 20.0f};
    lblFPS->textColor = sf::Color(215, 220, 230);
    lblFPS->position = {popupX + 280.0f, popupY + 305.0f};
    lblFPS->zIndex = 12;
    lblFPS->UpdateDrawables();

    auto* txtFPS = ui.CreateElement("txt_fps", UIElementType::TextInput);
    txtFPS->text = curFps;
    txtFPS->characterSize = 14;
    txtFPS->textColor = sf::Color::White;
    txtFPS->position = {popupX + 375.0f, popupY + 299.0f};
    txtFPS->size = {140.0f, 30.0f};
    txtFPS->normalColor = sf::Color(18, 19, 23);
    txtFPS->borderColor = sf::Color(55, 60, 75);
    txtFPS->borderThickness = 1.0f;
    txtFPS->textOffset = {8.0f, 4.0f};
    txtFPS->zIndex = 12;
    txtFPS->UpdateDrawables();

    auto* divider = ui.CreateElement("hub_divider", UIElementType::Panel);
    divider->position = {popupX + 25.0f, popupY + 355.0f};
    divider->size = {490.0f, 1.0f};
    divider->color = sf::Color(45, 48, 58);
    divider->zIndex = 11;
    divider->UpdateDrawables();

    auto* btnCancel = ui.CreateElement("btn_cancel", UIElementType::Button);
    btnCancel->text = "Quit";
    btnCancel->characterSize = 13;
    btnCancel->position = {popupX + 25.0f, popupY + 435.0f};
    btnCancel->size = {120.0f, 40.0f};
    btnCancel->normalColor = sf::Color(48, 51, 60);
    btnCancel->hoverColor = sf::Color(65, 69, 82);
    btnCancel->pressedColor = sf::Color(35, 37, 44);
    btnCancel->textColor = sf::Color(220, 222, 230);
    btnCancel->zIndex = 12;
    btnCancel->UpdateDrawables();

    auto* btnCreate = ui.CreateElement("btn_create", UIElementType::Button);
    btnCreate->text = "Create Project & Start";
    btnCreate->characterSize = 13;
    btnCreate->position = {popupX + 160.0f, popupY + 435.0f};
    btnCreate->size = {355.0f, 40.0f};
    btnCreate->normalColor = sf::Color(36, 140, 75);
    btnCreate->hoverColor = sf::Color(44, 168, 90);
    btnCreate->pressedColor = sf::Color(26, 105, 55);
    btnCreate->textColor = sf::Color::White;
    btnCreate->zIndex = 12;
    btnCreate->UpdateDrawables();
}

void ProjectHubScene::Update(float deltaTime)
{
    static bool wasClicked = false;
    bool mouseClicked = sf::Mouse::isButtonPressed(sf::Mouse::Left);
    bool justClicked = mouseClicked && !wasClicked;
    bool justReleased = !mouseClicked && wasClicked;
    wasClicked = mouseClicked;

    sf::Vector2i pixelPos = sf::Mouse::getPosition(m_Window);
    sf::Vector2f mousePos = m_Window.mapPixelToCoords(pixelPos, m_Window.getDefaultView());

    UIManager::Get().Update(deltaTime, mousePos, justClicked, justReleased);

    if (UIManager::Get().IsButtonClicked("btn_cancel"))
    {
        UIManager::Get().ClearClickedButton();
        if (g_App) g_App->Quit();
        return;
    }

    if (UIManager::Get().IsButtonClicked("btn_create"))
    {
        UIManager::Get().ClearClickedButton();
        CreateProject();
    }
}

void ProjectHubScene::HandleEvent(const sf::Event &event)
{
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
    {
        if (g_App) g_App->Quit();
    }
    else if (event.type == sf::Event::Resized)
    {
        sf::FloatRect visibleArea(0.f, 0.f, static_cast<float>(event.size.width), static_cast<float>(event.size.height));
        m_Window.setView(sf::View(visibleArea));
        SetupUI();
    }
}

void ProjectHubScene::Render(sf::RenderWindow &window)
{
    if (Scene* editor = m_manager.GetScene("editor"))
    {
        editor->Render(window);
    }
    else
    {
        window.clear(sf::Color(18, 20, 23));
    }

    window.setView(window.getDefaultView());

    UIManager::Get().Render(window);
}

void ProjectHubScene::CreateProject()
{
    auto& ui = UIManager::Get();
    std::string pName = ui.GetElement("txt_projName") ? ui.GetElement("txt_projName")->text : "Project";
    std::string pAuthor = ui.GetElement("txt_author") ? ui.GetElement("txt_author")->text : "";
    std::string pWidthStr = ui.GetElement("txt_width") ? ui.GetElement("txt_width")->text : "1280";
    std::string pHeightStr = ui.GetElement("txt_height") ? ui.GetElement("txt_height")->text : "720";
    std::string pFpsStr = ui.GetElement("txt_fps") ? ui.GetElement("txt_fps")->text : "60";
    bool vSync = ui.GetElement("chk_vsync") ? ui.GetElement("chk_vsync")->isChecked : true;

    int width = 1280;
    int height = 720;
    int fps = 60;
    try { width = std::stoi(pWidthStr); } catch(...) {}
    try { height = std::stoi(pHeightStr); } catch(...) {}
    try { fps = std::stoi(pFpsStr); } catch(...) {}

    nlohmann::json j;
    j["ProjectName"] = pName;
    j["Author"] = pAuthor;
    j["WindowWidth"] = width;
    j["WindowHeight"] = height;
    j["VSync"] = vSync;
    j["TargetFPS"] = fps;
    j["Version"] = "1.0.0";
    j["StartScene"] = "scenes/game.json";

    std::string dir = ENGINE_ASSET_PATH;
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }
    std::ofstream o(dir + "/project_settings.json");
    o << j.dump(4);
    o.close();

    std::string assetsDir = ASSET_PATH;
    std::filesystem::create_directories(assetsDir + "/scenes");
    std::filesystem::create_directories(assetsDir + "/scripts");

    if (g_App) {
        g_App->SetProjectName(pName);
        g_App->SetProjectAuthor(pAuthor);
        g_App->SetVSync(vSync);
        g_App->SetTargetFPS(fps);
    }

    std::cout << "[INFO] [ProjectHubScene] Project created, switching to editor...\n";
    m_manager.SwitchSceneTo("editor");
}
