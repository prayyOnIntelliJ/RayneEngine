#include "SceneManager.h"

SceneManager::SceneManager()
{
    currentScene = nullptr;
}

Scene* SceneManager::GetCurrentScene() const
{
    return this->currentScene;
}

void SceneManager::AddScene(std::unique_ptr<Scene> newScene)
{
    scenes.push_back(std::move(newScene));
}

void SceneManager::SetSceneByName(const std::string &sceneName)
{
    this->currentScene = GetSceneByName(sceneName);
}

void SceneManager::SetSceneByReference(Scene *newScene)
{
    this->currentScene = newScene;
}

Scene* SceneManager::GetSceneByName(std::string sceneName) const
{
    for (auto& scene : scenes)
    {
        if (scene->name == sceneName)
            return scene.get();
    }

    return nullptr;
}
