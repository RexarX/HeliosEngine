#include "RendererAPI.h"
#include "RenderQueue.h"

#include "Vulkan/VulkanContext.h"

namespace Helios {
	std::unique_ptr<RendererAPI> RendererAPI::Create(API api, void* window) {
    switch (api) {
      case API::None: {
        CORE_ASSERT_CRITICAL(false, "Failed to create RendererAPI: RendererAPI::None is not supported!");
        return nullptr;
      }

      case API::Vulkan: {
        m_API = api;
        return std::make_unique<VulkanContext>(static_cast<GLFWwindow*>(window));
      }

      case API::OpenGL: {
        m_API = api;
        return nullptr;
      }

      default: CORE_ASSERT_CRITICAL(false, "Failed to create RendererAPI: Unknown RendererAPI!"); return nullptr;
    }
  }
}