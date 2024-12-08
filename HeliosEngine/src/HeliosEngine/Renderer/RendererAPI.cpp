#include "RendererAPI.h"
#include "RenderQueue.h"

#include "Vulkan/VulkanContext.h"

namespace Helios {
	std::unique_ptr<RendererAPI> RendererAPI::Create(API api, void* window) {
    m_API = api;
    switch (api) {
      case API::None: {
        CORE_ASSERT_CRITICAL(false, "RendererAPI::None is not supported!");
        return nullptr;
      }

      case API::Vulkan: {
        return std::make_unique<VulkanContext>(static_cast<GLFWwindow*>(window));
      }

      case API::OpenGL: {
        return nullptr;
      }

      default: CORE_ASSERT_CRITICAL(false, "Unknown RendererAPI!"); return nullptr;
    }
  }
}