#pragma once

#include "helios/renderer/device.h"
#include <memory>
#include <vector>
#include <mutex>

// Forward declarations for NVRHI types
namespace nvrhi {
    class IDevice;
    class DeviceHandle;
    class ICommandList;
}

struct GLFWwindow;

namespace helios {
namespace renderer {

/**
 * @brief Vulkan implementation of the Device interface using NVRHI
 * 
 * This class implements the Device interface using NVRHI's Vulkan backend.
 * It provides thread-safe access to GPU resources and manages the Vulkan
 * device, queues, and command pools for multi-threaded rendering.
 */
class VulkanDevice final : public Device {
public:
    /**
     * @brief Create a Vulkan device instance
     */
    static std::unique_ptr<VulkanDevice> Create(void* window_handle, bool enable_validation = false);
    
    ~VulkanDevice() override;
    
    // Device interface implementation
    nvrhi::IDevice* GetNVRHIDevice() const override { return nvrhi_device_.get(); }
    
    std::unique_ptr<Buffer> CreateBuffer(size_t size, uint32_t usage_flags, bool host_visible = false) override;
    std::unique_ptr<Texture> CreateTexture2D(uint32_t width, uint32_t height, 
                                             uint32_t format, uint32_t usage_flags) override;
    std::unique_ptr<Shader> CreateShader(const std::vector<uint8_t>& spirv_code, 
                                        uint32_t shader_stage) override;
    std::unique_ptr<GraphicsPipeline> CreateGraphicsPipeline() override;
    std::unique_ptr<ComputePipeline> CreateComputePipeline() override;
    
    void WaitIdle() override;
    void BeginFrame() override;
    void EndFrame() override;
    
    /**
     * @brief Create a command context for the specified thread
     */
    std::unique_ptr<class VulkanCommandContext> CreateCommandContext(uint32_t thread_id);
    
    /**
     * @brief Get the GLFW window handle
     */
    GLFWwindow* GetWindowHandle() const { return window_handle_; }
    
    /**
     * @brief Get the current frame index
     */
    uint32_t GetCurrentFrameIndex() const { return current_frame_index_; }

private:
    VulkanDevice() = default;
    
    bool Initialize(void* window_handle, bool enable_validation);
    void Shutdown();
    
    // NVRHI device and related objects
    nvrhi::DeviceHandle nvrhi_device_;
    
    // Window management
    GLFWwindow* window_handle_ = nullptr;
    
    // Frame management
    uint32_t current_frame_index_ = 0;
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    
    // Thread safety
    mutable std::mutex device_mutex_;
    
    // Command context tracking
    std::vector<std::unique_ptr<class VulkanCommandContext>> command_contexts_;
    mutable std::mutex contexts_mutex_;
};

} // namespace renderer
} // namespace helios