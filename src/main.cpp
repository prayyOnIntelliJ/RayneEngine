#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <filesystem>
#include "Runtime/Application/Application.h"
#include "Editor/Panels/ConsolePanel.h"

int main()
{
#ifdef _WIN32
    char exePathBuf[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePathBuf, MAX_PATH))
    {
        std::filesystem::path exeDir = std::filesystem::path(exePathBuf).parent_path();
        if (!std::filesystem::exists("assets") && std::filesystem::exists(exeDir / "assets"))
        {
            std::filesystem::current_path(exeDir);
        }
    }
#endif

    ConsolePanel::InitRedirectors();
    std::cout << "[INFO] [Main] RayneEngine launched.\n";

    Application application;

    application.Run();
    std::cout << "[INFO] [Main] RayneEngine process finished successfully.\n";
    return 0;
}
