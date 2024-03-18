#pragma once

#include <glm/glm.hpp>

#include "VertexArray.h"

namespace VoxelEngine 
{
	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0,
			OpenGL = 1
		};

	public:
		virtual void Init() = 0;
		virtual void SetViewport(const uint32_t& x, const uint32_t& y, const uint32_t& width, const uint32_t& height) = 0;
		virtual void SetClearColor(const glm::vec4& color) = 0;
		virtual void Clear() = 0;

		virtual void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;

		static std::unique_ptr<RendererAPI> Create();

		inline static API GetAPI() { return s_API; }

	private:
		static API s_API;
	};
}