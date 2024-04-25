#include "vepch.h"

#include "OpenGLRendererAPI.h"

#include <glad/glad.h>

namespace VoxelEngine
{
	void OpenGLRendererAPI::Init()
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LINE_SMOOTH);

		glEnable(GL_CULL_FACE);

		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		SetClearColor({ 0.0f, 0.0f, 0.0f, 0.0f });
	}

	void OpenGLRendererAPI::SetClearColor(const glm::vec4& color)
	{
		glClearColor(color.r, color.g, color.b, color.a);
	}

	void OpenGLRendererAPI::SetViewport(const uint32_t x, const uint32_t y,
																			const uint32_t width, const uint32_t height)
	{
		glViewport(x, y, width, height);
	}

	void OpenGLRendererAPI::SetDepthMask(const bool mask)
	{
		if (mask) { glDepthMask(GL_TRUE); }
		else { glDepthMask(GL_FALSE); }
	}

	void OpenGLRendererAPI::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray,
																			const uint32_t indexCount)
	{
		uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
	}

	void OpenGLRendererAPI::DrawIndexedInstanced(const std::shared_ptr<VertexArray>& vertexArray, const uint32_t indexCount, const uint32_t instanceCount)
	{
		uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
		glDrawElementsInstanced(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr, instanceCount);
	}

	void OpenGLRendererAPI::DrawArray(const std::shared_ptr<VertexArray>& vertexArray,
																		const uint32_t vertexCount)
	{
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
	}

	void OpenGLRendererAPI::DrawArraysInstanced(const std::shared_ptr<VertexArray>& vertexArray,
		const uint32_t vertexCount, const uint32_t instanceCount)
	{
		glDrawArraysInstanced(GL_TRIANGLES, 0, vertexCount, instanceCount);
	}

	void OpenGLRendererAPI::DrawLine(const std::shared_ptr<VertexArray>& vertexArray,
																	 const uint32_t vertexCount)
	{
    glDrawArrays(GL_LINES, 0, vertexCount);
	}
}