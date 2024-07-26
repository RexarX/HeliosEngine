#pragma once

#include "Renderer/Vulkan/VulkanUtils.h"

namespace Helios
{
  const AllocatedImage CreateImage(const uint32_t width, const uint32_t height, const VkFormat format, const VkImageTiling tiling,
                                   const VkImageUsageFlags usage, const VmaMemoryUsage memoryUsage, const VmaAllocator allocator)
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

  const VkImageView CreateImageView(const VkImage image, const VkFormat format, const VkImageAspectFlags aspectFlags,
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