#pragma once

#include <memory>
#include <vector>
#include <cstdint>

// Forward declarations for NVRHI types
namespace nvrhi {
    class ITexture;
    class IFramebuffer;
}

namespace helios {
namespace renderer {

class Texture;

/**
 * @brief Render target wrapper that encapsulates NVRHI framebuffer functionality
 * 
 * This class provides a RAII-based interface for render targets including
 * color attachments, depth-stencil attachments, and multi-target rendering.
 * It ensures proper resource lifetime management and thread-safe access.
 */
class RenderTarget {
public:
    /**
     * @brief Color attachment description
     */
    struct ColorAttachment {
        std::shared_ptr<Texture> texture;
        uint32_t mip_level = 0;
        uint32_t array_layer = 0;
        bool clear = true;
        float clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    };
    
    /**
     * @brief Depth-stencil attachment description
     */
    struct DepthStencilAttachment {
        std::shared_ptr<Texture> texture;
        uint32_t mip_level = 0;
        uint32_t array_layer = 0;
        bool clear_depth = true;
        bool clear_stencil = false;
        float clear_depth_value = 1.0f;
        uint32_t clear_stencil_value = 0;
    };
    
    virtual ~RenderTarget() = default;
    
    /**
     * @brief Get the width of the render target
     */
    virtual uint32_t GetWidth() const = 0;
    
    /**
     * @brief Get the height of the render target
     */
    virtual uint32_t GetHeight() const = 0;
    
    /**
     * @brief Get the number of color attachments
     */
    virtual uint32_t GetColorAttachmentCount() const = 0;
    
    /**
     * @brief Get a color attachment texture
     */
    virtual std::shared_ptr<Texture> GetColorAttachment(uint32_t index) const = 0;
    
    /**
     * @brief Get the depth-stencil attachment texture
     */
    virtual std::shared_ptr<Texture> GetDepthStencilAttachment() const = 0;
    
    /**
     * @brief Check if the render target has a depth-stencil attachment
     */
    virtual bool HasDepthStencil() const = 0;
    
    /**
     * @brief Resize the render target
     */
    virtual void Resize(uint32_t width, uint32_t height) = 0;
    
    /**
     * @brief Get the underlying NVRHI framebuffer (for internal use only)
     */
    virtual nvrhi::IFramebuffer* GetNVRHIFramebuffer() const = 0;

protected:
    RenderTarget() = default;
};

/**
 * @brief Render target creation parameters
 */
struct RenderTargetDesc {
    uint32_t width = 1;
    uint32_t height = 1;
    std::vector<ColorAttachment> color_attachments;
    DepthStencilAttachment depth_stencil_attachment;
    const char* debug_name = nullptr;
};

} // namespace renderer
} // namespace helios