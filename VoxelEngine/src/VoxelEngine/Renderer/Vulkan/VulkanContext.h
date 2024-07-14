#pragma once

#include "Renderer/GraphicsContext.h"

#include "vulkan/vulkan.h"

#ifdef DEBUG_MODE
  constexpr bool enableValidationLayers = true;
#else
  constexpr bool enableValidationLayers = false;
#endif

struct GLFWwindow;

namespace Engine
{
  class VulkanContext : public GraphicsContext
  {
  public:
    VulkanContext(GLFWwindow* windowHandle);

    void Init() override;
    void Shutdown() override;
    void Update() override;
    void SetViewport(const uint32_t width, const uint32_t height) override;

    void InitImGui() override;
    void ShutdownImGui() override;
    void Begin() override;
    void End() override;

    void SetVSync(const bool enabled) override;
    void SetResized(const bool resized) override { m_Resized = resized; }

    void SetImGuiState(const bool enabled) override { m_ImGuiEnabled = enabled; }

    static VulkanContext& Get() { return *m_Instance; }

  private:

  private:
    const std::vector<const char*> validationLayers = {
      "VK_LAYER_KHRONOS_validation"
    };

    static VulkanContext* m_Instance;

    GLFWwindow* m_WindowHandle;

    bool m_Resized = false;
    bool m_ImGuiEnabled = false;
  };
}