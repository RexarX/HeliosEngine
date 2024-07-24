#include "Renderer/Vulkan/VulkanContext.h"

namespace Helios
{
  VulkanContext* VulkanContext::m_Instance = nullptr;

  VulkanContext::VulkanContext(GLFWwindow* windowHandle)
    : m_WindowHandle(windowHandle)
  {
    CORE_ASSERT(m_Instance == nullptr, "Context already exists!");
    CORE_ASSERT(windowHandle, "Window handle is null!")

    m_Instance = this;
  }

  void VulkanContext::Init()
  {
  }

  void VulkanContext::Shutdown()
  {
  }

  void VulkanContext::Update()
  {
  }

  void VulkanContext::BeginFrame()
  {
  }

  void VulkanContext::EndFrame()
  {
  }

  void VulkanContext::SetViewport(const uint32_t width, const uint32_t height, const uint32_t x, const uint32_t y)
  {
  }

  void VulkanContext::InitImGui()
  {
  }

  void VulkanContext::ShutdownImGui()
  {
  }

  void VulkanContext::BeginFrameImGui()
  {
  }

  void VulkanContext::EndFrameImGui()
  {
  }

  void VulkanContext::SetVSync(const bool enabled)
  {
  }
}