#pragma once

#include "helios/renderer/resources/buffer.h"

namespace nvrhi {
    class IBuffer;
}

namespace helios {
namespace renderer {

/**
 * @brief Vulkan implementation of Buffer using NVRHI
 */
class VulkanBuffer final : public Buffer {
public:
    VulkanBuffer(std::shared_ptr<nvrhi::IBuffer> nvrhi_buffer, size_t size, uint32_t usage, bool host_visible);
    ~VulkanBuffer() override = default;
    
    // Buffer interface implementation
    size_t GetSize() const override { return size_; }
    uint32_t GetUsage() const override { return usage_; }
    bool IsHostVisible() const override { return host_visible_; }
    void* Map() override;
    void Unmap() override;
    void UpdateData(const void* data, size_t size, size_t offset = 0) override;
    nvrhi::IBuffer* GetNVRHIBuffer() const override { return nvrhi_buffer_.get(); }

private:
    std::shared_ptr<nvrhi::IBuffer> nvrhi_buffer_;
    size_t size_;
    uint32_t usage_;
    bool host_visible_;
    void* mapped_ptr_ = nullptr;
};

} // namespace renderer
} // namespace helios