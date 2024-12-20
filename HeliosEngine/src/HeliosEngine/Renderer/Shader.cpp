#include "Shader.h"
#include "RendererAPI.h"

#include "Vulkan/VulkanShader.h"

namespace Helios {
  std::shared_ptr<Shader> Shader::Create(const std::initializer_list<Info>& shaderInfos) {
    switch (RendererAPI::GetAPI()) {
      case RendererAPI::API::None: {
        CORE_ASSERT_CRITICAL(false, "Failed to create Shader: RendererAPI::None is not supported!");
        return nullptr;
      }

      case RendererAPI::API::Vulkan: {
        return std::make_shared<VulkanShader>(shaderInfos);
      }

      default: CORE_ASSERT_CRITICAL(false, "Failed to create Shader: Unknown RendererAPI!"); return nullptr;
    }
  }
}