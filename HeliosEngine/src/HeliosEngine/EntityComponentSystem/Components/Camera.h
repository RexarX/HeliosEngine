#pragma once

#include "Events/ApplicationEvent.h"

#include <glm/glm.hpp>

namespace Helios
{
	class HELIOSENGINE_API Camera
	{
	public:
		Camera(const glm::vec3& cameraPos = glm::vec3(0.0f),
					 const glm::vec3& cameraRotation = glm::vec3(0.0f),
					 const float aspectRatio = 16.0f / 9.0f, const float fov = 45.0f);

		~Camera() = default;

		void OnEvent(Event& event);
		void SetPosition(const glm::vec3& position);

		void SetRotation(const glm::vec3& rotation);

		void SetProjection(const float aspectRatio, const float fov);

		void OnResize(const float width, const float height);

		inline const float GetAspectRatio() const { return m_AspectRatio; }
		inline const float GetFov() const { return m_Fov; }

		inline const glm::vec3& GetPosition() const { return m_Position; }
		inline const glm::vec3& GetRotation() const { return m_Rotation; }
		inline const glm::vec3& GetDirection() const { return m_Direction; }
		
		inline const glm::vec3& GetCameraUp() const { return m_CameraUp; }
		inline const glm::vec3& GetCameraLeft() const { return m_CameraLeft; }
		inline const glm::vec3& GetCameraForward() const { return m_CameraForward; }

		inline const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		inline const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }

	private:
		void NormalizeDirection();

		const bool OnWindowResize(WindowResizeEvent& event);

	private:
		float m_AspectRatio = 16.0f / 9.0f;
		float m_Fov = 45.0f;

		glm::vec3 m_Position = glm::vec3(0.0f);
		glm::vec3 m_Rotation = glm::vec3(0.0f);
		glm::vec3 m_Direction = glm::vec3(0.0f);

		glm::vec3 m_CameraUp = glm::vec3(0.0f);
		glm::vec3 m_CameraLeft = glm::vec3(0.0f);
		glm::vec3 m_CameraForward = glm::vec3(0.0f);

		glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);
		glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
	};
}