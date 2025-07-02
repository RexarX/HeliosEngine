#include "vulkan_utils.h"
#include <HeliosEngine/Log.h>
#include <GLFW/glfw3.h>

namespace helios {
namespace renderer {
namespace vulkan_utils {

VkFormat RendererFormatToVulkan(uint32_t renderer_format) {
    // Map renderer formats to Vulkan formats
    // This is a simplified mapping - real implementation would be more comprehensive
    switch (renderer_format) {
        case 0: return VK_FORMAT_R8G8B8A8_UNORM;    // R8G8B8A8_UNORM
        case 1: return VK_FORMAT_R8G8B8A8_SRGB;     // R8G8B8A8_SRGB
        case 2: return VK_FORMAT_B8G8R8A8_UNORM;    // B8G8R8A8_UNORM
        case 3: return VK_FORMAT_B8G8R8A8_SRGB;     // B8G8R8A8_SRGB
        case 4: return VK_FORMAT_R16G16B16A16_SFLOAT; // R16G16B16A16_FLOAT
        case 5: return VK_FORMAT_R32G32B32A32_SFLOAT; // R32G32B32A32_FLOAT
        case 6: return VK_FORMAT_D32_SFLOAT;        // D32_FLOAT
        case 7: return VK_FORMAT_D24_UNORM_S8_UINT; // D24_UNORM_S8_UINT
        default:
            CORE_WARN("Unknown renderer format: {}, defaulting to R8G8B8A8_UNORM", renderer_format);
            return VK_FORMAT_R8G8B8A8_UNORM;
    }
}

uint32_t VulkanFormatToRenderer(VkFormat vulkan_format) {
    // Map Vulkan formats back to renderer formats
    switch (vulkan_format) {
        case VK_FORMAT_R8G8B8A8_UNORM: return 0;
        case VK_FORMAT_R8G8B8A8_SRGB: return 1;
        case VK_FORMAT_B8G8R8A8_UNORM: return 2;
        case VK_FORMAT_B8G8R8A8_SRGB: return 3;
        case VK_FORMAT_R16G16B16A16_SFLOAT: return 4;
        case VK_FORMAT_R32G32B32A32_SFLOAT: return 5;
        case VK_FORMAT_D32_SFLOAT: return 6;
        case VK_FORMAT_D24_UNORM_S8_UINT: return 7;
        default:
            CORE_WARN("Unknown Vulkan format: {}, defaulting to 0", static_cast<uint32_t>(vulkan_format));
            return 0;
    }
}

VkBufferUsageFlags RendererUsageToVulkanBuffer(uint32_t renderer_usage) {
    VkBufferUsageFlags vulkan_usage = 0;
    
    if (renderer_usage & (1 << 0)) vulkan_usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (renderer_usage & (1 << 1)) vulkan_usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (renderer_usage & (1 << 2)) vulkan_usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (renderer_usage & (1 << 3)) vulkan_usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (renderer_usage & (1 << 4)) vulkan_usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (renderer_usage & (1 << 5)) vulkan_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    return vulkan_usage;
}

VkImageUsageFlags RendererUsageToVulkanImage(uint32_t renderer_usage) {
    VkImageUsageFlags vulkan_usage = 0;
    
    if (renderer_usage & (1 << 0)) vulkan_usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (renderer_usage & (1 << 1)) vulkan_usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (renderer_usage & (1 << 2)) vulkan_usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (renderer_usage & (1 << 3)) vulkan_usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    if (renderer_usage & (1 << 4)) vulkan_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (renderer_usage & (1 << 5)) vulkan_usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    
    return vulkan_usage;
}

VkShaderStageFlagBits RendererShaderStageToVulkan(uint32_t renderer_stage) {
    switch (renderer_stage) {
        case (1 << 0): return VK_SHADER_STAGE_VERTEX_BIT;
        case (1 << 1): return VK_SHADER_STAGE_FRAGMENT_BIT;
        case (1 << 2): return VK_SHADER_STAGE_COMPUTE_BIT;
        case (1 << 3): return VK_SHADER_STAGE_GEOMETRY_BIT;
        case (1 << 4): return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case (1 << 5): return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        default:
            CORE_WARN("Unknown renderer shader stage: {}, defaulting to vertex", renderer_stage);
            return VK_SHADER_STAGE_VERTEX_BIT;
    }
}

uint32_t GetFormatSize(VkFormat format) {
    switch (format) {
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SINT:
            return 1;
            
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R16_SFLOAT:
        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_R16_UINT:
            return 2;
            
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_R32_SFLOAT:
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return 4;
            
        case VK_FORMAT_R16G16B16A16_SFLOAT:
        case VK_FORMAT_R16G16B16A16_UNORM:
            return 8;
            
        case VK_FORMAT_R32G32B32A32_SFLOAT:
        case VK_FORMAT_R32G32B32A32_UINT:
            return 16;
            
        default:
            CORE_WARN("Unknown format size for format: {}", static_cast<uint32_t>(format));
            return 4; // Default to 4 bytes
    }
}

bool IsDepthFormat(VkFormat format) {
    switch (format) {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return true;
        default:
            return false;
    }
}

bool IsStencilFormat(VkFormat format) {
    switch (format) {
        case VK_FORMAT_S8_UINT:
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return true;
        default:
            return false;
    }
}

const char* VulkanObjectTypeToString(VkObjectType object_type) {
    switch (object_type) {
        case VK_OBJECT_TYPE_BUFFER: return "Buffer";
        case VK_OBJECT_TYPE_IMAGE: return "Image";
        case VK_OBJECT_TYPE_PIPELINE: return "Pipeline";
        case VK_OBJECT_TYPE_SHADER_MODULE: return "ShaderModule";
        case VK_OBJECT_TYPE_RENDER_PASS: return "RenderPass";
        case VK_OBJECT_TYPE_COMMAND_BUFFER: return "CommandBuffer";
        case VK_OBJECT_TYPE_DEVICE_MEMORY: return "DeviceMemory";
        default: return "Unknown";
    }
}

void SetDebugName(VkDevice device, uint64_t object_handle, VkObjectType object_type, const char* name) {
    // This would use VK_EXT_debug_utils to set debug names
    // For now, just log the operation
    CORE_INFO("Setting debug name for {} ({}): {}", 
              VulkanObjectTypeToString(object_type), object_handle, name ? name : "unnamed");
}

const char* VulkanResult::ToString() const {
    switch (result_) {
        case VK_SUCCESS: return "VK_SUCCESS";
        case VK_NOT_READY: return "VK_NOT_READY";
        case VK_TIMEOUT: return "VK_TIMEOUT";
        case VK_EVENT_SET: return "VK_EVENT_SET";
        case VK_EVENT_RESET: return "VK_EVENT_RESET";
        case VK_INCOMPLETE: return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
        default: return "Unknown VkResult";
    }
}

namespace extensions {

bool IsInstanceExtensionAvailable(const char* extension_name) {
    // This would check available instance extensions
    // For now, just return true for common extensions
    return true;
}

bool IsDeviceExtensionAvailable(VkPhysicalDevice physical_device, const char* extension_name) {
    // This would check available device extensions
    // For now, just return true for common extensions
    return true;
}

bool IsValidationLayerAvailable(const char* layer_name) {
    // This would check available validation layers
    // For now, just return true for common layers
    return true;
}

std::vector<const char*> GetRequiredInstanceExtensions() {
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    
    std::vector<const char*> extensions;
    for (uint32_t i = 0; i < glfw_extension_count; ++i) {
        extensions.push_back(glfw_extensions[i]);
    }
    
    return extensions;
}

std::vector<const char*> GetRecommendedDeviceExtensions() {
    return {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_MAINTENANCE1_EXTENSION_NAME,
        VK_KHR_MULTIVIEW_EXTENSION_NAME
    };
}

} // namespace extensions

} // namespace vulkan_utils
} // namespace renderer
} // namespace helios