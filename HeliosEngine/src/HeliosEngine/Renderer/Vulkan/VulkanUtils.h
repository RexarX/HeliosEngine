#pragma once

#include "VulkanShader.h"

#include "pch.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Helios {
  class DeletionQueue {
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

  struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    inline const bool IsComplete() const {
      return graphicsFamily.has_value() && presentFamily.has_value();
    }
  };

  struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };

  struct FrameData {
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkSemaphore presentSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderSemaphore = VK_NULL_HANDLE;
    VkFence renderFence = VK_NULL_HANDLE;
  };

  struct AllocatedImage {
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkExtent3D imageExtent{};
    VkFormat imageFormat = VK_FORMAT_UNDEFINED;

    VmaAllocation allocation = VK_NULL_HANDLE;
  };

  struct AllocatedBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo info{};
  };

  struct PipelineBuilder {
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

    std::vector<VulkanShader::ShaderInfo> shaderInfos;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    VkPipelineMultisampleStateCreateInfo multisampling{};
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    VkPipelineRenderingCreateInfoKHR renderInfo{};

    VkFormat colorAttachmentFormat = VK_FORMAT_UNDEFINED;
  };

  static AllocatedImage CreateImage(const VmaAllocator allocator, VmaMemoryUsage memoryUsage,
                                    uint32_t width, uint32_t height, VkFormat format,
                                    VkImageTiling tiling, VkImageUsageFlags usage) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = VkExtent3D(width, height, 1);
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
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    AllocatedImage allocatedImage{};
    VkResult result = vmaCreateImage(allocator, &imageInfo, &allocInfo, &allocatedImage.image,
                                     &allocatedImage.allocation, nullptr);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to create image!");

    return allocatedImage;
  }

  static AllocatedImage CreateImage(const VmaAllocator allocator, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags,
                                    uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling,
                                    VkImageUsageFlags usage) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = VkExtent3D(width, height, 1);
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = flags;

    AllocatedImage allocatedImage{};
    VkResult result = vmaCreateImage(allocator, &imageInfo, &allocInfo, &allocatedImage.image,
                                     &allocatedImage.allocation, nullptr);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to create image!");

    return allocatedImage;
  }

  static VkImageView CreateImageView(const VkDevice device, const VkImage image, VkFormat format,
                                     VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange = {
      .aspectMask = aspectFlags,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1
    };

    VkImageView imageView;
    VkResult result = vkCreateImageView(device, &viewInfo, nullptr, &imageView);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to create image view!");

    return imageView;
  }
}