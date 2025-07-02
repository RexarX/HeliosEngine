#pragma once

#include "helios/renderer/pipeline/graphics_pipeline.h"

namespace nvrhi {
    class IGraphicsPipeline;
}

namespace helios {
namespace renderer {

/**
 * @brief Vulkan implementation of GraphicsPipeline using NVRHI
 */
class VulkanGraphicsPipeline final : public GraphicsPipeline {
public:
    VulkanGraphicsPipeline(std::shared_ptr<nvrhi::IGraphicsPipeline> nvrhi_pipeline, 
                          const GraphicsPipelineDesc& desc);
    ~VulkanGraphicsPipeline() override = default;
    
    // GraphicsPipeline interface implementation
    std::shared_ptr<Shader> GetVertexShader() const override { return vertex_shader_; }
    std::shared_ptr<Shader> GetFragmentShader() const override { return fragment_shader_; }
    const std::vector<VertexBinding>& GetVertexBindings() const override { return vertex_bindings_; }
    const std::vector<VertexAttribute>& GetVertexAttributes() const override { return vertex_attributes_; }
    uint64_t GetHash() const override { return hash_; }
    nvrhi::IGraphicsPipeline* GetNVRHIPipeline() const override { return nvrhi_pipeline_.get(); }

private:
    std::shared_ptr<nvrhi::IGraphicsPipeline> nvrhi_pipeline_;
    std::shared_ptr<Shader> vertex_shader_;
    std::shared_ptr<Shader> fragment_shader_;
    std::vector<VertexBinding> vertex_bindings_;
    std::vector<VertexAttribute> vertex_attributes_;
    uint64_t hash_;
    
    uint64_t ComputeHash(const GraphicsPipelineDesc& desc) const;
};

} // namespace renderer
} // namespace helios