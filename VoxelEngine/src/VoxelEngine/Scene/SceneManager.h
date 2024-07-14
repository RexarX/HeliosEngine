#pragma once

#include "Scene/Scene.h"

namespace Engine
{
  class VOXELENGINE_API SceneManager
  {
  public:
    SceneManager() = default;
    ~SceneManager() = default;

    static void AddScene(const std::string& name);
    static void AddScene(const Scene& scene, const std::string& name = std::string());

    static void SetActiveScene(const std::string& name);

    static const Scene& GetScene(const std::string& name);

    static Scene& GetActiveScene();

  private:
    static std::map<std::string, Scene> m_Scenes;

    static std::string m_ActiveScene;
  };
}