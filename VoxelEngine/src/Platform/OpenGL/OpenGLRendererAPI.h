#pragma once

#include "VoxelEngine/Render/RendererAPI.h"

namespace VoxelEngine
{
	class OpenGLRendererAPI : public RendererAPI
	{
	public:
		virtual void Init() override;

		virtual void SetClearColor(const glm::vec4& color) override;
		virtual void SetViewport(const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height) override;

		virtual void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, const uint32_t indexCount = 0) override;
		virtual void DrawArray(const std::shared_ptr<VertexArray>& vertexArray, const uint32_t vertexCount = 0) override;
		virtual void DrawLine(const std::shared_ptr<VertexArray>& vertexArray, const uint32_t vertexCount = 0) override;
	};
}