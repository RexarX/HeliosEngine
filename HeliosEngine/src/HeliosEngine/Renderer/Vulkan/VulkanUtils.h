#pragma once

#include "pch.h"

#include <vulkan/vulkan.h>

#include <vma/vk_mem_alloc.h>

namespace Helios
{
  struct FrameData
  {
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore presentSemaphore;
    VkSemaphore renderSemaphore;
    VkFence renderFence;
  };

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

  struct AllocatedImage
  {
    VkImage image;
    VkImageView imageView;
    VkExtent3D imageExtent;
    VkFormat imageFormat;

    VmaAllocation allocation;
  };

  const AllocatedImage CreateImage(const uint32_t width, const uint32_t height, const VkFormat format, const VkImageTiling tiling,
                                   const VkImageUsageFlags usage, const VmaMemoryUsage memoryUsage, const VmaAllocator allocator);

  const VkImageView CreateImageView(const VkImage image, const VkFormat format, const VkImageAspectFlags aspectFlags,
                                    const VkDevice device);
}