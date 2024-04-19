#include "UniformBuffer.h"

#include "Renderer.h"

#include "Platform/OpenGL/OpenGLUniformBuffer.h"

namespace VoxelEngine
{
  std::unique_ptr<UniformBuffer> UniformBuffer::Create(const uint32_t size, const uint32_t binding)
  {
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    VE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return std::make_unique<OpenGLUniformBuffer>(size, binding);
		}

		VE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
  }
}