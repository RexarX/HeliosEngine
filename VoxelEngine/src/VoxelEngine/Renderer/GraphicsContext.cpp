#include "Renderer/GraphicsContext.h"
#include "Renderer/RendererAPI.h"

#include "Renderer/Vulkan/VulkanContext.h"

struct GLFWwindow;

namespace Engine
{
  std::unique_ptr<GraphicsContext> GraphicsContext::Create(void* window)
  {
    switch (RendererAPI::GetAPI())
    {
    case RendererAPI::API::None: CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
    case RendererAPI::API::Vulkan: return std::make_unique<VulkanContext>(static_cast<GLFWwindow*>(window));
    }

    CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
  }
}