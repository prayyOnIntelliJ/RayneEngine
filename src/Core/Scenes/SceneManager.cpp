#include "SceneManager.h"

#include <iostream>

void SceneManager::SwitchSceneTo(const std::string &name)
{
    auto it = m_scenes.find(name);
    if (it == m_scenes.end()) { throw std::runtime_error("[SceneManager] Scene not found: " + name); }

    if (m_current)
    {
        m_current->OnExit();
        std::cout << "[SceneManager] Exit: " << m_currentName << "\n";
    }

    m_current = it->second.get();
    m_currentName = name;
    m_current->OnEnter();

    std::cout << "[SceneManager] Enter: " << m_currentName << "\n";
}

void SceneManager::HandleEvent(const sf::Event &event)
{
    if (m_current)
        m_current->HandleEvent(event);
}

void SceneManager::Update(float dt)
{
    if (m_current)
        m_current->Update(dt);
}

void SceneManager::Render(sf::RenderWindow &window)
{
    if (m_current)
        m_current->Render(window);
}

bool SceneManager::HasScene(const std::string &name) const { return m_scenes.contains(name); }

const std::string &SceneManager::CurrentName() const { return m_currentName; }
