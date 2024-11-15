#include "RendererAPI.h"
#include "RenderQueue.h"

#include "Vulkan/VulkanContext.h"

namespace Helios {
	RendererAPI::API RendererAPI::m_API = RendererAPI::API::Vulkan;

	std::unique_ptr<RendererAPI> RendererAPI::Create(void* window) {
    switch (m_API) {
      case API::None: {
        CORE_ASSERT_CRITICAL(false, "RendererAPI::None is not supported!");
        return nullptr;
      }
      case API::Vulkan: {
        return std::make_unique<VulkanContext>(static_cast<GLFWwindow*>(window));
      }
    }

    CORE_ASSERT_CRITICAL(false, "Unknown RendererAPI!");
    return nullptr;
  }
}