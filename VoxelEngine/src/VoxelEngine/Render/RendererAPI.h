#pragma once

#include "VertexArray.h"

#include <glm/glm.hpp>

namespace VoxelEngine 
{
	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0,
			OpenGL = 1,
			Vulkan = 2
		};

	public:
		virtual void Init() = 0;

		virtual void SetViewport(const uint32_t x, const uint32_t y,
														 const uint32_t width, const uint32_t height) = 0;
		virtual void SetClearColor(const glm::vec4& color) = 0;
		virtual void SetDepthMask(const bool mask) = 0;

		virtual void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray,
														 const uint32_t indexCount = 0) = 0;
		virtual void DrawIndexedInstanced(const std::shared_ptr<VertexArray>& vertexArray,
																			const uint32_t indexCount = 0,
																			const uint32_t instanceCount = 0) = 0;
		virtual void DrawArraysInstanced(const std::shared_ptr<VertexArray>& vertexArray,
																		 const uint32_t vertexCount = 0, const uint32_t instanceCount = 0) = 0;
		virtual void DrawArray(const std::shared_ptr<VertexArray>& vertexArray,
													 const uint32_t vertexCount = 0) = 0;
		virtual void DrawLine(const std::shared_ptr<VertexArray>& vertexArray,
													const uint32_t vertexCount = 0) = 0;

		static std::unique_ptr<RendererAPI> Create();

		inline static API GetAPI() { return s_API; }

	private:
		static API s_API;
	};
}