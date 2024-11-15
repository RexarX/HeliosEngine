#pragma once

#include "pch.h"

namespace Helios {
  class HELIOSENGINE_API Texture {
  public:
    enum class ImageFormat {
      None = 0,
      R8,
      RGB8,
      RGBA8,
      RGBA32F
    };

    virtual ~Texture() = default;

    virtual void Load() = 0;
    virtual void Unload() = 0;

    virtual void SetSlot(uint32_t slot) = 0;

    virtual void SetFormat(ImageFormat format) = 0;

    virtual void SetMipLevel(uint32_t mipLevel) = 0;
    virtual void SetAnisoLevel(uint32_t anisoLevel) = 0;

    virtual inline uint32_t GetWidth() const = 0;
    virtual inline uint32_t GetHeight() const = 0;

    virtual inline uint32_t GetSlot() const = 0;

    virtual inline ImageFormat GetFormat() const = 0;

    virtual inline uint32_t GetMipLevel() const = 0;
    virtual inline uint32_t GetAnisoLevel() const = 0;

    static std::shared_ptr<Texture> Create(std::string_view path, uint32_t mipLevel = 0,
                                           uint32_t anisoLevel = 0, ImageFormat format = ImageFormat::None);
  };
}