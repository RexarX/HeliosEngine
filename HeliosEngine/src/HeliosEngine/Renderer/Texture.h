#pragma once

#include "pch.h"

namespace Helios
{
  enum class ImageFormat
  {
    None = 0,
    R8,
    RGB8,
    RGBA8,
    RGBA32F
  };

  class Texture
  {
  public:
    Texture(const std::string& path, const uint32_t mipLevel = 0, const uint32_t anisoLevel = 0,
            const ImageFormat format = ImageFormat::RGBA8);

    ~Texture() = default;

    void SetFormat(const ImageFormat format) { m_Format = format; }

    void SetMipLevel(const uint32_t mipLevel) { m_MipLevel = mipLevel; }
    void SetAnisoLevel(const uint32_t anisoLevel) { m_AnisoLevel = anisoLevel; }

    inline const void* GetImageData() const { return m_ImageData.get(); }

    inline const ImageFormat GetFormat() const { return m_Format; }

    inline const uint32_t GetWidth() const { return m_Width; }
    inline const uint32_t GetHeight() const { return m_Height; }

    inline const uint32_t GetMipLevel() const { return m_MipLevel; }
    inline const uint32_t GetAnisoLevel() const { return m_AnisoLevel; }

  private:
    std::unique_ptr<uint8_t[]> m_ImageData = nullptr;

    ImageFormat m_Format = ImageFormat::None;

    uint32_t m_Width = 0, m_Height = 0;

    uint32_t m_MipLevel = 0;
    uint32_t m_AnisoLevel = 0;
  };
}