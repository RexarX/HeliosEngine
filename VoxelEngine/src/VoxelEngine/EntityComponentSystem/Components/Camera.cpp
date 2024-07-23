#include "EntityComponentSystem/Components/Camera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
	Camera::Camera(const glm::vec3& cameraPos, const glm::vec3& cameraRotation,
								 const float aspectRatio, const float fov)
		: m_Position(cameraPos), m_Rotation(cameraRotation), m_AspectRatio(aspectRatio), m_Fov(fov),
		m_ProjectionMatrix(glm::perspective(fov, aspectRatio, 1000.0f, 0.1f))
	{
		NormalizeDirection();

		m_CameraLeft = glm::cross(m_Direction, glm::vec3(0.0f, 1.0f, 0.0f));
		m_CameraUp = glm::cross(m_CameraLeft, m_Direction);
		m_CameraForward = glm::cross(m_CameraLeft, m_CameraUp);

		m_ViewMatrix = glm::lookAt(m_Position, m_Position - m_Direction, m_CameraUp);
	}

	void Camera::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Camera::OnWindowResize));
	}

	void Camera::SetProjection(const float aspectRatio, const float fov)
	{
		m_ProjectionMatrix = glm::perspective(fov, aspectRatio, 1000.0f, 0.1f);
	}

	void Camera::SetPosition(const glm::vec3& position)
	{
		m_Position = position;

		m_ViewMatrix = glm::lookAt(m_Position, m_Position - m_Direction, m_CameraUp);
	}

	void Camera::SetRotation(const glm::vec3& rotation)
	{
		m_Rotation = rotation;

		NormalizeDirection();

		m_CameraLeft = glm::cross(m_Direction, glm::vec3(0.0f, 1.0f, 0.0f));
		m_CameraUp = glm::cross(m_CameraLeft, m_Direction);
		m_CameraForward = glm::cross(m_CameraLeft, m_CameraUp);

		m_ViewMatrix = glm::lookAt(m_Position, m_Position - m_Direction, m_CameraUp);
	}

	void Camera::OnResize(const float width, const float height)
	{
		if (width != 0.0f && height != 0.0f) {
			m_AspectRatio = width / height;
			SetProjection(m_AspectRatio, m_Fov);
		}
	}

	void Camera::NormalizeDirection()
	{
		m_Direction.x = cos(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
		m_Direction.y = sin(glm::radians(m_Rotation.x));
		m_Direction.z = sin(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
	}

	const bool Camera::OnWindowResize(WindowResizeEvent& event)
	{
		OnResize((float)event.GetWidth(), (float)event.GetHeight());
		return true;
	}
}