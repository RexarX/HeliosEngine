#pragma once

#include "RenderStart.h"

#include "Camera.h"

#include "Shader.h"

#include "Texture.h"

namespace VoxelEngine
{
	class Renderer
	{
	public:
		static void Init();

		static void BeginScene(Camera& camera);
		static void EndScene();

		static void OnWindowResize(const uint32_t& width, const uint32_t& height);

		static void Submit();

		inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

		static void DrawCube(const glm::vec3& position, const glm::vec3& size, const std::shared_ptr<Texture>& texture);

	private:
		struct SceneData
		{
			glm::mat4 ViewProjectionMatrix;
		};

		static std::unique_ptr<SceneData> s_SceneData;
	};
}