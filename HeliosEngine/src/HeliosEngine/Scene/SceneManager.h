#pragma once

#include "pch.h"

namespace Helios {
  class Scene;

  class HELIOSENGINE_API SceneManager {
  public:
    static Scene& AddScene(const std::string& name);
    static void RemoveScene(const std::string& name);
    static void RemoveScene(const Scene& scene);

    static Scene& GetScene(const std::string& name);
    static Scene& GetActiveScene();

  private:
    static inline std::map<std::string, Scene> m_Scenes;
  };
}