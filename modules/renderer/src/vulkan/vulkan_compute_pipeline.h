#pragma once

#include "helios/renderer/pipeline/compute_pipeline.h"

namespace nvrhi {
    class IComputePipeline;
}

namespace helios {
namespace renderer {

/**
 * @brief Vulkan implementation of ComputePipeline using NVRHI
 */
class VulkanComputePipeline final : public ComputePipeline {
public:
    VulkanComputePipeline(std::shared_ptr<nvrhi::IComputePipeline> nvrhi_pipeline, 
                         const ComputePipelineDesc& desc);
    ~VulkanComputePipeline() override = default;
    
    // ComputePipeline interface implementation
    std::shared_ptr<Shader> GetComputeShader() const override { return compute_shader_; }
    void GetLocalWorkGroupSize(uint32_t& x, uint32_t& y, uint32_t& z) const override;
    uint64_t GetHash() const override { return hash_; }
    nvrhi::IComputePipeline* GetNVRHIPipeline() const override { return nvrhi_pipeline_.get(); }

private:
    std::shared_ptr<nvrhi::IComputePipeline> nvrhi_pipeline_;
    std::shared_ptr<Shader> compute_shader_;
    uint32_t local_size_x_ = 1;
    uint32_t local_size_y_ = 1;
    uint32_t local_size_z_ = 1;
    uint64_t hash_;
    
    uint64_t ComputeHash(const ComputePipelineDesc& desc) const;
};

} // namespace renderer
} // namespace helios