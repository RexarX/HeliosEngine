#include "EntityComponentSystem/Components/CameraControllerComponent.h"
#include "EntityComponentSystem/Components/TransformComponent.h"

#include "EntityComponentSystem/Manager/ECSManager.h"

#include "Input.h"
#include "KeyCodes.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
  CameraControllerComponent::CameraControllerComponent(const CameraControllerType type)
    : m_Type(type), m_FollowedEntity(-1)
  {
  }

  void CameraControllerComponent::OnUpdate(const Timestep ts, ECSManager& manager, const EntityID entity)
  {
    if (m_Type == CameraControllerType::FreeRoam) { UpdateFreeRoam(ts); }
    else { UpdateFollowEntity(ts, manager, entity); }
  }

  void CameraControllerComponent::OnMouseMoved(const glm::vec2 mousePos, const glm::vec2 mouseOffset)
  {
    m_Yaw += mouseOffset.x * m_CameraRotationSpeed;
    m_Pitch += mouseOffset.y * m_CameraRotationSpeed;

    if (m_Pitch > 89.0f) { m_Pitch = 89.0f; }
    else if (m_Pitch < -89.0f) { m_Pitch = -89.0f; }
  }

  void CameraControllerComponent::OnKeyPressed(const KeyCode key)
  {
  }

  void CameraControllerComponent::UpdateFreeRoam(const Timestep ts)
  {
    glm::vec3 front;
    front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.y = sin(glm::radians(m_Pitch));
    front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front = glm::normalize(front);

    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, front));

    if (Input::IsKeyPressed(Key::W)) {
      m_Position += front * m_CameraTranslationSpeed * (float)ts;
    }

    if (Input::IsKeyPressed(Key::S)) {
      m_Position -= front * m_CameraTranslationSpeed * (float)ts;
    }

    if (Input::IsKeyPressed(Key::A)) {
      m_Position -= right * m_CameraTranslationSpeed * (float)ts;
    }

    if (Input::IsKeyPressed(Key::D)) {
      m_Position += right * m_CameraTranslationSpeed * (float)ts;
    }

    if (Input::IsKeyPressed(Key::Space)) {
      m_Position += up * m_CameraTranslationSpeed * (float)ts;
    }

    if (Input::IsKeyPressed(Key::LeftShift)) {
      m_Position -= up * m_CameraTranslationSpeed * (float)ts;
    }
  }

  void CameraControllerComponent::UpdateFollowEntity(const Timestep ts, ECSManager& manager, const EntityID entity)
  {
    if (m_FollowedEntity == -1) { return; }

    glm::vec3 entityPosition = manager.GetComponent<TransformComponent>(m_FollowedEntity)->position;

    glm::vec3 offset = glm::vec3(0.0f, 2.0f, -5.0f);

    float yaw = glm::radians(m_Yaw);
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec3 rotatedOffset = glm::vec3(rotation * glm::vec4(offset, 0.0f));

    m_Position = entityPosition + rotatedOffset;
  }
}