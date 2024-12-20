#pragma once

#include "VulkanShader.h"

#include "pch.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Helios {
  class DeletionQueue {
  public:
    void PushFunction(std::function<void()>&& function) {
      m_Deletors.emplace_back(function);
    }

    void Flush() {
      for (auto it = m_Deletors.rbegin(); it != m_Deletors.rend(); ++it) {
        (*it)();
      }
      m_Deletors.clear();
    }

  private:
    std::vector<std::function<void()>> m_Deletors;
  };

  struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily = std::nullopt;
    std::optional<uint32_t> presentFamily = std::nullopt;

    inline bool IsComplete() const {
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
    void Destroy(VkDevice device, VmaAllocator allocator);

    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkExtent3D imageExtent{};
    VkFormat imageFormat = VK_FORMAT_UNDEFINED;
    VmaAllocation allocation = nullptr;
  };

  struct AllocatedBuffer {
    void Destroy(VmaAllocator allocator);

    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    VmaAllocationInfo info{};
  };

  struct PipelineBuilder {
    PipelineBuilder();

    void Clear();
    std::optional<const char*> BuildPipeline(VkDevice device, VkPipelineLayout& layout,
                                             VkPipeline& pipeline, VkRenderPass renderPass,
                                             const VkDescriptorSetLayout* descriptorSetLayouts,
                                             uint32_t descriptorSetLayoutCount);

    PipelineBuilder& SetInputTopology(VkPrimitiveTopology topology);
    PipelineBuilder& SetPolygonMode(VkPolygonMode mode);
    PipelineBuilder& SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    PipelineBuilder& SetMultisamplingNone();
    PipelineBuilder& DisableBlending();
    PipelineBuilder& SetColorAttachmentFormat(VkFormat format);
    PipelineBuilder& SetDepthFormat(VkFormat format);
    PipelineBuilder& DisableDepthTest();
    PipelineBuilder& EnableDepthTest();

    VkPushConstantRange pushConstantRange{};

    std::vector<VkVertexInputAttributeDescription> vertexInputStates;
    std::vector<VkVertexInputBindingDescription> vertexInputBindings;

    std::vector<VulkanShader::VulkanInfo> VulkanInfos;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    VkPipelineMultisampleStateCreateInfo multisampling{};
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    VkPipelineRenderingCreateInfoKHR renderInfo{};

    VkFormat colorAttachmentFormat = VK_FORMAT_UNDEFINED;
  };

  static std::expected<AllocatedBuffer, const char*> CreateBuffer(VmaAllocator allocator,
                                                                  VmaMemoryUsage memoryUsage,
                                                                  VmaAllocationCreateFlags flags,
                                                                  VkDeviceSize size,
                                                                  VkBufferUsageFlags usage) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = flags;

    AllocatedBuffer buffer{};
    VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer,
                                      &buffer.allocation, &buffer.info);
    if (result != VK_SUCCESS) {
      return std::unexpected("Failed to create buffer");
    }

    return buffer;
  }

  static std::expected<AllocatedBuffer, const char*> CreateBuffer(VmaAllocator allocator,
                                                                  VmaMemoryUsage memoryUsage,
                                                                  VkDeviceSize size,
                                                                  VkBufferUsageFlags usage) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    AllocatedBuffer buffer{};
    VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer,
                                      &buffer.allocation, &buffer.info);
    if (result != VK_SUCCESS) {
      return std::unexpected("Failed to create buffer");
    }

    return buffer;
  }

  static std::expected<AllocatedImage, const char*> CreateImage(VmaAllocator allocator,
                                                                VmaMemoryUsage memoryUsage,
                                                                uint32_t width, uint32_t height,
                                                                uint32_t mipLevels, VkFormat format,
                                                                VkImageTiling tiling,
                                                                VkImageUsageFlags usage) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = VkExtent3D{ width, height, 1 };
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
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    AllocatedImage allocatedImage{
      .imageExtent = imageInfo.extent,
      .imageFormat = imageInfo.format
    };

    VkResult result = vmaCreateImage(allocator, &imageInfo, &allocInfo, &allocatedImage.image,
                                     &allocatedImage.allocation, nullptr);
    if (result != VK_SUCCESS) {
      return std::unexpected("Failed to create image");
    }

    return allocatedImage;
  }

  static std::expected<AllocatedImage, const char*> CreateImage(VmaAllocator allocator,
                                                                VmaMemoryUsage memoryUsage,
                                                                VmaAllocationCreateFlags flags,
                                                                uint32_t width, uint32_t height,
                                                                uint32_t mipLevels, VkFormat format,
                                                                VkImageTiling tiling,
                                                                VkImageUsageFlags usage) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = VkExtent3D{ width, height, 1 };
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

    AllocatedImage allocatedImage{
      .imageExtent = imageInfo.extent,
      .imageFormat = imageInfo.format
    };

    VkResult result = vmaCreateImage(allocator, &imageInfo, &allocInfo, &allocatedImage.image,
                                     &allocatedImage.allocation, nullptr);
    if (result != VK_SUCCESS) {
      return std::unexpected("Failed to create image");
    }

    return allocatedImage;
  }

  static std::expected<VkImageView, const char*> CreateImageView(VkDevice device, VkImage image,
                                                                 VkFormat format,
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
    if (result != VK_SUCCESS) {
      return std::unexpected("Failed to create image view");
    }

    return imageView;
  }

  static std::expected<VkDescriptorPool, const char*> CreateDescriptorPool(VkDevice device, uint32_t maxSets,
                                                                           std::span<VkDescriptorPoolSize> poolSizes) {
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = maxSets;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VkDescriptorPool descriptorPool;
    VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
    if (result != VK_SUCCESS) {
      return std::unexpected("Failed to create descriptor pool");
    }

    return descriptorPool;
  }

  static std::expected<VkDescriptorSetLayout, const char*> CreateDescriptorSetLayout(VkDevice device,
                                                                                     std::span<VkDescriptorSetLayoutBinding> bindings) {
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkDescriptorSetLayout layout;
    VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout);
    if (result != VK_SUCCESS) {
      return std::unexpected("Failed to create descriptor set layout");
    }

    return layout;
  }

  static std::expected<std::vector<VkDescriptorSet>, const char*> AllocateDescriptorSets(VkDevice device,
                                                                                         VkDescriptorPool pool,
                                                                                         std::span<VkDescriptorSetLayout> layouts) {
    std::vector<VkDescriptorSet> descriptorSets(layouts.size());
    
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();

    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data());
    if (result != VK_SUCCESS) {
      return std::unexpected("Failed to allocate descriptor sets");
    }

    return descriptorSets;
  }

  static VkWriteDescriptorSet WriteBuffer(VkDescriptorSet dstSet, uint32_t binding, VkDescriptorType type,
                                          const VkDescriptorBufferInfo* bufferInfo) {
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = dstSet;
    write.dstBinding = binding;
    write.dstArrayElement = 0;
    write.descriptorType = type;
    write.descriptorCount = 1;
    write.pBufferInfo = bufferInfo;

    return write;
  }

  static VkWriteDescriptorSet WriteImage(VkDescriptorSet dstSet, uint32_t binding, VkDescriptorType type,
                                         const VkDescriptorImageInfo* imageInfo) {
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = dstSet;
    write.dstBinding = binding;
    write.dstArrayElement = 0;
    write.descriptorType = type;
    write.descriptorCount = 1;
    write.pImageInfo = imageInfo;

    return write;
  }
}