#include "VulkanTexture.h"
#include "VulkanContext.h"

#include "Application.h"

#include <stb_image/stb_image.h>

namespace Helios {
  VulkanTexture::VulkanTexture(Type type, std::string_view path, const Info& info)
    : m_Type(type), m_Info(info) {
    LoadFromFile(path);
  }

  VulkanTexture::~VulkanTexture() {
    Unload();
    stbi_image_free(m_Data);
  }

  void VulkanTexture::Load() {
    if (m_Loaded) { return; }

    if (m_Data == nullptr) {
      CORE_ASSERT(false, "Failed to load texture: No texture data!");
      return;
    }

    switch (m_Type) {
      case Type::Static: CreateStaticImage(); return;
      case Type::Dynamic: CreateDynamicImage(); return;
      default: CORE_ASSERT(false, "Failed to load texture: Unknown texture type!"); return;
    }

    m_Loaded = true;
  }

  void VulkanTexture::Unload() {
    if (!m_Loaded) { return; }

    if (m_ImageBuffer.image != VK_NULL_HANDLE) {
      VulkanContext& context = VulkanContext::Get();
      VkDevice device = context.GetDevice();
      VmaAllocator allocator = context.GetAllocator();

      if (m_ImageBuffer.imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_ImageBuffer.imageView, nullptr);
        m_ImageBuffer.imageView = VK_NULL_HANDLE;
      }

      if (m_ImageBuffer.allocation != nullptr) {
        if (m_Type == Type::Dynamic) {
          vmaUnmapMemory(allocator, m_ImageBuffer.allocation);
        }
        vmaDestroyImage(allocator, m_ImageBuffer.image, m_ImageBuffer.allocation);
        m_ImageBuffer.image = VK_NULL_HANDLE;
        m_ImageBuffer.allocation = nullptr;
      }
    }
  }

  void VulkanTexture::SetData(const void* data) {
    if (m_Type == Type::Static) {
      CORE_ASSERT(false, "Failed to set texture data: Cannot modify static texture!");
      return;
    }

    if (data == nullptr) {
      CORE_ASSERT(false, "Failed to set texture data: Invalid data!");
      return;
    }

    VulkanContext& context = VulkanContext::Get();
    VmaAllocator allocator = context.GetAllocator();
    
    std::memcpy(m_Data, data, m_Width * m_Height * (m_Info.format == ImageFormat::RGB8 ? 3 : 4));

    if (m_Loaded) {
      void* mappedData = nullptr;
      vmaMapMemory(allocator, m_ImageBuffer.allocation, &mappedData);
      std::memcpy(mappedData, data, m_Width * m_Height * (m_Info.format == ImageFormat::RGB8 ? 3 : 4));

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
  }

  void VulkanTexture::SetMipLevel(uint32_t mipLevel) {
    if (mipLevel == 0) {
      CORE_ASSERT(false, "Failed to set mip level: value cannot be '0'!");
      return;
    }

    if ((mipLevel & (mipLevel - 1)) != 0) {
      CORE_ASSERT(false, "Failed to set mip level: value must be a power of 2, got '{}'!", mipLevel);
      return;
    }

    uint32_t maxMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_Width, m_Height)))) + 1;
    if (mipLevel > maxMipLevels) {
      CORE_ASSERT(false, "Failed to set mip level: value '{}' exceeds maximum possible levels for this texture!",
                  mipLevel);
      return;
    }

    m_Info.mipLevel = mipLevel;

    if (m_Loaded) {
      Unload();
      Load();
    }
  }

  void VulkanTexture::SetAnisoLevel(uint32_t anisoLevel) {
    if (anisoLevel == 0) {
      CORE_ASSERT(false, "Failed to set anisotropy level: value cannot be '0'!");
      return;
    }

    if ((anisoLevel & (anisoLevel - 1)) != 0) {
      CORE_ASSERT(false, "Failed to set anisotropy level: value must be a power of 2, got '{}'!", anisoLevel);
      return;
    }

    VulkanContext& context = VulkanContext::Get();
    VkPhysicalDeviceProperties properties = context.GetPhysicalDeviceProperties();
    float maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    if (static_cast<float>(anisoLevel) > maxAnisotropy) {
      CORE_ASSERT(false, "Failed to set anisotropy level: value '{}' exceeds maximum supported level '{}'!",
                  anisoLevel, maxAnisotropy);
      return;
    }

    m_Info.anisoLevel = anisoLevel;
  }

  void VulkanTexture::LoadFromFile(std::string_view path) {
    int width(0), height(0), channels(0);
    stbi_set_flip_vertically_on_load(1);
    m_Data = stbi_load(path.data(), &width, &height, &channels, 4);

    if (m_Data == nullptr) {
      CORE_ASSERT(false, "Failed to load texture from file '{}'!");
      return;
    }
    
    m_Width = static_cast<uint32_t>(width);
    m_Height = static_cast<uint32_t>(height);

    if (m_Info.format == ImageFormat::Unspecified) {
      switch (channels) {
        case 4: m_Info.format = ImageFormat::RGBA8; break;  // Changed from return to break
        case 3: m_Info.format = ImageFormat::RGB8; break;   // Changed from return to break
        default: {
          stbi_image_free(m_Data);  // Added cleanup
          m_Data = nullptr;
          CORE_ASSERT(false, "Failed to load texture from file '{}': Unsupported image format!", path);
          return;
        }
      }
    }
  }

  void VulkanTexture::CreateStaticImage() {
    VulkanContext& context = VulkanContext::Get();
    VmaAllocator allocator = context.GetAllocator();

    auto stagingBuffer = CreateBuffer(allocator, VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                      m_Width * m_Height * (m_Info.format == ImageFormat::RGB8 ? 3 : 4),
                                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    if (!stagingBuffer) {
      CORE_ASSERT(false, "Failed to create texture: {}!", stagingBuffer.error());
      Unload();
      return;
    }

    VkDevice device = context.GetDevice();
    VkFormat format = GetVulkanFormat(m_Info.format);

    std::memcpy(stagingBuffer->info.pMappedData, m_Data, stagingBuffer->info.size);

    auto imageResult = CreateImage(allocator, VMA_MEMORY_USAGE_GPU_ONLY, m_Width, m_Height, m_Info.mipLevel, format,
                                   VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    if (!imageResult) {
      CORE_ASSERT(false, "Failed to create texture: {}!", imageResult.error());
      Unload();
      return;
    }

    m_ImageBuffer = std::move(*imageResult);

    auto imageViewResult = CreateImageView(device, m_ImageBuffer.image, format, VK_IMAGE_ASPECT_COLOR_BIT);
    if (!imageViewResult) {
      CORE_ASSERT(false, "Failed to create texture: {}!", imageViewResult.error());
      Unload();
      return;
    }
    
    m_ImageBuffer.imageView = *imageViewResult;

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
        .mipLevel = 0,
        .baseArrayLayer = 0,
        .layerCount = 1
      };
      copyRegion.imageExtent = VkExtent3D{ m_Width, m_Height, 1 };

      vkCmdCopyBufferToImage(cmd, stagingBuffer->buffer, m_ImageBuffer.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             1, &copyRegion);

      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

      vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                           0, nullptr, 0, nullptr, 1, &barrier);
    });

    vmaUnmapMemory(allocator, stagingBuffer->allocation);
    vmaDestroyBuffer(allocator, stagingBuffer->buffer, stagingBuffer->allocation);
  }

  void VulkanTexture::CreateDynamicImage() {
    VulkanContext& context = VulkanContext::Get();
    VkDevice device = context.GetDevice();
    VmaAllocator allocator = context.GetAllocator();

    VkFormat format = GetVulkanFormat(m_Info.format);
    
    auto imageResult = CreateImage(allocator, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                   m_Width, m_Height, m_Info.mipLevel, format, VK_IMAGE_TILING_LINEAR,
                                   VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    if (!imageResult) {
      CORE_ASSERT(false, "Failed to create texture: {}!", imageResult.error());
      Unload();
      return;
    }

    m_ImageBuffer = std::move(*imageResult);

    auto imageViewResult = CreateImageView(device, m_ImageBuffer.image, format, VK_IMAGE_ASPECT_COLOR_BIT);
    if (!imageViewResult) {
      CORE_ASSERT(false, "Failed to create texture: {}!", imageViewResult.error());
      Unload();
      return;
    }

    m_ImageBuffer.imageView = *imageViewResult;

    void* mappedData = nullptr;
    vmaMapMemory(allocator, m_ImageBuffer.allocation, &mappedData);
    std::memcpy(mappedData, m_Data, m_Width * m_Height * (m_Info.format == ImageFormat::RGB8 ? 3 : 4));

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
      default: CORE_ASSERT(false, "Cannot get vulkan format: Unknown image format!"); return VK_FORMAT_UNDEFINED;
    }
  }
}