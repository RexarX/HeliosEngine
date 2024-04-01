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

		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		SetClearColor({ 0.0f, 0.0f, 0.0f, 0.0f });
	}

	void OpenGLRendererAPI::SetClearColor(const glm::vec4& color)
	{
		glClearColor(color.r, color.g, color.b, color.a);
	}

	void OpenGLRendererAPI::SetViewport(const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height)
	{
		glViewport(x, y, width, height);
	}

	void OpenGLRendererAPI::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, const uint32_t indexCount)
	{
		uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
	}

	void OpenGLRendererAPI::DrawArray(const std::shared_ptr<VertexArray>& vertexArray, const uint32_t vertexCount)
	{
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
	}
}