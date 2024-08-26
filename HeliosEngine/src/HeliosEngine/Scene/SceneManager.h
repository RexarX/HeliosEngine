#pragma once

#include "Scene.h"

namespace Helios
{
  class HELIOSENGINE_API SceneManager
  {
  public:
    SceneManager() = default;
    ~SceneManager() = default;

    static Scene& AddScene(const std::string& name);
    static void RemoveScene(const std::string& name);
    static void RemoveScene(const Scene& scene);

    static Scene& GetScene(const std::string& name);
    static Scene& GetActiveScene();

  private:
    static std::map<std::string, Scene> m_Scenes;
  };
}