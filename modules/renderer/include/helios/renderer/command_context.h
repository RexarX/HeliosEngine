#pragma once

#include <memory>
#include <cstdint>

// Forward declarations for NVRHI types
namespace nvrhi {
    class ICommandList;
}

namespace helios {
namespace renderer {

class Buffer;
class Texture;
class GraphicsPipeline;
class ComputePipeline;

/**
 * @brief Command context for thread-safe command recording
 * 
 * This class provides a thread-local interface for recording GPU commands.
 * Each thread should have its own command context to ensure thread safety.
 * Commands are recorded into internal command buffers and submitted to the GPU.
 */
class CommandContext {
public:
    virtual ~CommandContext() = default;
    
    /**
     * @brief Begin recording commands
     */
    virtual void Begin() = 0;
    
    /**
     * @brief End recording and prepare for submission
     */
    virtual void End() = 0;
    
    /**
     * @brief Submit recorded commands to the GPU
     */
    virtual void Submit() = 0;
    
    /**
     * @brief Begin a render pass
     */
    virtual void BeginRenderPass() = 0;
    
    /**
     * @brief End the current render pass
     */
    virtual void EndRenderPass() = 0;
    
    /**
     * @brief Set the graphics pipeline for subsequent draw calls
     */
    virtual void SetGraphicsPipeline(const GraphicsPipeline& pipeline) = 0;
    
    /**
     * @brief Set the compute pipeline for subsequent dispatch calls
     */
    virtual void SetComputePipeline(const ComputePipeline& pipeline) = 0;
    
    /**
     * @brief Bind vertex buffer
     */
    virtual void BindVertexBuffer(const Buffer& buffer, uint32_t binding = 0, uint64_t offset = 0) = 0;
    
    /**
     * @brief Bind index buffer
     */
    virtual void BindIndexBuffer(const Buffer& buffer, uint64_t offset = 0, bool is_16bit = false) = 0;
    
    /**
     * @brief Bind descriptor sets (uniforms, textures, etc.)
     */
    virtual void BindDescriptorSet(uint32_t set_index, const void* descriptor_set) = 0;
    
    /**
     * @brief Draw indexed primitives
     */
    virtual void DrawIndexed(uint32_t index_count, uint32_t instance_count = 1, 
                            uint32_t first_index = 0, int32_t vertex_offset = 0, 
                            uint32_t first_instance = 0) = 0;
    
    /**
     * @brief Draw non-indexed primitives
     */
    virtual void Draw(uint32_t vertex_count, uint32_t instance_count = 1, 
                     uint32_t first_vertex = 0, uint32_t first_instance = 0) = 0;
    
    /**
     * @brief Dispatch compute work
     */
    virtual void Dispatch(uint32_t group_count_x, uint32_t group_count_y = 1, uint32_t group_count_z = 1) = 0;
    
    /**
     * @brief Insert a memory barrier
     */
    virtual void MemoryBarrier() = 0;
    
    /**
     * @brief Copy data between buffers
     */
    virtual void CopyBuffer(const Buffer& src, const Buffer& dst, uint64_t src_offset = 0, 
                           uint64_t dst_offset = 0, uint64_t size = 0) = 0;
    
    /**
     * @brief Copy data from buffer to texture
     */
    virtual void CopyBufferToTexture(const Buffer& src, const Texture& dst) = 0;
    
    /**
     * @brief Get the underlying NVRHI command list (for internal use only)
     */
    virtual nvrhi::ICommandList* GetNVRHICommandList() const = 0;

protected:
    CommandContext() = default;
};

} // namespace renderer
} // namespace helios