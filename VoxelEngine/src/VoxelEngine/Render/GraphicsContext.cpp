#include "GraphicsContext.h"

#include "vepch.h"

#include "Platform/OpenGL/OpenGLContext.h"

#include "Renderer.h"

namespace VoxelEngine
{
	std::unique_ptr<GraphicsContext> GraphicsContext::Create(void* window)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    VE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_unique<OpenGLContext>(static_cast<GLFWwindow*>(window));
		}

		VE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}