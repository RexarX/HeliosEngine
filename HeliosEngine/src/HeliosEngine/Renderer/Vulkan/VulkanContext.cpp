#include "Renderer/Vulkan/VulkanContext.h"

#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

namespace Helios
{
  VulkanContext* VulkanContext::m_Instance = nullptr;

  static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    const VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
  {
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
      CORE_ERROR("{0} Validation Layer: {1}: {2}", pCallbackData->messageIdNumber,
                                                   pCallbackData->pMessageIdName,
                                                   pCallbackData->pMessage);
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
      CORE_WARN("{0} Validation Layer: {1}: {2}", pCallbackData->messageIdNumber,
                                                  pCallbackData->pMessageIdName,
                                                  pCallbackData->pMessage);
    }
    else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
      CORE_WARN("{0} Validation Layer: Performance warning: {1}: {2}", pCallbackData->messageIdNumber,
                                                                       pCallbackData->pMessageIdName,
                                                                       pCallbackData->pMessage);
    }
    else {
      CORE_INFO("{0} Validation Layer: {1}: {2}", pCallbackData->messageIdNumber,
                                                  pCallbackData->pMessageIdName,
                                                  pCallbackData->pMessage);
    }

    return VK_FALSE;
  }

  VulkanContext::VulkanContext(GLFWwindow* windowHandle)
    : m_WindowHandle(windowHandle)
  {
    CORE_ASSERT(m_Instance == nullptr, "Context already exists!");
    CORE_ASSERT(windowHandle, "Window handle is null!")

    m_Instance = this;
  }

  void VulkanContext::Init()
  {
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapchain();
    CreateAllocator();
    CreateCommandPool();
    CreateCommandBuffers();
    CreateSyncObjects();
    CreateRenderPass();
    CreateDepthResources();
    CreateFramebuffers();
  }

  void VulkanContext::Shutdown()
  {
    vkDeviceWaitIdle(m_Device);

    //ShutdownImGui();

    m_MainDeletionQueue.Flush();

    CleanupSwapchain();

    vmaDestroyAllocator(m_Allocator);

    vkDestroyDevice(m_Device, nullptr);

    vkDestroySurfaceKHR(m_VkInstance, m_Surface, nullptr);

    if (enableValidationLayers) {
      DestroyDebugUtilsMessengerEXT(m_VkInstance, m_DebugMessenger, nullptr);
    }

    vkDestroyInstance(m_VkInstance, nullptr);
  }

  void VulkanContext::Update()
  {
  }

  void VulkanContext::BeginFrame()
  {
    if (m_SwapchainRecreated) {
      RecreateSwapchain();
      return;
    }
    auto result = vkWaitForFences(m_Device, 1, &m_Frames[m_CurrentFrame].renderFence, VK_TRUE,
                                  std::numeric_limits<uint64_t>::max());

    CORE_ASSERT(result == VK_SUCCESS, "Failed to wait for fence!");

    result = vkResetFences(m_Device, 1, &m_Frames[m_CurrentFrame].renderFence);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to reset fence!");

    result = vkAcquireNextImageKHR(m_Device, m_Swapchain, std::numeric_limits<uint64_t>::max(),
                                   m_Frames[m_CurrentFrame].presentSemaphore, VK_NULL_HANDLE, &m_ImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      RecreateSwapchain();
      m_SwapchainRecreated = true;
      return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      CORE_ASSERT(false, "Failed to acquire next image!");
    }

    result = vkResetCommandBuffer(m_Frames[m_CurrentFrame].commandBuffer, 0);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to reset command buffer!");

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    result = vkBeginCommandBuffer(m_Frames[m_CurrentFrame].commandBuffer, &beginInfo);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to begin recording command buffer!");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_RenderPass;
    renderPassInfo.framebuffer = m_SwapchainFramebuffers[m_ImageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_SwapchainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdSetViewport(m_Frames[m_CurrentFrame].commandBuffer, 0, 1, &m_Viewport);
    vkCmdSetScissor(m_Frames[m_CurrentFrame].commandBuffer, 0, 1, &m_Scissor);

    vkCmdBeginRenderPass(m_Frames[m_CurrentFrame].commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  }

  void VulkanContext::EndFrame()
  {
    if (m_SwapchainRecreated) {
      m_SwapchainRecreated = false;
      return;
    }

    vkCmdEndRenderPass(m_Frames[m_CurrentFrame].commandBuffer);

    auto result = vkEndCommandBuffer(m_Frames[m_CurrentFrame].commandBuffer);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to end command buffer!");

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_Frames[m_CurrentFrame].presentSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_Frames[m_CurrentFrame].commandBuffer;

    VkSemaphore signalSemaphores[] = { m_Frames[m_CurrentFrame].renderSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_Frames[m_CurrentFrame].renderFence);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to submit draw command buffer!");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_Swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &m_ImageIndex;

    result = vkQueuePresentKHR(m_GraphicsQueue, &presentInfo);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to present swap chain image!");

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  void VulkanContext::Record(const RenderQueue& queue)
  {
    if (m_SwapchainRecreated) { return; }

    // Record commands
  }

  void VulkanContext::SetViewport(const uint32_t width, const uint32_t height, const uint32_t x, const uint32_t y)
  {
    m_Viewport.x = static_cast<float>(x);
    m_Viewport.y = static_cast<float>(y + height);
    m_Viewport.width = static_cast<float>(width);
    m_Viewport.height = -static_cast<float>(height);
    m_Viewport.minDepth = 0.0f;
    m_Viewport.maxDepth = 1.0f;

    m_Scissor.offset = { static_cast<int32_t>(x), static_cast<int32_t>(y) };
    m_Scissor.extent = { width, height };
  }

  void VulkanContext::InitImGui()
  {
  }

  void VulkanContext::ShutdownImGui()
  {
    ImGui_ImplVulkan_Shutdown();
  }

  void VulkanContext::BeginFrameImGui()
  {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
  }

  void VulkanContext::EndFrameImGui()
  {
    ImGui::Render();

    GLFWwindow* backup_current_context = glfwGetCurrentContext();

    #ifdef PLATFORM_WINDOWS
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    #endif

    glfwMakeContextCurrent(backup_current_context);
  }

  void VulkanContext::SetVSync(const bool enabled)
  {
    m_Vsync = enabled;
    RecreateSwapchain();
  }

  void VulkanContext::CreateInstance()
  {
    if (enableValidationLayers && !CheckValidationLayerSupport()) {
      throw std::runtime_error("Validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Helios Engine";
    appInfo.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
    appInfo.pEngineName = "Helios";
    appInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
      createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
      createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

      PopulateDebugMessengerCreateInfo(debugCreateInfo);
      createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
      createInfo.enabledLayerCount = 0;
      createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &m_VkInstance) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create Vulkan instance!");
    }
  }

  void VulkanContext::SetupDebugMessenger()
  {
    if (!enableValidationLayers) { return; }

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    PopulateDebugMessengerCreateInfo(createInfo);

    auto result = CreateDebugUtilsMessengerEXT(m_VkInstance, &createInfo, nullptr, &m_DebugMessenger);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to set up debug messenger!");
  }

  void VulkanContext::CreateSurface()
  {
    auto result = glfwCreateWindowSurface(m_VkInstance, m_WindowHandle, nullptr, &m_Surface);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to create window surface!");
  }

  void VulkanContext::PickPhysicalDevice()
  {
    uint32_t deviceCount = 0;
    auto result = vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, nullptr);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to enumerate physical devices!");

    CORE_ASSERT(deviceCount > 0, "Failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    result = vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, devices.data());
    CORE_ASSERT(result == VK_SUCCESS, "Failed to enumerate physical devices!");

    std::multimap<uint32_t, VkPhysicalDevice> candidates;
    uint32_t score = 0;
    for (const auto& device : devices) {
      if (IsDeviceSuitable(device)) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
          score += 1000;
        }

        score += deviceProperties.limits.maxImageDimension2D;
        candidates.insert(std::make_pair(score, device));
        score = 0;
      }
    }

    if (candidates.rbegin()->first > 0) {
      m_PhysicalDevice = candidates.rbegin()->second;

      VkPhysicalDeviceProperties deviceProperties;
      vkGetPhysicalDeviceProperties(m_PhysicalDevice, &deviceProperties);
      CORE_INFO("Vulkan Info:");
      CORE_INFO("  GPU: {0}", deviceProperties.deviceName);
      CORE_INFO("  Version: {0}", deviceProperties.driverVersion);
    }
    else {
      CORE_ASSERT(false, "Failed to find a suitable GPU!");
    }
  }

  void VulkanContext::CreateLogicalDevice()
  {
    QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
      VkDeviceQueueCreateInfo queueCreateInfo{};
      queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = queueFamily;
      queueCreateInfo.queueCount = 1;
      queueCreateInfo.pQueuePriorities = &queuePriority;
      queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

    if (enableValidationLayers) {
      createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
      createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
    }
    else {
      createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create logical device!");
    }

    vkGetDeviceQueue(m_Device, indices.graphicsFamily.value(), 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, indices.presentFamily.value(), 0, &m_PresentQueue);
  }

  void VulkanContext::CreateSwapchain()
  {
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_PhysicalDevice);

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = ChooseImageCount(swapChainSupport.capabilities);

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_Surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
      createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      createInfo.queueFamilyIndexCount = 2;
      createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
      createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    auto result = vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_Swapchain);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to create swapchain!");

    result = vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to get swapchain images!");

    m_SwapchainImages.resize(imageCount);

    result = vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_SwapchainImages.data());
    CORE_ASSERT(result == VK_SUCCESS, "Failed to get swapchain images!");

    m_SwapchainImageFormat = surfaceFormat.format;
    m_SwapchainExtent = extent;

    m_SwapchainImageViews.resize(m_SwapchainImages.size());
    for (uint32_t i = 0; i < m_SwapchainImages.size(); ++i) {
      VkImageViewCreateInfo viewInfo{};
      viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      viewInfo.image = m_SwapchainImages[i];
      viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      viewInfo.format = m_SwapchainImageFormat;
      viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      viewInfo.subresourceRange.baseMipLevel = 0;
      viewInfo.subresourceRange.levelCount = 1;
      viewInfo.subresourceRange.baseArrayLayer = 0;
      viewInfo.subresourceRange.layerCount = 1;

      result = vkCreateImageView(m_Device, &viewInfo, nullptr, &m_SwapchainImageViews[i]);
      CORE_ASSERT(result == VK_SUCCESS, "Failed to create image view!");
    }
  }

  void VulkanContext::CreateAllocator()
  {
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = m_PhysicalDevice;
    allocatorInfo.device = m_Device;
    allocatorInfo.instance = m_VkInstance;

    auto result = vmaCreateAllocator(&allocatorInfo, &m_Allocator);

    CORE_ASSERT(result == VK_SUCCESS, "Failed to create VMA allocator!");
  }

  void VulkanContext::CreateCommandPool()
  {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = 0;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    for (auto& frame : m_Frames) {
      auto result = vkCreateCommandPool(m_Device, &poolInfo, nullptr, &frame.commandPool);
      CORE_ASSERT(result == VK_SUCCESS, "Failed to create command pool!");

      m_MainDeletionQueue.PushFunction([&]() {
        vkDestroyCommandPool(m_Device, frame.commandPool, nullptr);
      });
    }
  }

  void VulkanContext::CreateCommandBuffers()
  {
    for (auto& frame : m_Frames) {
      VkCommandBufferAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      allocInfo.commandPool = frame.commandPool;
      allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      allocInfo.commandBufferCount = 1;

      auto result = vkAllocateCommandBuffers(m_Device, &allocInfo, &frame.commandBuffer);
      CORE_ASSERT(result == VK_SUCCESS, "Failed to allocate command buffer!");
    }
  }

  void VulkanContext::CreateSyncObjects()
  {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto& frame : m_Frames) {
      auto result = vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &frame.presentSemaphore) == VK_SUCCESS &&
                    vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &frame.renderSemaphore) == VK_SUCCESS &&
                    vkCreateFence(m_Device, &fenceInfo, nullptr, &frame.renderFence) == VK_SUCCESS;
        
      CORE_ASSERT(result, "Failed to create synchronization objects!");

      m_MainDeletionQueue.PushFunction([&]() {
        vkDestroySemaphore(m_Device, frame.presentSemaphore, nullptr);
        vkDestroySemaphore(m_Device, frame.renderSemaphore, nullptr);
        vkDestroyFence(m_Device, frame.renderFence, nullptr);
      });
    }
  }

  void VulkanContext::CreateRenderPass()
  {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_SwapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = FindDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    auto result = vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to create render pass!");

    m_MainDeletionQueue.PushFunction([&]() {
      vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
    });
  }

  void VulkanContext::CreateDepthResources()
  {
    VkFormat depthFormat = FindDepthFormat();

    m_DepthImage = CreateImage(m_SwapchainExtent.width, m_SwapchainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY, m_Allocator);

    m_DepthImage.imageView = CreateImageView(m_DepthImage.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, m_Device);
  }

  void VulkanContext::CreateFramebuffers()
  {
    m_SwapchainFramebuffers.resize(m_SwapchainImageViews.size());

    for (uint32_t i = 0; i < m_SwapchainImageViews.size(); ++i) {
      std::array<VkImageView, 2> attachments = {
          m_SwapchainImageViews[i],
          m_DepthImage.imageView
      };

      VkFramebufferCreateInfo framebufferInfo{};
      framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass = m_RenderPass;
      framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
      framebufferInfo.pAttachments = attachments.data();
      framebufferInfo.width = m_SwapchainExtent.width;
      framebufferInfo.height = m_SwapchainExtent.height;
      framebufferInfo.layers = 1;

      auto result = vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_SwapchainFramebuffers[i]);
      CORE_ASSERT(result == VK_SUCCESS, "Failed to create framebuffer!");
    }
  }

  void VulkanContext::CleanupSwapchain()
  {
    vkDestroyImageView(m_Device, m_DepthImage.imageView, nullptr);
    vmaDestroyImage(m_Allocator, m_DepthImage.image, m_DepthImage.allocation);

    for (auto framebuffer : m_SwapchainFramebuffers) {
      vkDestroyFramebuffer(m_Device, framebuffer, nullptr);
    }

    for (auto imageView : m_SwapchainImageViews) {
      vkDestroyImageView(m_Device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
  }

  void VulkanContext::RecreateSwapchain()
  {
    vkDeviceWaitIdle(m_Device);

    CleanupSwapchain();
    CreateSwapchain();
    CreateDepthResources();
    CreateFramebuffers();
  }

  const bool VulkanContext::CheckValidationLayerSupport() const
  {
    uint32_t layerCount;
    auto result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to enumerate instance layer properties!");

    std::vector<VkLayerProperties> availableLayers(layerCount);
    result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    CORE_ASSERT(result == VK_SUCCESS, "Failed to enumerate instance layer properties!");

    for (const char* layerName : m_ValidationLayers) {
      bool layerFound = false;

      for (const auto& layerProperties : availableLayers) {
        if (strcmp(layerName, layerProperties.layerName) == 0) {
          layerFound = true;
          break;
        }
      }

      if (!layerFound) { return false; }
    }

    return true;
  }

  const std::vector<const char*> VulkanContext::GetRequiredExtensions() const
  {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
  }

  void VulkanContext::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
  {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    createInfo.pfnUserCallback = DebugCallback;
  }

  const VkResult VulkanContext::CreateDebugUtilsMessengerEXT(const VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger)
  {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
      return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else { return VK_ERROR_EXTENSION_NOT_PRESENT; }
  }

  void VulkanContext::DestroyDebugUtilsMessengerEXT(const VkInstance instance, const VkDebugUtilsMessengerEXT debugMessenger,
                                                    const VkAllocationCallbacks* pAllocator)
  {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
      func(instance, debugMessenger, pAllocator);
    }
  }

  const bool VulkanContext::CheckDeviceExtensionSupport(const VkPhysicalDevice device) const
  {
    uint32_t extensionCount;
    auto result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to enumerate device extension properties!");

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    CORE_ASSERT(result == VK_SUCCESS, "Failed to enumerate device extension properties!");

    std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

    for (const auto& extension : availableExtensions) {
      requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
  }

  const QueueFamilyIndices VulkanContext::FindQueueFamilies(const VkPhysicalDevice device) const
  {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies) {
      if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        indices.graphicsFamily = i;
      }

      VkBool32 presentSupport = false;
      auto result = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);
      CORE_ASSERT(result == VK_SUCCESS, "Failed to get physical device surface support!");

      if (presentSupport) { indices.presentFamily = i; }
      if (indices.IsComplete()) { break; }
      ++i;
    }

    return indices;
  }

  const SwapChainSupportDetails VulkanContext::QuerySwapChainSupport(const VkPhysicalDevice device) const
  {
    SwapChainSupportDetails details;

    auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.capabilities);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to get physical device surface capabilities!");

    uint32_t formatCount;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to get physical device surface formats!");

    if (formatCount != 0) {
      details.formats.resize(formatCount);
      result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.formats.data());
      CORE_ASSERT(result == VK_SUCCESS, "Failed to get physical device surface formats!");
    }

    uint32_t presentModeCount;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to get physical device surface present modes!");

    if (presentModeCount != 0) {
      details.presentModes.resize(presentModeCount);
      result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.presentModes.data());
      CORE_ASSERT(result == VK_SUCCESS, "Failed to get physical device surface present modes!");
    }

    return details;
  }

  const bool VulkanContext::IsDeviceSuitable(const VkPhysicalDevice device) const
  {
    QueueFamilyIndices indices = FindQueueFamilies(device);

    bool extensionsSupported = CheckDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
      SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
      swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures{};
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.IsComplete() && extensionsSupported && swapChainAdequate &&
           supportedFeatures.samplerAnisotropy;
  }

  const VkSurfaceFormatKHR VulkanContext::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const
  {
    for (const auto& availableFormat : availableFormats) {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        return availableFormat;
      }
    }
    return availableFormats[0];
  }

  const VkPresentModeKHR VulkanContext::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const
  {
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

    if (m_Vsync) { return bestMode; }

    for (const auto& availablePresentMode : availablePresentModes) {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) { return availablePresentMode; }
      else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) { bestMode = availablePresentMode; }
    }

    return bestMode;
  }

  const VkExtent2D VulkanContext::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
  {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
    }
    else {
      int width, height;
      glfwGetFramebufferSize(m_WindowHandle, &width, &height);

      VkExtent2D actualExtent = {
          static_cast<uint32_t>(width),
          static_cast<uint32_t>(height)
      };

      actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
      actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

      return actualExtent;
    }
  }

  const uint32_t VulkanContext::ChooseImageCount(const VkSurfaceCapabilitiesKHR& capabilities) const
  {
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
      imageCount = capabilities.maxImageCount;
    }
    return std::max(imageCount, MAX_FRAMES_IN_FLIGHT);
  }

  const VkFormat VulkanContext::FindSupportedFormat(const std::vector<VkFormat>& candidates, const VkImageTiling tiling,
                                                    const VkFormatFeatureFlags features) const
  {
    for (const VkFormat format : candidates) {
      VkFormatProperties props;
      vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

      if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
        return format;
      }
      else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
        return format;
      }
    }

    CORE_ASSERT(false, "Failed to find supported format!");
  }

  const VkFormat VulkanContext::FindDepthFormat() const
  {
    return FindSupportedFormat(
      { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
  }
}