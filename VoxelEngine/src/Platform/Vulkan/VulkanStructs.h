#pragma once

#include "VulkanBuffer.h"

#include "vepch.h"

#include <vulkan/vulkan.hpp>

#include <vma/vk_mem_alloc.h>

namespace VoxelEngine
{
	struct DeletionQueue
	{
		std::deque<std::function<void()>> deletors;

		void push_function(std::function<void()>&& function)
		{
			deletors.push_back(function);
		}

		void flush()
		{
			for (auto it = deletors.rbegin(); it != deletors.rend(); ++it) {
				(*it)();
			}

			deletors.clear();
		}
	};

	struct WritingQueue
	{
		std::deque<std::function<void()>> writings;

		void Push(std::function<void()>&& function)
		{
			writings.push_back(function);
		}

		void Flush()
		{
			for (auto it = writings.begin(); it != writings.end(); ++it) {
				(*it)();
			}

			writings.clear();
		}
	};

	struct PipelineBuilder
	{
		void Clear();
		void BuildPipeline(const vk::Device device, vk::PipelineLayout& layout, vk::Pipeline& pipeline,
											 const vk::DescriptorSetLayout descriptorSetLayout);

		void SetInputTopology(const vk::PrimitiveTopology topology);
		void SetPolygonMode(const vk::PolygonMode mode);
		void SetCullMode(const vk::CullModeFlags cullMode, const vk::FrontFace frontFace);
		void SetMultisamplingNone();
		void DisableBlending();
		void SetColorAttachmentFormat(const vk::Format format);
		void SetDepthFormat(const vk::Format format);
		void DisableDepthTest();
		void EnableDepthTest();

		std::vector<vk::PushConstantRange> pushConstantRanges;

		std::vector<vk::VertexInputAttributeDescription> vertexInputStates;
		std::vector<vk::VertexInputBindingDescription> vertexInputBindings;

		std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
		vk::PipelineRasterizationStateCreateInfo rasterizer;
		vk::PipelineColorBlendAttachmentState colorBlendAttachment;
		vk::PipelineMultisampleStateCreateInfo multisampling;
		vk::PipelineDepthStencilStateCreateInfo depthStencil;
		vk::PipelineRenderingCreateInfoKHR renderInfo;

		vk::Format colorAttachmentFormat;
	};

	struct QueueFamilyIndices
	{
		inline bool isComplete() const
		{
			return graphicsFamily.has_value() && presentFamily.has_value();
		}

		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;
	};

	struct SwapChainSupportDetails
	{
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presentModes;
	};

	struct AllocatedImage
	{
		VkImage image;
		vk::ImageView imageView;
		vk::Extent3D imageExtent;
		vk::Format imageFormat;

		VmaAllocation allocation;
	};

	struct DescriptorLayoutBuilder
	{
		void AddBinding(const uint32_t binding, const vk::DescriptorType type,
										const vk::ShaderStageFlags shaderStage);
		void Clear() { bindings.clear(); }

		vk::DescriptorSetLayout Build(const vk::Device device,
																	const vk::DescriptorSetLayoutCreateFlags flags,
																	const void* pNext = nullptr);

    std::vector<vk::DescriptorSetLayoutBinding> bindings;
	};

	struct PoolSizeRatio
	{
		vk::DescriptorType type;
		float ratio = 1.0f;
	};

	struct DescriptorAllocator
	{
		vk::DescriptorPool pool;
		void InitPool(const vk::Device device, const uint32_t maxSets, const std::span<PoolSizeRatio>& poolRatios);
		void ClearDescriptors(const vk::Device device) const;
		void DestroyPool(const vk::Device device) const;

		vk::DescriptorSet Allocate(const vk::Device device, const vk::DescriptorSetLayout layout) const;
	};

	struct AllocatedBuffer
	{
		VkBuffer buffer;
		
		VmaAllocation allocation;
		VmaAllocationInfo info;
	};

	struct DescriptorAllocatorGrowable
	{
	public:
		void Init(const vk::Device device, const uint32_t maxSets);
		void AddRatios(PoolSizeRatio poolRatios);
		void ClearPools(const vk::Device device);
		void DestroyPools(const vk::Device device);

		vk::DescriptorSet Allocate(const vk::Device device, const vk::DescriptorSetLayout layout,
															 const void* pNext = nullptr);
	private:
		vk::DescriptorPool GetPool(const vk::Device device);
		vk::DescriptorPool CreatePool(const vk::Device device, const uint32_t setCount);

		std::vector<PoolSizeRatio> ratios;
		std::vector<vk::DescriptorPool> fullPools;
		std::vector<vk::DescriptorPool> readyPools;
		uint32_t setsPerPool = 0;
	};

	struct DescriptorWriter
	{
		void WriteImage(const uint32_t binding, const vk::ImageView image, const vk::Sampler sampler,
										const vk::ImageLayout layout, const vk::DescriptorType type);
		void WriteBuffer(const uint32_t binding, const vk::Buffer buffer, const uint64_t size,
										 const uint64_t offset, const vk::DescriptorType type);

		void Clear();
		void UpdateSet(const vk::Device device, const vk::DescriptorSet set);

		std::deque<vk::DescriptorImageInfo> imageInfos;
		std::deque<vk::DescriptorBufferInfo> bufferInfos;
		std::vector<vk::WriteDescriptorSet> writes;
	};

	struct ComputeEffect
	{
    ~ComputeEffect() = default;

		void Init();
		void Build();
		void Destroy();

		PipelineBuilder pipelineBuilder;

		vk::Pipeline pipeline;
		vk::PipelineLayout pipelineLayout;

    DescriptorWriter descriptorWriter;

		DescriptorAllocatorGrowable descriptorAllocator;
		DescriptorLayoutBuilder descriptorLayoutBuilder;

		vk::DescriptorSet descriptorSet;
		vk::DescriptorSetLayout descriptorSetLayout;

		std::shared_ptr<VulkanVertexBuffer> vertexBuffer;
		std::shared_ptr<VulkanIndexBuffer> indexBuffer;

		const void* pushConstant = nullptr;
		uint32_t pushConstantSize = 0;
	};

	struct FrameData
	{
		vk::Semaphore swapchainSemaphore, renderSemaphore;
		vk::Fence renderFence;

		vk::CommandPool commandPool;
		vk::CommandBuffer commandBuffer;

		DeletionQueue deletionQueue;
	};
}