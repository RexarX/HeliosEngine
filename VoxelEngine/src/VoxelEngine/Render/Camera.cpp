#include "vepch.h"

#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace VoxelEngine
{
	Camera::Camera(const glm::vec3& cameraPos, const glm::vec3& cameraRotation,
								 const float aspectRatio, const float fov)
		: m_Position(cameraPos), m_Rotation(cameraRotation),
		m_ProjectionMatrix(glm::perspective(fov, aspectRatio, 0.1f, 100.0f))
	{
		RecalculateMatrix();
	}

	void Camera::SetProjection(const float aspectRatio, const float fov)
	{
		m_ProjectionMatrix = glm::perspective(fov, aspectRatio, 0.1f, 100.0f);

		RecalculateMatrix();
	}

	void Camera::NormalizeDirection() 
	{
		m_Direction.x = cos(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
		m_Direction.y = sin(glm::radians(m_Rotation.x));
		m_Direction.z = sin(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
	}

	void Camera::RecalculateMatrix()
	{
		NormalizeDirection();

		m_CameraLeft = glm::cross(m_Direction, glm::vec3(0.0f, 1.0f, 0.0f));
		m_CameraUp = glm::cross(m_CameraLeft, m_Direction);
		m_CameraForward = glm::cross(m_CameraLeft, m_CameraUp);

		m_ViewMatrix = glm::lookAt(m_Position, m_Position - m_Direction, m_CameraUp);

		m_ProjectionViewMatrix = m_ProjectionMatrix * m_ViewMatrix;

		m_ModelMatrix = glm::translate(glm::mat4(1.0f), m_Direction);

		m_ProjectionViewModelMatrix = m_ProjectionViewMatrix * m_ModelMatrix;
	}
}