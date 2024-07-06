#include "Buffer.h"
#include "Renderer.h"

#include "vepch.h"

#include "Platform/OpenGL/OpenGLBuffer.h"
#include "Platform/Vulkan/VulkanBuffer.h"

namespace VoxelEngine
{
	std::shared_ptr<VertexBuffer> VertexBuffer::Create(const std::string& name, const float* vertices, const uint64_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLVertexBuffer>(name, vertices, size);
    case RendererAPI::API::Vulkan:  return std::make_shared<VulkanVertexBuffer>(name, vertices, size);
		}

		CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	std::shared_ptr<VertexBuffer> VertexBuffer::Create(const std::string& name, const uint64_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLVertexBuffer>(name, size);
    case RendererAPI::API::Vulkan:  return std::make_shared<VulkanVertexBuffer>(name, size);
		}

		CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	std::shared_ptr<IndexBuffer> IndexBuffer::Create(const std::string& name, const uint32_t* indices, const uint64_t count)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLIndexBuffer>(name, indices, count);
    case RendererAPI::API::Vulkan:  return std::make_shared<VulkanIndexBuffer>(name, indices, count);
		}

		CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	std::shared_ptr<IndexBuffer> IndexBuffer::Create(const std::string& name, const uint64_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLIndexBuffer>(name, size);
    case RendererAPI::API::Vulkan:  return std::make_shared<VulkanIndexBuffer>(name, size);
		}

		CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}