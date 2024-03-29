#pragma once

#include "Camera.h"

#include "Timestep.h"

#include "Events/ApplicationEvent.h"
#include "Events/MouseEvent.h"

namespace VoxelEngine
{
	class CameraController
	{
	public:
		CameraController(const glm::vec3 position, const glm::vec3 rotation, const float aspectRatio);

		void OnUpdate(Timestep ts);
		void OnEvent(Event& event);

		void OnResize(const float width, const float height);

		Camera& GetCamera() { return m_Camera; }
		const Camera& GetCamera() const { return m_Camera; }

		float GetZoomLevel() const { return m_ZoomLevel; }
		void SetZoomLevel(const float level) { m_ZoomLevel = level; }

	private:
		bool OnMouseScrolled(MouseScrolledEvent& event);
		bool OnWindowResized(WindowResizeEvent& event);

		void ClampPitch();

	private:
		float m_AspectRatio;
		float m_ZoomLevel = 1.0f;
		float m_CameraTranslationSpeed = 5.0f;
		float m_CameraRotationSpeed = 180.0f;

		glm::vec3 m_CameraPosition = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_CameraRotation = { 0.0f, 0.0f, 0.0f };

		Camera m_Camera;
	};
}