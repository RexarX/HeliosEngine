#pragma once

#include "RendererAPI.h"

namespace VoxelEngine
{
	class RenderCommand
	{
	public:
		inline static void Init()
		{
			s_RendererAPI->Init();
		}

		inline static void SetViewport(const uint32_t x, const uint32_t y,
																	 const uint32_t width, const uint32_t height)
		{
			s_RendererAPI->SetViewport(x, y, width, height);
		}

		inline static void SetClearColor(const glm::vec4& color)
		{
			s_RendererAPI->SetClearColor(color);
		}

		inline static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray,
																	 const uint32_t indexCount)
		{
			s_RendererAPI->DrawIndexed(vertexArray, indexCount);
		}

		inline static void DrawArraysInstanced(const std::shared_ptr<VertexArray>& vertexArray,
			const uint32_t indexCount, const uint32_t instanceCount)
		{
			s_RendererAPI->DrawArraysInstanced(vertexArray, indexCount, instanceCount);
		}

		inline static void DrawArray(const std::shared_ptr<VertexArray>& vertexArray,
																 const uint32_t vertexCount)
		{
      s_RendererAPI->DrawArray(vertexArray, vertexCount);
		}

		inline static void DrawLine(const std::shared_ptr<VertexArray>& vertexArray,
																const uint32_t vertexCount)
		{
      s_RendererAPI->DrawLine(vertexArray, vertexCount);
		}

	private:
		static std::unique_ptr<RendererAPI> s_RendererAPI;
	};
}