#include "vepch.h"

#include "Buffer.h"

#include "Renderer.h"

#include "Platform/OpenGL/OpenGLBuffer.h"
#include "Platform/Vulkan/VulkanBuffer.h"

namespace VoxelEngine
{
	std::shared_ptr<VertexBuffer> VertexBuffer::Create(const char* name, const float* vertices, const uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    VE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLVertexBuffer>(name, vertices, size);
    case RendererAPI::API::Vulkan:  return std::make_shared<VulkanVertexBuffer>(name, vertices, size);
		}

		VE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	std::shared_ptr<VertexBuffer> VertexBuffer::Create(const char* name, const uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    VE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLVertexBuffer>(name, size);
    case RendererAPI::API::Vulkan:  return std::make_shared<VulkanVertexBuffer>(name, size);
		}

		VE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	std::shared_ptr<IndexBuffer> IndexBuffer::Create(const char* name, const uint32_t* indices, const uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    VE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLIndexBuffer>(name, indices, size);
    case RendererAPI::API::Vulkan:  return std::make_shared<VulkanIndexBuffer>(name, indices, size);
		}

		VE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	std::shared_ptr<IndexBuffer> IndexBuffer::Create(const char* name, const uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    VE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLIndexBuffer>(name, size);
    case RendererAPI::API::Vulkan:  return std::make_shared<VulkanIndexBuffer>(name, size);
		}

		VE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}