#ifndef RAYNEENGINE_FILEDROPMANAGER_H
#define RAYNEENGINE_FILEDROPMANAGER_H

#include <vector>
#include <string>
#include <functional>
#include <SFML/Window/WindowHandle.hpp>
#include <SFML/System/Vector2.hpp>

class FileDropManager
{
public:
    using DropCallback = std::function<void(const std::vector<std::string> &/*paths*/, sf::Vector2f /*mousePos*/)>;

    static FileDropManager &Get();

    void Init(sf::WindowHandle handle);
    void Shutdown();

    void SetDropCallback(DropCallback cb);
    void ClearDropCallback();

    void OnDropFiles(const std::vector<std::string> &files, sf::Vector2f pos);
    void *GetOriginalWndProc() const { return m_OriginalWndProc; }

private:
    FileDropManager() = default;
    ~FileDropManager();

    FileDropManager(const FileDropManager &) = delete;
    FileDropManager &operator=(const FileDropManager &) = delete;

    sf::WindowHandle m_WindowHandle = 0;
    DropCallback m_Callback;
    void *m_OriginalWndProc = nullptr;
};

#endif // RAYNEENGINE_FILEDROPMANAGER_H
