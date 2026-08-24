#include "SceneManager.h"

#include <iostream>

void SceneManager::SwitchSceneTo(const std::string &name)
{
    std::cout << "[INFO] [SceneManager] Requesting scene switch to '" << name << "'...\n";
    auto it = m_scenes.find(name);
    if (it == m_scenes.end()) { 
        std::cerr << "[ERROR] [SceneManager] Scene not found: " << name << "\n";
        throw std::runtime_error("[SceneManager] Scene not found: " + name); 
    }

    if (m_current)
    {
        std::cout << "[INFO] [SceneManager] Triggering OnExit() for current scene '" << m_currentName << "'\n";
        m_current->OnExit();
    }

    m_current = it->second.get();
    m_currentName = name;
    
    std::cout << "[INFO] [SceneManager] Triggering OnEnter() for new scene '" << m_currentName << "'\n";
    m_current->OnEnter();

    std::cout << "[INFO] [SceneManager] Scene switch to '" << m_currentName << "' completed successfully.\n";
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
