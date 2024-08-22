#include "VulkanTexture.h"
#include "VulkanContext.h"

#include <stb_image/stb_image.h>

namespace Helios
{
  VulkanTexture::VulkanTexture(const std::string& path, uint32_t mipLevel, uint32_t anisoLevel, ImageFormat format)
    : m_MipLevel(mipLevel), m_AnisoLevel(anisoLevel), m_Format(format)
  {
  }

  void VulkanTexture::Load()
  {
  }

  void VulkanTexture::Unload()
  {
  }

  void VulkanTexture::LoadFromFile(const std::string& path)
  {
    int32_t width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    m_Data = stbi_load(path.c_str(), &width, &height, &channels, 4);

    if (m_Data == nullptr) { CORE_ERROR("Failed to load texture!"); }

    m_Width = width;
    m_Height = height;

    if (m_Format == ImageFormat::None) {
      switch (channels)
      {
      case 4: m_Format = ImageFormat::RGBA8; break;
      case 3: m_Format = ImageFormat::RGB8; break;
      }
    }
  }
}