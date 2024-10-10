#pragma once

#include "Renderer/RendererAPI.h"

#include "VulkanUtils.h"

#ifdef DEBUG_MODE
  constexpr bool enableValidationLayers = true;
#else
  constexpr bool enableValidationLayers = false;
#endif

static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

struct GLFWwindow;

namespace Helios
{
  class VulkanContext final : public RendererAPI
  {
  public:
    VulkanContext(GLFWwindow* windowHandle);
    virtual ~VulkanContext() = default;

    void Init() override;
    void Shutdown() override;
    void Update() override;

    void BeginFrame() override;
    void EndFrame() override;
    void Record(const RenderQueue& queue, const ResourceManager& manager) override;

    void SetViewport(uint32_t width, uint32_t height, uint32_t x = 0, uint32_t y = 0) override;

    void InitImGui() override;
    void ShutdownImGui() override;
    void BeginFrameImGui() override;
    void EndFrameImGui() override;

    void SetVSync(bool enabled) override;
    void SetResized(bool resized) override { m_SwapchainRecreated = resized; }

    void SetImGuiState(bool enabled) override { m_ImGuiEnabled = enabled; }

    void ImmediateSubmit(std::function<void(const VkCommandBuffer cmd)>&& function);

    static inline VulkanContext& Get() { return *m_Instance; }

    inline const VkDevice GetDevice() const { return m_Device; }
    inline const VkRenderPass GetRenderPass() const { return m_RenderPass; }

    inline const VkPhysicalDeviceLimits& GetPhysicalDeviceLimits() const;

    inline const VmaAllocator GetAllocator() const { return m_Allocator; }

    inline DeletionQueue& GetDeletionQueue() { return m_MainDeletionQueue; }

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

    bool CheckValidationLayerSupport() const;

    std::vector<const char*> GetRequiredExtensions() const;

    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    VkResult CreateDebugUtilsMessengerEXT(const VkInstance instance,
                                          const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                          const VkAllocationCallbacks* pAllocator,
                                          VkDebugUtilsMessengerEXT* pDebugMessenger);

    void DestroyDebugUtilsMessengerEXT(const VkInstance instance, const VkDebugUtilsMessengerEXT debugMessenger,
                                       const VkAllocationCallbacks* pAllocator);

    bool CheckDeviceExtensionSupport(const VkPhysicalDevice device) const;

    QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice device) const;
    SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice device) const;

    bool IsDeviceSuitable(const VkPhysicalDevice device) const;

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
    uint32_t ChooseImageCount(const VkSurfaceCapabilitiesKHR& capabilities) const;

    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                 VkFormatFeatureFlags features) const;

    VkFormat FindDepthFormat() const;

  private:
    const std::vector<const char*> m_ValidationLayers = {
      "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> m_DeviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    static VulkanContext* m_Instance;

    GLFWwindow* m_WindowHandle = nullptr;

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
    VkRenderPass m_RenderPass;
    AllocatedImage m_DepthImage;
    VkViewport m_Viewport;
    VkRect2D m_Scissor;

    VkCommandPool m_ImCommandPool;
    VkCommandBuffer m_ImCommandBuffer;
    VkFence m_ImFence;

    VkDescriptorPool m_ImGuiPool;

    VmaAllocator m_Allocator;
    
    uint32_t m_ImageIndex = 0;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> m_Frames;
    uint32_t m_CurrentFrame = 0;

    DeletionQueue m_MainDeletionQueue;
  };
}