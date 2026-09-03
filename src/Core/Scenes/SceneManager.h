#ifndef RAYNEENGINE_SCENEMANAGER_H
#define RAYNEENGINE_SCENEMANAGER_H

#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include "Scene.h"

class Scene;

class SceneManager
{
public:
    template<typename T, typename... Args>
    void RegisterScene(const std::string &name, Args &&... args)
    {
        m_scenes[name] = std::make_unique<T>(*this, std::forward<Args>(args)...);
        std::cout << "[SceneManager] Register scene " << name << "\n";
    }

    void SwitchSceneTo(const std::string &name);

    void HandleEvent(const sf::Event &event);

    void Update(float dt);

    void Render(sf::RenderWindow &window);

    [[nodiscard]] bool HasScene(const std::string &name) const;

    [[nodiscard]] const std::string &CurrentName() const;

private:
    std::unordered_map<std::string, std::unique_ptr<Scene> > m_scenes;
    Scene *m_current = nullptr;
    std::string m_currentName;
};

#endif
