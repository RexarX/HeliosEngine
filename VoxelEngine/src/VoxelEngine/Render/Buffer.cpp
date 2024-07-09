#include "Buffer.h"
#include "Renderer.h"

#include "vepch.h"

#include "Platform/OpenGL/OpenGLBuffer.h"
#include "Platform/Vulkan/VulkanBuffer.h"

namespace VoxelEngine
{
	std::shared_ptr<VertexBuffer> VertexBuffer::Create(const float* vertices, const uint64_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLVertexBuffer>(vertices, size);
    case RendererAPI::API::Vulkan:  return std::make_shared<VulkanVertexBuffer>(vertices, size);
		}

		CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	std::shared_ptr<VertexBuffer> VertexBuffer::Create(const uint64_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLVertexBuffer>(size);
    case RendererAPI::API::Vulkan:  return std::make_shared<VulkanVertexBuffer>(size);
		}

		CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	std::shared_ptr<IndexBuffer> IndexBuffer::Create(const uint32_t* indices, const uint32_t count)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLIndexBuffer>(indices, count);
    case RendererAPI::API::Vulkan:  return std::make_shared<VulkanIndexBuffer>(indices, count);
		}

		CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	std::shared_ptr<IndexBuffer> IndexBuffer::Create(const uint64_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLIndexBuffer>(size);
    case RendererAPI::API::Vulkan:  return std::make_shared<VulkanIndexBuffer>(size);
		}

		CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}