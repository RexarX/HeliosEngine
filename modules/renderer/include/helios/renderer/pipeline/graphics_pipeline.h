#pragma once

#include <memory>
#include <vector>
#include <cstdint>

// Forward declarations for NVRHI types
namespace nvrhi {
    class IGraphicsPipeline;
}

namespace helios {
namespace renderer {

class Shader;
class RenderTarget;

/**
 * @brief Graphics pipeline state object
 * 
 * This class encapsulates all graphics pipeline state including shaders,
 * vertex input layout, rasterization state, depth-stencil state, and
 * blend state. It provides efficient state switching and caching.
 */
class GraphicsPipeline {
public:
    /**
     * @brief Vertex input attribute description
     */
    struct VertexAttribute {
        uint32_t location;
        uint32_t format;
        uint32_t offset;
        uint32_t input_rate; // 0 = per vertex, 1 = per instance
    };
    
    /**
     * @brief Vertex input binding description
     */
    struct VertexBinding {
        uint32_t binding;
        uint32_t stride;
        uint32_t input_rate; // 0 = per vertex, 1 = per instance
    };
    
    /**
     * @brief Rasterization state
     */
    struct RasterizationState {
        uint32_t polygon_mode = 0; // 0 = fill, 1 = line, 2 = point
        uint32_t cull_mode = 0;    // 0 = none, 1 = front, 2 = back
        uint32_t front_face = 0;   // 0 = CCW, 1 = CW
        bool depth_clamp_enable = false;
        bool depth_bias_enable = false;
        float depth_bias_constant_factor = 0.0f;
        float depth_bias_clamp = 0.0f;
        float depth_bias_slope_factor = 0.0f;
        float line_width = 1.0f;
    };
    
    /**
     * @brief Depth-stencil state
     */
    struct DepthStencilState {
        bool depth_test_enable = true;
        bool depth_write_enable = true;
        uint32_t depth_compare_op = 4; // LESS_OR_EQUAL
        bool stencil_test_enable = false;
        // Stencil operations omitted for brevity
    };
    
    /**
     * @brief Color blend attachment state
     */
    struct ColorBlendAttachment {
        bool blend_enable = false;
        uint32_t src_color_blend_factor = 1; // ONE
        uint32_t dst_color_blend_factor = 0; // ZERO
        uint32_t color_blend_op = 0;         // ADD
        uint32_t src_alpha_blend_factor = 1; // ONE
        uint32_t dst_alpha_blend_factor = 0; // ZERO
        uint32_t alpha_blend_op = 0;         // ADD
        uint32_t color_write_mask = 0xF;     // R|G|B|A
    };
    
    virtual ~GraphicsPipeline() = default;
    
    /**
     * @brief Get the vertex shader
     */
    virtual std::shared_ptr<Shader> GetVertexShader() const = 0;
    
    /**
     * @brief Get the fragment shader
     */
    virtual std::shared_ptr<Shader> GetFragmentShader() const = 0;
    
    /**
     * @brief Get vertex input bindings
     */
    virtual const std::vector<VertexBinding>& GetVertexBindings() const = 0;
    
    /**
     * @brief Get vertex input attributes
     */
    virtual const std::vector<VertexAttribute>& GetVertexAttributes() const = 0;
    
    /**
     * @brief Get the pipeline hash for caching
     */
    virtual uint64_t GetHash() const = 0;
    
    /**
     * @brief Get the underlying NVRHI pipeline (for internal use only)
     */
    virtual nvrhi::IGraphicsPipeline* GetNVRHIPipeline() const = 0;

protected:
    GraphicsPipeline() = default;
};

/**
 * @brief Graphics pipeline creation parameters
 */
struct GraphicsPipelineDesc {
    std::shared_ptr<Shader> vertex_shader;
    std::shared_ptr<Shader> fragment_shader;
    std::shared_ptr<Shader> geometry_shader;
    std::shared_ptr<Shader> tess_control_shader;
    std::shared_ptr<Shader> tess_eval_shader;
    
    std::vector<GraphicsPipeline::VertexBinding> vertex_bindings;
    std::vector<GraphicsPipeline::VertexAttribute> vertex_attributes;
    
    GraphicsPipeline::RasterizationState rasterization_state;
    GraphicsPipeline::DepthStencilState depth_stencil_state;
    std::vector<GraphicsPipeline::ColorBlendAttachment> color_blend_attachments;
    
    std::shared_ptr<RenderTarget> render_target;
    
    const char* debug_name = nullptr;
};

} // namespace renderer
} // namespace helios