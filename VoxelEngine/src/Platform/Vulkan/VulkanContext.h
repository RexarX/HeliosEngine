#pragma once

#include "VoxelEngine/Render/GraphicsContext.h"

#include "vepch.h"

#include <vulkan/vulkan.hpp>

#define VMA_VULKAN_VERSION 1003000
#include "vma/vk_mem_alloc.h"

#ifdef VE_DEBUG
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = false;
#endif

struct GLFWwindow;

namespace VoxelEngine
{
	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void Shutdown() override;
		virtual void Update() override;
		virtual void SwapBuffers() override;
		virtual void ClearBuffer() override;
		virtual void SetViewport(const uint32_t width, const uint32_t height) override;

		virtual void SetVSync(const bool enabled) override;

		virtual void SetResized(const bool resized) override { m_Resized = resized; }

		static inline vk::Device& GetDevice() { return m_Device; }

	private:
		struct QueueFamilyIndices
		{
			std::optional<uint32_t> graphicsFamily;
			std::optional<uint32_t> presentFamily;

			inline bool isComplete() const
			{
				return graphicsFamily.has_value() && presentFamily.has_value();
			}
		};

		struct SwapChainSupportDetails
		{
			vk::SurfaceCapabilitiesKHR capabilities;
			std::vector<vk::SurfaceFormatKHR> formats;
			std::vector<vk::PresentModeKHR> presentModes;
		};

		const std::vector<const char*> validationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

		const std::vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
			VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
			VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
			VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME
		};

		bool m_Resized = false;

		VmaAllocator m_Allocator;

		static vk::Instance m_Instance;

		static vk::Device m_Device;

		VkDebugUtilsMessengerEXT m_Callback;

		vk::SurfaceKHR m_Surface;

		vk::PhysicalDevice m_PhysicalDevice;

		vk::Queue m_GraphicsQueue;
		vk::Queue m_PresentQueue;

		vk::SwapchainKHR m_SwapChain;
		vk::Extent2D m_SwapChainExtent;
		std::vector<vk::Image> m_SwapChainImages;
		std::vector<vk::ImageView> m_SwapChainImageViews;
    vk::Format m_SwapChainImageFormat;
				
		vk::CommandPool m_CommandPool;
		std::vector<vk::CommandBuffer, std::allocator<vk::CommandBuffer>> m_CommandBuffers;

		vk::Semaphore m_SwapChainSemaphore;
		vk::Semaphore m_RenderSemaphore;
		vk::Fence m_RenderFence;

	private:
		void CreateInstance();
		void SetupDebugCallback();
    void CreateSurface();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSwapChain();
		void CreateImageViews();
		void CreateCommands();
		void CreateSyncObjects();
		void CreateAllocator();

		void RecreateSwapChain();

		void transition_image(const vk::Image& image, const vk::ImageLayout& currentLayout,
													const vk::ImageLayout& newLayout);

		vk::SemaphoreSubmitInfo semaphore_submit_info(const vk::PipelineStageFlags2& stageMask,
																								const vk::Semaphore& semaphore) const;

		bool IsDeviceSuitable();
		QueueFamilyIndices FindQueueFamilies() const;
		std::vector<const char*> GetRequiredExtensions() const;
		bool CheckDeviceExtensionSupport() const;
		bool CheckValidationLayerSupport() const;

		VkResult CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator);
		void DestroyDebugUtilsMessengerEXT(const VkAllocationCallbacks* pAllocator) const;

		vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const;
		vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) const;
		vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const;
		SwapChainSupportDetails QuerySwapChainSupport();

	private:
		GLFWwindow* m_WindowHandle;
	};
}