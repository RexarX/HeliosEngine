#include "UniformBuffer.h"
#include "Renderer.h"

#include "Platform/OpenGL/OpenGLUniformBuffer.h"
#include "Platform/Vulkan/VulkanUniformBuffer.h"

namespace VoxelEngine
{
  std::unique_ptr<UniformBuffer> UniformBuffer::Create(const std::string& name, const uint32_t size,
																											 const uint32_t binding)
  {
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return std::make_unique<OpenGLUniformBuffer>(name, size, binding);
			case RendererAPI::API::Vulkan:  return std::make_unique<VulkanUniformBuffer>(name, size, binding);
		}

		CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
  }
}