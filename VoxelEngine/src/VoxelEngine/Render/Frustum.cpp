#include "Frustum.h"

#include "vepch.h"

#include "Renderer.h"

#include <glm/glm.hpp>

namespace VoxelEngine
{
  void Frustum::CreateFrustum(const Camera& camera)
  {
    glm::mat4 viewProjectionModel = camera.GetProjectionViewModelMatrix();

    //right plane
    m_Frustum[0][0] = viewProjectionModel[0][3] - viewProjectionModel[0][0];
    m_Frustum[0][1] = viewProjectionModel[1][3] - viewProjectionModel[1][0];
    m_Frustum[0][2] = viewProjectionModel[2][3] - viewProjectionModel[2][0];
    m_Frustum[0][3] = viewProjectionModel[3][3] - viewProjectionModel[3][0];

    //left plane
    m_Frustum[1][0] = viewProjectionModel[0][3] + viewProjectionModel[0][0];
    m_Frustum[1][1] = viewProjectionModel[1][3] + viewProjectionModel[1][0];
    m_Frustum[1][2] = viewProjectionModel[2][3] + viewProjectionModel[2][0];
    m_Frustum[1][3] = viewProjectionModel[3][3] + viewProjectionModel[3][0];

    //bottom plane
    m_Frustum[2][0] = viewProjectionModel[0][3] + viewProjectionModel[0][1];
    m_Frustum[2][1] = viewProjectionModel[1][3] + viewProjectionModel[1][1];
    m_Frustum[2][2] = viewProjectionModel[2][3] + viewProjectionModel[2][1];
    m_Frustum[2][3] = viewProjectionModel[3][3] + viewProjectionModel[3][1];

    //top plane
    m_Frustum[3][0] = viewProjectionModel[0][3] - viewProjectionModel[0][1];
    m_Frustum[3][1] = viewProjectionModel[1][3] - viewProjectionModel[1][1];
    m_Frustum[3][2] = viewProjectionModel[2][3] - viewProjectionModel[2][1];
    m_Frustum[3][3] = viewProjectionModel[3][3] - viewProjectionModel[3][1];

    //far plane
    m_Frustum[4][0] = viewProjectionModel[0][3] - viewProjectionModel[0][2];
    m_Frustum[4][1] = viewProjectionModel[1][3] - viewProjectionModel[1][2];
    m_Frustum[4][2] = viewProjectionModel[2][3] - viewProjectionModel[2][2];
    m_Frustum[4][3] = viewProjectionModel[3][3] - viewProjectionModel[3][2];

    //near plane
    m_Frustum[5][0] = viewProjectionModel[0][2];
    m_Frustum[5][1] = viewProjectionModel[1][2];
    m_Frustum[5][2] = viewProjectionModel[2][2];
    m_Frustum[5][3] = viewProjectionModel[3][2];

    //normalization
    for (int32_t i = 0; i < 6; ++i) {
      m_Normalize = sqrt(m_Frustum[i][0] * m_Frustum[i][0] + m_Frustum[i][1] * m_Frustum[i][1] + m_Frustum[i][2] * m_Frustum[i][2]);

      m_Frustum[i][0] /= m_Normalize;
      m_Frustum[i][1] /= m_Normalize;
      m_Frustum[i][2] /= m_Normalize;
      m_Frustum[i][3] /= m_Normalize;
    }
  }

  bool Frustum::IsCubeInFrustrum(const float size, const glm::vec3& position) const
  {
    int32_t point_cnt = 0;
    int32_t plane_cnt = 0;

    for (int32_t i = 0; i < 6; ++i) {
      point_cnt = 0;
      if (m_Frustum[i][0] * (position.x - size) + m_Frustum[i][1] * (position.y - size) +
        m_Frustum[i][2] * (position.z - size) + m_Frustum[i][3] >= 0) {
        ++point_cnt;
      }
      if (m_Frustum[i][0] * (position.x - size) + m_Frustum[i][1] * (position.y - size) +
        m_Frustum[i][2] * (position.z - size) + m_Frustum[i][3] >= 0) {
        ++point_cnt;
      }

      if (m_Frustum[i][0] * (position.x + size) + m_Frustum[i][1] * (position.y - size) +
        m_Frustum[i][2] * (position.z - size) + m_Frustum[i][3] >= 0) {
        ++point_cnt;
      }

      if (m_Frustum[i][0] * (position.x - size) + m_Frustum[i][1] * (position.y + size) +
        m_Frustum[i][2] * (position.z - size) + m_Frustum[i][3] >= 0) {
        ++point_cnt;
      }

      if (m_Frustum[i][0] * (position.x + size) + m_Frustum[i][1] * (position.y + size) +
        m_Frustum[i][2] * (position.z - size) + m_Frustum[i][3] >= 0) {
        ++point_cnt;
      }

      if (m_Frustum[i][0] * (position.x - size) + m_Frustum[i][1] * (position.y - size) +
        m_Frustum[i][2] * (position.z + size) + m_Frustum[i][3] >= 0) {
        ++point_cnt;
      }

      if (m_Frustum[i][0] * (position.x + size) + m_Frustum[i][1] * (position.y - size) +
        m_Frustum[i][2] * (position.z + size) + m_Frustum[i][3] >= 0) {
        ++point_cnt;
      }

      if (m_Frustum[i][0] * (position.x - size) + m_Frustum[i][1] * (position.y + size) +
        m_Frustum[i][2] * (position.z + size) + m_Frustum[i][3] >= 0) {
        ++point_cnt;
      }

      if (m_Frustum[i][0] * (position.x + size) + m_Frustum[i][1] * (position.y + size) +
        m_Frustum[i][2] * (position.z + size) + m_Frustum[i][3] >= 0) {
        ++point_cnt;
      }

      if (point_cnt != 0) { ++plane_cnt; }
      else { return false; }
    }
    return plane_cnt == 6;
  }
}