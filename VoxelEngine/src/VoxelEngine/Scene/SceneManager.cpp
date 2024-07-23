#include "Scene/SceneManager.h"

namespace Engine
{
  std::string SceneManager::m_ActiveScene = std::string();
  std::map<std::string, Scene> SceneManager::m_Scenes = std::map<std::string, Scene>();

  void SceneManager::AddScene(const std::string& name)
  {
    if (m_Scenes.contains(name)) {
      CORE_ASSERT(false, "Scene already with that name exists!");
      return;
    }

    m_Scenes.emplace(name, Scene(name));
  }

  void SceneManager::EmplaceScene(Scene&& scene, const std::string& name)
  {
    if (name.empty()) {
      if (m_Scenes.contains(scene.GetName())) {
        CORE_ASSERT(false, "Scene already with that name exists!");
        return;
      }

      m_Scenes[scene.GetName()] = std::move(scene);
    } 
    else {
      if (m_Scenes.contains(name)) {
        CORE_ASSERT(false, "Scene already with that name exists!");
        return;
      }

      m_Scenes[name] = std::move(scene);
    }
  }

  void SceneManager::SetActiveScene(const std::string& name)
  {
    if (m_ActiveScene == name) {
      return;
    }

    if (!m_Scenes.contains(name)) {
      CORE_ASSERT(false, "Scene not found!");
      return;
    }

    if (!m_ActiveScene.empty()) {
      m_Scenes[m_ActiveScene].SetActive(false);
    }

    m_Scenes[name].SetActive(true);

    m_ActiveScene = name;
  }

  Scene& SceneManager::GetScene(const std::string& name)
  {
    if (!m_Scenes.contains(name)) {
      CORE_ASSERT(false, "Scene not found!");
    }

    return m_Scenes[name];
  }

  Scene& SceneManager::GetActiveScene()
  {
    if (m_ActiveScene.empty()) {
      CORE_ASSERT(false, "No active scene!");
    }

    return m_Scenes[m_ActiveScene];
  }
}