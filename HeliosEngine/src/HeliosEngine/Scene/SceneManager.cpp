#include "SceneManager.h"

#include "Scene.h"

namespace Helios {
  Scene& SceneManager::AddScene(const std::string& name) {
    if (m_Scenes.contains(name)) {
      CORE_ASSERT(false, "Scene already exists ('{}')!", name);
      return m_Scenes[name];
    }

    return m_Scenes.emplace(name, name).first->second;
  }

  void SceneManager::RemoveScene(const std::string& name) {
    if (!m_Scenes.contains(name)) {
      CORE_ASSERT(false, "Scene not found ('{}')!", name);
      return;
    }

    m_Scenes.erase(name);
  }

  void SceneManager::RemoveScene(const Scene& scene) {
    std::string name(scene.GetName());
    if (!m_Scenes.contains(name)) {
      CORE_ASSERT(false, "Scene not found ('{}')!", scene.GetName());
      return;
    }

    m_Scenes.erase(name);
  }

  Scene& SceneManager::GetScene(const std::string& name) {
    if (!m_Scenes.contains(name)) { CORE_ASSERT(false, "Scene not found ('{}')!", name); }
    return m_Scenes[name];
  }

  Scene& SceneManager::GetActiveScene() {
    for (auto& scene : m_Scenes) {
      if (scene.second.IsActive()) { return scene.second; }
    }

    CORE_ASSERT(false, "No active scene found!");
  }
}