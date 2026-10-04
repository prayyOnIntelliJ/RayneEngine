#pragma once
#include "EditorScene.h"
#include "../../Runtime/Scenes/SceneManager.h"
#include "../../Runtime/Application/Application.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <map>
#include <chrono>
#include <ctime>

#include "../../Runtime/ECS/Components.h"
#include "../../Runtime/Scripting/ScriptComponent.h"
#include "../../Runtime/Scripting/LuaState.h"
#include "../../Runtime/Resources/ResourceManager.h"
#include "../../Runtime/Audio/AudioManager.h"
#include "../../Runtime/UI/UIManager.h"
#include "../../Runtime/Application/EngineVersion.h"
#include "SFML/Window/Event.hpp"
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Window/Clipboard.hpp>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#undef CreateWindow
#pragma comment(lib, "comdlg32.lib")

inline bool OpenColorPickerDialog(sf::Color &ioColor, HWND hwnd = nullptr)
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

inline std::string OpenTemplateFileDialog(HWND hwnd = nullptr)
{
    char filename[MAX_PATH] = {0};
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "Rayne Template (*.template)\0*.template\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    std::filesystem::path curDir = std::filesystem::current_path();
    std::filesystem::path templatesDir = curDir / "assets" / "templates";
    std::string initDir = std::filesystem::exists(templatesDir) ? templatesDir.string() : (curDir / "assets").string();
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
#endif

inline std::filesystem::path GetAppDir()
{
#ifdef _WIN32
    char pathBuf[MAX_PATH];
    if (GetModuleFileNameA(NULL, pathBuf, MAX_PATH)) { return std::filesystem::path(pathBuf).parent_path(); }
#endif
    return std::filesystem::current_path();
}

inline std::filesystem::path FindProjectRoot()
{
    std::error_code ec;
    std::filesystem::path cur = std::filesystem::current_path(ec);
    std::filesystem::path appDir = GetAppDir();

    if (std::filesystem::exists(cur / "CMakeLists.txt") && std::filesystem::exists(cur / "assets")) return cur;
    if (std::filesystem::exists(cur.parent_path() / "CMakeLists.txt") && std::filesystem::exists(
            cur.parent_path() / "assets")) return cur.parent_path();

    if (std::filesystem::exists(appDir / "CMakeLists.txt") && std::filesystem::exists(appDir / "assets")) return appDir;
    if (std::filesystem::exists(appDir.parent_path() / "CMakeLists.txt") && std::filesystem::exists(
            appDir.parent_path() / "assets")) return appDir.parent_path();

    if (!ec && std::filesystem::exists(cur / "assets")) return cur;
    if (std::filesystem::exists(appDir / "assets")) return appDir;

    return cur;
}

inline void LaunchProcessDetached(const std::string &commandLine)
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

inline bool IsExecutableInPath(const std::string &name)
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

#ifdef _WIN32
inline void SetPathReadOnly(const std::filesystem::path &targetPath, bool recursive = true)
{
    std::error_code ec;
    if (!std::filesystem::exists(targetPath, ec)) return;

    DWORD attrs = GetFileAttributesA(targetPath.string().c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES)
    {
        SetFileAttributesA(targetPath.string().c_str(), attrs | FILE_ATTRIBUTE_READONLY);
    }

    if (recursive && std::filesystem::is_directory(targetPath, ec))
    {
        for (const auto &entry: std::filesystem::recursive_directory_iterator(targetPath, ec))
        {
            DWORD subAttrs = GetFileAttributesA(entry.path().string().c_str());
            if (subAttrs != INVALID_FILE_ATTRIBUTES)
            {
                SetFileAttributesA(entry.path().string().c_str(), subAttrs | FILE_ATTRIBUTE_READONLY);
            }
        }
    }
}
#endif

inline std::string FormatFloat(float value, int precision = 2)
{
    if (std::abs(value) < 1e-6f) value = 0.0f;
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", precision, value);
    return std::string(buf);
}


inline const sf::Color C_PANEL_BG = sf::Color(30, 32, 38);

inline ShapeType MapToShapeType(ObjectType type)
{
    switch (type)
    {
        case ObjectType::Circle:
        case ObjectType::PhysicsBall:
            return ShapeType::Circle;
        case ObjectType::Triangle: return ShapeType::Triangle;
        case ObjectType::Pentagon: return ShapeType::Pentagon;
        case ObjectType::Hexagon: return ShapeType::Hexagon;
        default: return ShapeType::Rectangle;
    }
}

inline bool IsPolygonType(ObjectType type)
{
    return type == ObjectType::Circle || type == ObjectType::PhysicsBall ||
           type == ObjectType::Triangle || type == ObjectType::Pentagon || type == ObjectType::Hexagon;
}

inline size_t GetPolygonPointCount(ObjectType type)
{
    switch (type)
    {
        case ObjectType::Triangle: return 3;
        case ObjectType::Pentagon: return 5;
        case ObjectType::Hexagon: return 6;
        case ObjectType::PhysicsBall:
        case ObjectType::Circle: default: return 30;
    }
}

namespace fs = std::filesystem;

inline const sf::Color C_BG_CANVAS = sf::Color(18, 20, 23);
inline const sf::Color C_BG_PANEL = sf::Color(26, 29, 34);
inline const sf::Color C_BG_ELEVATED = sf::Color(33, 37, 43);
inline const sf::Color C_BG_INPUT = sf::Color(20, 23, 27);
inline const sf::Color C_BORDER = sf::Color(42, 46, 53);
inline const sf::Color C_BORDER_LIGHT = sf::Color(58, 63, 72);
inline const sf::Color C_TEXT_PRIMARY = sf::Color(232, 234, 237);
inline const sf::Color C_TEXT_SECONDARY = sf::Color(154, 160, 172);
inline const sf::Color C_TEXT_MUTED = sf::Color(92, 97, 107);

inline const sf::Color C_ACCENT = sf::Color(124, 108, 240);
inline const sf::Color C_ACCENT_HOV = sf::Color(146, 132, 245);
inline const sf::Color C_ACCENT_ACT = sf::Color(100, 85, 217);
inline const sf::Color C_ACCENT_DIM = sf::Color(40, 35, 80, 200);
inline const sf::Color C_ACCENT_BRIGHT = sf::Color(146, 132, 245);

inline const sf::Color C_ACCENT2 = sf::Color(67, 217, 200);

inline const sf::Color C_SUCCESS = sf::Color(74, 222, 128);
inline const sf::Color C_SUCCESS_DIM = sf::Color(20, 55, 35, 200);
inline const sf::Color C_WARNING = sf::Color(245, 185, 77);
inline const sf::Color C_DANGER = sf::Color(241, 104, 94);
inline const sf::Color C_DANGER_DIM = sf::Color(70, 20, 18, 200);

inline const sf::Color C_GRID_MINOR = sf::Color(38, 43, 51);
inline const sf::Color C_GRID_MAJOR = sf::Color(51, 58, 69);


void DrawPill(sf::RenderWindow &window, sf::FloatRect r, sf::Color fill, sf::Color outline);

std::string GetInspectorTooltip(const std::string &key);

sf::Vector2f RotatePoint(sf::Vector2f point, sf::Vector2f center, float angleDegrees);

sf::Vector2f HandlePos(const EditorObject *obj, int idx);

std::string ResolveCMakeExecutable();

std::string ColorToHex(sf::Color c);

sf::Color HexToColor(const std::string &hex, sf::Color def = sf::Color(18, 20, 23));
