#pragma once

#include "Scene.h"

namespace VoxelEngine
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

    static inline Scene& GetActiveScene();

  private:
    static std::unordered_map<std::string, Scene> m_Scenes;

    static std::string m_ActiveScene;
  };
}