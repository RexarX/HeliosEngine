#pragma once

#include "EntityComponentSystem/Components/Camera.h"

#include "EntityComponentSystem/Systems/EventSystem.h"

#include "Events/InputEvent.h"
#include "Events/ApplicationEvent.h"

#include <glm/glm.hpp>

namespace Engine
{
	class VOXELENGINE_API CameraController
	{
	public:
		CameraController(ECSManager& manager, Camera& camera);
		~CameraController();

		void OnUpdate(AppUpdateEvent& event);

		void OnMouseMoved(MouseMovedAction& action);
		void OnMouseButtonPressed(MouseButtonPressedAction& action);
		void OnMouseButtonReleased(MouseButtonReleasedAction& action);

		void OnKeyPressed(KeyPressedAction& action);
		void OnKeyReleased(KeyReleasedAction& action);

		void SetCameraTranslationSpeed(const float speed) { m_CameraTranslationSpeed = speed; }
		void SetCameraRotationSpeed(const float speed) { m_CameraRotationSpeed = speed; }

		inline const glm::vec3& GetPosition() const { return m_Position; }
		inline const glm::vec3& GetRotation() const { return glm::vec3(m_Pitch, m_Yaw, 0.0f); }

	private:
		void UpdateFreeRoam(const Timestep ts);

	private:
		Camera& m_Camera;

		glm::vec3 m_Position = glm::vec3(0.0f);

		float m_CameraTranslationSpeed = 5.0f;
		float m_CameraRotationSpeed = 0.1f;

		float m_Yaw = 0.0f;
		float m_Pitch = 0.0f;

		ECSManager& m_Manager;
	};
}