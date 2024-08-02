#include "Renderer/Shader.h"
#include "Renderer/RendererAPI.h"

#include "Renderer/Vulkan/VulkanShader.h"

namespace Helios
{
  std::shared_ptr<Shader> Shader::Create(const std::initializer_list<ShaderInfo>& shaderInfos)
  {
    switch (RendererAPI::GetAPI())
    {
    case RendererAPI::API::None: CORE_ASSERT(false, "RendererAPI::None is not supported!"); return nullptr;
    case RendererAPI::API::Vulkan: return std::make_shared<VulkanShader>(shaderInfos);
    }

    CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
  }
}