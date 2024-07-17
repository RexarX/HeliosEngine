#include "Renderer/RendererAPI.h"
#include "Renderer/Vulkan/VulkanContext.h"

namespace Engine
{
	RendererAPI::API RendererAPI::m_API = RendererAPI::API::Vulkan;

	std::unique_ptr<RendererAPI> RendererAPI::Create()
  {
    switch (m_API)
    {
    case API::None:    CORE_ASSERT(false, "RendererAPI::None is not supported!"); return nullptr;
    case API::Vulkan:  return std::make_unique<VulkanContext>();
    }

    CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
  }
}