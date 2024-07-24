#pragma once

#include "pch.h"

#include <glm/glm.hpp>

namespace Helios
{
  struct Vertex
  {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
  };
  
  struct Mesh
  {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
  };
}