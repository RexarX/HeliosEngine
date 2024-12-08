#include "VulkanTexture.h"
#include "VulkanContext.h"

#include "Application.h"

#include <stb_image/stb_image.h>

namespace Helios {
  VulkanTexture::VulkanTexture(Type type, std::string_view path, const Info& info)
    : m_Type(type), m_Info(info) {
    LoadFromFile(path);
  }

  void VulkanTexture::Load() {
    if (m_Data == nullptr) {
      CORE_ERROR("No texture data loaded from file.");
      return;
    }

    switch (m_Type) {
      case Type::Static: CreateStaticImage(); return;
      case Type::Dynamic: CreateDynamicImage(); return;
      default: CORE_ASSERT(false, "Unknown texture type!"); return;
    }
  }

  void VulkanTexture::Unload() {
    if (m_ImageBuffer.image != VK_NULL_HANDLE) {
      VulkanContext& context = VulkanContext::Get();
      const VkDevice device = context.GetDevice();
      const VmaAllocator allocator = context.GetAllocator();

      if (m_ImageBuffer.imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_ImageBuffer.imageView, nullptr);
        m_ImageBuffer.imageView = VK_NULL_HANDLE;
      }

      vmaUnmapMemory(allocator, m_ImageBuffer.allocation);
      vmaDestroyImage(allocator, m_ImageBuffer.image, m_ImageBuffer.allocation);
      m_ImageBuffer.image = VK_NULL_HANDLE;
      m_ImageBuffer.allocation = nullptr;
    }
  }

  void VulkanTexture::SetData() {
    if (m_Type == Type::Static) {
      CORE_ASSERT(false, "Cannot modify static texture!");
      return;
    }

    VulkanContext& context = VulkanContext::Get();
    const VmaAllocator allocator = context.GetAllocator();
    uint64_t imageSize = m_Width * m_Height * (m_Info.format == ImageFormat::RGB8 ? 3 : 4);

    void* mappedData = nullptr;
    vmaMapMemory(allocator, m_ImageBuffer.allocation, &mappedData);
    std::memcpy(mappedData, m_Data, imageSize);
    
    context.ImmediateSubmit([this](const VkCommandBuffer cmd) {
      VkImageMemoryBarrier barrier{};
      barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.image = m_ImageBuffer.image;
      barrier.subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = m_Info.mipLevel,
        .baseArrayLayer = 0,
        .layerCount = 1
      };

      vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                           0, 0, nullptr, 0, nullptr, 1, &barrier);
    });
  }

  void VulkanTexture::SetMipLevel(uint32_t mipLevel) {
    if (mipLevel == 0) {
      CORE_ASSERT(false, "Mip level cannot be 0!");
      return;
    }

    if ((mipLevel & (mipLevel - 1)) != 0) {
      CORE_ASSERT(false, "Mip level must be a power of 2, got '{}'!", mipLevel);
      return;
    }

    uint32_t maxMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_Width, m_Height)))) + 1;
    if (mipLevel > maxMipLevels) {
      CORE_ASSERT(false, "Mip level '{}' exceeds maximum possible levels for this texture!", mipLevel);
      return;
    }

    m_Info.mipLevel = mipLevel;

    Unload();
    if (m_Type == Type::Static) { CreateStaticImage(); }
    else { CreateDynamicImage(); }
  }

  void VulkanTexture::SetAnisoLevel(uint32_t anisoLevel) {
    if (anisoLevel == 0) {
      CORE_ASSERT(false, "Anisotropy level cannot be 0!");
      return;
    }

    if ((anisoLevel & (anisoLevel - 1)) != 0) {
      CORE_ASSERT(false, "Anisotropy level must be a power of 2, got '{}'!", anisoLevel);
      return;
    }

    VulkanContext& context = VulkanContext::Get();
    VkPhysicalDeviceProperties properties = context.GetPhysicalDeviceProperties();
    float maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    if (static_cast<float>(anisoLevel) > maxAnisotropy) {
      CORE_ASSERT(false, "Anisotropy level '{}' exceeds maximum supported level '{}'!", anisoLevel, maxAnisotropy);
      return;
    }

    m_Info.anisoLevel = anisoLevel;
  }

  void VulkanTexture::LoadFromFile(std::string_view path) {
    int width(0), height(0), channels(0);
    stbi_set_flip_vertically_on_load(1);
    m_Data = stbi_load(path.data(), &width, &height, &channels, 4);

    if (m_Data == nullptr) {
      CORE_ASSERT(false, "Failed to load texture!");
      return;
    }
    
    m_Width = static_cast<uint32_t>(width);
    m_Height = static_cast<uint32_t>(height);

    if (m_Info.format == ImageFormat::Unspecified) {
      switch (channels) {
        case 4: m_Info.format = ImageFormat::RGBA8; break;
        case 3: m_Info.format = ImageFormat::RGB8; break;
      }
    }
  }

  void VulkanTexture::CreateStaticImage() {
    VulkanContext& context = VulkanContext::Get();
    const VkDevice device = context.GetDevice();
    const VmaAllocator allocator = context.GetAllocator();

    AllocatedBuffer stagingBuffer{};

    VkBufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = m_Width * m_Height * (m_Info.format == ImageFormat::RGB8 ? 3 : 4);
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkResult result = vmaCreateBuffer(allocator, &stagingBufferInfo, &allocInfo, &stagingBuffer.buffer,
                                      &stagingBuffer.allocation, nullptr);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to create staging buffer for texture!");

    std::memcpy(stagingBuffer.info.pMappedData, m_Data, stagingBufferInfo.size);

    VkFormat format = GetVulkanFormat(m_Info.format);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent = VkExtent3D{ m_Width, m_Height, 1 };
    imageInfo.mipLevels = m_Info.mipLevel;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    result = vmaCreateImage(allocator, &imageInfo, &allocInfo, &m_ImageBuffer.image,
                            &m_ImageBuffer.allocation, nullptr);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to create image!");

    m_ImageBuffer.imageView = CreateImageView(device, m_ImageBuffer.image, format, VK_IMAGE_ASPECT_COLOR_BIT);

    context.ImmediateSubmit([this, &stagingBuffer](const VkCommandBuffer cmd) {
      VkImageMemoryBarrier barrier{};
      barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.image = m_ImageBuffer.image;
      barrier.subresourceRange =  {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = m_Info.mipLevel,
        .baseArrayLayer = 0,
        .layerCount = 1
      };

      vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                           0, nullptr, 0, nullptr, 1, &barrier);

      VkBufferImageCopy copyRegion{};
      copyRegion.bufferOffset = 0;
      copyRegion.imageSubresource = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = m_Info.mipLevel,
        .baseArrayLayer = 0,
        .layerCount = 1
      };
      copyRegion.imageSubresource.mipLevel = m_Info.mipLevel;
      copyRegion.imageSubresource.baseArrayLayer = 0;
      copyRegion.imageSubresource.layerCount = 1;
      copyRegion.imageExtent = VkExtent3D{ m_Width, m_Height, 1 };

      vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, m_ImageBuffer.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             1, &copyRegion);

      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

      vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                           0, nullptr, 0, nullptr, 1, &barrier);
    });

    vmaUnmapMemory(allocator, stagingBuffer.allocation);
    vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
  }

  void VulkanTexture::CreateDynamicImage() {
    VulkanContext& context = VulkanContext::Get();
    const VkDevice device = context.GetDevice();
    const VmaAllocator allocator = context.GetAllocator();

    VkFormat format = GetVulkanFormat(m_Info.format);
    uint64_t imageSize = m_Width * m_Height * (m_Info.format == ImageFormat::RGB8 ? 3 : 4);
    
    m_ImageBuffer = CreateImage(
      allocator,
      VMA_MEMORY_USAGE_CPU_TO_GPU,
      VMA_ALLOCATION_CREATE_MAPPED_BIT,
      m_Width,
      m_Height,
      m_Info.mipLevel,
      format,
      VK_IMAGE_TILING_LINEAR,
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
    );

    m_ImageBuffer.imageView = CreateImageView(device, m_ImageBuffer.image, format, VK_IMAGE_ASPECT_COLOR_BIT);
    m_ImageBuffer.imageFormat = format;
    m_ImageBuffer.imageExtent = VkExtent3D{ m_Width, m_Height, 1 };

    void* mappedData = nullptr;
    vmaMapMemory(allocator, m_ImageBuffer.allocation, &mappedData);
    std::memcpy(mappedData, m_Data, imageSize);

    context.ImmediateSubmit([this](const VkCommandBuffer cmd) {
      VkImageMemoryBarrier barrier{};
      barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.image = m_ImageBuffer.image;
      barrier.subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = m_Info.mipLevel,
        .baseArrayLayer = 0,
        .layerCount = 1
      };

      vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                           0, 0, nullptr, 0, nullptr, 1, &barrier);
    });
  }

  VkFormat VulkanTexture::GetVulkanFormat(ImageFormat format) {
    switch (format) {
      case ImageFormat::R8: return VK_FORMAT_R8_UNORM;
      case ImageFormat::RGB8: return VK_FORMAT_R8G8B8_UNORM;
      case ImageFormat::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
      case ImageFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
      default: CORE_ASSERT(false, "Unknown image format!"); return VK_FORMAT_UNDEFINED;
    }
  }
}