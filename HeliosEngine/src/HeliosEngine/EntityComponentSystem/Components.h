#pragma once

#include "UUID.h"

#include "Scene/SceneCamera.h"

#include "Renderer/Mesh.h"
#include "Renderer/Material.h"

#include "Systems/EventSystem.h"

#include <glm/glm.hpp>

namespace Helios
{
	class Scene;

	struct HELIOSENGINE_API ID // Every entity must have an ID
	{
		UUID id;
	};

	struct HELIOSENGINE_API Tag // Every entity must have a Tag
	{
		std::string tag;
	};

	struct HELIOSENGINE_API Relationship // Every entity must have a Relationship
	{
		bool isRoot = false;
		entt::entity parent = entt::null;
    std::vector<entt::entity> children;
	};

	struct HELIOSENGINE_API Transform
	{
		glm::vec3 position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
	};

	struct HELIOSENGINE_API Camera
	{
		SceneCamera camera;
		bool currect = false;
	};

	struct HELIOSENGINE_API Renderable
	{
		std::shared_ptr<Mesh> mesh = nullptr;
		std::shared_ptr<Material> material = nullptr;

		bool visible = true;

		inline bool operator==(const Renderable& other) const {
			return mesh == other.mesh && material == other.material;
		}
	};

	struct HELIOSENGINE_API Quaternion
	{
		glm::mat4 matrix;
	};

	class HELIOSENGINE_API Script : public std::enable_shared_from_this<Script>
	{
	public:
		virtual ~Script() = default;

		virtual void OnAttach() = 0;
		virtual void OnUpdate(Timestep deltaTime) = 0;
		virtual void OnEvent(Event& event) = 0;

		friend class Entity;

	protected:
		template <typename T>
		void AddListener(const std::function<void(T&)>& callback) {
			m_EventSystem->AddListener<T>(shared_from_this(), callback);
		}

	protected:
		EventSystem* m_EventSystem = nullptr;
	};
}