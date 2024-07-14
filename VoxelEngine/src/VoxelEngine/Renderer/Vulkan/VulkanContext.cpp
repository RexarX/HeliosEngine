#include "Renderer/Vulkan/VulkanContext.h"

namespace Engine
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

  void VulkanContext::SetViewport(const uint32_t width, const uint32_t height)
  {
  }

  void VulkanContext::InitImGui()
  {
  }

  void VulkanContext::ShutdownImGui()
  {
  }

  void VulkanContext::Begin()
  {
  }

  void VulkanContext::End()
  {
  }

  void VulkanContext::SetVSync(const bool enabled)
  {
  }
}