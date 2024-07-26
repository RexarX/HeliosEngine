#pragma once

#include "Renderer/GraphicsContext.h"

#include "Renderer/Vulkan/VulkanUtils.h"

#ifdef DEBUG_MODE
  constexpr bool enableValidationLayers = true;
#else
  constexpr bool enableValidationLayers = false;
#endif

static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

struct GLFWwindow;

namespace Helios
{
  class VulkanContext : public RendererAPI
  {
  public:
    VulkanContext(GLFWwindow* windowHandle);
    virtual ~VulkanContext() = default;

    void Init() override;
    void Shutdown() override;
    void Update() override;
    void BeginFrame() override;
    void EndFrame() override;
    void Record(const RenderQueue& queue) override;

    void SetViewport(const uint32_t width, const uint32_t height,
                     const uint32_t x = 0, const uint32_t y = 0) override;

    void InitImGui() override;
    void ShutdownImGui() override;
    void BeginFrameImGui() override;
    void EndFrameImGui() override;

    void SetVSync(const bool enabled) override;
    void SetResized(const bool resized) override { m_SwapchainRecreated = resized; }

    void SetImGuiState(const bool enabled) override { m_ImGuiEnabled = enabled; }

    static inline VulkanContext& Get() { return *m_Instance; }

  private:
    void CreateInstance();
    void SetupDebugMessenger();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapchain();
    void CreateAllocator();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSyncObjects();
    void CreateRenderPass();
    void CreateDepthResources();
    void CreateFramebuffers();

    void CleanupSwapchain();
    void RecreateSwapchain();

    const bool CheckValidationLayerSupport() const;

    const std::vector<const char*> GetRequiredExtensions() const;

    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    const VkResult CreateDebugUtilsMessengerEXT(const VkInstance instance,
                                                const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                const VkAllocationCallbacks* pAllocator,
                                                VkDebugUtilsMessengerEXT* pDebugMessenger);

    void DestroyDebugUtilsMessengerEXT(const VkInstance instance, const VkDebugUtilsMessengerEXT debugMessenger,
                                       const VkAllocationCallbacks* pAllocator);

    const bool CheckDeviceExtensionSupport(const VkPhysicalDevice device) const;

    const QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice device) const;
    const SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice device) const;

    const bool IsDeviceSuitable(const VkPhysicalDevice device) const;

    const VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
    const VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    const VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
    const uint32_t ChooseImageCount(const VkSurfaceCapabilitiesKHR& capabilities) const;

    const VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, const VkImageTiling tiling,
                                       const VkFormatFeatureFlags features) const;

    const VkFormat FindDepthFormat() const;

  private:
    const std::vector<const char*> m_ValidationLayers = {
      "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> m_DeviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    static VulkanContext* m_Instance;

    GLFWwindow* m_WindowHandle;

    bool m_SwapchainRecreated = false;
    bool m_ImGuiEnabled = false;
    bool m_Vsync = false;

    VkInstance m_VkInstance;
    VkDebugUtilsMessengerEXT m_DebugMessenger;
    VkSurfaceKHR m_Surface;
    VkPhysicalDevice m_PhysicalDevice;
    VkDevice m_Device;
    VkQueue m_GraphicsQueue;
    VkQueue m_PresentQueue;
    VkSwapchainKHR m_Swapchain;
    std::vector<VkImage> m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainImageViews;
    std::vector<VkFramebuffer> m_SwapchainFramebuffers;
    VkFormat m_SwapchainImageFormat;
    VkExtent2D m_SwapchainExtent;
    std::vector<VkCommandBuffer> m_CommandBuffers;
    VkRenderPass m_RenderPass;
    AllocatedImage m_DepthImage;
    VkViewport m_Viewport;
    VkRect2D m_Scissor;

    VmaAllocator m_Allocator;
    
    uint32_t m_ImageIndex = 0;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> m_Frames;
    uint32_t m_CurrentFrame = 0;

    DeletionQueue m_MainDeletionQueue;
  };
}