#include "VulkanContext.h"
#include "VulkanResourceManager.h"
#include "VulkanMesh.h"

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

    ShutdownImGui();

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
    if (m_SwapchainRecreated) { m_SwapchainRecreated = false; return; }
    Submit();
  }

  void VulkanContext::BeginFrame()
  {
    if (m_SwapchainRecreated) { RecreateSwapchain(); return; }

    auto result = vkWaitForFences(m_Device, 1, &m_Frames[m_CurrentFrame].renderFence, VK_TRUE,
                                  std::numeric_limits<uint64_t>::max());

    CORE_ASSERT(result == VK_SUCCESS, "Failed to wait for fence!");

    result = vkResetFences(m_Device, 1, &m_Frames[m_CurrentFrame].renderFence);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to reset fence!");

    result = vkAcquireNextImageKHR(m_Device, m_Swapchain, std::numeric_limits<uint64_t>::max(),
                                   m_Frames[m_CurrentFrame].presentSemaphore, VK_NULL_HANDLE, &m_ImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) { RecreateSwapchain(); return; }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      CORE_ASSERT(false, "Failed to acquire next image!");
    }

    VkCommandBuffer commandBuffer = m_Frames[m_CurrentFrame].commandBuffer;

    result = vkResetCommandBuffer(commandBuffer, 0);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to reset command buffer!");

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to begin recording command buffer!");

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

    vkCmdSetViewport(commandBuffer, 0, 1, &m_Viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &m_Scissor);

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  }

  void VulkanContext::EndFrame()
  {
    if (m_SwapchainRecreated) { return; }
  }

  void VulkanContext::Record(const RenderQueue& queue, const ResourceManager& manager)
  {
    if (m_SwapchainRecreated) { return; }

    const VulkanResourceManager& resourceManager = static_cast<const VulkanResourceManager&>(manager);
    VkCommandBuffer commandBuffer = m_Frames[m_CurrentFrame].commandBuffer;
    std::unordered_map<const Effect*, std::vector<const Renderable*>> pipelineGroups;

    for (const auto& [renderable, transform] : queue.GetRenderObjects()) {
      const Effect& effect = resourceManager.GetEffect(renderable, PipelineType::Regular);
      pipelineGroups[&effect].push_back(&renderable);
    }

    for (const auto& [effect, renderables] : pipelineGroups) {
      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, effect->pipeline);

      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, effect->pipelineLayout, 0,
                              effect->descriptorSets.size(), effect->descriptorSets.data(), 0, nullptr);

      for (const Renderable* renderable : renderables) {
        std::shared_ptr<VulkanMesh> mesh = std::static_pointer_cast<VulkanMesh>(renderable->mesh);

        struct PushConstantData
        {
          glm::mat4 projectionViewMatrix;
        };

        const SceneData& pushConstantData = queue.GetSceneData();
        vkCmdPushConstants(commandBuffer, effect->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(pushConstantData), &pushConstantData);

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mesh->GetVertexBuffer().buffer, { 0 });

        if (renderable->mesh->GetIndexCount() > 0) {
          vkCmdBindIndexBuffer(commandBuffer, mesh->GetIndexBuffer().buffer, 0, VK_INDEX_TYPE_UINT32);
          vkCmdDrawIndexed(commandBuffer, renderable->mesh->GetIndexCount(), 1, 0, 0, 0);
        } else {
          vkCmdDraw(commandBuffer, renderable->mesh->GetVertexCount(), 1, 0, 0);
        }
      }
    }
  }

  void VulkanContext::SetViewport(const uint32_t width, const uint32_t height, const uint32_t x, const uint32_t y)
  {
    m_SwapchainRecreated = true;

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
    VkDescriptorPoolSize pool_sizes[] =
    {
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
  }

  void VulkanContext::ShutdownImGui()
  {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    vkDestroyDescriptorPool(m_Device, m_ImGuiPool, nullptr);
  }

  void VulkanContext::BeginFrameImGui()
  {
    if (m_SwapchainRecreated) {
      ImGui::NewFrame();
      ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
      return;
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
  }

  void VulkanContext::EndFrameImGui()
  {
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
  }

  void VulkanContext::SetVSync(const bool enabled)
  {
    m_Vsync = enabled;
    RecreateSwapchain();
  }

  void VulkanContext::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
  {
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

  inline const VkPhysicalDeviceLimits& VulkanContext::GetPhysicalDeviceLimits() const
  {
    static VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
    return properties.limits;
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
    for (const auto device : devices) {
      if (IsDeviceSuitable(device)) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
          score += 1000;
        }

        score += deviceProperties.limits.maxImageDimension2D;
        candidates.emplace(score, device);
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

    auto result = vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to create logical device!");

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
    } else {
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
    QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    for (auto& frame : m_Frames) {
      auto result = vkCreateCommandPool(m_Device, &poolInfo, nullptr, &frame.commandPool);
      CORE_ASSERT(result == VK_SUCCESS, "Failed to create command pool!");

      m_MainDeletionQueue.PushFunction([=]() {
        vkDestroyCommandPool(m_Device, frame.commandPool, nullptr);
      });
    }

    auto result = vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_ImCommandPool);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to create command pool!");

    m_MainDeletionQueue.PushFunction([=]() {
      vkDestroyCommandPool(m_Device, m_ImCommandPool, nullptr);
    });
  }

  void VulkanContext::CreateCommandBuffers()
  {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    for (auto& frame : m_Frames) {
      allocInfo.commandPool = frame.commandPool;

      auto result = vkAllocateCommandBuffers(m_Device, &allocInfo, &frame.commandBuffer);
      CORE_ASSERT(result == VK_SUCCESS, "Failed to allocate command buffer!");
    }

    allocInfo.commandPool = m_ImCommandPool;

    auto result = vkAllocateCommandBuffers(m_Device, &allocInfo, &m_ImCommandBuffer);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to allocate command buffer!");
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

      m_MainDeletionQueue.PushFunction([=]() {
        vkDestroySemaphore(m_Device, frame.presentSemaphore, nullptr);
        vkDestroySemaphore(m_Device, frame.renderSemaphore, nullptr);
        vkDestroyFence(m_Device, frame.renderFence, nullptr);
      });
    }

    auto result = vkCreateFence(m_Device, &fenceInfo, nullptr, &m_ImFence);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to create fence!");

    m_MainDeletionQueue.PushFunction([=]() {
      vkDestroyFence(m_Device, m_ImFence, nullptr);
    });
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
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency depthDependency{};
    depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    depthDependency.dstSubpass = 0;
    depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.srcAccessMask = 0;
    depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
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

    auto result = vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to create render pass!");

    m_MainDeletionQueue.PushFunction([=]() {
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

    m_SwapchainRecreated = true;
  }

  void VulkanContext::Submit()
  {
    VkCommandBuffer commandBuffer = m_Frames[m_CurrentFrame].commandBuffer;

    vkCmdEndRenderPass(commandBuffer);

    auto result = vkEndCommandBuffer(commandBuffer);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to end command buffer!");

    VkSemaphore waitSemaphores[] = { m_Frames[m_CurrentFrame].presentSemaphore };
    VkSemaphore signalSemaphores[] = { m_Frames[m_CurrentFrame].renderSemaphore };

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_Frames[m_CurrentFrame].renderFence);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to submit draw command buffer!");

    VkSwapchainKHR swapChains[] = { m_Swapchain };

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &m_ImageIndex;

    result = vkQueuePresentKHR(m_GraphicsQueue, &presentInfo);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to present swap chain image!");

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  bool VulkanContext::CheckValidationLayerSupport() const
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

  std::vector<const char*> VulkanContext::GetRequiredExtensions() const
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

  VkResult VulkanContext::CreateDebugUtilsMessengerEXT(const VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                       const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
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

  bool VulkanContext::CheckDeviceExtensionSupport(const VkPhysicalDevice device) const
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

  QueueFamilyIndices VulkanContext::FindQueueFamilies(const VkPhysicalDevice device) const
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

  SwapChainSupportDetails VulkanContext::QuerySwapChainSupport(const VkPhysicalDevice device) const
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

  bool VulkanContext::IsDeviceSuitable(const VkPhysicalDevice device) const
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

  VkSurfaceFormatKHR VulkanContext::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const
  {
    for (const auto& availableFormat : availableFormats) {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        return availableFormat;
      }
    }
    return availableFormats[0];
  }

  VkPresentModeKHR VulkanContext::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const
  {
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

    if (m_Vsync) { return bestMode; }

    for (const auto& availablePresentMode : availablePresentModes) {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) { return availablePresentMode; }
      else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) { bestMode = availablePresentMode; }
    }

    return bestMode;
  }

  VkExtent2D VulkanContext::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
  {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
    } else {
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

  uint32_t VulkanContext::ChooseImageCount(const VkSurfaceCapabilitiesKHR& capabilities) const
  {
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
      imageCount = capabilities.maxImageCount;
    }
    return std::max(imageCount, MAX_FRAMES_IN_FLIGHT);
  }

  VkFormat VulkanContext::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                              VkFormatFeatureFlags features) const
  {
    for (VkFormat format : candidates) {
      VkFormatProperties props;
      vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

      if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
        return format;
      } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
        return format;
      }
    }

    CORE_ASSERT(false, "Failed to find supported format!");
  }

  VkFormat VulkanContext::FindDepthFormat() const
  {
    return FindSupportedFormat(
      { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
  }
}