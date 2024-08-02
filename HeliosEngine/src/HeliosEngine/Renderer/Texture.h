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

  class HELIOSENGINE_API Texture
  {
  public:
    virtual ~Texture() = default;

    virtual void Load() = 0;
    virtual void Unload() = 0;

    virtual void SetSlot(const uint32_t slot) = 0;

    virtual void SetFormat(const ImageFormat format) = 0;

    virtual void SetMipLevel(const uint32_t mipLevel) = 0;
    virtual void SetAnisoLevel(const uint32_t anisoLevel) = 0;

    virtual inline const uint32_t GetWidth() const = 0;
    virtual inline const uint32_t GetHeight() const = 0;

    virtual inline const uint32_t GetSlot() const = 0;

    virtual inline const ImageFormat GetFormat() const = 0;

    virtual inline const uint32_t GetMipLevel() const = 0;
    virtual inline const uint32_t GetAnisoLevel() const = 0;

    static std::shared_ptr<Texture> Create(const std::string& path, const uint32_t mipLevel = 0,
                                           const uint32_t anisoLevel = 0, const ImageFormat format = ImageFormat::None);
  };
}