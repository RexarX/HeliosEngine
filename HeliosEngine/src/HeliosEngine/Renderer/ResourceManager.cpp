#include "ResourceManager.h"
#include "RendererAPI.h"
#include "RenderQueue.h"

#include "Vulkan/VulkanResourceManager.h"

namespace Helios
{
  std::unique_ptr<ResourceManager> ResourceManager::Create()
  {
    switch (RendererAPI::GetAPI())
    {
    case RendererAPI::API::None: CORE_ASSERT_CRITICAL(false, "RendererAPI::None is not supported!"); return nullptr;
    case RendererAPI::API::Vulkan: return std::make_unique<VulkanResourceManager>();
    }

    CORE_ASSERT_CRITICAL(false, "Unknown RendererAPI!");
    return nullptr;
  }
}