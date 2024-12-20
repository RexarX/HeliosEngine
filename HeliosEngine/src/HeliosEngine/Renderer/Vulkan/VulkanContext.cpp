#include "VulkanContext.h"
#include "VulkanPipelineManager.h"
#include "VulkanMesh.h"

#include "Window.h"

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace Helios {
  VulkanContext::VulkanContext(GLFWwindow* windowHandle)
    : m_WindowHandle(windowHandle) {
    CORE_ASSERT(m_Instance == nullptr, "Failed to create VulkanContext: Context already exists!");
    CORE_ASSERT_CRITICAL(windowHandle != nullptr, "Failed to create VulkanContext: Window handle is null!");

    m_Instance = this;
  }

  void VulkanContext::Init() {
    PROFILE_FUNCTION();
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

  void VulkanContext::Shutdown() {
    PROFILE_FUNCTION();

    vkDeviceWaitIdle(m_Device);

    m_MainDeletionQueue.Flush();
    CleanupSwapchain();

    vmaDestroyAllocator(m_Allocator);
    vkDestroyDevice(m_Device, nullptr);
    vkDestroySurfaceKHR(m_VkInstance, m_Surface, nullptr);

    if (ENABLE_VALIDATION_LAYERS) {
      DestroyDebugUtilsMessengerEXT(m_VkInstance, m_DebugMessenger, nullptr);
    }

    vkDestroyInstance(m_VkInstance, nullptr);
  }

  void VulkanContext::Update() {
    PROFILE_FUNCTION();

    if (m_SwapchainRecreated) { m_SwapchainRecreated = false; return; }
    
    FrameData& frameData = m_Frames[m_CurrentFrame];

    vkCmdEndRenderPass(frameData.commandBuffer);

    VkResult result = vkEndCommandBuffer(frameData.commandBuffer);
    CORE_ASSERT_CRITICAL(result == VK_SUCCESS, "Failed to update Vulkan: Failed to record command buffer!");

    VkSemaphore waitSemaphores[] = { frameData.presentSemaphore };
    VkSemaphore signalSemaphores[] = { frameData.renderSemaphore };

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frameData.commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, frameData.renderFence);
    CORE_ASSERT_CRITICAL(result == VK_SUCCESS, "Failed to update Vulkan: Failed to submit graphics queue!");

    VkSwapchainKHR swapChains[] = { m_Swapchain };

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &m_ImageIndex;

    result = vkQueuePresentKHR(m_GraphicsQueue, &presentInfo);
    CORE_ASSERT_CRITICAL(result == VK_SUCCESS, "Failed to update Vulkan: Failed to present graphics queue!");

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  void VulkanContext::BeginFrame() {
    PROFILE_FUNCTION();

    if (m_SwapchainRecreated) { RecreateSwapchain(); return; }

    FrameData& frameData = m_Frames[m_CurrentFrame];

    VkResult result = vkWaitForFences(m_Device, 1, &frameData.renderFence, VK_TRUE,
                                      std::numeric_limits<uint64_t>::max());

    CORE_ASSERT(result == VK_SUCCESS, "Failed to begin frame Vulkan: Failed to wait for fence!");

    result = vkResetFences(m_Device, 1, &frameData.renderFence);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to begin frame Vulkan: Failed to reset fence!");

    result = vkAcquireNextImageKHR(m_Device, m_Swapchain, std::numeric_limits<uint64_t>::max(),
                                   frameData.presentSemaphore, VK_NULL_HANDLE, &m_ImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) { RecreateSwapchain(); return; }
    CORE_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR,
                "Failed to begin frame Vulkan: Failed to acquire next image!");

    result = vkResetCommandBuffer(frameData.commandBuffer, 0);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to begin frame Vulkan: Failed to reset command buffer!");

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(frameData.commandBuffer, &beginInfo);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to begin frame Vulkan: Failed to begin recording command buffer!");

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_RenderPass;
    renderPassInfo.framebuffer = m_SwapchainFramebuffers[m_ImageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_SwapchainExtent;
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdSetViewport(frameData.commandBuffer, 0, 1, &m_Viewport);
    vkCmdSetScissor(frameData.commandBuffer, 0, 1, &m_Scissor);

    vkCmdBeginRenderPass(frameData.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  }

  void VulkanContext::EndFrame() {
    PROFILE_FUNCTION();

    if (m_SwapchainRecreated) { return; }
  }

  void VulkanContext::Record(const RenderQueue& queue, const PipelineManager& manager) {
    PROFILE_FUNCTION();

    if (m_SwapchainRecreated) { return; }

    const VulkanPipelineManager& pipelineManager = static_cast<const VulkanPipelineManager&>(manager);
    VkCommandBuffer commandBuffer = m_Frames[m_CurrentFrame].commandBuffer;
    std::unordered_map<const VulkanPipelineManager::VulkanEffect*, std::vector<const RenderQueue::RenderObject*>> pipelineGroups;

    for (const RenderQueue::RenderQueue::RenderObject& object : queue.GetRenderObjects()) {
      const VulkanPipelineManager::VulkanEffect& effect = pipelineManager.GetPipeline(
        object.renderable,
        PipelineManager::PipelineType::Regular
      );
      pipelineGroups[&effect].push_back(&object);
    }

    const RenderQueue::SceneData& sceneData = queue.GetSceneData();

    for (const auto& [effect, renderObjects] : pipelineGroups) {
      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, effect->pipeline);

      vkCmdPushConstants(commandBuffer, effect->pipelineLayout, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0,
                         sizeof(sceneData), &sceneData);

      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, effect->pipelineLayout, 0,
                              effect->descriptorSets.size(), effect->descriptorSets.data(), 0, nullptr);

      for (const RenderQueue::RenderObject* renderObject : renderObjects) {
        VulkanMesh* mesh = static_cast<VulkanMesh*>(renderObject->renderable.mesh.get());

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mesh->GetVertexBuffer().buffer, offsets);

        if (mesh->GetIndexCount() > 0) {
          vkCmdBindIndexBuffer(commandBuffer, mesh->GetIndexBuffer().buffer, 0, VK_INDEX_TYPE_UINT32);
          vkCmdDrawIndexed(commandBuffer, mesh->GetIndexCount(), 1, 0, 0, 0);
        } else {
          vkCmdDraw(commandBuffer, mesh->GetVertexCount(), 1, 0, 0);
        }
      }
    }
  }

  void VulkanContext::SetViewport(uint32_t width, uint32_t height, uint32_t x, uint32_t y) {
    m_Viewport.x = static_cast<float>(x);
    m_Viewport.y = static_cast<float>(y + height);
    m_Viewport.width = static_cast<float>(width);
    m_Viewport.height = -static_cast<float>(height);
    m_Viewport.minDepth = 0.0f;
    m_Viewport.maxDepth = 1.0f;

    m_Scissor.offset = { static_cast<int32_t>(x), static_cast<int32_t>(y) };
    m_Scissor.extent = { width, height };

    if (!m_SwapchainImages.empty()) { RecreateSwapchain(); }
  }

  void VulkanContext::InitImGui() {
#ifndef RELEASE_MODE
    PROFILE_FUNCTION();

    VkDescriptorPoolSize pool_sizes[] = {
      { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
      { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
      { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
      { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    vkCreateDescriptorPool(m_Device, &pool_info, nullptr, &m_ImGuiPool);

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = m_VkInstance;
    init_info.PhysicalDevice = m_PhysicalDevice;
    init_info.Device = m_Device;
    init_info.QueueFamily = FindQueueFamilies(m_PhysicalDevice).graphicsFamily.value();
    init_info.Queue = m_GraphicsQueue;
    init_info.RenderPass = m_RenderPass;
    init_info.Subpass = 0;
    init_info.DescriptorPool = m_ImGuiPool;
    init_info.MinImageCount = 2;
    init_info.ImageCount = static_cast<uint32_t>(m_SwapchainImages.size());
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplGlfw_InitForVulkan(m_WindowHandle, true);
    ImGui_ImplVulkan_Init(&init_info);

    ImGui_ImplVulkan_CreateFontsTexture();
#endif
  }

  void VulkanContext::ShutdownImGui() {
#ifndef RELEASE_MODE
    PROFILE_FUNCTION();

    vkDeviceWaitIdle(m_Device);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    vkDestroyDescriptorPool(m_Device, m_ImGuiPool, nullptr);
#endif
  }

  void VulkanContext::BeginFrameImGui() {
#ifndef RELEASE_MODE
    PROFILE_FUNCTION();

    ImGuiViewport* viewPort = ImGui::GetMainViewport();
    if (m_SwapchainRecreated) {
      ImGui::NewFrame();
      ImGui::DockSpaceOverViewport(viewPort->ID, viewPort, ImGuiDockNodeFlags_PassthruCentralNode);
      return;
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(viewPort->ID, viewPort, ImGuiDockNodeFlags_PassthruCentralNode);
#endif
  }

  void VulkanContext::EndFrameImGui() {
#ifndef RELEASE_MODE
    PROFILE_FUNCTION();

    if (m_SwapchainRecreated) {
      ImGui::Render();

#ifdef PLATFORM_WINDOWS
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
#endif

      return;
    }

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_Frames[m_CurrentFrame].commandBuffer);

    GLFWwindow* backupContext = glfwGetCurrentContext();

#ifdef PLATFORM_WINDOWS
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
#endif

    glfwMakeContextCurrent(backupContext);
#endif
  }

  void VulkanContext::SetVSync(bool enabled) {
    if (m_Vsync != enabled) {
      m_Vsync = enabled;
      if (!m_SwapchainImages.empty()) { RecreateSwapchain(); }
    }
  }

  void VulkanContext::ImmediateSubmit(std::function<void(const VkCommandBuffer cmd)>&& function) {
    PROFILE_FUNCTION();

    vkResetFences(m_Device, 1, &m_ImFence);
    vkResetCommandBuffer(m_ImCommandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(m_ImCommandBuffer, &beginInfo);

    function(m_ImCommandBuffer);

    vkEndCommandBuffer(m_ImCommandBuffer);

    VkCommandBufferSubmitInfo commandBufferSubmitInfo{};
    commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandBufferSubmitInfo.deviceMask = 0;
    commandBufferSubmitInfo.commandBuffer = m_ImCommandBuffer;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_ImCommandBuffer;

    vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_ImFence);
    vkWaitForFences(m_Device, 1, &m_ImFence, true, std::numeric_limits<uint64_t>::max());
  }

  void VulkanContext::CreateInstance() {
    PROFILE_FUNCTION();

	  bool validationLayersAvaliable = CheckValidationLayerSupport();
    if (ENABLE_VALIDATION_LAYERS && !validationLayersAvaliable) {
      CORE_ERROR("Error while creating Vulkan instance: Validation layers requested, but not available!");
    }

    uint32_t apiVersion = VK_API_VERSION_1_0;
    if (vkEnumerateInstanceVersion) {
      vkEnumerateInstanceVersion(&apiVersion);
    }

    if (apiVersion == VK_API_VERSION_1_0) {
      m_DeviceExtensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Helios Engine";
    appInfo.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
    appInfo.pEngineName = "Helios Engine";
    appInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
    appInfo.apiVersion = apiVersion;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (ENABLE_VALIDATION_LAYERS && validationLayersAvaliable) {
      createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
      createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

      PopulateDebugMessengerCreateInfo(debugCreateInfo);
      createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    } else {
      createInfo.enabledLayerCount = 0;
      createInfo.pNext = nullptr;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_VkInstance);
    CORE_ASSERT_CRITICAL(result == VK_SUCCESS, "Failed to create Vulkan instance!");

    CORE_INFO("Initializing Vulkan {}.{}.{}!", appInfo.apiVersion >> 22, (appInfo.apiVersion >> 12)
                                               & 0x3FF, appInfo.apiVersion & 0xFFF);
  }

  void VulkanContext::SetupDebugMessenger() {
    PROFILE_FUNCTION();

    if (!ENABLE_VALIDATION_LAYERS) { return; }

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    PopulateDebugMessengerCreateInfo(createInfo);

    VkResult result = CreateDebugUtilsMessengerEXT(m_VkInstance, &createInfo, nullptr, &m_DebugMessenger);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to set up Vulkan debug messenger!")
  }

  void VulkanContext::CreateSurface() {
    PROFILE_FUNCTION();

    VkResult result = glfwCreateWindowSurface(m_VkInstance, m_WindowHandle, nullptr, &m_Surface);
    CORE_ASSERT_CRITICAL(result == VK_SUCCESS, "Failed to create Vulkan window surface!");
  }

  void VulkanContext::PickPhysicalDevice() {
    PROFILE_FUNCTION();

    uint32_t deviceCount = 0;
    VkResult result = vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, nullptr);
    CORE_ASSERT_CRITICAL(result == VK_SUCCESS,
                         "Failed to pick Vulkan physical device: Failed to enumerate physical devices!");
    CORE_ASSERT_CRITICAL(deviceCount != 0,
                         "Failed to pick Vulkan physical device: Failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    result = vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, devices.data());
    CORE_ASSERT_CRITICAL(result == VK_SUCCESS,
                         "Failed to pick Vulkan physical device: Failed to enumerate physical devices!");

    std::multimap<uint32_t, VkPhysicalDevice> candidates;
    
    for (VkPhysicalDevice device : devices) {
      uint32_t score = 0;

      if (IsDeviceSuitable(device)) {
        VkPhysicalDeviceProperties deviceProperties{};
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
          score += 1000;
        }

        score += deviceProperties.limits.maxImageDimension2D;

        candidates.emplace(score, device);
      }
    }

    CORE_ASSERT_CRITICAL(!candidates.empty(),
                         "Failed to pick Vulkan physical device: Failed to find a suitable GPU!");

    if (candidates.rbegin()->first != 0) {
      m_PhysicalDevice = candidates.rbegin()->second;

      VkPhysicalDeviceProperties deviceProperties{};
      vkGetPhysicalDeviceProperties(m_PhysicalDevice, &deviceProperties);
      CORE_INFO("Device Info:");
      CORE_INFO("  GPU: {}", deviceProperties.deviceName);
      CORE_INFO("  Version: {}", deviceProperties.driverVersion);
    } else {
      CORE_ASSERT_CRITICAL(false, "Failed to pick Vulkan physical device: Failed to find a suitable GPU!");
    }
  }

  void VulkanContext::CreateLogicalDevice() {
    PROFILE_FUNCTION();

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

    if (ENABLE_VALIDATION_LAYERS) {
      createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
      createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
    }
    else { createInfo.enabledLayerCount = 0; }

    VkResult result = vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device);
    CORE_ASSERT_CRITICAL(result == VK_SUCCESS, "Failed create Vulkan logical device!");

    vkGetDeviceQueue(m_Device, indices.graphicsFamily.value(), 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, indices.presentFamily.value(), 0, &m_PresentQueue);
  }

  void VulkanContext::CreateSwapchain() {
    PROFILE_FUNCTION();

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
    } else {
      createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_Swapchain);
    CORE_ASSERT_CRITICAL(result == VK_SUCCESS, "Failed to create Vulkan swapchain!");

    result = vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr);
    CORE_ASSERT_CRITICAL(result == VK_SUCCESS,
                         "Failed to create Vulkan swapchain: Failed to get swapchain images!");

    m_SwapchainImages.resize(imageCount);

    result = vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_SwapchainImages.data());
    CORE_ASSERT_CRITICAL(result == VK_SUCCESS,
                         "Failed to create Vulkan swapchain: Failed to get swapchain images!");

    m_SwapchainImageFormat = surfaceFormat.format;
    m_SwapchainExtent = extent;

    m_SwapchainImageViews.resize(m_SwapchainImages.size());
    for (uint32_t i = 0; i < m_SwapchainImages.size(); ++i) {
      auto imageViewResult = CreateImageView(m_Device, m_SwapchainImages[i], m_SwapchainImageFormat,
                                             VK_IMAGE_ASPECT_COLOR_BIT);
      if (!imageViewResult) {
        CORE_ASSERT_CRITICAL(false, "Failed to create Vulkan swapchain: Failed to create image view '{}': {}!",
                             i, imageViewResult.error());
        return;
      }

      m_SwapchainImageViews[i] = *imageViewResult;
    }
  }

  void VulkanContext::CreateAllocator() {
    PROFILE_FUNCTION();

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = m_PhysicalDevice;
    allocatorInfo.device = m_Device;
    allocatorInfo.instance = m_VkInstance;

    VkResult result = vmaCreateAllocator(&allocatorInfo, &m_Allocator);
    CORE_ASSERT_CRITICAL(result == VK_SUCCESS, "Failed to create Vulkan memory allocator!");
  }

  void VulkanContext::CreateCommandPool() {
    PROFILE_FUNCTION();

    QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    for (FrameData& frame : m_Frames) {
      VkResult result = vkCreateCommandPool(m_Device, &poolInfo, nullptr, &frame.commandPool);
      CORE_ASSERT_CRITICAL(result == VK_SUCCESS, "Failed to create Vulkan command pool!");

      m_MainDeletionQueue.PushFunction([this, &frame]() {
        vkDestroyCommandPool(m_Device, frame.commandPool, nullptr);
      });
    }

    VkResult result = vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_ImCommandPool);
    CORE_ASSERT_CRITICAL(result == VK_SUCCESS, "Failed to create Vulkan command pool!");

    m_MainDeletionQueue.PushFunction([this]() {
      vkDestroyCommandPool(m_Device, m_ImCommandPool, nullptr);
    });
  }

  void VulkanContext::CreateCommandBuffers() {
    PROFILE_FUNCTION();

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    for (FrameData& frame : m_Frames) {
      allocInfo.commandPool = frame.commandPool;

      VkResult result = vkAllocateCommandBuffers(m_Device, &allocInfo, &frame.commandBuffer);
      CORE_ASSERT_CRITICAL(result == VK_SUCCESS,
                           "Failed to create Vulkan command buffers: Failed to allocate command buffer!");
    }

    allocInfo.commandPool = m_ImCommandPool;

    VkResult result = vkAllocateCommandBuffers(m_Device, &allocInfo, &m_ImCommandBuffer);
    CORE_ASSERT_CRITICAL(result == VK_SUCCESS,
                         "Failed to create Vulkan command buffers: Failed to allocate command buffer!");
  }

  void VulkanContext::CreateSyncObjects() {
    PROFILE_FUNCTION();

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (FrameData& frame : m_Frames) {
      bool result = vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &frame.presentSemaphore) == VK_SUCCESS
                    && vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &frame.renderSemaphore) == VK_SUCCESS
                    && vkCreateFence(m_Device, &fenceInfo, nullptr, &frame.renderFence) == VK_SUCCESS;
      CORE_ASSERT_CRITICAL(result, "Failed to create Vulkan synchronization objects!");

      m_MainDeletionQueue.PushFunction([this, &frame]() {
        vkDestroySemaphore(m_Device, frame.presentSemaphore, nullptr);
        vkDestroySemaphore(m_Device, frame.renderSemaphore, nullptr);
        vkDestroyFence(m_Device, frame.renderFence, nullptr);
      });
    }

    VkResult result = vkCreateFence(m_Device, &fenceInfo, nullptr, &m_ImFence);
    CORE_ASSERT_CRITICAL(result == VK_SUCCESS,
                         "Failed to create Vulkan synchronization objects: Failed to create fence!");

    m_MainDeletionQueue.PushFunction([this]() {
      vkDestroyFence(m_Device, m_ImFence, nullptr);
    });
  }

  void VulkanContext::CreateRenderPass() {
    PROFILE_FUNCTION();

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
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                              | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                              | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                               | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency depthDependency{};
    depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    depthDependency.dstSubpass = 0;
    depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                                   | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.srcAccessMask = 0;
    depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                                   | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency dependencies[] = { dependency, depthDependency };
    VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(std::size(attachments));
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(std::size(dependencies));
    renderPassInfo.pDependencies = dependencies;

    VkResult result = vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass);
    CORE_ASSERT_CRITICAL(result == VK_SUCCESS, "Failed to create Vulkan render pass!");

    m_MainDeletionQueue.PushFunction([this]() {
      vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
    });
  }

  void VulkanContext::CreateDepthResources() {
    PROFILE_FUNCTION();

    VkFormat depthFormat = FindDepthFormat();

    auto imageResult = CreateImage(m_Allocator, VMA_MEMORY_USAGE_GPU_ONLY, m_SwapchainExtent.width,
                                   m_SwapchainExtent.height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    if (!imageResult) {
      CORE_ASSERT_CRITICAL(false, "Failed to create Vulkan depth resources: Failed to create depth image: {}!",
                           imageResult.error());
      return;
    }

    m_DepthImage = std::move(*imageResult);

    auto imageViewResult = CreateImageView(m_Device, m_DepthImage.image, depthFormat,
                                           VK_IMAGE_ASPECT_DEPTH_BIT);
    if (!imageViewResult) {
      CORE_ASSERT_CRITICAL(false, "Failed to create Vulkan depth resources: Failed to create depth image view: {}!",
                           imageViewResult.error());
      return;
    }
    
    m_DepthImage.imageView = *imageViewResult;
  }

  void VulkanContext::CreateFramebuffers() {
    PROFILE_FUNCTION();

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

      VkResult result = vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_SwapchainFramebuffers[i]);
      CORE_ASSERT_CRITICAL(result == VK_SUCCESS, "Failed to create Vulkan framebuffers!");
    }
  }

  void VulkanContext::CleanupSwapchain() {
    PROFILE_FUNCTION();

    m_DepthImage.Destroy(m_Device, m_Allocator);
    vkDestroyImageView(m_Device, m_DepthImage.imageView, nullptr);
    vmaDestroyImage(m_Allocator, m_DepthImage.image, m_DepthImage.allocation);

    for (VkFramebuffer framebuffer : m_SwapchainFramebuffers) {
      vkDestroyFramebuffer(m_Device, framebuffer, nullptr);
    }

    for (VkImageView imageView : m_SwapchainImageViews) {
      vkDestroyImageView(m_Device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
  }

  void VulkanContext::RecreateSwapchain() {
    vkDeviceWaitIdle(m_Device);

    CleanupSwapchain();
    CreateSwapchain();
    CreateDepthResources();
    CreateFramebuffers();

    m_SwapchainRecreated = true;
  }

  bool VulkanContext::CheckValidationLayerSupport() const {
    uint32_t layerCount = 0;
    VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    CORE_ASSERT(result == VK_SUCCESS,
                "Failed to check Vulkan validation layer support: Failed to enumerate instance layer properties!");

    std::vector<VkLayerProperties> availableLayers(layerCount);
    result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    CORE_ASSERT(result == VK_SUCCESS,
                "Failed to check Vulkan validation layer support: Failed to enumerate instance layer properties!");

    for (const char* layerName : m_ValidationLayers) {
      bool layerFound = false;
      for (const VkLayerProperties& layerProperties : availableLayers) {
        if (std::strcmp(layerName, layerProperties.layerName) == 0) {
          layerFound = true;
          break;
        }
      }

      if (!layerFound) { return false; }
    }

    return true;
  }

  VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                              VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                              void* pUserData) {
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
      CORE_ERROR("Vulkan validation layer: {}: {}", pCallbackData->pMessageIdName,
                                                    pCallbackData->pMessage);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
      CORE_WARN("Vulkan validation layer: {}: {}", pCallbackData->pMessageIdName,
                                                   pCallbackData->pMessage);
    } else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
      CORE_WARN("Vulkan validation layer: Performance warning: {}: {}", pCallbackData->pMessageIdName,
                                                                        pCallbackData->pMessage);
    } else {
      CORE_INFO("Vulkan validation layer: {}: {}", pCallbackData->pMessageIdName,
                                                   pCallbackData->pMessage);
    }

    return VK_FALSE;
  }

  std::vector<const char*> VulkanContext::GetRequiredExtensions() const {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (ENABLE_VALIDATION_LAYERS) {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
  }

  void VulkanContext::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) const {
    createInfo = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = DebugCallback
    };
  }

  VkResult VulkanContext::CreateDebugUtilsMessengerEXT(VkInstance instance,
                                                       const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                       const VkAllocationCallbacks* pAllocator,
                                                       VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
                                                                          "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
      return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else { return VK_ERROR_EXTENSION_NOT_PRESENT; }
  }

  void VulkanContext::DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                                    const VkDebugUtilsMessengerEXT debugMessenger,
                                                    const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
                                                                           "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
      func(instance, debugMessenger, pAllocator);
    }
  }

  bool VulkanContext::CheckDeviceExtensionSupport(VkPhysicalDevice device) const {
    uint32_t extensionCount = 0;
    VkResult result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    CORE_ASSERT(result == VK_SUCCESS,
                "Failed to check Vulkan device extension support: Failed to enumerate device extension properties!");

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    CORE_ASSERT(result == VK_SUCCESS,
                "Failed to check Vulkan device extension support: Failed to enumerate device extension properties!");

    std::set<std::string_view> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());
    
    for (const VkExtensionProperties& extensionProperties : availableExtensions) {
      requiredExtensions.erase(extensionProperties.extensionName);
    }

    if (!requiredExtensions.empty()) {
      for (const std::string_view& extension : requiredExtensions) {
        CORE_WARN("Missing required device extension: {}!", extension);
      }
      return false;
    }

    return true;
  }

  QueueFamilyIndices VulkanContext::FindQueueFamilies(VkPhysicalDevice device) const {
    QueueFamilyIndices indices{};

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;
    for (const VkQueueFamilyProperties& queueFamilyProperties : queueFamilies) {
      if (queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        indices.graphicsFamily = i;
      }

      VkBool32 presentSupport = false;
      VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);
      CORE_ASSERT(result == VK_SUCCESS,
                  "Failed to find Vulkan queue families: Failed to get physical device surface support!");

      if (presentSupport) { indices.presentFamily = i; }
      if (indices.IsComplete()) { break; }
      ++i;
    }

    return indices;
  }

  SwapChainSupportDetails VulkanContext::QuerySwapChainSupport(VkPhysicalDevice device) const {
    SwapChainSupportDetails details{};

    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.capabilities);
    CORE_ASSERT(result == VK_SUCCESS,
                "Failed to query Vulkan swapchain support: Failed to get physical device surface capabilities!");

    uint32_t formatCount = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);
    CORE_ASSERT(result == VK_SUCCESS,
                "Failed to query Vulkan swapchain support: Failed to get physical device surface formats!");

    if (formatCount != 0) {
      details.formats.resize(formatCount);
      result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.formats.data());
      CORE_ASSERT(result == VK_SUCCESS,
                  "Failed to query Vulkan swapchain support: Failed to get physical device surface formats!");
    }

    uint32_t presentModeCount = 0;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);
    CORE_ASSERT(result == VK_SUCCESS,
                "Failed to query Vulkan swapchain support: Failed to get physical device surface present modes!");

    if (presentModeCount != 0) {
      details.presentModes.resize(presentModeCount);
      result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount,
                                                         details.presentModes.data());
      CORE_ASSERT(result == VK_SUCCESS,
                  "Failed to query Vulkan swapchain support: Failed to get physical device surface present modes!");
    }

    return details;
  }

  bool VulkanContext::IsDeviceSuitable(VkPhysicalDevice device) const {
    QueueFamilyIndices indices = FindQueueFamilies(device);

    bool extensionsSupported = CheckDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
      SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
      swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures{};
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
    return indices.IsComplete() && extensionsSupported && swapChainAdequate
           && supportedFeatures.samplerAnisotropy;
  }

  VkSurfaceFormatKHR VulkanContext::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM
          && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        return availableFormat;
      }
    }
    return availableFormats[0];
  }

  VkPresentModeKHR VulkanContext::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const {
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

    if (m_Vsync) { return bestMode; }
    for (const VkPresentModeKHR& availablePresentMode : availablePresentModes) {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) { return availablePresentMode; }
      else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) { bestMode = availablePresentMode; }
    }

    return bestMode;
  }

  VkExtent2D VulkanContext::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
    } else {
      int width(0), height(0);
      glfwGetFramebufferSize(m_WindowHandle, &width, &height);

      VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
      };

      actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                      capabilities.maxImageExtent.width);
      actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                       capabilities.maxImageExtent.height);

      return actualExtent;
    }
  }

  uint32_t VulkanContext::ChooseImageCount(const VkSurfaceCapabilitiesKHR& capabilities) {
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
      imageCount = capabilities.maxImageCount;
    }
    return std::max(imageCount, MAX_FRAMES_IN_FLIGHT);
  }

  VkFormat VulkanContext::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                              VkFormatFeatureFlags features) const {
    for (VkFormat format : candidates) {
      VkFormatProperties props;
      vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

      if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
        return format;
      } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
        return format;
      }
    }

    CORE_ASSERT(false, "Failed to find Vulkan supported format!");
    return VK_FORMAT_UNDEFINED;
  }
}