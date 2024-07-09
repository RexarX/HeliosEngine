#include "VertexArray.h"
#include "Renderer.h"

#include "vepch.h"

#include "Platform/OpenGL/OpenGLVertexArray.h"
#include "Platform/Vulkan/VulkanVertexArray.h"

namespace VoxelEngine
{
	std::shared_ptr<VertexArray> VertexArray::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLVertexArray>();
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanVertexArray>();
		}

		CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}