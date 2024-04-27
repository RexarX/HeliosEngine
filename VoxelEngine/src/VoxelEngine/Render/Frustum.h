#pragma once

#include "Camera.h"

#include <glm/glm.hpp>

namespace VoxelEngine
{
  class Frustum
  {
  public:
    Frustum() = default;

    void CreateFrustum(const Camera& camera);

    bool IsParallelepipedInFrustrum(const glm::vec3& position, const glm::vec3& size) const;

  private:
    float m_Frustum[6][4];
    float m_Normalize;
  };
}