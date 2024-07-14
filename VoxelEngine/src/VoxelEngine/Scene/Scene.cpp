#include "Scene.h"

namespace Engine
{
  Scene::Scene()
    : m_Name("default")
  {
  }

  Scene::Scene(const std::string& name)
    : m_Name(name)
  {
  }
}