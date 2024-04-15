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

    bool IsCubeInFrustrum(const float size, const glm::vec3& position) const;

    float Get(const int32_t i, const int32_t j) const { return m_Frustum[i][j]; }

  private:
    float m_Frustum[6][4];
    float m_Normalize;
  };
}