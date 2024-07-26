#include "CameraController.h"

CameraController::CameraController(Helios::Camera& camera)
  : m_Camera(camera)
{
}

CameraController::~CameraController()
{
}

void CameraController::OnAttach()
{
}

void CameraController::OnDetach()
{
}

void CameraController::OnUpdate(const Helios::Timestep deltaTime)
{
  glm::vec3 front;
  front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
  front.y = sin(glm::radians(m_Pitch));
  front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
  front = glm::normalize(front);

  glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
  glm::vec3 up = glm::normalize(glm::cross(right, front));

  if (Helios::Input::IsKeyPressed(Helios::Key::W)) {
    m_Position += front * m_CameraTranslationSpeed * (float)deltaTime;
  }

  if (Helios::Input::IsKeyPressed(Helios::Key::S)) {
    m_Position -= front * m_CameraTranslationSpeed * (float)deltaTime;
  }

  if (Helios::Input::IsKeyPressed(Helios::Key::A)) {
    m_Position -= right * m_CameraTranslationSpeed * (float)deltaTime;
  }

  if (Helios::Input::IsKeyPressed(Helios::Key::D)) {
    m_Position += right * m_CameraTranslationSpeed * (float)deltaTime;
  }

  if (Helios::Input::IsKeyPressed(Helios::Key::Space)) {
    m_Position += up * m_CameraTranslationSpeed * (float)deltaTime;
  }

  if (Helios::Input::IsKeyPressed(Helios::Key::LeftShift)) {
    m_Position -= up * m_CameraTranslationSpeed * (float)deltaTime;
  }

  m_Camera.SetPosition(m_Position);
  m_Camera.SetRotation(GetRotation());
}

void CameraController::OnEvent(Helios::Event& event)
{
  Helios::EventDispatcher dispatcher(event);
  dispatcher.Dispatch<Helios::MouseMovedEvent>(BIND_EVENT_FN(CameraController::OnMouseMovedEvent));
}

const bool CameraController::OnMouseMovedEvent(Helios::MouseMovedEvent& event)
{
  if (m_FirstInput) {
    m_LastX = event.GetX();
    m_LastY = event.GetY();
    m_FirstInput = false;

    return true;
  }

  float x = event.GetX();
  float y = event.GetY();

  float deltaX = x - m_LastX;
  float deltaY = y - m_LastY;

  m_LastX = x;
  m_LastY = y;

  m_Yaw += deltaX * m_CameraRotationSpeed;
  m_Pitch += deltaY * m_CameraRotationSpeed;

  if (m_Pitch > 89.0f) { m_Pitch = 89.0f; }
  else if (m_Pitch < -89.0f) { m_Pitch = -89.0f; }

  return true;
}

const bool CameraController::OnWindowFocusedEvent(Helios::WindowFocusedEvent& event)
{
  m_FirstInput = true;
  return true;
}

const bool CameraController::OnWindowLostFocusEvent(Helios::WindowLostFocusEvent& event)
{
  m_FirstInput = true;
  return true;
}
