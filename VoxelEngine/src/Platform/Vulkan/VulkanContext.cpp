#include "vepch.h"

#include "VulkanContext.h"

#include <glfw/glfw3.h>

namespace VoxelEngine
{
  vk::UniqueInstance VulkanContext::m_Instance;

  vk::UniqueDevice VulkanContext::m_Device;

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    const VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
  {
    VE_CORE_INFO("Validation layer: {0}", pCallbackData->pMessage);

    return VK_FALSE;
  }

  VkImageSubresourceRange image_subresource_range(const vk::ImageAspectFlags& aspectMask)
  {
    vk::ImageSubresourceRange subImage;
    subImage.aspectMask = aspectMask;
    subImage.baseMipLevel = 0;
    subImage.levelCount = VK_REMAINING_MIP_LEVELS;
    subImage.baseArrayLayer = 0;
    subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return subImage;
  }

  void VulkanContext::transition_image(const vk::Image& image, const vk::ImageLayout& currentLayout,
                                       const vk::ImageLayout& newLayout)
  {
    vk::ImageMemoryBarrier2 imageBarrier;
    imageBarrier.sType = vk::StructureType::eImageMemoryBarrier2;
    imageBarrier.pNext = nullptr;
    imageBarrier.srcStageMask = vk::PipelineStageFlagBits2::eAllCommands;
    imageBarrier.srcAccessMask = vk::AccessFlagBits2::eMemoryWrite;
    imageBarrier.dstStageMask = vk::PipelineStageFlagBits2::eAllCommands;
    imageBarrier.dstAccessMask = vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead;
    imageBarrier.oldLayout = currentLayout;
    imageBarrier.newLayout = newLayout;

    vk::ImageAspectFlags aspectMask = (vk::ImageLayout::eGeneral == vk::ImageLayout::eDepthStencilAttachmentOptimal) ?
                                       vk::ImageAspectFlagBits::eDepth :
                                       vk::ImageAspectFlagBits::eColor;

    imageBarrier.subresourceRange = image_subresource_range(aspectMask);
    imageBarrier.image = image;

    vk::DependencyInfo depInfo;
    depInfo.sType = vk::StructureType::eDependencyInfo;
    depInfo.pNext = nullptr;
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &imageBarrier;

    for (auto& commandBuffer : m_CommandBuffers) {
      commandBuffer.pipelineBarrier2(depInfo);
    }
  }

  vk::SemaphoreSubmitInfo VulkanContext::semaphore_submit_info(const vk::PipelineStageFlags2& stageMask,
                                                             const vk::Semaphore& semaphore) const
  {
    vk::SemaphoreSubmitInfo submitInfo;
    submitInfo.sType = vk::StructureType::eSemaphoreSubmitInfo;
    submitInfo.pNext = nullptr;
    submitInfo.semaphore = semaphore;
    submitInfo.stageMask = stageMask;
    submitInfo.deviceIndex = 0;
    submitInfo.value = 1;

    return submitInfo;
  }

