#include "vepch.h"

#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace VoxelEngine
{
	Camera::Camera(const glm::vec3& cameraPos, const glm::vec3& cameraDirection, const float aspectRatio)
		: m_ProjectionMatrix(glm::perspective(45.0f, aspectRatio, 0.1f, 100.0f)) 
		, m_Position(cameraPos)
	{
		RecalculateViewMatrix();
	}

	void Camera::SetProjection(const float aspectRatio)
	{
		m_ProjectionMatrix = glm::perspective(45.0f, aspectRatio, 0.1f, 100.0f);

		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void Camera::NormalizeDirection() 
	{
		m_Direction.x = cos(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
		m_Direction.y = sin(glm::radians(m_Rotation.x));
		m_Direction.z = sin(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
	}

	void Camera::RecalculateViewMatrix()
	{
		NormalizeDirection();

		glm::vec3 cameraRight = glm::cross(m_Direction, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::vec3 cameraUp = glm::cross(cameraRight, m_Direction);

		m_ViewMatrix = glm::lookAt(m_Position, m_Position - m_Direction, cameraUp);

		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}
}