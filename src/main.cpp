#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <filesystem>
#include "Core/Application/Application.h"
#include "Core/Scenes/ConsolePanel.h"

int main()
{
#ifdef _WIN32
    char exePathBuf[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePathBuf, MAX_PATH)) {
        std::filesystem::path exeDir = std::filesystem::path(exePathBuf).parent_path();
        if (!std::filesystem::exists("assets") && std::filesystem::exists(exeDir / "assets")) {
            std::filesystem::current_path(exeDir);
        }
    }
#endif

    ConsolePanel::InitRedirectors();
    
    Application application;
    
    application.Run();
}