#include "SceneManager.h"

#include "Scene.h"

namespace Helios
{
  std::map<std::string, Scene> SceneManager::m_Scenes;

  Scene& SceneManager::AddScene(const std::string& name)
  {
    if (m_Scenes.contains(name)) { CORE_ASSERT(false, "Scene already exists!"); return m_Scenes[name]; }
    return m_Scenes.emplace(name, name).first->second;
  }

  void SceneManager::RemoveScene(const std::string& name)
  {
    if (!m_Scenes.contains(name)) { CORE_ASSERT(false, "Scene not found!"); return; }
    m_Scenes.erase(name);
  }

  void SceneManager::RemoveScene(const Scene& scene)
  {
    std::string name(scene.GetName());
    if (!m_Scenes.contains(name)) { CORE_ASSERT(false, "Scene not found!"); return; }
    m_Scenes.erase(name);
  }

  Scene& SceneManager::GetScene(const std::string& name)
  {
    if (!m_Scenes.contains(name)) { CORE_ASSERT(false, "Scene not found!"); }
    return m_Scenes.at(name);
  }

  Scene& SceneManager::GetActiveScene()
  {
    for (auto& scene : m_Scenes) {
      if (scene.second.IsActive()) { return scene.second; }
    }

    CORE_ASSERT(false, "No active scene found!");
  }
}