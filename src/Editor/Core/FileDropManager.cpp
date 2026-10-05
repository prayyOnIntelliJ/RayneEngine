#include "FileDropManager.h"
#include <iostream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

static FileDropManager *s_Instance = nullptr;

static LRESULT CALLBACK FileDropSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_DROPFILES)
    {
        HDROP hDrop = reinterpret_cast<HDROP>(wParam);
        POINT pt;
        DragQueryPoint(hDrop, &pt);

        UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
        std::vector<std::string> files;
        files.reserve(fileCount);

        for (UINT i = 0; i < fileCount; ++i)
        {
            UINT len = DragQueryFileW(hDrop, i, NULL, 0);
            if (len > 0)
            {
                std::wstring wpath(len + 1, L'\0');
                DragQueryFileW(hDrop, i, &wpath[0], len + 1);
                wpath.resize(len);
                files.push_back(std::filesystem::path(wpath).string());
            }
        }
        DragFinish(hDrop);

        if (s_Instance)
        {
            s_Instance->OnDropFiles(files, sf::Vector2f(static_cast<float>(pt.x), static_cast<float>(pt.y)));
        }
        return 0;
    }

    if (s_Instance && s_Instance->GetOriginalWndProc())
    {
        return CallWindowProc(reinterpret_cast<WNDPROC>(s_Instance->GetOriginalWndProc()), hwnd, uMsg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}
#endif

FileDropManager &FileDropManager::Get()
{
    static FileDropManager instance;
    return instance;
}

FileDropManager::~FileDropManager()
{
    Shutdown();
}

void FileDropManager::Init(sf::WindowHandle handle)
{
    m_WindowHandle = handle;
#ifdef _WIN32
    s_Instance = this;
    HWND hwnd = reinterpret_cast<HWND>(handle);
    if (hwnd)
    {
        DragAcceptFiles(hwnd, TRUE);
        ChangeWindowMessageFilterEx(hwnd, WM_DROPFILES, MSGFLT_ALLOW, NULL);
        ChangeWindowMessageFilterEx(hwnd, WM_COPYDATA, MSGFLT_ALLOW, NULL);
        ChangeWindowMessageFilterEx(hwnd, 0x0049 /* WM_COPYGLOBALDATA */, MSGFLT_ALLOW, NULL);

        if (!m_OriginalWndProc)
        {
            m_OriginalWndProc = reinterpret_cast<void *>(SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(FileDropSubclassProc)));
        }
    }
#endif
}

void FileDropManager::Shutdown()
{
#ifdef _WIN32
    if (m_WindowHandle && m_OriginalWndProc)
    {
        HWND hwnd = reinterpret_cast<HWND>(m_WindowHandle);
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_OriginalWndProc));
        m_OriginalWndProc = nullptr;
    }
    s_Instance = nullptr;
#endif
    m_Callback = nullptr;
}

void FileDropManager::SetDropCallback(DropCallback cb)
{
    m_Callback = std::move(cb);
}

void FileDropManager::ClearDropCallback()
{
    m_Callback = nullptr;
}

void FileDropManager::OnDropFiles(const std::vector<std::string> &files, sf::Vector2f pos)
{
    if (m_Callback && !files.empty())
    {
        m_Callback(files, pos);
    }
}
