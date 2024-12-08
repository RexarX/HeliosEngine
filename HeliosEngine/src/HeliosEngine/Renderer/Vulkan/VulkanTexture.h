#pragma once

#include "Renderer/Texture.h"

#include "VulkanUtils.h"

namespace Helios {
  class VulkanTexture final : public Texture {
  public:
    VulkanTexture(Type type, std::string_view path, const Info& info);

    virtual ~VulkanTexture() = default;

    void Load() override;
    void Unload() override;

    void SetData() override;

    void SetSlot(uint32_t slot) override { m_Slot = slot; }

    void SetMipLevel(uint32_t mipLevel) override;
    void SetAnisoLevel(uint32_t anisoLevel) override;

    inline Type GetType() const override { return m_Type; }

    inline uint32_t GetWidth() const override { return m_Width; }
    inline uint32_t GetHeight() const override { return m_Height; }

    inline uint32_t GetSlot() const override { return m_Slot; }

    inline ImageFormat GetFormat() const override { return m_Info.format; }

    inline uint32_t GetMipLevel() const override { return m_Info.mipLevel; }
    inline uint32_t GetAnisoLevel() const override { return m_Info.anisoLevel; }

    inline AllocatedImage& GetImageBuffer() { return m_ImageBuffer; }

  private:
    void LoadFromFile(std::string_view path);

    void CreateStaticImage();
    void CreateDynamicImage();

    static VkFormat GetVulkanFormat(ImageFormat format);

  private:
    Type m_Type;

    void* m_Data = nullptr;

    uint32_t m_Width = 0;
    uint32_t m_Height = 0;

    uint32_t m_Slot = 0;

    Info m_Info;

    AllocatedImage m_ImageBuffer;
  };
}