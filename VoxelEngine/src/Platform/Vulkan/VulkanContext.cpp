#define VMA_IMPLEMENTATION

#include "VulkanContext.h"

#include "vepch.h"

#include <glfw/glfw3.h>

namespace VoxelEngine
{
  vk::Instance VulkanContext::m_Instance;

  vk::Device VulkanContext::m_Device;

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                      const VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                      void* pUserData)
  {
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
      VE_CORE_ERROR("{0} Validation Layer: {1}: {2}", pCallbackData->messageIdNumber,
                                                      pCallbackData->pMessageIdName,
                                                      pCallbackData->pMessage);
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
      VE_CORE_WARN("{0} Validation Layer: {1}: {2}", pCallbackData->messageIdNumber,
                                                     pCallbackData->pMessageIdName,
                                                     pCallbackData->pMessage);
    }
    else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
      VE_CORE_WARN("{0} Validation Layer: Performance warning: {1}: {2}", pCallbackData->messageIdNumber,
                                                                          pCallbackData->pMessageIdName,
                                                                          pCallbackData->pMessage);
    }
    else {
      VE_CORE_INFO("{0} Validation Layer: {1}: {2}", pCallbackData->messageIdNumber,
                                                     pCallbackData->pMessageIdName,
                                                     pCallbackData->pMessage);
    }
    
    return VK_FALSE;
  }

  vk::ImageSubresourceRange image_subresource_range(const vk::ImageAspectFlags aspectMask)
  {
    vk::ImageSubresourceRange subImage;
    subImage.aspectMask = aspectMask;
    subImage.baseMipLevel = 0;
    subImage.levelCount = VK_REMAINING_MIP_LEVELS;
    subImage.baseArrayLayer = 0;
    subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return subImage;
  }

  void VulkanContext::transition_image(const vk::CommandBuffer cmd, const vk::Image image,
                                       const vk::ImageLayout currentLayout, const vk::ImageLayout newLayout)
  {
    vk::ImageMemoryBarrier2 imageBarrier;
    imageBarrier.sType = vk::StructureType::eImageMemoryBarrier2;
    imageBarrier.srcStageMask = vk::PipelineStageFlagBits2::eAllCommands;
    imageBarrier.srcAccessMask = vk::AccessFlagBits2::eMemoryWrite;
    imageBarrier.dstStageMask = vk::PipelineStageFlagBits2::eAllCommands;
    imageBarrier.dstAccessMask = vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead;
    imageBarrier.oldLayout = currentLayout;
    imageBarrier.newLayout = newLayout;

    vk::ImageAspectFlags aspectMask = (newLayout == vk::ImageLayout::eDepthAttachmentOptimal) ?
                                       vk::ImageAspectFlagBits::eDepth :
                                       vk::ImageAspectFlagBits::eColor;

    imageBarrier.subresourceRange = image_subresource_range(aspectMask);
    imageBarrier.image = image;

    vk::DependencyInfo depInfo;
    depInfo.sType = vk::StructureType::eDependencyInfo;
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &imageBarrier;

    cmd.pipelineBarrier2(depInfo);
  }

  vk::SemaphoreSubmitInfo& VulkanContext::semaphore_submit_info(const vk::PipelineStageFlags2 stageMask,
                                                                const vk::Semaphore semaphore) const
  {
    vk::SemaphoreSubmitInfo submitInfo;
    submitInfo.sType = vk::StructureType::eSemaphoreSubmitInfo;
    submitInfo.semaphore = semaphore;
    submitInfo.stageMask = stageMask;
    submitInfo.deviceIndex = 0;
    submitInfo.value = 1;

    return submitInfo;
  }

  vk::ImageCreateInfo& VulkanContext::image_create_info(const vk::ImageUsageFlags usageFlags,
                                                        const vk::Extent3D extent) const
  {
    vk::ImageCreateInfo info;
    info.sType = vk::StructureType::eImageCreateInfo;
    info.imageType = vk::ImageType::e2D;
    info.format = m_DrawImage.imageFormat;
    info.extent = extent;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = vk::SampleCountFlagBits::e1;
    info.tiling = vk::ImageTiling::eOptimal;
    info.usage = usageFlags;

    return info;
  }

  vk::ImageViewCreateInfo VulkanContext::imageview_create_info(const vk::ImageAspectFlags aspectFlags) const
  {
    vk::ImageViewCreateInfo info;
    info.sType = vk::StructureType::eImageViewCreateInfo;
    info.viewType = vk::ImageViewType::e2D;
    info.image = m_DrawImage.image;
    info.format = m_DrawImage.imageFormat;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    info.subresourceRange.aspectMask = aspectFlags;

    return info;
  }

  void VulkanContext::copy_image_to_image(const vk::CommandBuffer cmd, const vk::Image source,
                                          const vk::Image destination, const vk::Extent2D srcSize,
                                          const vk::Extent2D dstSize) const
  {
    vk::ImageBlit2 blitRegion;
    blitRegion.sType = vk::StructureType::eImageBlit2KHR;;

    blitRegion.srcOffsets[1].x = srcSize.width;
    blitRegion.srcOffsets[1].y = srcSize.height;
    blitRegion.srcOffsets[1].z = 1;

    blitRegion.dstOffsets[1].x = dstSize.width;
    blitRegion.dstOffsets[1].y = dstSize.height;
    blitRegion.dstOffsets[1].z = 1;

    blitRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount = 1;
    blitRegion.srcSubresource.mipLevel = 0;

    blitRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount = 1;
    blitRegion.dstSubresource.mipLevel = 0;

    vk::BlitImageInfo2 blitInfo;
    blitInfo.sType = vk::StructureType::eBlitImageInfo2;
    blitInfo.dstImage = destination;
    blitInfo.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
    blitInfo.srcImage = source;
    blitInfo.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;
    blitInfo.filter = vk::Filter::eLinear;
    blitInfo.regionCount = 1;
    blitInfo.pRegions = &blitRegion;

    cmd.blitImage2(blitInfo);
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
    CreateAllocator();
    CreateSwapChain();
    CreateImageViews();
    CreateCommands();
    CreateSyncObjects();
    CreateDescriptors();
    CreatePipeline();
	}

  void VulkanContext::Shutdown()
  {
    m_Device.waitIdle();

    m_Device.destroyDescriptorSetLayout(m_DrawImageDescriptorLayout);

    m_DescriptorAllocator.DestroyPool();
    
    m_Device.destroyFence(m_RenderFence);

    m_Device.destroySemaphore(m_SwapChainSemaphore);
    m_Device.destroySemaphore(m_RenderSemaphore);

    m_Device.destroyCommandPool(m_CommandPool);

    for (auto& imageView : m_SwapChainImageViews) {
      m_Device.destroyImageView(imageView);
    }

    vmaDestroyImage(m_Allocator, m_DrawImage.image, m_DrawImage.allocation);

    m_Device.destroyImageView(m_DrawImage.imageView);

    vmaDestroyAllocator(m_Allocator);

    m_Device.destroySwapchainKHR(m_SwapChain);

    m_Instance.destroySurfaceKHR(m_Surface);

    m_Device.destroy();

    if (enableValidationLayers) { DestroyDebugUtilsMessengerEXT(nullptr); }

    m_Instance.destroy();
  }

  void VulkanContext::Update()
  {
    m_Device.waitForFences(1, &m_RenderFence, true, 1000000000);

    if (m_Resized) {
      m_Resized = false;
      RecreateSwapChain();
    }

    uint32_t swapchainImageIndex;
    auto result = m_Device.acquireNextImageKHR(m_SwapChain, 1000000000, m_SwapChainSemaphore, nullptr,
                                               &swapchainImageIndex);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
      RecreateSwapChain();
      return;
    }

    VE_CORE_ASSERT(result == vk::Result::eSuccess, "Failed to acquire next swap chain image!");

    m_Device.resetFences(1, &m_RenderFence);

    vk::CommandBufferBeginInfo bufferBeginInfo;
    bufferBeginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    bufferBeginInfo.pInheritanceInfo = nullptr;
    bufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    for (auto& commandBuffer : m_CommandBuffers) {
      commandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
      commandBuffer.begin(bufferBeginInfo);
    }

    transition_image(m_CommandBuffers[0], m_DrawImage.image, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
    
    vk::ClearColorValue clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };

    vk::ImageSubresourceRange clearRange = image_subresource_range(vk::ImageAspectFlagBits::eColor);

    m_CommandBuffers[0].clearColorImage(m_DrawImage.image, vk::ImageLayout::eGeneral, clearValue, clearRange);

    transition_image(m_CommandBuffers[0], m_DrawImage.image, vk::ImageLayout::eGeneral,
                     vk::ImageLayout::eTransferSrcOptimal);

    transition_image(m_CommandBuffers[0], m_SwapChainImages[swapchainImageIndex], vk::ImageLayout::eUndefined,
                     vk::ImageLayout::eTransferDstOptimal);

    copy_image_to_image(m_CommandBuffers[0], m_DrawImage.image, m_SwapChainImages[swapchainImageIndex],
                        m_DrawExtent, m_SwapChainExtent);

    transition_image(m_CommandBuffers[0], m_SwapChainImages[swapchainImageIndex],
                     vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eColorAttachmentOptimal);
    
    //draw imgui

    transition_image(m_CommandBuffers[0], m_SwapChainImages[swapchainImageIndex],
                     vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR);

    m_CommandBuffers[0].end();

    vk::CommandBufferSubmitInfo commandBufferSubmitInfo;
    commandBufferSubmitInfo.sType = vk::StructureType::eCommandBufferSubmitInfo;
    commandBufferSubmitInfo.commandBuffer = m_CommandBuffers[0];
    commandBufferSubmitInfo.deviceMask = 0;

    vk::SemaphoreSubmitInfo waitInfo = semaphore_submit_info(vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                                             m_SwapChainSemaphore);
    vk::SemaphoreSubmitInfo signalInfo = semaphore_submit_info(vk::PipelineStageFlagBits2::eAllCommands,
                                                               m_RenderSemaphore);

    vk::SubmitInfo2 submitInfo;
    submitInfo.sType = vk::StructureType::eSubmitInfo2;
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
    presentInfo.pSwapchains = &m_SwapChain;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &m_RenderSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices = &swapchainImageIndex;

    result = m_PresentQueue.presentKHR(presentInfo);

    VE_CORE_ASSERT(result == vk::Result::eSuccess, "Failed to set presentKHR in GraphicsQueue!");
    ++cnt;
  }

  void VulkanContext::SwapBuffers()
  {
    //already done in update
  }

	void VulkanContext::ClearBuffer()
  {
    //already done in update
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
    m_Vsync = enabled;
    RecreateSwapChain();
  }

  void VulkanContext::CreateInstance()
  {
    if (enableValidationLayers && !CheckValidationLayerSupport()) {
      VE_CORE_ASSERT(false, "Validation layers requested, but not available!");
    }

    vk::ApplicationInfo appInfo;

    appInfo.pApplicationName = "Application";
    appInfo.pEngineName = "VoxelEngine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 3, 0);
    appInfo.engineVersion = VK_MAKE_VERSION(1, 3, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    auto extensions = GetRequiredExtensions();

    vk::InstanceCreateInfo createInfo;
    createInfo.flags = vk::InstanceCreateFlags();
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (enableValidationLayers) {
      createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();
    }

    m_Instance = vk::createInstance(createInfo);
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

    auto result = glfwCreateWindowSurface(m_Instance, m_WindowHandle, nullptr, &rawSurface);

    VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create window surface!");
    
    m_Surface = rawSurface;
  }

  void VulkanContext::PickPhysicalDevice()
  {
    auto devices = m_Instance.enumeratePhysicalDevices();
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

    vk::DeviceCreateInfo createInfo;
    createInfo.flags = vk::DeviceCreateFlags();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.pNext = &features12;

    if (enableValidationLayers) {
      createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();
    }

    m_Device = m_PhysicalDevice.createDevice(createInfo);

    m_GraphicsQueue = m_Device.getQueue(indices.graphicsFamily.value(), 0);
    m_PresentQueue = m_Device.getQueue(indices.presentFamily.value(), 0);
  }

  void VulkanContext::CreateSwapChain()
  {
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport();

    vk::SurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    vk::PresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    vk::Extent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = ++swapChainSupport.capabilities.minImageCount;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
      imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo;
    createInfo.flags = vk::SwapchainCreateFlagsKHR();
    createInfo.surface = m_Surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment |
                            vk::ImageUsageFlagBits::eTransferDst;

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

    m_SwapChain = m_Device.createSwapchainKHR(createInfo);

    VE_CORE_ASSERT(m_SwapChain, "Failed to create swap chain!");

    m_SwapChainImages = m_Device.getSwapchainImagesKHR(m_SwapChain);

    m_SwapChainImageFormat = surfaceFormat.format;
    m_SwapChainExtent = extent;
  }

  void VulkanContext::CreateImageViews()
  {
    m_SwapChainImageViews.resize(m_SwapChainImages.size());

    for (uint32_t i = 0; i < m_SwapChainImages.size(); ++i) {
      vk::ImageViewCreateInfo createInfo;
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

      m_SwapChainImageViews[i] = m_Device.createImageView(createInfo);

      VE_CORE_ASSERT(m_SwapChainImageViews[i], "Failed to create image views!");
    }

    int width, height;
    glfwGetWindowSize(m_WindowHandle, &width, &height);
    
    vk::Extent3D drawImageExtent =
    {
      (uint32_t)width,
      (uint32_t)height,
      1
    };

    m_DrawImage.imageFormat = vk::Format::eR16G16B16A16Sfloat;
    m_DrawImage.imageExtent = drawImageExtent;

    vk::ImageUsageFlags drawImageUsages;
    drawImageUsages |= vk::ImageUsageFlagBits::eTransferSrc;
    drawImageUsages |= vk::ImageUsageFlagBits::eTransferDst;
    drawImageUsages |= vk::ImageUsageFlagBits::eStorage;
    drawImageUsages |= vk::ImageUsageFlagBits::eColorAttachment;

    VkImageCreateInfo rimg_info = image_create_info(drawImageUsages, drawImageExtent);

    VmaAllocationCreateInfo rimg_allocinfo = {};
    rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vmaCreateImage(m_Allocator, &rimg_info, &rimg_allocinfo, &m_DrawImage.image, &m_DrawImage.allocation, nullptr);

    vk::ImageViewCreateInfo rview_info = imageview_create_info(vk::ImageAspectFlagBits::eColor);

    auto result = m_Device.createImageView(&rview_info, nullptr, &m_DrawImage.imageView);

    VE_CORE_ASSERT(result == vk::Result::eSuccess, "Failed to create image view!");
  }

  void VulkanContext::CreatePipeline()
  {
  }

  void VulkanContext::CreateDescriptors()
  {
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes =
	  {
		  { vk::DescriptorType::eStorageImage, 1 }
	  };

    m_DescriptorAllocator.InitPool(10, sizes);

    {
      DescriptorLayoutBuilder builder;
      builder.AddBinding(0, vk::DescriptorType::eStorageImage);
      m_DrawImageDescriptorLayout = builder.Build(vk::ShaderStageFlagBits::eCompute, vk::DescriptorSetLayoutCreateFlagBits(0));
    }
    
    m_DrawImageDescriptors = m_DescriptorAllocator.Allocate(m_DrawImageDescriptorLayout);

    vk::DescriptorImageInfo imgInfo;
    imgInfo.imageLayout = vk::ImageLayout::eGeneral;
    imgInfo.imageView = m_DrawImage.imageView;

    vk::WriteDescriptorSet drawImageWrite;
    drawImageWrite.sType = vk::StructureType::eWriteDescriptorSet;
    drawImageWrite.dstBinding = 0;
    drawImageWrite.dstSet = m_DrawImageDescriptors;
    drawImageWrite.descriptorCount = 1;
    drawImageWrite.descriptorType = vk::DescriptorType::eStorageImage;
    drawImageWrite.pImageInfo = &imgInfo;

    m_Device.updateDescriptorSets(drawImageWrite, nullptr);
  }

  void VulkanContext::CreateCommands()
  {
    QueueFamilyIndices indices = FindQueueFamilies();

    vk::CommandPoolCreateInfo commandPoolInfo;
    commandPoolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
    commandPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    commandPoolInfo.queueFamilyIndex = indices.graphicsFamily.value();

    m_CommandPool = m_Device.createCommandPool(commandPoolInfo);

    VE_CORE_ASSERT(m_CommandPool, "Failed to create command pool!");

    vk::CommandBufferAllocateInfo cmdAllocInfo;
    cmdAllocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
    cmdAllocInfo.commandPool = m_CommandPool;
    cmdAllocInfo.commandBufferCount = 1;
    cmdAllocInfo.level = vk::CommandBufferLevel::ePrimary;

    m_CommandBuffers = m_Device.allocateCommandBuffers(cmdAllocInfo);

    VE_CORE_ASSERT(!m_CommandBuffers.empty(), "Failed to create command buffer!");
  }

  void VulkanContext::CreateSyncObjects()
  {
    vk::FenceCreateInfo fenceInfo;
    fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    m_RenderFence = m_Device.createFence(fenceInfo);

    VE_CORE_ASSERT(m_RenderFence, "Failed to create fence!");

    vk::SemaphoreCreateInfo semaphoreInfo;
    semaphoreInfo.sType = vk::StructureType::eSemaphoreCreateInfo;

    m_SwapChainSemaphore = m_Device.createSemaphore(semaphoreInfo);
    
    VE_CORE_ASSERT(m_SwapChainSemaphore, "Failed to create semaphore!");

    m_RenderSemaphore = m_Device.createSemaphore(semaphoreInfo);

    VE_CORE_ASSERT(m_RenderSemaphore, "Failed to create semaphore!");
  }

  void VulkanContext::CreateAllocator()
  {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = m_PhysicalDevice;
    allocatorInfo.device = m_Device;
    allocatorInfo.instance = m_Instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    vmaCreateAllocator(&allocatorInfo, &m_Allocator);
  }

  void VulkanContext::RecreateSwapChain()
  {
    m_Device.waitIdle();
    
    for (auto& imageView : m_SwapChainImageViews) {
      m_Device.destroyImageView(imageView);
    }

    vmaDestroyImage(m_Allocator, m_DrawImage.image, m_DrawImage.allocation);

    m_Device.destroyImageView(m_DrawImage.imageView);
    
    m_Device.destroySwapchainKHR(m_SwapChain);  
    
    CreateSwapChain();
    CreateImageViews();

    vk::DescriptorImageInfo imgInfo;
    imgInfo.imageLayout = vk::ImageLayout::eGeneral;
    imgInfo.imageView = m_DrawImage.imageView;

    vk::WriteDescriptorSet drawImageWrite;
    drawImageWrite.sType = vk::StructureType::eWriteDescriptorSet;

    drawImageWrite.dstBinding = 0;
    drawImageWrite.dstSet = m_DrawImageDescriptors;
    drawImageWrite.descriptorCount = 1;
    drawImageWrite.descriptorType = vk::DescriptorType::eStorageImage;
    drawImageWrite.pImageInfo = &imgInfo;

    m_Device.updateDescriptorSets(drawImageWrite, nullptr);
  }

  vk::SurfaceFormatKHR VulkanContext::ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const
  {
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

  vk::PresentModeKHR VulkanContext::ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) const
  {
    vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;

    if (m_Vsync) { return bestMode; }

    for (const auto& availablePresentMode : availablePresentModes) {
      if (availablePresentMode == vk::PresentModeKHR::eMailbox) { return availablePresentMode; }
      else if (availablePresentMode == vk::PresentModeKHR::eImmediate) { bestMode = availablePresentMode; }
    }

    return bestMode;
  }

  vk::Extent2D VulkanContext::ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const
  {
    int width, height;
    glfwGetWindowSize(m_WindowHandle, &width, &height);
    vk::Extent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

    actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

    return actualExtent;
  }

  VulkanContext::SwapChainSupportDetails VulkanContext::QuerySwapChainSupport() const
  {
    SwapChainSupportDetails details;
    details.capabilities = m_PhysicalDevice.getSurfaceCapabilitiesKHR(m_Surface);
    details.formats = m_PhysicalDevice.getSurfaceFormatsKHR(m_Surface);
    details.presentModes = m_PhysicalDevice.getSurfacePresentModesKHR(m_Surface);

    return details;
  }

  bool VulkanContext::IsDeviceSuitable() const
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
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) { return func(m_Instance, pCreateInfo, pAllocator, &m_Callback); } 
    else { return VK_ERROR_EXTENSION_NOT_PRESENT; }
  }

  void VulkanContext::DestroyDebugUtilsMessengerEXT(const VkAllocationCallbacks* pAllocator) const
  {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) { func(m_Instance, m_Callback, pAllocator); }
  }

  void VulkanContext::DescriptorLayoutBuilder::AddBinding(const uint32_t binding, vk::DescriptorType type)
  {
    vk::DescriptorSetLayoutBinding newBind;
    newBind.binding = binding;
    newBind.descriptorCount = 1;
    newBind.descriptorType = type;

    bindings.push_back(newBind);
  }

  vk::DescriptorSetLayout VulkanContext::DescriptorLayoutBuilder::Build(const vk::ShaderStageFlags shaderStages,
                                                                        const vk::DescriptorSetLayoutCreateFlags flags,
                                                                        const void* pNext)
  {
    for (auto& bind : bindings) {
      bind.stageFlags |= shaderStages;
    }

    vk::DescriptorSetLayoutCreateInfo info;
    info.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
    info.pNext = pNext;
    info.pBindings = bindings.data();
    info.bindingCount = (uint32_t)(bindings.size());
    info.flags = flags;

    vk::DescriptorSetLayout set;
    set = m_Device.createDescriptorSetLayout(info);

    return set;
  }

  void VulkanContext::DescriptorAllocator::InitPool(const uint32_t maxSets, const std::span<PoolSizeRatio> poolRatios)
  {
    std::vector<vk::DescriptorPoolSize> poolSizes;
    for (auto& poolRatio : poolRatios) {
      poolSizes.push_back(vk::DescriptorPoolSize{
            poolRatio.type,
            uint32_t(poolRatio.ratio * maxSets)
        });
    }

    vk::DescriptorPoolCreateInfo info;
    info.sType = vk::StructureType::eDescriptorPoolCreateInfo;
    info.maxSets = maxSets;
    info.poolSizeCount = (uint32_t)poolSizes.size();
    info.pPoolSizes = poolSizes.data();

    pool = m_Device.createDescriptorPool(info);
  }

  void VulkanContext::DescriptorAllocator::ClearDescriptors()
  {
    m_Device.resetDescriptorPool(pool);
  }

  void VulkanContext::DescriptorAllocator::DestroyPool()
  {
    m_Device.destroyDescriptorPool(pool);
  }

  vk::DescriptorSet VulkanContext::DescriptorAllocator::Allocate(const vk::DescriptorSetLayout layout)
  {
    vk::DescriptorSetAllocateInfo info;
    info.sType = vk::StructureType::eDescriptorSetAllocateInfo;
    info.descriptorPool = pool;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layout;

    vk::DescriptorSet set = m_Device.allocateDescriptorSets(info).front();

    return set;
  }
}