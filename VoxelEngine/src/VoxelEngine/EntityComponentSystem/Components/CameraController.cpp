#include "EntityComponentSystem/Components/CameraController.h"

#include "EntityComponentSystem/Manager/ECSManager.h"

#include "KeyCodes.h"

#include <glm/glm.hpp>

namespace Engine
{
  CameraController::CameraController(ECSManager& manager, Camera& camera)
    : m_Camera(camera), m_Manager(manager)
  {
    EventSystem& eventSystem = manager.GetSystem<EventSystem>();
    eventSystem.AddListener<MouseMovedAction>(this, BIND_EVENT_FN(CameraController::OnMouseMoved));
    eventSystem.AddListener<AppUpdateEvent>(this, BIND_EVENT_FN(CameraController::OnUpdate));
  }

  CameraController::~CameraController()
  {
    EventSystem& eventSystem = m_Manager.GetSystem<EventSystem>();
    eventSystem.RemoveListener<MouseMovedAction>(this);
    eventSystem.RemoveListener<AppUpdateEvent>(this);
  }

  void CameraController::OnUpdate(AppUpdateEvent& event)
  {
    UpdateFreeRoam(event.GetDeltaTime());

    m_Camera.SetPosition(m_Position);
    m_Camera.SetRotation(GetRotation());
  }

  void CameraController::OnMouseMoved(MouseMovedAction& action)
  {
    m_Yaw += action.GetDeltaX() * m_CameraRotationSpeed;
    m_Pitch += action.GetDeltaY() * m_CameraRotationSpeed;

    if (m_Pitch > 89.0f) { m_Pitch = 89.0f; }
    else if (m_Pitch < -89.0f) { m_Pitch = -89.0f; }
  }

  void CameraController::OnMouseButtonPressed(MouseButtonPressedAction& action)
  {
  }

  void CameraController::OnMouseButtonReleased(MouseButtonReleasedAction& action)
  {
  }

  void CameraController::OnKeyPressed(KeyPressedAction& action)
  {
  }

  void CameraController::OnKeyReleased(KeyReleasedAction& action)
  {
  }

  void CameraController::UpdateFreeRoam(const Timestep ts)
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
}