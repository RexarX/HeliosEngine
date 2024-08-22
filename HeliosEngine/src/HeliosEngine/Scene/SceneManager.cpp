#include "SceneManager.h"

namespace Helios
{
  std::string SceneManager::m_ActiveScene = std::string();
  std::map<std::string, Scene> SceneManager::m_Scenes;

  Scene& SceneManager::AddScene(const std::string& name)
  {
    CORE_ASSERT(!m_Scenes.contains(name), "Scene already with that name exists!");
    return m_Scenes.emplace(name, name).first->second;
  }

  void SceneManager::RemoveScene(const std::string& name)
  {
    if (!m_Scenes.contains(name)) { CORE_ASSERT(false, "Scene not found!"); return; }
    m_Scenes.erase(name);
  }

  void SceneManager::SetActiveScene(const std::string& name)
  {
    if (m_ActiveScene == name) { return; }
    if (!m_Scenes.contains(name)) { CORE_ASSERT(false, "Scene not found!"); return; }

    if (!m_ActiveScene.empty()) { m_Scenes[m_ActiveScene].SetActive(false); }
    m_Scenes[name].SetActive(true);
    m_ActiveScene = name;
  }

  Scene& SceneManager::GetScene(const std::string& name)
  {
    if (!m_Scenes.contains(name) || m_ActiveScene.empty()) { CORE_ASSERT(false, "Scene not found!"); }
    return m_Scenes[name];
  }

  Scene& SceneManager::GetActiveScene()
  {
    if(m_ActiveScene.empty()) { CORE_ASSERT(false, "No active scene!"); }
    return m_Scenes[m_ActiveScene];
  }
}