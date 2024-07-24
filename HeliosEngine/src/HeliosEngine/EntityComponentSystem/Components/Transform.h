#pragma once

#include <glm/glm.hpp>

namespace Helios
{
  struct HELIOSENGINE_API Transform
  {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
  };
}