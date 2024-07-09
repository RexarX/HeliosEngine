#include "UniformBuffer.h"
#include "Renderer.h"

#include "Platform/OpenGL/OpenGLUniformBuffer.h"
#include "Platform/Vulkan/VulkanUniformBuffer.h"

namespace VoxelEngine
{
  std::shared_ptr<UniformBuffer> UniformBuffer::Create(const uint32_t size, const uint32_t binding)
  {
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLUniformBuffer>(size, binding);
			case RendererAPI::API::Vulkan:  return std::make_shared<VulkanUniformBuffer>(size, binding);
		}

		CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
  }
}