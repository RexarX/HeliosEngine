#pragma once

#include "VulkanShader.h"

#include "pch.h"

#include <vulkan/vulkan.h>

#include <vma/vk_mem_alloc.h>

namespace Helios
{
  class DeletionQueue
  {
  public:
    void PushFunction(std::function<void()>&& function) {
      deletors.push_back(function);
    }

    void Flush() {
      for (auto it = deletors.rbegin(); it != deletors.rend(); ++it) {
        (*it)();
      }
      deletors.clear();
    }

  private:
    std::vector<std::function<void()>> deletors;
  };

  struct QueueFamilyIndices
  {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    inline const bool IsComplete() const {
      return graphicsFamily.has_value() && presentFamily.has_value();
    }
  };

  struct SwapChainSupportDetails
  {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };

  struct FrameData
  {
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore presentSemaphore;
    VkSemaphore renderSemaphore;
    VkFence renderFence;
  };

  struct AllocatedImage
  {
    VkImage image;
    VkImageView imageView;
    VkExtent3D imageExtent;
    VkFormat imageFormat;

    VmaAllocation allocation;
  };

  struct AllocatedBuffer
  {
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
  };

  struct PipelineBuilder
  {
    PipelineBuilder();

    void Clear();
    void BuildPipeline(const VkDevice device, VkPipelineLayout& layout, VkPipeline& pipeline,
                       const VkRenderPass renderPass, const VkDescriptorSetLayout* descriptorSetLayouts,
                       uint32_t descriptorSetLayoutCount);

    void SetInputTopology(VkPrimitiveTopology topology);
    void SetPolygonMode(VkPolygonMode mode);
    void SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void SetMultisamplingNone();
    void DisableBlending();
    void SetColorAttachmentFormat(VkFormat format);
    void SetDepthFormat(VkFormat format);
    void DisableDepthTest();
    void EnableDepthTest();

    VkPushConstantRange pushConstantRange{};

    std::vector<VkVertexInputAttributeDescription> vertexInputStates;
    std::vector<VkVertexInputBindingDescription> vertexInputBindings;

    std::vector<VulkanShaderInfo> shaderInfos;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    VkPipelineMultisampleStateCreateInfo multisampling{};
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    VkPipelineRenderingCreateInfoKHR renderInfo{};

    VkFormat colorAttachmentFormat;
  };

  static AllocatedImage CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                                    VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, const VmaAllocator allocator)
  {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;
    allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    AllocatedImage allocatedImage;
    auto result = vmaCreateImage(allocator, &imageInfo, &allocInfo, &allocatedImage.image, &allocatedImage.allocation, nullptr);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to create image!");

    return allocatedImage;
  }

  static VkImageView CreateImageView(const VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,
                                     const VkDevice device)
  {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    auto result = vkCreateImageView(device, &viewInfo, nullptr, &imageView);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to create image view!");

    return imageView;
  }
}