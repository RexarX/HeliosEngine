#pragma once

#include "Renderer/Texture.h"

#include "VulkanUtils.h"

namespace Helios
{
  class VulkanTexture final : public Texture
  {
  public:
    VulkanTexture(std::string_view path, uint32_t mipLevel = 0,
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
    void LoadFromFile(std::string_view path);

  private:
    void* m_Data = nullptr;

    uint32_t m_Width = 0;
    uint32_t m_Height = 0;

    uint32_t m_Slot = 0;

    ImageFormat m_Format = ImageFormat::None;

    uint32_t m_MipLevel = 0;
    uint32_t m_AnisoLevel = 0;

    AllocatedImage m_Image;
    AllocatedImage m_ImageStaging;
  };
}