#pragma once

#include "helios/renderer/renderer.h"
#include "helios/renderer/device.h"
#include "helios/renderer/command_context.h"
#include "helios/renderer/resources/buffer.h"
#include "helios/renderer/resources/texture.h"
#include "helios/renderer/resources/shader.h"
#include "helios/renderer/pipeline/graphics_pipeline.h"

#include <HeliosEngine/Core.h>
#include <HeliosEngine/Log.h>

#include <memory>
#include <future>

namespace Helios {

/**
 * @brief Integration layer between HeliosEngine and the new renderer module
 * 
 * This class demonstrates how the new multi-threaded renderer module
 * integrates with the existing HeliosEngine architecture while maintaining
 * clean separation of concerns.
 */
class RendererIntegration {
public:
    /**
     * @brief Initialize the integration layer
     */
    static bool Initialize(void* window_handle, bool enable_validation = false);
    
    /**
     * @brief Shutdown the integration layer
     */
    static void Shutdown();
    
    /**
     * @brief Get the integration instance
     */
    static RendererIntegration& Get();
    
    /**
     * @brief Example: Create a triangle rendering setup
     * Demonstrates multi-threaded resource creation and pipeline setup
     */
    void CreateTriangleExample();
    
    /**
     * @brief Example: Render the triangle using multi-threaded command recording
     */
    void RenderTriangleExample();
    
    /**
     * @brief Example: Async resource loading
     * Demonstrates how to load resources asynchronously using the renderer's task system
     */
    std::future<bool> LoadTextureAsync(const char* file_path);
    
    /**
     * @brief Begin frame rendering
     */
    void BeginFrame();
    
    /**
     * @brief End frame rendering
     */
    void EndFrame();
    
    /**
     * @brief Get the underlying renderer
     */
    helios::renderer::Renderer& GetRenderer() { return helios::renderer::Renderer::GetInstance(); }

private:
    RendererIntegration() = default;
    
    static std::unique_ptr<RendererIntegration> instance_;
    
    // Example resources for demonstration
    std::shared_ptr<helios::renderer::Buffer> triangle_vertex_buffer_;
    std::shared_ptr<helios::renderer::Shader> triangle_vertex_shader_;
    std::shared_ptr<helios::renderer::Shader> triangle_fragment_shader_;
    std::shared_ptr<helios::renderer::GraphicsPipeline> triangle_pipeline_;
    
    // Example command contexts for different threads
    std::unique_ptr<helios::renderer::CommandContext> main_command_context_;
    std::unique_ptr<helios::renderer::CommandContext> async_command_context_;
};

} // namespace Helios