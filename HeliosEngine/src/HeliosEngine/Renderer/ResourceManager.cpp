#include "Renderer/ResourceManager.h"
#include "Renderer/RendererAPI.h"
#include "Renderer/Vulkan/VulkanResourceManager.h"

namespace Helios
{
  std::unique_ptr<ResourceManager> ResourceManager::Create()
  {
    switch (RendererAPI::GetAPI())
    {
    case RendererAPI::API::None: CORE_ASSERT(false, "RendererAPI::None is not supported!"); return nullptr;
    case RendererAPI::API::Vulkan: return std::make_unique<VulkanResourceManager>();
    }

    CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
  }
}