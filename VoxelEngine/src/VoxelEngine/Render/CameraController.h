#pragma once

#include "Camera.h"
#include "Frustum.h"

#include "Timestep.h"

#include "Events/ApplicationEvent.h"
#include "Events/MouseEvent.h"

namespace VoxelEngine
{
	class VOXELENGINE_API CameraController
	{
	public:
		CameraController(const glm::vec3& position, const glm::vec3& rotation,
			const float aspectRatio, const float fov = 45.0f);

		void OnUpdate(const Timestep ts);
		void OnEvent(Event& event);

		void OnResize(const float width, const float height);

		Camera& GetCamera() { return m_Camera; }
		const Camera& GetCamera() const { return m_Camera; }

		Frustum& GetFrustum() { return m_Frustum; }
		const Frustum& GetFrustum() const { return m_Frustum; }

		float GetFov() const { return m_Fov; }
		void SetFov(const float fov) { m_Fov = fov; }

		void SetFirstInput() { m_FirstInput = true; }

	private:
		bool OnMouseScrolled(MouseScrolledEvent& event);
		bool OnWindowResized(WindowResizeEvent& event);

		void CalculateMouseOffset();

	private:
		float m_AspectRatio = 16.0f / 9.0f;
		float m_Fov = 45.0f;
		float m_CameraTranslationSpeed = 5.0f;
		float m_CameraRotationSpeed = 0.1f;

		float xLast;
		float yLast;

		bool m_FirstInput = true;

		glm::vec3 m_CameraPosition = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_CameraRotation = { 0.0f, 0.0f, 0.0f };

		Camera m_Camera;
		Frustum m_Frustum;
	};
}