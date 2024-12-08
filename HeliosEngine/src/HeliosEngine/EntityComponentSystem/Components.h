#pragma once

#include "Entity.h"

#include "Scene/SceneCamera.h"

#include "Systems/EventSystem.h"
#include "Systems/ScriptSystem.h"

#include "Renderer/Mesh.h"
#include "Renderer/Material.h"

#include <glm/glm.hpp>

namespace Helios {
	struct HELIOSENGINE_API ID { // Every entity has an ID
		UUID id;
	};

	struct HELIOSENGINE_API Tag { // Every entity has a Tag
		std::string tag;
	};

	struct HELIOSENGINE_API Relationship { // Every entity has a Relationship
		Entity* parent = nullptr;
    std::vector<Entity*> children;
	};

	struct HELIOSENGINE_API Transform {
		glm::vec3 position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 scale = { 1.0f, 1.0f, 1.0f };

		inline bool operator==(const Transform& transform) const {
			return position == transform.position && rotation == transform.rotation && scale == transform.scale;
		}
	};

	struct HELIOSENGINE_API Camera {
		SceneCamera camera;
		bool currect = false;
	};

	struct HELIOSENGINE_API Renderable {
		std::shared_ptr<Mesh> mesh = nullptr;
		std::shared_ptr<Material> material = nullptr;

		inline bool operator==(const Renderable& renderable) const {
			return mesh == renderable.mesh && material == renderable.material;
		}
	};

	struct RenderableHash {
	public:
		size_t operator()(const Renderable& renderable) const {
			size_t seed = 0;

			if (renderable.mesh != nullptr) {
				const VertexLayout& layout = renderable.mesh->GetVertexLayout();
				for (const VertexElement& element : layout.GetElements()) {
					CombineHash(seed, static_cast<uint32_t>(element.type));
				}
			}

			if (renderable.material != nullptr) {
				CombineHash(seed, renderable.material->GetAlbedo() != nullptr);
				CombineHash(seed, renderable.material->GetNormalMap() != nullptr
													|| renderable.material->GetSpecularMap() != nullptr
													|| renderable.material->GetRoughnessMap() != nullptr
													|| renderable.material->GetMetallicMap() != nullptr
													|| renderable.material->GetAoMap() != nullptr);

				CombineHash(seed, renderable.material->GetColor().x >= 0.0f);
				CombineHash(seed, renderable.material->GetSpecular() >= 0.0f
													|| renderable.material->GetRoughness() >= 0.0f
													|| renderable.material->GetMetallic() >= 0.0f);
			}

			return seed;
		}

	private:
		template <typename T>
		void CombineHash(size_t& seed, const T& v) const {
			std::hash<T> hasher;
			seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
	};

	class HELIOSENGINE_API Scriptable {
	public:
		virtual ~Scriptable() = default;

		virtual void OnAttach() = 0;
		virtual void OnDetach() = 0;
		virtual void OnUpdate(Timestep deltaTime) = 0;
		virtual void OnEvent(Event& event) = 0;

		friend class Entity;

	protected:
		template <EventTrait T>
		void AddListener(std::function<void(T&)> callback) {
			m_EventSystem->AddListener<T>(this, std::move(callback));
		}

		template <EventTrait T>
		void RemoveListener() const { m_EventSystem->RemoveListener<T>(this); }

		template <EventTrait T>
		void PushEvent(const T& event) const { m_EventSystem->PushEvent(event); }

		template <typename T>
		inline T& GetComponent() const { return m_Entity->GetComponent<T>(); }

	private:
		const Entity* m_Entity = nullptr;
		EventSystem* m_EventSystem = nullptr;
	};

	struct HELIOSENGINE_API Script {
		std::unique_ptr<Scriptable> scriptable = nullptr;
  };
}