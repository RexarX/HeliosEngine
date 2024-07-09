#pragma once

#include "RenderStart.h"
#include "Object.h"

namespace VoxelEngine
{
	class VOXELENGINE_API Renderer
	{
	public:
		static void Init();

		static void Shutdown();

		inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

		static void OnWindowResize(const uint32_t width, const uint32_t height);

		static void SetGenerateMipmaps(const bool value) { Texture::SetGenerateMipmaps(value); }
		static void SetAnisoLevel(const float value) { Texture::SetAnisoLevel(value); }

		static void DrawTriangle(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& size);

		static void DrawCube(const glm::vec3& position, const glm::vec3& size = glm::vec3(1.0f),
												 const glm::vec3& rotation = glm::vec3(0.0f),
												 const std::shared_ptr<Texture>& texture = nullptr);

		static void DrawLine(const glm::vec3& position, const glm::vec3& rotation, const float lenght);

		static void DrawSkybox(const std::shared_ptr<Texture>& texture);

		static void DrawMesh(const std::shared_ptr<Mesh>& mesh, const glm::vec3& position = glm::vec3(0.0f),
												 const glm::vec3& scale = glm::vec3(1.0f), const glm::vec3& rotation = glm::vec3(0.0f));

		static void DrawObject(const SceneData& sceneData, const Object& object);
	};
}