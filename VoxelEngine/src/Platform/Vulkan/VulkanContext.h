#pragma once

#include "Render/GraphicsContext.h"

#include "vepch.h"

#include <vulkan/vulkan.hpp>

#define VMA_VULKAN_VERSION 1003000
#include <vma/vk_mem_alloc.h>

#ifdef VE_DEBUG
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = false;
#endif

struct GLFWwindow;

namespace VoxelEngine
{
	struct PipelineData
	{
		PipelineData() { Clear(); }

		void Clear();
		void BuildPipeline(const vk::Device device, vk::PipelineLayout& layout, vk::Pipeline& pipeline);

		void SetInputTopology(const vk::PrimitiveTopology topology);
		void SetPolygonMode(const vk::PolygonMode mode);
		void SetCullMode(const vk::CullModeFlags cullMode, const vk::FrontFace frontFace);
		void SetMultisamplingNone();
		void DisableBlending();
		void SetColorAttachmentFormat(const vk::Format format);
		void SetDepthFormat(const vk::Format format);
		void DisableDepthTest();
		void EnableDepthTest();

		std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
		vk::PipelineRasterizationStateCreateInfo rasterizer;
		vk::PipelineColorBlendAttachmentState colorBlendAttachment;
		vk::PipelineMultisampleStateCreateInfo multisampling;
		vk::PipelineDepthStencilStateCreateInfo depthStencil;
		vk::PipelineRenderingCreateInfoKHR renderInfo;
		vk::Format colorAttachmentformat;
	};

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

	struct AllocatedImage {
		VkImage image;
		vk::ImageView imageView;
		vk::Extent3D imageExtent;
		vk::Format imageFormat;

		VmaAllocation allocation;
	};

	struct DescriptorLayoutBuilder
	{
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		void AddBinding(const uint32_t binding, const vk::DescriptorType type);
		void Clear() { bindings.clear(); }
		vk::DescriptorSetLayout Build(const vk::Device device, const vk::ShaderStageFlags shaderStages,
																	const vk::DescriptorSetLayoutCreateFlags flags,
																	const void* pNext = nullptr);
	};

	struct DescriptorAllocator
	{
		struct PoolSizeRatio
		{
			vk::DescriptorType type;
			float ratio = 1.0f;
		};

		vk::DescriptorPool pool;
		void InitPool(const vk::Device device, const uint32_t maxSets, const std::span<PoolSizeRatio> poolRatios);
		void ClearDescriptors(const vk::Device device);
		void DestroyPool(const vk::Device device);

		vk::DescriptorSet Allocate(const vk::Device device, const vk::DescriptorSetLayout layout);
	};

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

		virtual void InitImGui() override;
		virtual void ShutdownImGui() override;
		virtual void Begin() override;
		virtual void End() override;

		virtual void SetVSync(const bool enabled) override;
		virtual void SetResized(const bool resized) override { m_Resized = resized; }

		virtual void SetImGuiState(const bool enabled) override { m_ImGuiEnabled = enabled; }

		void BuildPipeline() { m_PipelineBuilder.BuildPipeline(m_Device, m_PipelineLayout, m_Pipeline); }

		static inline VulkanContext& Get() { return *m_Context; }

		inline vk::Device GetDevice() { return m_Device; }

		inline PipelineData& GetPipelineData() { return m_PipelineBuilder; }

	private:
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

		static VulkanContext* m_Context;

		bool m_Resized = false;
		bool m_Vsync = true;
		bool m_RebuildPipeline = false;
		bool m_ImGuiEnabled = false;

		VmaAllocator m_Allocator;

		vk::Instance m_Instance;

		vk::Device m_Device;

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

		vk::Extent2D m_DrawExtent;
		
		PipelineData m_PipelineBuilder;

		DescriptorAllocator m_DescriptorAllocator;
		DescriptorAllocator m_ImGuiDescriptorAllocator;

		vk::DescriptorSet m_DrawImageDescriptors;
		vk::DescriptorSetLayout m_DrawImageDescriptorLayout;

		vk::CommandPool m_CommandPool;
		std::vector<vk::CommandBuffer, std::allocator<vk::CommandBuffer>> m_CommandBuffers;

		vk::Semaphore m_SwapChainSemaphore;
		vk::Semaphore m_RenderSemaphore;
		vk::Fence m_RenderFence;

		AllocatedImage m_DrawImage;
		AllocatedImage m_DepthImage;

		vk::Pipeline m_Pipeline;
		vk::PipelineLayout m_PipelineLayout;

	private:
		void CreateInstance();
		void SetupDebugCallback();
    void CreateSurface();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateAllocator();
		void CreateSwapChain();
		void CreateImageViews();
		void CreateCommands();
		void CreateSyncObjects();
		void CreateDescriptors();
		void CreatePipeline();
		
		void RecreateSwapChain();
		void RebuildPipeline();

		void DrawGeometry(const vk::CommandBuffer cmd);
		void DrawImGui(const vk::ImageView view);

		void transition_image(const vk::CommandBuffer cmd, const vk::Image image,
													const vk::ImageLayout currentLayout, const vk::ImageLayout newLayout);

		vk::SemaphoreSubmitInfo& semaphore_submit_info(const vk::PipelineStageFlags2 stageMask,
																									 const vk::Semaphore semaphore) const;

		vk::ImageCreateInfo& image_create_info(const vk::Format format, const vk::ImageUsageFlags usageFlags,
																					 const vk::Extent3D extent) const;
		vk::ImageViewCreateInfo imageview_create_info(const vk::Image image, const vk::Format format,
																									const vk::ImageAspectFlags aspectFlags) const;
		void copy_image_to_image(const vk::CommandBuffer cmd, const vk::Image source, const vk::Image destination,
														 const vk::Extent2D srcSize, const vk::Extent2D dstSize) const;

		void immediate_submit(const vk::CommandBuffer cmd);

		bool IsDeviceSuitable() const;
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
		SwapChainSupportDetails QuerySwapChainSupport() const;

	private:
		GLFWwindow* m_WindowHandle;
	};
}