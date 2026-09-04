#pragma once
#include <string>

namespace Rayne
{
    constexpr int VERSION_MAJOR = 0;
    constexpr int VERSION_MINOR = 1;
    constexpr int VERSION_PATCH = 0;

    inline std::string VersionString()
    {
        return std::to_string(VERSION_MAJOR) + "."
               + std::to_string(VERSION_MINOR) + "."
               + std::to_string(VERSION_PATCH);
    }

    inline std::string PlatformString()
    {
#if defined(_WIN32) || defined(_WIN64)
        return "Windows";
#elif defined(__APPLE__)
        return "macOS";
#elif defined(__linux__)
        return "Linux";
#else
        return "Unknown";
#endif
    }

    constexpr const char *DEFAULT_PROJECT_NAME = "MyProject";
}
