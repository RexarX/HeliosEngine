#pragma once

#include <memory>
#include <vector>
#include <cstdint>

// Forward declarations for NVRHI types
namespace nvrhi {
    class IDevice;
    class ICommandList;
}

namespace helios {
namespace renderer {

class Buffer;
class Texture;
class Shader;
class GraphicsPipeline;
class ComputePipeline;

/**
 * @brief Device abstraction that manages GPU resources and operations
 * 
 * This class wraps NVRHI device functionality and provides a clean interface
 * for creating and managing GPU resources. It ensures thread-safe access to
 * device operations and hides NVRHI implementation details.
 */
class Device {
public:
    /**
     * @brief Create a device instance
     * @param window_handle Native window handle
     * @param enable_validation Enable validation layers
     */
    static std::unique_ptr<Device> Create(void* window_handle, bool enable_validation = false);
    
    virtual ~Device() = default;
    
    /**
     * @brief Get the underlying NVRHI device (for internal use only)
     */
    virtual nvrhi::IDevice* GetNVRHIDevice() const = 0;
    
    /**
     * @brief Create a buffer with the specified parameters
     */
    virtual std::unique_ptr<Buffer> CreateBuffer(size_t size, uint32_t usage_flags, bool host_visible = false) = 0;
    
    /**
     * @brief Create a 2D texture
     */
    virtual std::unique_ptr<Texture> CreateTexture2D(uint32_t width, uint32_t height, 
                                                     uint32_t format, uint32_t usage_flags) = 0;
    
    /**
     * @brief Create a shader from SPIR-V bytecode
     */
    virtual std::unique_ptr<Shader> CreateShader(const std::vector<uint8_t>& spirv_code, 
                                                 uint32_t shader_stage) = 0;
    
    /**
     * @brief Create a graphics pipeline
     */
    virtual std::unique_ptr<GraphicsPipeline> CreateGraphicsPipeline() = 0;
    
    /**
     * @brief Create a compute pipeline
     */
    virtual std::unique_ptr<ComputePipeline> CreateComputePipeline() = 0;
    
    /**
     * @brief Wait for all GPU operations to complete
     */
    virtual void WaitIdle() = 0;
    
    /**
     * @brief Begin a new frame
     */
    virtual void BeginFrame() = 0;
    
    /**
     * @brief End the current frame
     */
    virtual void EndFrame() = 0;

protected:
    Device() = default;
};

} // namespace renderer
} // namespace helios