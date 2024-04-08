#include "Frustum.h"

namespace VoxelEngine
{
  void Frustum::CreateFrustum(const glm::mat4& viewProjection)
  {
    m_Frustum[0][0] = viewProjection[0][3] - viewProjection[0][0];
    m_Frustum[0][1] = viewProjection[1][3] - viewProjection[1][0];
    m_Frustum[0][2] = viewProjection[2][3] - viewProjection[2][0];
    m_Frustum[0][3] = viewProjection[3][3] - viewProjection[3][0];

    m_Normalize = sqrt(m_Frustum[0][0] * m_Frustum[0][0] + m_Frustum[0][1] * m_Frustum[0][1] +
      m_Frustum[0][2] * m_Frustum[0][2]);

    m_Frustum[0][0] /= m_Normalize;
    m_Frustum[0][1] /= m_Normalize;
    m_Frustum[0][2] /= m_Normalize;
    m_Frustum[0][3] /= m_Normalize;

    m_Frustum[0][0] = viewProjection[0][3] + viewProjection[0][0];
    m_Frustum[0][1] = viewProjection[1][3] + viewProjection[1][0];
    m_Frustum[1][2] = viewProjection[2][3] + viewProjection[2][0];
    m_Frustum[1][3] = viewProjection[3][3] + viewProjection[3][0];

    m_Normalize = sqrt(m_Frustum[1][0] * m_Frustum[1][0] + m_Frustum[1][1] * m_Frustum[1][1] +
      m_Frustum[1][2] * m_Frustum[1][2]);

    m_Frustum[1][0] /= m_Normalize;
    m_Frustum[1][1] /= m_Normalize;
    m_Frustum[1][2] /= m_Normalize;
    m_Frustum[1][3] /= m_Normalize;

    m_Frustum[2][0] = viewProjection[0][3] + viewProjection[0][1];
    m_Frustum[2][1] = viewProjection[1][3] + viewProjection[1][1];
    m_Frustum[2][2] = viewProjection[2][3] + viewProjection[2][1];
    m_Frustum[2][3] = viewProjection[3][3] + viewProjection[3][1];

    m_Normalize = sqrt(m_Frustum[2][0] * m_Frustum[2][0] + m_Frustum[2][1] * m_Frustum[2][1] +
      m_Frustum[2][2] * m_Frustum[2][2]);

    m_Frustum[2][0] /= m_Normalize;
    m_Frustum[2][1] /= m_Normalize;
    m_Frustum[2][2] /= m_Normalize;
    m_Frustum[2][3] /= m_Normalize;

    m_Frustum[3][0] = viewProjection[0][3] - viewProjection[0][1];
    m_Frustum[3][1] = viewProjection[1][3] - viewProjection[1][1];
    m_Frustum[3][2] = viewProjection[2][3] - viewProjection[2][1];
    m_Frustum[3][3] = viewProjection[3][3] - viewProjection[3][1];

    m_Normalize = sqrt(m_Frustum[3][0] * m_Frustum[3][0] + m_Frustum[3][1] * m_Frustum[3][1] +
      m_Frustum[3][2] * m_Frustum[3][2]);

    m_Frustum[3][0] /= m_Normalize;
    m_Frustum[3][1] /= m_Normalize;
    m_Frustum[3][2] /= m_Normalize;
    m_Frustum[3][3] /= m_Normalize;

    m_Frustum[4][0] = viewProjection[0][3] - viewProjection[0][2];
    m_Frustum[4][1] = viewProjection[1][3] - viewProjection[1][2];
    m_Frustum[4][2] = viewProjection[2][3] - viewProjection[2][2];
    m_Frustum[4][3] = viewProjection[3][3] - viewProjection[3][2];

    m_Normalize = sqrt(m_Frustum[4][0] * m_Frustum[4][0] + m_Frustum[4][1] * m_Frustum[4][1] +
      m_Frustum[4][2] * m_Frustum[4][2]);

    m_Frustum[4][0] /= m_Normalize;
    m_Frustum[4][1] /= m_Normalize;
    m_Frustum[4][2] /= m_Normalize;
    m_Frustum[4][3] /= m_Normalize;

    m_Frustum[5][0] = viewProjection[0][3] + viewProjection[0][2];
    m_Frustum[5][1] = viewProjection[1][3] + viewProjection[1][2];
    m_Frustum[5][2] = viewProjection[2][3] + viewProjection[2][2];
    m_Frustum[5][3] = viewProjection[3][3] + viewProjection[3][2];

    m_Normalize = sqrt(m_Frustum[5][0] * m_Frustum[5][0] + m_Frustum[5][1] * m_Frustum[5][1] +
      m_Frustum[5][2] * m_Frustum[5][2]);

    m_Frustum[5][0] /= m_Normalize;
    m_Frustum[5][1] /= m_Normalize;
    m_Frustum[5][2] /= m_Normalize;
    m_Frustum[5][3] /= m_Normalize;
  }

  bool Frustum::IsCubeInFrustrum(const float size, const glm::vec3& position)
  {
    for (int i = 0; i < 6; ++i) {
      if (m_Frustum[i][0] * (position.x - size) + m_Frustum[i][1] * (position.y - size) +
        m_Frustum[i][2] * (position.z - size) + m_Frustum[i][3] > 0) {
        return true;
      }

      if (m_Frustum[i][0] * (position.x + size) + m_Frustum[i][1] * (position.y - size) +
        m_Frustum[i][2] * (position.z - size) + m_Frustum[i][3] > 0) {
        return true;
      }

      if (m_Frustum[i][0] * (position.x - size) + m_Frustum[i][1] * (position.y + size) +
      m_Frustum[i][2] * (position.z - size) + m_Frustum[i][3] > 0) {
        return true;
      }

      if (m_Frustum[i][0] * (position.x + size) + m_Frustum[i][1] * (position.y + size) +
      m_Frustum[i][2] * (position.z - size) + m_Frustum[i][3] > 0) {
        return true;
      }

      if (m_Frustum[i][0] * (position.x - size) + m_Frustum[i][1] * (position.y - size) +
      m_Frustum[i][2] * (position.z + size) + m_Frustum[i][3] > 0) {
        return true;
      }

      if (m_Frustum[i][0] * (position.x + size) + m_Frustum[i][1] * (position.y - size) +
      m_Frustum[i][2] * (position.z + size) + m_Frustum[i][3] > 0) {
        return true;
      }

      if (m_Frustum[i][0] * (position.x - size) + m_Frustum[i][1] * (position.y + size) +
      m_Frustum[i][2] * (position.z + size) + m_Frustum[i][3] > 0) {
        return true;
      }

      else if (m_Frustum[i][0] * (position.x + size) + m_Frustum[i][1] * (position.y + size) +
      m_Frustum[i][2] * (position.z + size) + m_Frustum[i][3] > 0) {
        return true;
      }
    }
    return false;
  }
}