#include "VulkanContext.h"
#include "VulkanBuffer.h"

#include <glfw/glfw3.h>

#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

namespace VoxelEngine
{
  VulkanContext* VulkanContext::m_Context = nullptr;

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

	VulkanContext::VulkanContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
    VE_CORE_ASSERT(!m_Context, "Context already exists!");
    VE_CORE_ASSERT(windowHandle, "Window handle is null!")

    m_Context = this;
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
	}

  void VulkanContext::Shutdown()
  {
    m_Device.waitIdle();

    ShutdownImGui();

    m_ImGuiDescriptorAllocator.DestroyPool(m_Device);

    for (auto& frame : m_Frames) {
      m_Device.destroyCommandPool(frame.commandPool);

      m_Device.destroyFence(frame.renderFence);

      m_Device.destroySemaphore(frame.renderSemaphore);
      m_Device.destroySemaphore(frame.swapchainSemaphore);

      frame.deletionQueue.flush();
    }

    m_DeletionQueue.flush();
  }

  void VulkanContext::Update()
  {
    m_Device.waitForFences(1, &GetCurrentFrame().renderFence, true, 1000000000);

    if (m_Resized) {
      m_Resized = false;
      RecreateSwapChain();
    }

    GetCurrentFrame().deletionQueue.flush();

    uint32_t swapchainImageIndex;
    auto result = m_Device.acquireNextImageKHR(m_SwapChain, 1000000000, GetCurrentFrame().swapchainSemaphore, nullptr,
                                               &swapchainImageIndex);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
      RecreateSwapChain();
      return;
    }

    VE_CORE_ASSERT(result == vk::Result::eSuccess, "Failed to acquire next swap chain image!");

    m_DrawExtent.height = std::min(m_SwapChainExtent.height, m_DrawImage.imageExtent.height);
    m_DrawExtent.width = std::min(m_SwapChainExtent.width, m_DrawImage.imageExtent.width);

    m_Device.resetFences(1, &GetCurrentFrame().renderFence);

    vk::CommandBufferBeginInfo bufferBeginInfo;
    bufferBeginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    bufferBeginInfo.pInheritanceInfo = nullptr;
    bufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    vk::CommandBuffer cmd = GetCurrentFrame().commandBuffer;

    cmd.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
    cmd.begin(bufferBeginInfo);

    transition_image(cmd, static_cast<vk::Image>(m_DrawImage.image),
                     vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
    
    vk::ClearColorValue clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
    vk::ImageSubresourceRange clearRange = image_subresource_range(vk::ImageAspectFlagBits::eColor);

    cmd.clearColorImage(static_cast<vk::Image>(m_DrawImage.image),
                                                      vk::ImageLayout::eGeneral, clearValue, clearRange);

    transition_image(cmd, static_cast<vk::Image>(m_DrawImage.image),
                     vk::ImageLayout::eGeneral, vk::ImageLayout::eColorAttachmentOptimal);

    transition_image(cmd, static_cast<vk::Image>(m_DepthImage.image),
                     vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal);

    DrawGeometry(cmd);

    transition_image(cmd, static_cast<vk::Image>(m_DrawImage.image),
                     vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal);

    transition_image(cmd, m_SwapChainImages[swapchainImageIndex],
                     vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    copy_image_to_image(cmd, static_cast<vk::Image>(m_DrawImage.image),
                        m_SwapChainImages[swapchainImageIndex], m_DrawExtent, m_SwapChainExtent);

    transition_image(cmd, m_SwapChainImages[swapchainImageIndex],
                     vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eColorAttachmentOptimal);
    
    if (m_ImGuiEnabled) { DrawImGui(m_SwapChainImageViews[swapchainImageIndex]); }

    transition_image(cmd, m_SwapChainImages[swapchainImageIndex],
                     vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR);

    cmd.end();

    vk::CommandBufferSubmitInfo commandBufferSubmitInfo;
    commandBufferSubmitInfo.sType = vk::StructureType::eCommandBufferSubmitInfo;
    commandBufferSubmitInfo.commandBuffer = cmd;
    commandBufferSubmitInfo.deviceMask = 0;

    vk::SemaphoreSubmitInfo waitInfo = semaphore_submit_info(vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                                             GetCurrentFrame().swapchainSemaphore);
    vk::SemaphoreSubmitInfo signalInfo = semaphore_submit_info(vk::PipelineStageFlagBits2::eAllCommands,
                                                               GetCurrentFrame().renderSemaphore);

    vk::SubmitInfo2 submitInfo;
    submitInfo.sType = vk::StructureType::eSubmitInfo2;
    submitInfo.waitSemaphoreInfoCount = &waitInfo == nullptr ? 0 : 1;
    submitInfo.pWaitSemaphoreInfos = &waitInfo;
    submitInfo.signalSemaphoreInfoCount = &signalInfo == nullptr ? 0 : 1;
    submitInfo.pSignalSemaphoreInfos = &signalInfo;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &commandBufferSubmitInfo;

    m_GraphicsQueue.submit2(submitInfo, GetCurrentFrame().renderFence);
    VE_CORE_ASSERT(m_GraphicsQueue, "Failed to submit RenderFence!");

    vk::PresentInfoKHR presentInfo;
    presentInfo.sType = vk::StructureType::ePresentInfoKHR;
    presentInfo.pSwapchains = &m_SwapChain;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &GetCurrentFrame().renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices = &swapchainImageIndex;

    result = m_PresentQueue.presentKHR(presentInfo);
    VE_CORE_ASSERT(result == vk::Result::eSuccess, "Failed to set presentKHR in GraphicsQueue!");

    ++m_FrameNumber;
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
    std::vector<PoolSizeRatio> pool_sizes =
    {
      { vk::DescriptorType::eSampler, 1000 },
      { vk::DescriptorType::eCombinedImageSampler, 1000 },
      { vk::DescriptorType::eSampledImage, 1000 },
      { vk::DescriptorType::eStorageImage, 1000 },
      { vk::DescriptorType::eUniformTexelBuffer, 1000 },
      { vk::DescriptorType::eStorageTexelBuffer, 1000 },
      { vk::DescriptorType::eUniformBuffer, 1000 },
      { vk::DescriptorType::eStorageBuffer, 1000 },
      { vk::DescriptorType::eUniformBufferDynamic, 1000 },
      { vk::DescriptorType::eStorageBufferDynamic, 1000 },
      { vk::DescriptorType::eInputAttachment, 1000 }
    };

    m_ImGuiDescriptorAllocator.InitPool(m_Device, 1000, pool_sizes);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_Instance;
    init_info.PhysicalDevice = m_PhysicalDevice;
    init_info.Device = m_Device;
    init_info.Queue = m_GraphicsQueue;
    init_info.DescriptorPool = static_cast<VkDescriptorPool>(m_ImGuiDescriptorAllocator.pool);
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.UseDynamicRendering = true;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    init_info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = (const VkFormat*)&m_SwapChainImageFormat;

    ImGui_ImplGlfw_InitForVulkan(m_WindowHandle, true);
    ImGui_ImplVulkan_Init(&init_info);

    ImGui_ImplVulkan_CreateFontsTexture();
  }

  void VulkanContext::ShutdownImGui()
  {
    ImGui_ImplVulkan_Shutdown();
  }

  void VulkanContext::Begin()
  {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
  }

  void VulkanContext::End()
  {
    ImGui::Render();

    GLFWwindow* backup_current_context = glfwGetCurrentContext();

    #ifdef VE_PLATFORM_WINDOWS
		  ImGui::UpdatePlatformWindows();
		  ImGui::RenderPlatformWindowsDefault();
    #endif

		glfwMakeContextCurrent(backup_current_context);
  }

  void VulkanContext::SetVSync(const bool enabled)
  {
    m_Vsync = enabled;
    RecreateSwapChain();
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

  vk::SemaphoreSubmitInfo VulkanContext::semaphore_submit_info(const vk::PipelineStageFlags2 stageMask,
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

  vk::ImageCreateInfo VulkanContext::image_create_info(const vk::Format format, const vk::ImageUsageFlags usageFlags,
    const vk::Extent3D extent) const
  {
    vk::ImageCreateInfo info;
    info.sType = vk::StructureType::eImageCreateInfo;
    info.imageType = vk::ImageType::e2D;
    info.format = format;
    info.extent = extent;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = vk::SampleCountFlagBits::e1;
    info.tiling = vk::ImageTiling::eOptimal;
    info.usage = usageFlags;

    return info;
  }

  vk::ImageViewCreateInfo VulkanContext::imageview_create_info(const vk::Image image, const vk::Format format,
    const vk::ImageAspectFlags aspectFlags) const
  {
    vk::ImageViewCreateInfo info;
    info.sType = vk::StructureType::eImageViewCreateInfo;
    info.viewType = vk::ImageViewType::e2D;
    info.image = image;
    info.format = format;
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

  void VulkanContext::Build()
  {
    for (auto& [name, effect] : m_ComputeEffects) {
      effect.Build();
    }

    m_DeletionQueue.push_function([&]() {
      for (auto& [name, effect] : m_ComputeEffects) {
        effect.Destroy();
      }
      });
  }

  void VulkanContext::ImmediateSubmit(std::function<void(vk::CommandBuffer cmd)>&& function)
  {
    m_Device.resetFences(1, &m_ImFence);
    m_ImCommandBuffer.reset();

    vk::CommandBuffer cmd = m_ImCommandBuffer;

    vk::CommandBufferBeginInfo cmdBeginInfo;
    cmdBeginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    cmdBeginInfo.pInheritanceInfo = nullptr;
    cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    cmd.begin(cmdBeginInfo);

    function(cmd);

    cmd.end();

    vk::CommandBufferSubmitInfo commandBufferSubmitInfo;
    commandBufferSubmitInfo.sType = vk::StructureType::eCommandBufferSubmitInfo;
    commandBufferSubmitInfo.commandBuffer = cmd;
    commandBufferSubmitInfo.deviceMask = 0;

    vk::SubmitInfo2 submitInfo;
    submitInfo.sType = vk::StructureType::eSubmitInfo2;
    submitInfo.waitSemaphoreInfoCount = 0;
    submitInfo.pWaitSemaphoreInfos = nullptr;
    submitInfo.signalSemaphoreInfoCount = 0;
    submitInfo.pSignalSemaphoreInfos = nullptr;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &commandBufferSubmitInfo;

    m_GraphicsQueue.submit2(submitInfo, m_ImFence);

    m_Device.waitForFences(1, &m_ImFence, true, 9999999999);
  }

  void VulkanContext::AddComputeEffect(const std::string& name)
  {
    ComputeEffect effect;

    m_ComputeEffects.emplace(name, effect);
    m_ComputeEffects[name].Init();
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

    m_DeletionQueue.push_function([&]() {
      m_Instance.destroy();
      });
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

    m_DeletionQueue.push_function([&]() {
      DestroyDebugUtilsMessengerEXT(nullptr);
      });
  } 

  void VulkanContext::CreateSurface()
  {
    VkSurfaceKHR rawSurface;

    auto result = glfwCreateWindowSurface(m_Instance, m_WindowHandle, nullptr, &rawSurface);

    VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create window surface!");
    
    m_Surface = static_cast<vk::SurfaceKHR>(rawSurface);

    m_DeletionQueue.push_function([&]() {
      m_Instance.destroySurfaceKHR(m_Surface);
      });
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

    m_DeletionQueue.push_function([&]() {
      m_Device.destroy();
      });
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

    m_DeletionQueue.push_function([&]() {
      m_Device.destroySwapchainKHR(m_SwapChain);
      });
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

    VkImageCreateInfo rimg_info = image_create_info(m_DrawImage.imageFormat, drawImageUsages, drawImageExtent);

    VmaAllocationCreateInfo rimg_allocinfo = {};
    rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vmaCreateImage(m_Allocator, &rimg_info, &rimg_allocinfo, &m_DrawImage.image, &m_DrawImage.allocation, nullptr);

    vk::ImageViewCreateInfo rview_info = imageview_create_info(static_cast<vk::Image>(m_DrawImage.image), m_DrawImage.imageFormat,
                                                               vk::ImageAspectFlagBits::eColor);

    auto result = m_Device.createImageView(&rview_info, nullptr, &m_DrawImage.imageView);

    VE_CORE_ASSERT(result == vk::Result::eSuccess, "Failed to create image view!");

    m_DepthImage.imageFormat = vk::Format::eD32Sfloat;
    m_DepthImage.imageExtent = drawImageExtent;

    vk::ImageUsageFlags depthImageUsages;
    depthImageUsages |= vk::ImageUsageFlagBits::eDepthStencilAttachment;

    VkImageCreateInfo dimg_info = image_create_info(m_DepthImage.imageFormat, depthImageUsages, drawImageExtent);

    vmaCreateImage(m_Allocator, &dimg_info, &rimg_allocinfo, &m_DepthImage.image, &m_DepthImage.allocation, nullptr);

    vk::ImageViewCreateInfo dview_info = imageview_create_info(static_cast<vk::Image>(m_DepthImage.image), m_DepthImage.imageFormat,
                                                               vk::ImageAspectFlagBits::eDepth);

    result = m_Device.createImageView(&dview_info, nullptr, &m_DepthImage.imageView);

    VE_CORE_ASSERT(result == vk::Result::eSuccess, "Failed to create image view!");

    m_DeletionQueue.push_function([&]() {
      vmaDestroyImage(m_Allocator, m_DrawImage.image, m_DrawImage.allocation);

      m_Device.destroyImageView(m_DrawImage.imageView);

      for (auto& imageView : m_SwapChainImageViews) {
        m_Device.destroyImageView(imageView);
      }

      vmaDestroyImage(m_Allocator, m_DepthImage.image, m_DepthImage.allocation);

      m_Device.destroyImageView(m_DepthImage.imageView);
      });
  }

  void VulkanContext::CreateCommands()
  {
    QueueFamilyIndices indices = FindQueueFamilies();

    vk::CommandPoolCreateInfo commandPoolInfo;
    commandPoolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
    commandPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    commandPoolInfo.queueFamilyIndex = indices.graphicsFamily.value();

    vk::CommandBufferAllocateInfo cmdAllocInfo;
    cmdAllocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
    cmdAllocInfo.commandBufferCount = 1;
    cmdAllocInfo.level = vk::CommandBufferLevel::ePrimary;

    for (auto& frame : m_Frames) {
      frame.commandPool = m_Device.createCommandPool(commandPoolInfo);
      VE_CORE_ASSERT(frame.commandPool, "Failed to create command pool!");
      
      cmdAllocInfo.commandPool = frame.commandPool;

      frame.commandBuffer = m_Device.allocateCommandBuffers(cmdAllocInfo).front();
      VE_CORE_ASSERT(frame.commandBuffer, "Failed to create command buffer!");
    }

    m_ImCommandPool = m_Device.createCommandPool(commandPoolInfo);
    VE_CORE_ASSERT(m_ImCommandPool, "Failed to create command pool!");

    cmdAllocInfo.commandPool = m_ImCommandPool;

    m_ImCommandBuffer = m_Device.allocateCommandBuffers(cmdAllocInfo).front();
    VE_CORE_ASSERT(m_ImCommandBuffer, "Failed to create command buffer!");

    m_DeletionQueue.push_function([&]() {
      m_Device.destroyCommandPool(m_ImCommandPool);
      });
  }

  void VulkanContext::CreateSyncObjects()
  {
    vk::FenceCreateInfo fenceInfo;
    fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    vk::SemaphoreCreateInfo semaphoreInfo;
    semaphoreInfo.sType = vk::StructureType::eSemaphoreCreateInfo;

    for (auto& frame : m_Frames) {
      frame.renderFence = m_Device.createFence(fenceInfo);
      VE_CORE_ASSERT(frame.renderFence, "Failed to create fence!");

      frame.swapchainSemaphore = m_Device.createSemaphore(semaphoreInfo);
      VE_CORE_ASSERT(frame.swapchainSemaphore, "Failed to create semaphore!");

      frame.renderSemaphore = m_Device.createSemaphore(semaphoreInfo);
      VE_CORE_ASSERT(frame.renderSemaphore, "Failed to create semaphore!");
    }

    m_ImFence = m_Device.createFence(fenceInfo);
    VE_CORE_ASSERT(m_ImFence, "Failed to create fence!");

    m_DeletionQueue.push_function([&]() {
      m_Device.destroyFence(m_ImFence);
      });
  }

  void VulkanContext::CreateAllocator()
  {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = m_PhysicalDevice;
    allocatorInfo.device = m_Device;
    allocatorInfo.instance = m_Instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    vmaCreateAllocator(&allocatorInfo, &m_Allocator);

    m_DeletionQueue.push_function([&]() {
      vmaDestroyAllocator(m_Allocator);
      });
  }

  void VulkanContext::RecreateSwapChain()
  {
    m_Device.waitIdle();
    
    m_Device.destroyImageView(m_DepthImage.imageView);

    vmaDestroyImage(m_Allocator, m_DepthImage.image, m_DepthImage.allocation);

    for (auto& imageView : m_SwapChainImageViews) {
      m_Device.destroyImageView(imageView);
    }

    vmaDestroyImage(m_Allocator, m_DrawImage.image, m_DrawImage.allocation);

    m_Device.destroyImageView(m_DrawImage.imageView);
    
    m_Device.destroySwapchainKHR(m_SwapChain);  
    
    CreateSwapChain();
    CreateImageViews();

    for (auto& [name, effect] : m_ComputeEffects) {
      effect.descriptorWriter.UpdateSet(m_Device, effect.descriptorSet);
    }
  }

  void VulkanContext::DrawGeometry(const vk::CommandBuffer cmd)
  {
    vk::RenderingAttachmentInfo colorAttachment;
    colorAttachment.sType = vk::StructureType::eRenderingAttachmentInfo;
    colorAttachment.imageView = m_DrawImage.imageView;
    colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;

    vk::RenderingAttachmentInfo depthAttachment;
    depthAttachment.sType = vk::StructureType::eRenderingAttachmentInfo;
    depthAttachment.imageView = m_DepthImage.imageView;
    depthAttachment.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    depthAttachment.clearValue.depthStencil.depth = 0.0f;

    vk::RenderingInfo renderInfo;
    renderInfo.sType = vk::StructureType::eRenderingInfo;
    renderInfo.renderArea = vk::Rect2D{ vk::Offset2D { 0, 0 }, m_DrawExtent };
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachment;
    renderInfo.pDepthAttachment = &depthAttachment;
    renderInfo.pStencilAttachment = nullptr;

    vk::Viewport viewport;
    viewport.x = 0;
    viewport.y = m_DrawExtent.height;
    viewport.width = m_DrawExtent.width;
    viewport.height = -static_cast<float>(m_DrawExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = m_DrawExtent.width;
    scissor.extent.height = m_DrawExtent.height;

    cmd.beginRendering(&renderInfo);

    cmd.setViewport(0, 1, &viewport);
    cmd.setScissor(0, 1, &scissor);

    for (auto& [name, effect] : m_ComputeEffects) {
      cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, effect.pipeline);

      cmd.bindVertexBuffers(0, static_cast<vk::Buffer>(effect.vertexBuffer->GetBuffer().buffer), { 0 });

      cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, effect.pipelineLayout, 0, 1,
                             &effect.descriptorSet, 0, nullptr);

      if (effect.pushConstant != nullptr) {
        cmd.pushConstants(effect.pipelineLayout, effect.pipelineBuilder.pushConstantRanges[0].stageFlags,
                          0, effect.pushConstantSize, effect.pushConstant);
      }

      if (effect.indexBuffer != nullptr) {
        cmd.bindIndexBuffer(effect.indexBuffer->GetBuffer().buffer, 0, vk::IndexType::eUint32);
        cmd.drawIndexed(effect.indexBuffer->GetCount(), 1, 0, 0, 0);
      } else {
        cmd.draw(effect.vertexBuffer->GetVertices().size(), 1, 0, 0);
      }
    }

    cmd.endRendering();
  }

  void VulkanContext::DrawImGui(const vk::ImageView view)
  {
    vk::RenderingAttachmentInfo colorAttachment;
    colorAttachment.sType = vk::StructureType::eRenderingAttachmentInfo;
    colorAttachment.imageView = view;
    colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;

    vk::RenderingInfo renderInfo;
    renderInfo.sType = vk::StructureType::eRenderingInfo;
    renderInfo.renderArea = vk::Rect2D{ vk::Offset2D { 0, 0 }, m_SwapChainExtent };
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachment;
    renderInfo.pDepthAttachment = nullptr;
    renderInfo.pStencilAttachment = nullptr;

    GetCurrentFrame().commandBuffer.beginRendering(renderInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), GetCurrentFrame().commandBuffer);

    GetCurrentFrame().commandBuffer.endRendering();
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

  SwapChainSupportDetails VulkanContext::QuerySwapChainSupport() const
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

  QueueFamilyIndices VulkanContext::FindQueueFamilies() const
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

  const std::vector<const char*> VulkanContext::GetRequiredExtensions() const
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
}
