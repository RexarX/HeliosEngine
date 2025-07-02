#pragma once

#include "helios/renderer/command_context.h"
#include <memory>

namespace nvrhi {
    class ICommandList;
}

namespace helios {
namespace renderer {

class VulkanDevice;

/**
 * @brief Vulkan implementation of CommandContext using NVRHI
 */
class VulkanCommandContext final : public CommandContext {
public:
    VulkanCommandContext(VulkanDevice& device, uint32_t thread_id);
    ~VulkanCommandContext() override;
    
    // CommandContext interface implementation
    void Begin() override;
    void End() override;
    void Submit() override;
    void BeginRenderPass() override;
    void EndRenderPass() override;
    void SetGraphicsPipeline(const GraphicsPipeline& pipeline) override;
    void SetComputePipeline(const ComputePipeline& pipeline) override;
    void BindVertexBuffer(const Buffer& buffer, uint32_t binding = 0, uint64_t offset = 0) override;
    void BindIndexBuffer(const Buffer& buffer, uint64_t offset = 0, bool is_16bit = false) override;
    void BindDescriptorSet(uint32_t set_index, const void* descriptor_set) override;
    void DrawIndexed(uint32_t index_count, uint32_t instance_count = 1, 
                    uint32_t first_index = 0, int32_t vertex_offset = 0, 
                    uint32_t first_instance = 0) override;
    void Draw(uint32_t vertex_count, uint32_t instance_count = 1, 
             uint32_t first_vertex = 0, uint32_t first_instance = 0) override;
    void Dispatch(uint32_t group_count_x, uint32_t group_count_y = 1, uint32_t group_count_z = 1) override;
    void MemoryBarrier() override;
    void CopyBuffer(const Buffer& src, const Buffer& dst, uint64_t src_offset = 0, 
                   uint64_t dst_offset = 0, uint64_t size = 0) override;
    void CopyBufferToTexture(const Buffer& src, const Texture& dst) override;
    nvrhi::ICommandList* GetNVRHICommandList() const override { return command_list_.get(); }

private:
    VulkanDevice& device_;
    uint32_t thread_id_;
    std::shared_ptr<nvrhi::ICommandList> command_list_;
};

} // namespace renderer
} // namespace helios