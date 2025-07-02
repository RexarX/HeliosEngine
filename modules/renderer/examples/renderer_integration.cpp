#include "renderer_integration.h"

namespace Helios {

std::unique_ptr<RendererIntegration> RendererIntegration::instance_ = nullptr;

bool RendererIntegration::Initialize(void* window_handle, bool enable_validation) {
    if (instance_) {
        CORE_WARN("RendererIntegration already initialized");
        return true;
    }
    
    // Initialize the new renderer module
    if (!helios::renderer::Renderer::Initialize(window_handle, enable_validation, 4)) {
        CORE_ERROR("Failed to initialize renderer module");
        return false;
    }
    
    instance_ = std::unique_ptr<RendererIntegration>(new RendererIntegration());
    
    CORE_INFO("RendererIntegration initialized successfully");
    return true;
}

void RendererIntegration::Shutdown() {
    if (!instance_) {
        return;
    }
    
    // Clean up resources
    instance_->triangle_vertex_buffer_.reset();
    instance_->triangle_vertex_shader_.reset();
    instance_->triangle_fragment_shader_.reset();
    instance_->triangle_pipeline_.reset();
    instance_->main_command_context_.reset();
    instance_->async_command_context_.reset();
    
    // Shutdown the renderer module
    helios::renderer::Renderer::Shutdown();
    
    instance_.reset();
    CORE_INFO("RendererIntegration shutdown completed");
}

RendererIntegration& RendererIntegration::Get() {
    if (!instance_) {
        throw std::runtime_error("RendererIntegration not initialized");
    }
    return *instance_;
}

void RendererIntegration::CreateTriangleExample() {
    CORE_INFO("Creating triangle example resources...");
    
    auto& renderer = GetRenderer();
    auto& device = renderer.GetDevice();
    
    // Create vertex buffer for triangle
    // This demonstrates the clean API for resource creation
    const float triangle_vertices[] = {
        // Position      // Color
         0.0f,  0.5f,    1.0f, 0.0f, 0.0f,  // Top vertex (red)
        -0.5f, -0.5f,    0.0f, 1.0f, 0.0f,  // Bottom-left (green)
         0.5f, -0.5f,    0.0f, 0.0f, 1.0f   // Bottom-right (blue)
    };
    
    triangle_vertex_buffer_ = device.CreateBuffer(
        sizeof(triangle_vertices),
        static_cast<uint32_t>(helios::renderer::Buffer::Usage::VERTEX_BUFFER),
        true // host_visible for easy updates
    );
    
    if (triangle_vertex_buffer_) {
        triangle_vertex_buffer_->UpdateData(triangle_vertices, sizeof(triangle_vertices));
        CORE_INFO("Triangle vertex buffer created successfully");
    } else {
        CORE_ERROR("Failed to create triangle vertex buffer");
    }
    
    // Example SPIR-V shader creation (in practice, these would be loaded from files)
    // For now, we'll just demonstrate the API structure
    std::vector<uint8_t> vertex_spirv = {}; // Would contain actual SPIR-V bytecode
    std::vector<uint8_t> fragment_spirv = {}; // Would contain actual SPIR-V bytecode
    
    triangle_vertex_shader_ = device.CreateShader(
        vertex_spirv, 
        static_cast<uint32_t>(helios::renderer::Shader::Stage::VERTEX)
    );
    
    triangle_fragment_shader_ = device.CreateShader(
        fragment_spirv,
        static_cast<uint32_t>(helios::renderer::Shader::Stage::FRAGMENT)
    );
    
    // Create graphics pipeline
    triangle_pipeline_ = device.CreateGraphicsPipeline();
    
    // Create command contexts for different threads
    main_command_context_ = renderer.CreateCommandContext(0); // Main thread
    async_command_context_ = renderer.CreateCommandContext(1); // Worker thread
    
    CORE_INFO("Triangle example resources created");
}

void RendererIntegration::RenderTriangleExample() {
    if (!triangle_vertex_buffer_ || !main_command_context_) {
        CORE_WARN("Triangle example not properly initialized");
        return;
    }
    
    // This demonstrates multi-threaded command recording
    // Main thread records primary commands
    main_command_context_->Begin();
    main_command_context_->BeginRenderPass();
    
    if (triangle_pipeline_) {
        main_command_context_->SetGraphicsPipeline(*triangle_pipeline_);
    }
    
    main_command_context_->BindVertexBuffer(*triangle_vertex_buffer_);
    main_command_context_->Draw(3); // Draw 3 vertices for triangle
    
    main_command_context_->EndRenderPass();
    main_command_context_->End();
    main_command_context_->Submit();
}

std::future<bool> RendererIntegration::LoadTextureAsync(const char* file_path) {
    // Demonstrate async task submission using the renderer's task system
    auto& renderer = GetRenderer();
    
    return renderer.SubmitTask([this, file_path]() -> bool {
        CORE_INFO("Loading texture asynchronously: {}", file_path);
        
        // In a real implementation, this would:
        // 1. Load image data from file
        // 2. Create staging buffer
        // 3. Create texture
        // 4. Upload data via command context
        // 5. Generate mipmaps if needed
        
        auto& device = renderer.GetDevice();
        
        // Example texture creation
        auto texture = device.CreateTexture2D(
            512, 512, // width, height
            static_cast<uint32_t>(helios::renderer::Texture::Format::R8G8B8A8_UNORM),
            static_cast<uint32_t>(helios::renderer::Texture::Usage::SHADER_RESOURCE)
        );
        
        if (texture) {
            CORE_INFO("Texture loaded successfully: {}", file_path);
            return true;
        } else {
            CORE_ERROR("Failed to load texture: {}", file_path);
            return false;
        }
    });
}

void RendererIntegration::BeginFrame() {
    GetRenderer().BeginFrame();
}

void RendererIntegration::EndFrame() {
    GetRenderer().EndFrame();
}

} // namespace Helios