#pragma once

#include "helios/renderer/resources/texture.h"

namespace nvrhi {
    class ITexture;
}

namespace helios {
namespace renderer {

/**
 * @brief Vulkan implementation of Texture using NVRHI
 */
class VulkanTexture final : public Texture {
public:
    VulkanTexture(std::shared_ptr<nvrhi::ITexture> nvrhi_texture, const TextureDesc& desc);
    ~VulkanTexture() override = default;
    
    // Texture interface implementation
    Type GetType() const override { return type_; }
    Format GetFormat() const override { return format_; }
    uint32_t GetWidth() const override { return width_; }
    uint32_t GetHeight() const override { return height_; }
    uint32_t GetDepth() const override { return depth_; }
    uint32_t GetMipLevels() const override { return mip_levels_; }
    uint32_t GetArrayLayers() const override { return array_layers_; }
    uint32_t GetUsage() const override { return usage_; }
    void UpdateData(const void* data, size_t size, uint32_t mip_level = 0, uint32_t array_layer = 0) override;
    void GenerateMips() override;
    nvrhi::ITexture* GetNVRHITexture() const override { return nvrhi_texture_.get(); }

private:
    std::shared_ptr<nvrhi::ITexture> nvrhi_texture_;
    Type type_;
    Format format_;
    uint32_t width_;
    uint32_t height_;
    uint32_t depth_;
    uint32_t mip_levels_;
    uint32_t array_layers_;
    uint32_t usage_;
};

} // namespace renderer
} // namespace helios