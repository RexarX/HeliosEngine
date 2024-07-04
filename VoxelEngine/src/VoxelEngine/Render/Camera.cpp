#include "Camera.h"
#include "RendererAPI.h"

#include "vepch.h"

#include <glm/gtc/matrix_transform.hpp>


namespace VoxelEngine
{
	Camera::Camera(const glm::vec3& cameraPos, const glm::vec3& cameraRotation,
								 const float aspectRatio, const float fov)
		: m_Position(cameraPos), m_Rotation(cameraRotation)
	{
		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan) { std::swap(m_NearPlane, m_FarPlane); }

		m_ProjectionMatrix = glm::perspective(fov, aspectRatio, m_NearPlane, m_FarPlane);

		NormalizeDirection();

		m_CameraLeft = glm::cross(m_Direction, glm::vec3(0.0f, 1.0f, 0.0f));
		m_CameraUp = glm::cross(m_CameraLeft, m_Direction);
		m_CameraForward = glm::cross(m_CameraLeft, m_CameraUp);

		m_ModelMatrix = glm::translate(glm::mat4(1.0f), m_Direction);

		m_ViewMatrix = glm::lookAt(m_Position, m_Position - m_Direction, m_CameraUp);

		m_ProjectionViewMatrix = m_ProjectionMatrix * m_ViewMatrix;

		m_ProjectionViewModelMatrix = m_ProjectionViewMatrix * m_ModelMatrix;
	}

	void Camera::SetProjection(const float aspectRatio, const float fov)
	{
		m_ProjectionMatrix = glm::perspective(fov, aspectRatio, m_NearPlane, m_FarPlane);

		m_ProjectionViewMatrix = m_ProjectionMatrix * m_ViewMatrix;

		m_ProjectionViewModelMatrix = m_ProjectionViewMatrix * m_ModelMatrix;
	}

	void Camera::SetPosition(const glm::vec3& position)
	{
		m_Position = position;

		m_ViewMatrix = glm::lookAt(m_Position, m_Position - m_Direction, m_CameraUp);

		m_ProjectionViewMatrix = m_ProjectionMatrix * m_ViewMatrix;

		m_ProjectionViewModelMatrix = m_ProjectionViewMatrix * m_ModelMatrix;
	}

	void Camera::SetRotation(const glm::vec3& rotation)
	{
		m_Rotation = rotation;

		NormalizeDirection();

		m_CameraLeft = glm::cross(m_Direction, glm::vec3(0.0f, 1.0f, 0.0f));
		m_CameraUp = glm::cross(m_CameraLeft, m_Direction);
		m_CameraForward = glm::cross(m_CameraLeft, m_CameraUp);

		m_ModelMatrix = glm::translate(glm::mat4(1.0f), m_Direction);

		m_ViewMatrix = glm::lookAt(m_Position, m_Position - m_Direction, m_CameraUp);

		m_ProjectionViewMatrix = m_ProjectionMatrix * m_ViewMatrix;

		m_ProjectionViewModelMatrix = m_ProjectionViewMatrix * m_ModelMatrix;
	}

	void Camera::NormalizeDirection() 
	{
		m_Direction.x = cos(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
		m_Direction.y = sin(glm::radians(m_Rotation.x));
		m_Direction.z = sin(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
	}
}