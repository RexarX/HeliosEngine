#pragma once

#include "VoxelEngine/Render/GraphicsContext.h"

#include "vepch.h"

#include <vulkan/vulkan.hpp>

#ifdef VE_DEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
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
		virtual void ClearBuffer() override;
		virtual void SetViewport(const uint32_t width, const uint32_t height) override;

		static inline vk::UniqueDevice& GetDevice() { return m_Device; }

	private:
		struct QueueFamilyIndices {
			std::optional<uint32_t> graphicsFamily;
			std::optional<uint32_t> presentFamily;

			inline bool isComplete() const {
				return graphicsFamily.has_value() && presentFamily.has_value();
			}
		};

		struct SwapChainSupportDetails {
			vk::SurfaceCapabilitiesKHR capabilities;
			std::vector<vk::SurfaceFormatKHR> formats;
			std::vector<vk::PresentModeKHR> presentModes;
		};

		const std::vector<const char*> validationLayers = {
			"VK_LAYER_LUNARG_standard_validation"
		};

		const std::vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		static vk::UniqueInstance m_Instance;

		static vk::UniqueDevice m_Device;

		VkDebugUtilsMessengerEXT m_Callback;

		vk::SurfaceKHR m_Surface;

		vk::PhysicalDevice m_PhysicalDevice;

		vk::Queue m_GraphicsQueue;
		vk::Queue m_PresentQueue;

		vk::SwapchainKHR m_SwapChain;
		std::vector<vk::Image> m_SwapChainImages;
		vk::Format m_SwapChainImageFormat;
		vk::Extent2D m_SwapChainExtent;
		std::vector<vk::ImageView> m_SwapChainImageViews;

	private:
		void CreateInstance();
		void SetupDebugCallback();
    void CreateSurface();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSwapChain();
		void CreateImageViews();
		void CreateGraphicsPipeline();

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