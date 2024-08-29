#pragma once

#include "UUID.h"

#include "Entity.h"

#include "Scene/SceneCamera.h"

#include "Renderer/Mesh.h"
#include "Renderer/Material.h"

#include "Systems/EventSystem.h"
#include "Systems/ScriptSystem.h"

#include <glm/glm.hpp>

namespace Helios
{
	class Scene;

	struct HELIOSENGINE_API ID // Every entity has an ID
	{
		UUID id;
	};

	struct HELIOSENGINE_API Tag // Every entity has a Tag
	{
		std::string tag;
	};

	struct HELIOSENGINE_API Relationship // Every entity has a Relationship
	{
		Entity* parent = nullptr;
    std::vector<Entity*> children;
		bool isRoot = false;
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

	class HELIOSENGINE_API Scriptable
	{
	public:
		virtual ~Scriptable() = default;

		virtual void OnAttach() = 0;
		virtual void OnDetach() = 0;
		virtual void OnUpdate(Timestep deltaTime) = 0;
		virtual void OnEvent(Event& event) = 0;

		friend class Entity;

	protected:
		template <typename T>
		void AddListener(const std::function<void(T&)>& callback) const {
			m_EventSystem->AddListener<T>(this, callback);
		}

		template <typename T>
		void RemoveListener() const { m_EventSystem->RemoveListener<T>(this); }

		template <typename T>
		void PushEvent(T& event) const { m_EventSystem->PushEvent(event); }

		template <typename T>
		inline T& GetComponent() const { return m_Entity->GetComponent<T>(); }

	private:
		const Entity* m_Entity = nullptr;
		EventSystem* m_EventSystem = nullptr;
	};

	struct HELIOSENGINE_API Script
  {
		std::unique_ptr<Scriptable> scriptable = nullptr;
  };
}