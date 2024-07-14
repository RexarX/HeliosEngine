#pragma once

#include "EntityComponentSystem/ForwardDecl.h"
#include "EntityComponentSystem/Entity/Entity.h"

#include "VoxelEngine/Timestep.h"

#include "Events/MouseEvent.h"
#include "Events/KeyEvent.h"

#include <glm/glm.hpp>

namespace Engine
{
	enum class CameraControllerType
	{
		FreeRoam = 0,
		FollowEntity = 1
	};

	class VOXELENGINE_API CameraControllerComponent
	{
	public:
		CameraControllerComponent(const CameraControllerType type = CameraControllerType::FreeRoam);

		void OnUpdate(const Timestep ts, ECSManager& manager, const EntityID entity);

		void OnMouseMoved(const glm::vec2 mousePos, const glm::vec2 mouseOffset);
		void OnKeyPressed(const KeyCode key);

		void SetCameraTranslationSpeed(const float speed) { m_CameraTranslationSpeed = speed; }
		void SetCameraRotationSpeed(const float speed) { m_CameraRotationSpeed = speed; }

		void SetFollowedEntity(const EntityID entity) { m_FollowedEntity = entity; }

		inline const glm::vec3& GetPosition() const { return m_Position; }
		inline const glm::vec3& GetRotation() const { return glm::vec3(m_Pitch, m_Yaw, 0.0f); }

	private:
		void UpdateFreeRoam(const Timestep ts);
		void UpdateFollowEntity(const Timestep ts, ECSManager& manager, const EntityID entity);

	private:
		CameraControllerType m_Type;
		EntityID m_FollowedEntity;

		glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };

		float m_CameraTranslationSpeed = 5.0f;
		float m_CameraRotationSpeed = 0.1f;

		float m_Yaw = 0.0f;
		float m_Pitch = 0.0f;
	};
}