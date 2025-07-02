#include "helios/renderer/renderer.h"
#include "helios/renderer/device.h"
#include "helios/renderer/command_context.h"
#include "vulkan/vulkan_device.h"

// Include existing HeliosEngine utilities
#include <HeliosEngine/Utils/ThreadPool.h>
#include <HeliosEngine/Log.h>

#include <stdexcept>

namespace helios {
namespace renderer {

std::unique_ptr<Renderer> Renderer::instance_ = nullptr;

bool Renderer::Initialize(void* window_handle, bool enable_validation, uint32_t num_worker_threads) {
    if (instance_) {
        CORE_WARN("Renderer already initialized");
        return true;
    }
    
    try {
        instance_ = std::unique_ptr<Renderer>(new Renderer());
        
        // Create the Vulkan device with NVRHI backend
        instance_->device_ = Device::Create(window_handle, enable_validation);
        if (!instance_->device_) {
            CORE_ERROR("Failed to create renderer device");
            instance_.reset();
            return false;
        }
        
        CORE_INFO("Renderer initialized successfully with {} worker threads", num_worker_threads);
        return true;
        
    } catch (const std::exception& e) {
        CORE_ERROR("Failed to initialize renderer: {}", e.what());
        instance_.reset();
        return false;
    }
}

void Renderer::Shutdown() {
    if (!instance_) {
        CORE_WARN("Renderer not initialized");
        return;
    }
    
    if (instance_->device_) {
        instance_->device_->WaitIdle();
    }
    
    instance_.reset();
    CORE_INFO("Renderer shutdown completed");
}

Renderer& Renderer::GetInstance() {
    if (!instance_) {
        throw std::runtime_error("Renderer not initialized");
    }
    return *instance_;
}

std::unique_ptr<CommandContext> Renderer::CreateCommandContext(uint32_t thread_id) {
    if (!device_) {
        throw std::runtime_error("Device not initialized");
    }
    
    // This will be implemented by the VulkanDevice to create thread-local command contexts
    // For now, return nullptr to indicate not implemented
    CORE_WARN("CommandContext creation not yet implemented");
    return nullptr;
}

void Renderer::BeginFrame() {
    if (device_) {
        device_->BeginFrame();
    }
}

void Renderer::EndFrame() {
    if (device_) {
        device_->EndFrame();
    }
}

void Renderer::WaitIdle() {
    if (device_) {
        device_->WaitIdle();
    }
}

} // namespace renderer
} // namespace helios