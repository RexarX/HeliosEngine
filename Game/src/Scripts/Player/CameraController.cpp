#include "CameraController.h"

void CameraController::OnAttach()
{
}

void CameraController::OnDetach()
{
}

void CameraController::OnUpdate(Helios::Timestep deltaTime)
{
  if (!GetComponent<Helios::Camera>().currect) { return; }

  Helios::Transform& transform = GetComponent<Helios::Transform>();

  glm::vec3 front;
  front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
  front.y = sin(glm::radians(m_Pitch));
  front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
  front = glm::normalize(front);

  glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
  glm::vec3 up = glm::normalize(glm::cross(right, front));

  if (Helios::Input::IsKeyPressed(Helios::Key::W)) {
    transform.position += front * m_CameraTranslationSpeed * (float)deltaTime;
  } 

  if (Helios::Input::IsKeyPressed(Helios::Key::A)) {
    transform.position -= right * m_CameraTranslationSpeed * (float)deltaTime;
  }

  if (Helios::Input::IsKeyPressed(Helios::Key::S)) {
    transform.position -= front * m_CameraTranslationSpeed * (float)deltaTime;
  }

  if (Helios::Input::IsKeyPressed(Helios::Key::D)) {
    transform.position += right * m_CameraTranslationSpeed * (float)deltaTime;
  }

  if (Helios::Input::IsKeyPressed(Helios::Key::Space)) {
    transform.position += up * m_CameraTranslationSpeed * (float)deltaTime;
  }

  if (Helios::Input::IsKeyPressed(Helios::Key::LeftShift)) {
    transform.position -= up * m_CameraTranslationSpeed * (float)deltaTime;
  }

  transform.rotation = glm::vec3(m_Pitch, m_Yaw, 0.0f);
}

void CameraController::OnEvent(Helios::Event& event)
{
  Helios::EventDispatcher dispatcher(event);
  dispatcher.Dispatch<Helios::MouseMovedEvent>(BIND_EVENT_FN(CameraController::OnMouseMovedEvent));
  dispatcher.Dispatch<Helios::WindowFocusedEvent>(BIND_EVENT_FN(CameraController::OnWindowFocusedEvent));
  dispatcher.Dispatch<Helios::WindowLostFocusEvent>(BIND_EVENT_FN(CameraController::OnWindowLostFocusEvent));
}

bool CameraController::OnMouseMovedEvent(Helios::MouseMovedEvent& event)
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

bool CameraController::OnWindowFocusedEvent(Helios::WindowFocusedEvent& event)
{
  m_FirstInput = true;
  return true;
}

bool CameraController::OnWindowLostFocusEvent(Helios::WindowLostFocusEvent& event)
{
  m_FirstInput = true;
  return true;
}
