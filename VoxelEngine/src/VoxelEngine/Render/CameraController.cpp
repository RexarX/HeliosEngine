#include "vepch.h"

#include "CameraController.h"

#include "Input.h"
#include "KeyCodes.h"

namespace VoxelEngine
{
	CameraController::CameraController(const glm::vec3& position, const glm::vec3& rotation, const float aspectRatio)
		: m_AspectRatio(aspectRatio), m_Camera(position, rotation, aspectRatio), m_CameraRotation(rotation), m_CameraPosition(position)
	{
	}

	void CameraController::OnUpdate(const Timestep ts)
	{
		if (Input::IsKeyPressed(Key::A) || Input::IsKeyPressed(Key::Left)) {
			m_CameraPosition += m_Camera.GetCameraLeft() * m_CameraTranslationSpeed * (float)ts;
		}
		else if (Input::IsKeyPressed(Key::D) || Input::IsKeyPressed(Key::Right)) {
			m_CameraPosition -= m_Camera.GetCameraLeft() * m_CameraTranslationSpeed * (float)ts;
		}
		if (Input::IsKeyPressed(Key::W) || Input::IsKeyPressed(Key::Up)) {
			m_CameraPosition += m_Camera.GetCameraForward() * m_CameraTranslationSpeed * (float)ts;
		}
		else if (Input::IsKeyPressed(Key::S) || Input::IsKeyPressed(Key::Down)) {
			m_CameraPosition -= m_Camera.GetCameraForward() * m_CameraTranslationSpeed * (float)ts;
		}
		if (Input::IsKeyPressed(Key::Space)) {
			m_CameraPosition += m_Camera.GetCameraUp() * m_CameraTranslationSpeed * (float)ts;
		}
		else if (Input::IsKeyPressed(Key::LeftShift)) {
			m_CameraPosition -= m_Camera.GetCameraUp() * m_CameraTranslationSpeed * (float)ts;
		}

		CalculateMouseOffset();

		m_Camera.SetRotation(m_CameraRotation);

		m_Camera.SetPosition(m_CameraPosition);
	}

	void CameraController::CalculateMouseOffset() 
	{
		if (m_FirstInput) {
			xLast = Input::GetMouseX();
      yLast = Input::GetMouseY();
			m_FirstInput = false;
		}

		float deltaX = Input::GetMouseX() - xLast;
    float deltaY = Input::GetMouseY() - yLast;

		deltaX *= m_CameraRotationSpeed;
    deltaY *= m_CameraRotationSpeed;

    xLast = Input::GetMouseX();
    yLast = Input::GetMouseY();

		m_CameraRotation.x += deltaY;
		m_CameraRotation.y += deltaX;

		if (m_CameraRotation.x > 89.0f) {
      m_CameraRotation.x = 89.0f;
    }
    else if (m_CameraRotation.x < -89.0f) {
      m_CameraRotation.x = -89.0f;
    }
	}

	void CameraController::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<MouseScrolledEvent>(VE_BIND_EVENT_FN(CameraController::OnMouseScrolled));
		dispatcher.Dispatch<WindowResizeEvent>(VE_BIND_EVENT_FN(CameraController::OnWindowResized));
	}

	void CameraController::OnResize(const float width, const float height)
	{
		m_AspectRatio = width / height;
		m_Camera.SetProjection(m_AspectRatio);
	}

	bool CameraController::OnMouseScrolled(MouseScrolledEvent& event)
	{
		m_ZoomLevel -= event.GetYOffset() * 0.25f;
		m_ZoomLevel = std::max(m_ZoomLevel, 0.25f);
		m_Camera.SetProjection(m_AspectRatio);
		return false;
	}

	bool CameraController::OnWindowResized(WindowResizeEvent& event)
	{
		OnResize((float)event.GetWidth(), (float)event.GetHeight());
		return false;
	}
}