	VulkanContext::VulkanContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		VE_CORE_ASSERT(windowHandle, "Window handle is null!")
	}

	void VulkanContext::Init()
	{
    CreateInstance();
    SetupDebugCallback();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateImageViews();
    CreateCommands();
    CreateSyncObjects();
	}

  void VulkanContext::Shutdown()
  {
    m_Device->destroyCommandPool(m_CommandPool);

    m_Device->destroyFence(m_RenderFence);

    m_Device->destroySemaphore(m_SwapChainSemaphore);
    m_Device->destroySemaphore(m_RenderSemaphore);

    for (auto& imageView : m_SwapChainImageViews) {
      m_Device->destroyImageView(imageView);
    }
    
    m_Instance->destroySurfaceKHR();

    m_Device->destroySwapchainKHR();

    if (enableValidationLayers) { DestroyDebugUtilsMessengerEXT(nullptr); }
  }

  void VulkanContext::Update()
  {
    m_Device->waitForFences(1, &m_RenderFence, true, 1000000000);

    m_Device->resetFences(1, &m_RenderFence);

    uint32_t swapchainImageIndex;
    auto result = m_Device->acquireNextImageKHR(m_SwapChain, 1000000000, m_SwapChainSemaphore, nullptr,
                                                &swapchainImageIndex);

    VE_CORE_ASSERT(result == vk::Result::eSuccess, "Failed to acquire next swap chain image!");

    vk::CommandBufferBeginInfo bufferBeginInfo;
    bufferBeginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    bufferBeginInfo.pNext = nullptr;
    bufferBeginInfo.pInheritanceInfo = nullptr;
    bufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    for (auto& commandBuffer : m_CommandBuffers) {
      commandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
      commandBuffer.begin(bufferBeginInfo);
    }

    transition_image(m_SwapChainImages[swapchainImageIndex], vk::ImageLayout::eUndefined,
                                                             vk::ImageLayout::eGeneral);

    vk::ClearColorValue clearValue;
    float flash = std::abs(std::sin(1 / 120.0f));
    clearValue = { 0.0f, 0.0f, flash, 1.0f };

    vk::ImageSubresourceRange clearRange = image_subresource_range(vk::ImageAspectFlagBits::eColor);

    for (auto& commandBuffer : m_CommandBuffers) {
      commandBuffer.clearColorImage(m_SwapChainImages[swapchainImageIndex], vk::ImageLayout::eGeneral,
                                    clearValue, clearRange);
    }

    for (auto& commandBuffer : m_CommandBuffers) {
      commandBuffer.end();
      VE_CORE_ASSERT(commandBuffer, "Failed to end command buffer!");
    }

    vk::CommandBufferSubmitInfo commandBufferSubmitInfo;
    commandBufferSubmitInfo.sType = vk::StructureType::eCommandBufferSubmitInfo;
    commandBufferSubmitInfo.pNext = nullptr;
    commandBufferSubmitInfo.commandBuffer = m_CommandBuffers[0];
    commandBufferSubmitInfo.deviceMask = 0;

    vk::SemaphoreSubmitInfo waitInfo = semaphore_submit_info(vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                                             m_SwapChainSemaphore);
    vk::SemaphoreSubmitInfo signalInfo = semaphore_submit_info(vk::PipelineStageFlagBits2::eAllCommands,
                                                               m_RenderSemaphore);

    vk::SubmitInfo2 submitInfo;
    submitInfo.sType = vk::StructureType::eSubmitInfo2;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreInfoCount = &waitInfo == nullptr ? 0 : 1;
    submitInfo.pWaitSemaphoreInfos = &waitInfo;
    submitInfo.signalSemaphoreInfoCount = &signalInfo == nullptr ? 0 : 1;
    submitInfo.pSignalSemaphoreInfos = &signalInfo;
    submitInfo.commandBufferInfoCount = m_CommandBuffers.size();
    submitInfo.pCommandBufferInfos = &commandBufferSubmitInfo;

    m_GraphicsQueue.submit2(submitInfo, m_RenderFence);

    VE_CORE_ASSERT(m_GraphicsQueue, "Failed to submit RenderFence!");

    vk::PresentInfoKHR presentInfo;
    presentInfo.sType = vk::StructureType::ePresentInfoKHR;
    presentInfo.pNext = nullptr;
    presentInfo.pSwapchains = &m_SwapChain;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &m_RenderSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices = &swapchainImageIndex;

    m_GraphicsQueue.presentKHR(presentInfo);

    VE_CORE_ASSERT(m_GraphicsQueue, "Failed to set presentKHR in GraphicsQueue!");
  }

  void VulkanContext::SwapBuffers()
  {
  }

	void VulkanContext::ClearBuffer()
  {
	}

	void VulkanContext::SetViewport(const uint32_t width, const uint32_t height)
	{
	}

  void VulkanContext::CreateInstance()
  {
    if (enableValidationLayers && !CheckValidationLayerSupport()) {
      VE_CORE_ASSERT(false, "Validation layers requested, but not available!");
    }
    auto appInfo = vk::ApplicationInfo(
      "Application",
      VK_MAKE_VERSION(1, 3, 0),
      "VoxelEngine",
      VK_MAKE_VERSION(1, 3, 0),
      VK_API_VERSION_1_3
    );

    auto extensions = GetRequiredExtensions();

    auto createInfo = vk::InstanceCreateInfo(
      vk::InstanceCreateFlags(),
      &appInfo,
      0, nullptr,
      static_cast<uint32_t>(extensions.size()), extensions.data()
    );

    if (enableValidationLayers) {
      createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();
    }

    m_Instance = vk::createInstanceUnique(createInfo);
    VE_CORE_ASSERT(m_Instance, "Failed to create instance!");
  }

  void VulkanContext::SetupDebugCallback()
  {
    if (!enableValidationLayers) { return; }

    auto createInfo = vk::DebugUtilsMessengerCreateInfoEXT(
      vk::DebugUtilsMessengerCreateFlagsEXT(),
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eError, vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
      vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
      debugCallback,
      nullptr
    );

    auto result = CreateDebugUtilsMessengerEXT(reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&createInfo),
                                            nullptr);
    VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to set up debug callback!");
  } 

  void VulkanContext::CreateSurface()
  {
    VkSurfaceKHR rawSurface;

    auto result = glfwCreateWindowSurface(*m_Instance, m_WindowHandle, nullptr, &rawSurface);

    VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create window surface!");
    
    m_Surface = rawSurface;
  }

  void VulkanContext::PickPhysicalDevice()
  {
    auto devices = m_Instance->enumeratePhysicalDevices();
    VE_CORE_ASSERT(!devices.empty(), "Failed to find GPUs with Vulkan support!");

    int flag = 0;
    vk::PhysicalDevice integratedDivice;
    for (const auto& device : devices) {
      m_PhysicalDevice = device;
      vk::PhysicalDeviceProperties properties = m_PhysicalDevice.getProperties();
      if (IsDeviceSuitable()) {
        if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
          flag = 1;
          break;
        }
        else if (properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
          flag = 2;
          integratedDivice = m_PhysicalDevice;
        }
      }    
    }
    switch (flag)
    {
    case 1: //discrete gpu
      break;

    case 2: //integrated gpu
      m_PhysicalDevice = integratedDivice;
      break;

    default: //no gpu :(
      VE_CORE_ASSERT(false, "Failed to find a suitable GPU!");
      break;
    }

    vk::PhysicalDeviceProperties properties = m_PhysicalDevice.getProperties();

    VE_CORE_INFO("Vulkan Info:");
    VE_CORE_INFO("  GPU: {0}", properties.deviceName);
    VE_CORE_INFO("  Version: {0}", properties.driverVersion);
  }

  void VulkanContext::CreateLogicalDevice()
  {
    QueueFamilyIndices indices = FindQueueFamilies();

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;

    for (const uint32_t queueFamily : uniqueQueueFamilies) {
      queueCreateInfos.push_back({
          vk::DeviceQueueCreateFlags(),
          queueFamily,
          1,
          &queuePriority
        });
    }

    auto deviceFeatures = vk::PhysicalDeviceFeatures();

    vk::PhysicalDeviceVulkan13Features features;
    features.dynamicRendering = true;
    features.synchronization2 = true;

    vk::PhysicalDeviceVulkan12Features features12;
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;
    features12.pNext = &features;

    auto createInfo = vk::DeviceCreateInfo(
      vk::DeviceCreateFlags(),
      static_cast<uint32_t>(queueCreateInfos.size()),
      queueCreateInfos.data()
    );
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.pNext = &features12;

    if (enableValidationLayers) {
      createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();
    }

    m_Device = m_PhysicalDevice.createDeviceUnique(createInfo);

    m_GraphicsQueue = m_Device->getQueue(indices.graphicsFamily.value(), 0);
    m_PresentQueue = m_Device->getQueue(indices.presentFamily.value(), 0);
  }

  void VulkanContext::CreateSwapChain()
  {
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport();

    vk::SurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    vk::PresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    vk::Extent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = ++swapChainSupport.capabilities.minImageCount;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
      imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo(
      vk::SwapchainCreateFlagsKHR(),
      m_Surface,
      imageCount,
      surfaceFormat.format,
      surfaceFormat.colorSpace,
      extent,
      1,
      vk::ImageUsageFlagBits::eColorAttachment
    );

    QueueFamilyIndices indices = FindQueueFamilies();
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
      createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
      createInfo.queueFamilyIndexCount = 2;
      createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
      createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = vk::SwapchainKHR(nullptr);
    
    m_SwapChain = m_Device->createSwapchainKHR(createInfo);
    
    VE_CORE_ASSERT(m_SwapChain, "Failed to create swap chain!");

    m_SwapChainImages = m_Device->getSwapchainImagesKHR(m_SwapChain);

    m_SwapChainImageFormat = surfaceFormat.format;
    m_SwapChainExtent = extent;
  }

  void VulkanContext::CreateImageViews()
  {
    m_SwapChainImageViews.resize(m_SwapChainImages.size());

    for (uint32_t i = 0; i < m_SwapChainImages.size(); ++i) {
      vk::ImageViewCreateInfo createInfo = {};
      createInfo.image = m_SwapChainImages[i];
      createInfo.viewType = vk::ImageViewType::e2D;
      createInfo.format = m_SwapChainImageFormat;
      createInfo.components.r = vk::ComponentSwizzle::eIdentity;
      createInfo.components.g = vk::ComponentSwizzle::eIdentity;
      createInfo.components.b = vk::ComponentSwizzle::eIdentity;
      createInfo.components.a = vk::ComponentSwizzle::eIdentity;
      createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
      createInfo.subresourceRange.baseMipLevel = 0;
      createInfo.subresourceRange.levelCount = 1;
      createInfo.subresourceRange.baseArrayLayer = 0;
      createInfo.subresourceRange.layerCount = 1;

      m_SwapChainImageViews[i] = m_Device->createImageView(createInfo);

      VE_CORE_ASSERT(m_SwapChainImageViews[i], "Failed to create image views!");
    }
  }

  void VulkanContext::CreateCommands()
  {
    QueueFamilyIndices indices = FindQueueFamilies();

    vk::CommandPoolCreateInfo commandPoolInfo;
    commandPoolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
    commandPoolInfo.pNext = nullptr;
    commandPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    commandPoolInfo.queueFamilyIndex = indices.graphicsFamily.value();

    m_CommandPool = m_Device->createCommandPool(commandPoolInfo);

    VE_CORE_ASSERT(m_CommandPool, "Failed to create command pool!");

    vk::CommandBufferAllocateInfo cmdAllocInfo;
    cmdAllocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
    cmdAllocInfo.pNext = nullptr;
    cmdAllocInfo.commandPool = m_CommandPool;
    cmdAllocInfo.commandBufferCount = 1;
    cmdAllocInfo.level = vk::CommandBufferLevel::ePrimary;

    m_CommandBuffers = m_Device->allocateCommandBuffers(cmdAllocInfo);

    VE_CORE_ASSERT(!m_CommandBuffers.empty(), "Failed to create command buffer!");
  }

  void VulkanContext::CreateSyncObjects()
  {
    vk::FenceCreateInfo fenceInfo;
    fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
    fenceInfo.pNext = nullptr;
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    m_RenderFence = m_Device->createFence(fenceInfo);

    VE_CORE_ASSERT(m_RenderFence, "Failed to create fence!");

    vk::SemaphoreCreateInfo semaphoreInfo;
    semaphoreInfo.sType = vk::StructureType::eSemaphoreCreateInfo;
    semaphoreInfo.pNext = nullptr;

    m_SwapChainSemaphore = m_Device->createSemaphore(semaphoreInfo);
    
    VE_CORE_ASSERT(m_SwapChainSemaphore, "Failed to create semaphore!");

    m_RenderSemaphore = m_Device->createSemaphore(semaphoreInfo);

    VE_CORE_ASSERT(m_RenderSemaphore, "Failed to create semaphore!");
  }

  vk::SurfaceFormatKHR VulkanContext::ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const {
    if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined) {
      return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
    }

    for (const auto& availableFormat : availableFormats) {
      if (availableFormat.format == vk::Format::eB8G8R8A8Unorm &&
          availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
        return availableFormat;
      }
    }

    return availableFormats[0];
  }

  vk::PresentModeKHR VulkanContext::ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) const {
    vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;

    for (const auto& availablePresentMode : availablePresentModes) {
      if (availablePresentMode == vk::PresentModeKHR::eMailbox) { return availablePresentMode; }
      else if (availablePresentMode == vk::PresentModeKHR::eImmediate) { bestMode = availablePresentMode; }
    }

    return bestMode;
  }

  vk::Extent2D VulkanContext::ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
    } else {
      int width, height;
      glfwGetWindowSize(m_WindowHandle, &width, &height);
      vk::Extent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

      actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
      actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

      return actualExtent;
    }
  }

  VulkanContext::SwapChainSupportDetails VulkanContext::QuerySwapChainSupport() {
    SwapChainSupportDetails details;
    details.capabilities = m_PhysicalDevice.getSurfaceCapabilitiesKHR(m_Surface);
    details.formats = m_PhysicalDevice.getSurfaceFormatsKHR(m_Surface);
    details.presentModes = m_PhysicalDevice.getSurfacePresentModesKHR(m_Surface);
    return details;
  }

  bool VulkanContext::IsDeviceSuitable()
  {
    QueueFamilyIndices indices = FindQueueFamilies();

    bool extensionsSupported = CheckDeviceExtensionSupport();

    bool swapChainAdequate = false;
    if (extensionsSupported) {
      SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport();
      swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
  }

  VulkanContext::QueueFamilyIndices VulkanContext::FindQueueFamilies() const
  {
    QueueFamilyIndices indices;
    auto queueFamilies = m_PhysicalDevice.getQueueFamilyProperties();

    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies) {
      if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
        indices.graphicsFamily = i;
      }

      if (queueFamily.queueCount > 0 && m_PhysicalDevice.getSurfaceSupportKHR(i, m_Surface)) {
        indices.presentFamily = i;
      }

      if (indices.isComplete()) { break; }
      ++i;
    }

    return indices;
  }

  std::vector<const char*> VulkanContext::GetRequiredExtensions() const
  {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) { extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); }

    return extensions;
  }

  bool VulkanContext::CheckDeviceExtensionSupport() const
  {
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : m_PhysicalDevice.enumerateDeviceExtensionProperties()) {
      requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
  }

  bool VulkanContext::CheckValidationLayerSupport() const
  {
    auto availableLayers = vk::enumerateInstanceLayerProperties();
    for (const char* layerName : validationLayers) {
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

  VkResult VulkanContext::CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                       const VkAllocationCallbacks* pAllocator)
  {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(*m_Instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) { return func(*m_Instance, pCreateInfo, pAllocator, &m_Callback); } 
    else { return VK_ERROR_EXTENSION_NOT_PRESENT; }
  }

  void VulkanContext::DestroyDebugUtilsMessengerEXT(const VkAllocationCallbacks* pAllocator) const
  {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(*m_Instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) { func(*m_Instance, m_Callback, pAllocator); }
  }
}