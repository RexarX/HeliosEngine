#pragma once

#include "Renderer/RendererAPI.h"

#include "VulkanUtils.h"

#ifdef DEBUG_MODE
static constexpr bool enableValidationLayers = true;
#else
static constexpr bool enableValidationLayers = false;
#endif

static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

struct GLFWwindow;

namespace Helios {
  class VulkanContext final : public RendererAPI {
  public:
    VulkanContext(GLFWwindow* windowHandle);
    virtual ~VulkanContext() = default;

    void Init() override;
    void Shutdown() override;
    void Update() override;

    void BeginFrame() override;
    void EndFrame() override;
    void Record(const RenderQueue& queue, const PipelineManager& manager) override;

    void SetViewport(uint32_t width, uint32_t height, uint32_t x = 0, uint32_t y = 0) override;

    void InitImGui() override;
    void ShutdownImGui() override;
    void BeginFrameImGui() override;
    void EndFrameImGui() override;

    void SetVSync(bool enabled) override;
    void SetResized(bool resized) override { m_SwapchainRecreated = resized; }

    void ImmediateSubmit(std::function<void(const VkCommandBuffer cmd)>&& function);

    static inline VulkanContext& Get() { return *m_Instance; }

    inline const VkDevice GetDevice() const { return m_Device; }
    inline const VkRenderPass GetRenderPass() const { return m_RenderPass; }

    inline VkPhysicalDeviceProperties GetPhysicalDeviceProperties() const {
      VkPhysicalDeviceProperties properties;
      vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
      return properties;
    }

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
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                        void* pUserData);

    std::vector<const char*> GetRequiredExtensions() const;

    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) const;
    static VkResult CreateDebugUtilsMessengerEXT(const VkInstance instance,
                                                 const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                 const VkAllocationCallbacks* pAllocator,
                                                 VkDebugUtilsMessengerEXT* pDebugMessenger);

    static void DestroyDebugUtilsMessengerEXT(const VkInstance instance, const VkDebugUtilsMessengerEXT debugMessenger,
                                              const VkAllocationCallbacks* pAllocator);

    bool CheckDeviceExtensionSupport(const VkPhysicalDevice device) const;

    QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice device) const;
    SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice device) const;

    bool IsDeviceSuitable(const VkPhysicalDevice device) const;

    static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
    static uint32_t ChooseImageCount(const VkSurfaceCapabilitiesKHR& capabilities);

    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                 VkFormatFeatureFlags features) const;

    inline VkFormat FindDepthFormat() {
      return FindSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
      );
    }

  private:
    const std::vector<const char*> m_ValidationLayers = {
      "VK_LAYER_KHRONOS_validation"
    };

    std::vector<const char*> m_DeviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    static inline VulkanContext* m_Instance = nullptr;

    GLFWwindow* m_WindowHandle = nullptr;

    bool m_SwapchainRecreated = false;
    bool m_ImGuiEnabled = false;
    bool m_Vsync = false;

    VkInstance m_VkInstance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_DebugMessenger;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;
    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainImageViews;
    std::vector<VkFramebuffer> m_SwapchainFramebuffers;
    VkFormat m_SwapchainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_SwapchainExtent{ 0, 0 };
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    AllocatedImage m_DepthImage;
    VkViewport m_Viewport;
    VkRect2D m_Scissor{ 0, 0 };

    VkCommandPool m_ImCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer m_ImCommandBuffer = VK_NULL_HANDLE;
    VkFence m_ImFence = VK_NULL_HANDLE;

    VkDescriptorPool m_ImGuiPool = VK_NULL_HANDLE;

    VmaAllocator m_Allocator = VK_NULL_HANDLE;
    
    uint32_t m_ImageIndex = 0;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> m_Frames;
    uint32_t m_CurrentFrame = 0;

    DeletionQueue m_MainDeletionQueue;
  };
}