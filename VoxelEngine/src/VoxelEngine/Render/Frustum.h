#pragma once

#include "Camera.h"

#include <glm/glm.hpp>

namespace VoxelEngine
{
  class Frustum
  {
  public:
    Frustum() = default;

    void CreateFrustum(const Camera& viewProjectionModel);

    bool IsCubeInFrustrum(const float size, const glm::vec3& position) const;

  private:
    float m_Frustum[6][4];
    float m_Normalize;
  };
}