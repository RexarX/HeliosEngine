#pragma once

#include <glm/glm.hpp>

namespace Engine
{
  struct VOXELENGINE_API TransformComponent
  {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
  };
}