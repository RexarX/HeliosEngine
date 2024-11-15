#pragma once

#include "pch.h"

#include <glm/glm.hpp>

namespace Helios {
  class HELIOSENGINE_API SceneCamera {
  public:
    enum class Projection {
      Perspective,
      Orthographic
    };

    SceneCamera() = default;
    ~SceneCamera() = default;

    void SetProjectionMatrix(const glm::mat4& projection) { m_ProjectionMatrix = projection; }
    void SetViewMatrix(const glm::mat4& view) { m_ViewMatrix = view; }

    void SetLeft(const glm::vec3& left) { m_Left = left; }
    void SetUp(const glm::vec3& up) { m_Up = up; }
    void SetForward(const glm::vec3& forward) { m_Forward = forward; }
    void SetDirection(const glm::vec3& direction) { m_Direction = direction; }

    void SetFOV(float fov) { m_Fov = glm::radians(fov); }
    void SetNearDistance(float nearDistance) { m_NearDistance = nearDistance; }
    void SetFarDistance(float farDistance) { m_FarDistance = farDistance; }
    void SetAspectRatio(float aspectRatio) { m_AspectRatio = aspectRatio; }

    inline Projection GetProjectionType() const { return m_Projection; }

    inline const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
    inline const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }

    inline const glm::vec3& GetLeft() const { return m_Left; }
    inline const glm::vec3& GetUp() const { return m_Up; }
    inline const glm::vec3& GetForward() const { return m_Forward; }
    inline const glm::vec3& GetDirection() const { return m_Direction; }

    inline float GetFOV() const { return m_Fov; }
    inline float GetNearDistance() const { return m_NearDistance; }
    inline float GetFarDistance() const { return m_FarDistance; }
    inline float GetAspectRatio() const { return m_AspectRatio; }

  private:
    Projection m_Projection = Projection::Perspective;

    glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);
    glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
    
    glm::vec3 m_Left = glm::vec3(0.0f);
    glm::vec3 m_Up = glm::vec3(0.0f);
    glm::vec3 m_Forward = glm::vec3(0.0f);
    glm::vec3 m_Direction = glm::vec3(0.0f);

    float m_Fov = glm::radians(45.0f);
    float m_NearDistance = 0.05f;
    float m_FarDistance = 4000.0f;
    float m_AspectRatio = 16.0f / 9.0f;
  };
}