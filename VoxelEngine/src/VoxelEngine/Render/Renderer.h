#pragma once

#include "RenderStart.h"

#include "Camera.h"

#include "Shader.h"

#include "Texture.h"

#include "Frustum.h"

namespace VoxelEngine
{
	class Renderer
	{
	public:
		static void Init();

		static void Shutdown();

		static void BeginScene(const Camera& camera);
		static void EndScene();

		static void OnWindowResize(const uint32_t width, const uint32_t height);

		inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

		static void DrawCube(const glm::vec3& position, const glm::vec3& rotation,
												 const glm::vec3& size, const std::shared_ptr<Texture>& texture);

		static void DrawCubesInstanced(const std::vector<glm::mat3>& cubes, const std::shared_ptr<Texture>& texture);

		static void DrawLine(const glm::vec3& position, const glm::vec3& rotation,
												 const float lenght);

	private:
		struct SceneData
		{
			glm::mat4 ViewProjectionMatrix;
			glm::mat4 ProjectionMatrix;
			glm::mat4 ViewMatrix;
		};

		static std::unique_ptr<SceneData> s_SceneData;
	};
}