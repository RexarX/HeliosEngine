#include "vepch.h"

#include "CameraController.h"

#include "Input.h"
#include "KeyCodes.h"

namespace VoxelEngine
{
	CameraController::CameraController(const glm::vec3 position, const glm::vec3 rotation, const float aspectRatio)
		: m_AspectRatio(aspectRatio), m_Camera(position, rotation, aspectRatio), m_CameraRotation(rotation), m_CameraPosition(position)
	{
	}

	void CameraController::OnUpdate(const Timestep ts)
	{
		/*if (Input::IsKeyPressed(Key::A)) {
			m_CameraPosition.x -= cos(glm::radians(m_CameraRotation.y)) * m_CameraTranslationSpeed * ts;
			m_CameraPosition.y -= sin(glm::radians(m_CameraRotation.y)) * m_CameraTranslationSpeed * ts;
		}
		if (Input::IsKeyPressed(Key::D)) {
			m_CameraPosition.x += cos(glm::radians(m_CameraRotation.y)) * m_CameraTranslationSpeed * ts;
			m_CameraPosition.y += sin(glm::radians(m_CameraRotation.y)) * m_CameraTranslationSpeed * ts;
		}
		if (Input::IsKeyPressed(Key::W)) {
			m_CameraPosition.x += -sin(glm::radians(m_CameraRotation.x)) * m_CameraTranslationSpeed * ts;
			m_CameraPosition.y += cos(glm::radians(m_CameraRotation.x)) * m_CameraTranslationSpeed * ts;
		}
		if (Input::IsKeyPressed(Key::S)) {
			m_CameraPosition.x -= -sin(glm::radians(m_CameraRotation.x)) * m_CameraTranslationSpeed * ts;
			m_CameraPosition.y -= cos(glm::radians(m_CameraRotation.x)) * m_CameraTranslationSpeed * ts;
		}*/
		if (Input::IsKeyPressed(Key::W)) { m_CameraRotation.x -= m_CameraRotationSpeed * ts; }
		if (Input::IsKeyPressed(Key::S)) { m_CameraRotation.x += m_CameraRotationSpeed * ts; }
		if (Input::IsKeyPressed(Key::A)) { m_CameraRotation.y -= m_CameraRotationSpeed * ts; }
		if (Input::IsKeyPressed(Key::D)) { m_CameraRotation.y += m_CameraRotationSpeed * ts; }

		ClampPitch();

		m_Camera.SetRotation(m_CameraRotation);

		m_Camera.SetPosition(m_CameraPosition);

		m_CameraTranslationSpeed = m_ZoomLevel;
	}

	void CameraController::ClampPitch() 
	{
		if (m_CameraRotation.x > 89.0f) { m_CameraRotation.x = 89.0f; }
		else if (m_CameraRotation.x < -89.0f) { m_CameraRotation.x = -89.0f; }
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