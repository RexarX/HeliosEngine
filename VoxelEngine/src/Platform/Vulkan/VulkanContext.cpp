#include "vepch.h"

#include "VulkanContext.h"

#include <glfw/glfw3.h>

namespace VoxelEngine
{
  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                      const VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                      void* pUserData)
  {
    VE_CORE_INFO("Validation layer: {0}", pCallbackData->pMessage);

    return VK_FALSE;
  }

	VulkanContext::VulkanContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		VE_CORE_ASSERT(windowHandle, "Window handle is null!")
	}

	void VulkanContext::Init()
	{
		glfwMakeContextCurrent(m_WindowHandle);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    CreateInstance();
    SetupDebugCallback();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
	}

  void VulkanContext::Shutdown()
  {
    for (const auto& imageView : m_SwapChainImageViews) {
      m_Device->destroyImageView(imageView);
    }

    m_Instance->destroySurfaceKHR();

    m_Device->destroySwapchainKHR();

    if (enableValidationLayers) { DestroyDebugUtilsMessengerEXT(nullptr); }
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
      VK_MAKE_VERSION(1, 0, 24),
      "VoxelEngine",
      VK_MAKE_VERSION(1, 0, 24),
      VK_API_VERSION_1_0
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

    m_Instance = vk::createInstanceUnique(createInfo, nullptr);
    VE_CORE_ASSERT(m_Instance, "Failed to create instance!");
  }

  void VulkanContext::SetupDebugCallback()
  {
    if (!enableValidationLayers) { return; }

    auto createInfo = vk::DebugUtilsMessengerCreateInfoEXT(
      vk::DebugUtilsMessengerCreateFlagsEXT(),
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
      vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
      vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
      debugCallback,
      nullptr
    );

    VE_CORE_ASSERT(CreateDebugUtilsMessengerEXT(reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&createInfo),
                                                nullptr) == VK_SUCCESS, "Failed to set up debug callback!");
  } 

  void VulkanContext::CreateSurface()
  {
    vk::SurfaceKHR rawSurface;

    VE_CORE_ASSERT(glfwCreateWindowSurface(*m_Instance, m_WindowHandle, nullptr, &rawSurface) = VK_SUCCESS,
                   "Failed to create window surface!");
    
    m_Surface = rawSurface;
  }

  void VulkanContext::PickPhysicalDevice()
  {
    auto devices = m_Instance->enumeratePhysicalDevices();
    VE_CORE_ASSERT(devices.empty(), "Failed to find GPUs with Vulkan support!");

    for (const auto& device : devices) {
      m_PhysicalDevice = device;
      if (IsDeviceSuitable()) { break; }
    }

    VE_CORE_ASSERT(m_PhysicalDevice, "Failed to find a suitable GPU!");
  }

  void VulkanContext::CreateLogicalDevice()
  {
    QueueFamilyIndices indices = FindQueueFamilies();

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;

    for (uint32_t queueFamily : uniqueQueueFamilies) {
      queueCreateInfos.push_back({
          vk::DeviceQueueCreateFlags(),
          queueFamily,
          1,
          &queuePriority
        });
    }

    auto deviceFeatures = vk::PhysicalDeviceFeatures();
    auto createInfo = vk::DeviceCreateInfo(
      vk::DeviceCreateFlags(),
      static_cast<uint32_t>(queueCreateInfos.size()),
      queueCreateInfos.data()
    );
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

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