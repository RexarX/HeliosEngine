#include "vepch.h"

#include "VulkanContext.h"

namespace VoxelEngine
{
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
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
	}

  void VulkanContext::Shutdown()
  {
    m_Instance->destroySurfaceKHR();
  }

	void VulkanContext::ClearBuffer()
	{
		
	}

	void VulkanContext::SetViewport(const uint32_t width, const uint32_t height)
	{
    
	}

  void VulkanContext::CreateInstance()
  {
    auto appInfo = vk::ApplicationInfo(
      "Application",
      VK_MAKE_VERSION(1, 3, 280),
      "VoxelEngine",
      VK_MAKE_VERSION(1, 3, 280),
      VK_API_VERSION_1_3
    );

    auto extensions = GetRequiredExtensions();

    auto createInfo = vk::InstanceCreateInfo(
      vk::InstanceCreateFlags(),
      &appInfo,
      0, nullptr,
      static_cast<uint32_t>(extensions.size()), extensions.data()
    );

    m_Instance = vk::createInstanceUnique(createInfo, nullptr);
    VE_CORE_ASSERT(m_Instance, "Failed to create instance!");
  }

  void VulkanContext::CreateSurface()
  {
    VkSurfaceKHR rawSurface;
    auto createSurface = glfwCreateWindowSurface(*m_Instance, m_WindowHandle, nullptr, &rawSurface);
    VE_CORE_ASSERT(createSurface = VK_SUCCESS, "Failed to create window surface!");
    
    m_Surface = rawSurface;
  }

  std::vector<const char*> VulkanContext::GetRequiredExtensions() const
  {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
  }
  void VulkanContext::PickPhysicalDevice()
  {
    auto devices = m_Instance->enumeratePhysicalDevices();
    VE_CORE_ASSERT(devices.empty(), "Failed to find GPUs with Vulkan support!");

    for (const auto& device : devices) {
      if (IsDeviceSuitable(device)) {
        m_PhysicalDevice = device;
        break;
      }
    }
    VE_CORE_ASSERT(m_PhysicalDevice, "Failed to find a suitable GPU!");
  }
  bool VulkanContext::IsDeviceSuitable(const vk::PhysicalDevice& device) const
  {
    QueueFamilyIndices indices = FindQueueFamilies(device);

    return indices.isComplete();
  }

  VulkanContext::QueueFamilyIndices VulkanContext::FindQueueFamilies(const vk::PhysicalDevice& device) const
  {
    QueueFamilyIndices indices;
    auto queueFamilies = device.getQueueFamilyProperties();

    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies) {
      if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
        indices.graphicsFamily = i;
      }
      if (indices.isComplete()) { break; }
      ++i;
    }

    return indices;
  }

  void VulkanContext::CreateLogicalDevice()
  {
    QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
    float queuePriority = 1.0f;
    auto queueCreateInfo = vk::DeviceQueueCreateInfo(
      vk::DeviceQueueCreateFlags(),
      indices.graphicsFamily.value(),
      1,
      &queuePriority
    );

    auto deviceFeatures = vk::PhysicalDeviceFeatures();
    auto createInfo = vk::DeviceCreateInfo(
      vk::DeviceCreateFlags(),
      1, &queueCreateInfo
    );
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 0;

    m_Device = m_PhysicalDevice.createDeviceUnique(createInfo);
    VE_CORE_ASSERT(m_Device, "Failed to create logical device!");

    m_GraphicsQueue = m_Device->getQueue(indices.graphicsFamily.value(), 0);
  }
}