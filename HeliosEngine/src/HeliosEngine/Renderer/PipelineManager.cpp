#include "PipelineManager.h"
#include "RendererAPI.h"

#include "Vulkan/VulkanPipelineManager.h"

namespace Helios {
  std::unique_ptr<PipelineManager> PipelineManager::Create() {
    switch (RendererAPI::GetAPI()) {
      case RendererAPI::API::None: {
        CORE_ASSERT_CRITICAL(false, "RendererAPI::None is not supported!");
        return nullptr;
      }

      case RendererAPI::API::Vulkan: {
        return std::make_unique<VulkanPipelineManager>();
      }

      default: CORE_ASSERT_CRITICAL(false, "Unknown RendererAPI!"); return nullptr;
    }
  }
}