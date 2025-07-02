# HeliosEngine Renderer Module

A high-performance, multi-threaded Vulkan renderer module with NVRHI backend integration for HeliosEngine.

## Overview

This renderer module provides a clean, thread-safe C++ API that abstracts Vulkan rendering operations through NVRHI. It's designed for high-performance real-time rendering with support for multi-threaded command recording and ECS-style async task submission.

## Architecture

### Core Components

- **Renderer**: Main singleton interface for renderer management
- **Device**: GPU device abstraction with resource creation
- **CommandContext**: Thread-safe command recording interface
- **Resource Wrappers**: RAII-based GPU resource management
  - Buffer (vertex, index, uniform, storage)
  - Texture (2D, 3D, cube, arrays)
  - Shader (vertex, fragment, compute, etc.)
  - RenderTarget (framebuffer abstraction)
- **Pipeline Management**: Graphics and compute pipeline state objects
- **Memory Management**: Resource lifetime and allocation management

### Threading Model

- **Thread-Safe Design**: All core interfaces support concurrent access
- **Per-Thread Command Contexts**: Separate command recording per thread
- **Lock-Free Resource Management**: Optimized for multi-threaded scenarios
- **Async Task Submission**: Integration with HeliosEngine's ThreadPool

## Usage Examples

### Basic Initialization

```cpp
#include "helios/renderer/renderer.h"

// Initialize renderer with window handle
bool success = helios::renderer::Renderer::Initialize(
    window_handle,     // GLFW window handle
    true,             // Enable validation layers
    4                 // Number of worker threads
);

// Get renderer instance
auto& renderer = helios::renderer::Renderer::GetInstance();
auto& device = renderer.GetDevice();
```

### Resource Creation

```cpp
// Create a vertex buffer
auto vertex_buffer = device.CreateBuffer(
    sizeof(vertices),
    helios::renderer::Buffer::Usage::VERTEX_BUFFER,
    true  // host_visible for CPU updates
);

// Update buffer data
vertex_buffer->UpdateData(vertices, sizeof(vertices));

// Create a texture
auto texture = device.CreateTexture2D(
    512, 512,  // width, height
    helios::renderer::Texture::Format::R8G8B8A8_UNORM,
    helios::renderer::Texture::Usage::SHADER_RESOURCE
);
```

### Multi-threaded Command Recording

```cpp
// Create command context for current thread
auto command_context = renderer.CreateCommandContext(thread_id);

// Record commands
command_context->Begin();
command_context->BeginRenderPass();
command_context->SetGraphicsPipeline(*pipeline);
command_context->BindVertexBuffer(*vertex_buffer);
command_context->DrawIndexed(index_count);
command_context->EndRenderPass();
command_context->End();
command_context->Submit();
```

### Async Task Submission

```cpp
// Submit work to be executed asynchronously
auto future = renderer.SubmitTask([&]() {
    // Load texture data in background
    auto texture = LoadTextureFromFile("texture.png");
    return texture;
});

// Wait for completion
auto result = future.get();
```

## Build Configuration

### CMake Options

- `HELIOS_RENDERER_BUILD_EXAMPLES`: Build example code (default: ON)
- `HELIOS_RENDERER_BUILD_TESTS`: Build unit tests (default: ON)

### Dependencies

- **NVRHI**: Vulkan rendering backend (integration in progress)
- **GLFW**: Window management
- **GLM**: Mathematics library
- **Vulkan**: Graphics API
- **HeliosEngine Core**: Logging and threading utilities

## Testing

Run the test suite to verify thread safety and basic functionality:

```bash
# Build tests
cmake -DHELIOS_RENDERER_BUILD_TESTS=ON ..
make HeliosRendererTests

# Run tests
./tests/HeliosRendererTests
```

### Test Coverage

- ✅ Resource manager thread safety
- ✅ Pipeline cache thread safety  
- ✅ Renderer singleton behavior
- 🔄 Multi-threaded command recording (pending NVRHI integration)
- 🔄 Memory allocation stress tests (pending)
- 🔄 Performance benchmarks (pending)

## Integration Status

### Completed ✅

- [x] Core interface design and implementation
- [x] Thread-safe resource management system
- [x] Pipeline state caching infrastructure
- [x] Memory management and allocation strategies
- [x] RAII-based resource lifetime management
- [x] Integration points with existing HeliosEngine utilities
- [x] Comprehensive error handling framework
- [x] Build system and project structure

### In Progress 🔄

- [ ] NVRHI dependency integration
- [ ] Complete Vulkan implementation classes
- [ ] Shader reflection using SPIRV-Reflect
- [ ] Multi-threaded command buffer allocation
- [ ] GPU memory budget management
- [ ] Pipeline hot-reloading capabilities

### Planned 📋

- [ ] Descriptor set pooling and caching
- [ ] Draw call batching and instancing
- [ ] GPU-driven rendering preparation
- [ ] Compute shader integration
- [ ] Performance profiling and metrics
- [ ] Documentation and examples expansion

## API Design Principles

1. **Hide Implementation Details**: NVRHI types are never exposed in public interfaces
2. **Thread Safety**: All public APIs are thread-safe or clearly documented as thread-local
3. **RAII Resource Management**: Automatic cleanup and lifetime management
4. **Performance First**: Optimized for high-frequency operations
5. **Clean Separation**: Clear boundaries between user API and implementation
6. **Google C++ Style**: Consistent with HeliosEngine coding standards

## Error Handling

The renderer module uses a comprehensive error handling strategy:

- **Exceptions**: For critical initialization and resource creation failures
- **Return Values**: For optional operations that can gracefully fail
- **Logging**: Detailed error reporting through HeliosEngine's logging system
- **Validation**: Debug-mode validation with detailed error messages

## Memory Management

- **RAII**: Automatic resource cleanup using smart pointers
- **Reference Counting**: Shared ownership for GPU resources
- **Garbage Collection**: Deferred deletion for GPU synchronization
- **Budget Tracking**: Memory usage monitoring and limits
- **Defragmentation**: Automatic memory compaction when needed

## Contributing

When contributing to the renderer module:

1. Follow Google C++ Style Guide
2. Maintain thread safety guarantees
3. Add comprehensive error handling
4. Include unit tests for new functionality
5. Update documentation and examples
6. Ensure NVRHI integration points are preserved