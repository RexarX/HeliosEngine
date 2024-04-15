#pragma once

#include <glm/glm.hpp>

namespace VoxelEngine
{
	class Camera
	{
	public:
		Camera(const glm::vec3& cameraPos, const glm::vec3& cameraRotation,
			const float aspectRatio, const float fov);

		void SetProjection(const float aspectRatio, const float fov);

		const glm::vec3& GetPosition() const { return m_Position; }
		void SetPosition(const glm::vec3& position);

		const glm::vec3& GetRotation() const { return m_Rotation; }
		void SetRotation(const glm::vec3& rotation);

		const glm::vec3& GetCameraUp() const { return m_CameraUp; }
    const glm::vec3& GetCameraLeft() const { return m_CameraLeft; }
    const glm::vec3& GetCameraForward() const { return m_CameraForward; }

		const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		const glm::mat4& GetProjectionViewMatrix() const { return m_ProjectionViewMatrix; }
    const glm::mat4& GetProjectionViewModelMatrix() const { return m_ProjectionViewModelMatrix; }

	private:
		void NormalizeDirection();

	private:
		glm::mat4 m_ProjectionMatrix;
		glm::mat4 m_ViewMatrix;
		glm::mat4 m_ModelMatrix;
		glm::mat4 m_ProjectionViewMatrix;
		glm::mat4 m_ProjectionViewModelMatrix;

		glm::vec3 m_CameraUp;
		glm::vec3 m_CameraLeft;
		glm::vec3 m_CameraForward;

		glm::vec3 m_Position;
		glm::vec3 m_Rotation;
		glm::vec3 m_Direction;
	};
}