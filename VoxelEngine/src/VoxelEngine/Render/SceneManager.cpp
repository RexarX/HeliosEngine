#include "SceneManager.h"

namespace VoxelEngine
{
  std::string SceneManager::m_ActiveScene = std::string();
  std::unordered_map<std::string, Scene> SceneManager::m_Scenes = std::unordered_map<std::string, Scene>();

  void SceneManager::AddScene(const std::string& name)
  {
    if (m_Scenes.contains(name)) {
      CORE_ASSERT(false, "Scene already with that name exists!");
      return;
    }

    m_Scenes.insert(std::make_pair(name, Scene(name)));
  }

  void SceneManager::AddScene(const Scene& scene, const std::string& name)
  {
    if (name.empty()) {
      if (m_Scenes.contains(scene.GetName())) {
        CORE_ASSERT(false, "Scene already with that name exists!");
        return;
      }

      m_Scenes.emplace(scene.GetName(), scene);
    } else {
      if (m_Scenes.contains(name)) {
        CORE_ASSERT(false, "Scene already with that name exists!");
        return;
      }

      m_Scenes.emplace(name, scene);
    }
  }

  void SceneManager::SetActiveScene(const std::string& name)
  {
    if (!m_Scenes.contains(name)) {
      CORE_ASSERT(false, "Scene not found!");
      return;
    }

    m_Scenes[m_ActiveScene].SetActive(false);
    m_Scenes[name].SetActive(true);

    m_ActiveScene = name;
  }

  const Scene& SceneManager::GetScene(const std::string& name)
  {
    if (!m_Scenes.contains(name)) {
      CORE_ASSERT(false, "Scene not found!");
    }

    return m_Scenes[name];
  }

  inline Scene& SceneManager::GetActiveScene()
  {
    if (m_ActiveScene.empty()) {
      CORE_ERROR("No active scene, picking first scene!");

      if (m_Scenes.size() > 0) {
        m_Scenes.begin()->second.SetActive(true);
        m_ActiveScene = m_Scenes.begin()->first;
      } else {
        CORE_ERROR("No scenes in scene manager, adding default scene and activating it!");
        m_Scenes.insert(std::make_pair("default", Scene("default")));
      }
    }

    return m_Scenes[m_ActiveScene];
  }
}