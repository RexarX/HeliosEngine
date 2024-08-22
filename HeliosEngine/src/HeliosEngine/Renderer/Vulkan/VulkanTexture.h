#pragma once

#include "Renderer/Texture.h"

#include "VulkanUtils.h"

namespace Helios
{
  class VulkanTexture : public Texture
  {
  public:
    VulkanTexture(const std::string& path, uint32_t mipLevel = 0,
                  uint32_t anisoLevel = 0, ImageFormat format = ImageFormat::None);

    ~VulkanTexture() = default;

    virtual void Load() override;
    virtual void Unload() override;

    void SetSlot(uint32_t slot) override { m_Slot = slot; }

    void SetFormat(ImageFormat format) override { m_Format = format; }

    void SetMipLevel(uint32_t mipLevel) override { m_MipLevel = mipLevel; }
    void SetAnisoLevel(uint32_t anisoLevel) override { m_AnisoLevel = anisoLevel; }

    inline uint32_t GetWidth() const override { return m_Width; }
    inline uint32_t GetHeight() const override { return m_Height; }

    inline uint32_t GetSlot() const override { return m_Slot; }

    inline ImageFormat GetFormat() const override { return m_Format; }

    inline uint32_t GetMipLevel() const override { return m_MipLevel; }
    inline uint32_t GetAnisoLevel() const override { return m_AnisoLevel; }

  private:
    void LoadFromFile(const std::string& path);

  private:
    void* m_Data = nullptr;

    uint32_t m_Width;
    uint32_t m_Height;

    uint32_t m_Slot;

    ImageFormat m_Format;

    uint32_t m_MipLevel;
    uint32_t m_AnisoLevel;

    AllocatedImage m_Image;
    AllocatedImage m_ImageStaging;
  };
}