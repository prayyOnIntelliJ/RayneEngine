#include "SceneManager.h"

SceneManager::SceneManager()
{
    m_CurrentScene = nullptr;
}

Scene* SceneManager::GetCurrentScene() const
{
    return this->m_CurrentScene;
}

void SceneManager::AddScene(std::unique_ptr<Scene> newScene)
{
    m_Scenes.push_back(std::move(newScene));
}

void SceneManager::SetSceneByName(const std::string &sceneName)
{
    this->m_CurrentScene = GetSceneByName(sceneName);
}

void SceneManager::SetSceneByReference(Scene *newScene)
{
    this->m_CurrentScene = newScene;
}

void SceneManager::SetSceneByIndex(int index)
{
    this->m_CurrentScene = m_Scenes[index].get();
}

Scene* SceneManager::GetSceneByName(std::string sceneName) const
{
    for (auto& scene : m_Scenes)
    {
        if (scene->m_Name == sceneName)
            return scene.get();
    }

    return nullptr;
}
