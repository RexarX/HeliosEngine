#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace helios {
namespace renderer {
namespace vulkan_utils {

/**
 * @brief Vulkan utility functions for NVRHI integration
 * 
 * This header provides utility functions for common Vulkan operations
 * that complement the NVRHI backend integration.
 */

/**
 * @brief Convert renderer format to Vulkan format
 */
VkFormat RendererFormatToVulkan(uint32_t renderer_format);

/**
 * @brief Convert Vulkan format to renderer format
 */
uint32_t VulkanFormatToRenderer(VkFormat vulkan_format);

/**
 * @brief Convert renderer usage flags to Vulkan buffer usage flags
 */
VkBufferUsageFlags RendererUsageToVulkanBuffer(uint32_t renderer_usage);

/**
 * @brief Convert renderer usage flags to Vulkan image usage flags
 */
VkImageUsageFlags RendererUsageToVulkanImage(uint32_t renderer_usage);

/**
 * @brief Convert renderer shader stage to Vulkan shader stage
 */
VkShaderStageFlagBits RendererShaderStageToVulkan(uint32_t renderer_stage);

/**
 * @brief Get Vulkan memory type index for allocation
 */
uint32_t FindMemoryType(VkPhysicalDevice physical_device, uint32_t type_filter, 
                       VkMemoryPropertyFlags properties);

/**
 * @brief Check if a Vulkan format supports specific features
 */
bool FormatSupportsFeatures(VkPhysicalDevice physical_device, VkFormat format, 
                           VkImageTiling tiling, VkFormatFeatureFlags features);

/**
 * @brief Get the size in bytes for a Vulkan format
 */
uint32_t GetFormatSize(VkFormat format);

/**
 * @brief Check if a format is depth format
 */
bool IsDepthFormat(VkFormat format);

/**
 * @brief Check if a format is stencil format
 */
bool IsStencilFormat(VkFormat format);

/**
 * @brief Get debug name for Vulkan object type
 */
const char* VulkanObjectTypeToString(VkObjectType object_type);

/**
 * @brief Set debug name for Vulkan object (when validation layers are enabled)
 */
void SetDebugName(VkDevice device, uint64_t object_handle, VkObjectType object_type, 
                 const char* name);

/**
 * @brief Vulkan error checking utility
 */
class VulkanResult {
public:
    VulkanResult(VkResult result) : result_(result) {}
    
    operator bool() const { return result_ == VK_SUCCESS; }
    operator VkResult() const { return result_; }
    
    const char* ToString() const;
    
    VulkanResult& operator=(VkResult result) {
        result_ = result;
        return *this;
    }

private:
    VkResult result_;
};

/**
 * @brief Vulkan extension and layer utilities
 */
namespace extensions {
    /**
     * @brief Check if an instance extension is available
     */
    bool IsInstanceExtensionAvailable(const char* extension_name);
    
    /**
     * @brief Check if a device extension is available
     */
    bool IsDeviceExtensionAvailable(VkPhysicalDevice physical_device, const char* extension_name);
    
    /**
     * @brief Check if a validation layer is available
     */
    bool IsValidationLayerAvailable(const char* layer_name);
    
    /**
     * @brief Get required instance extensions for GLFW
     */
    std::vector<const char*> GetRequiredInstanceExtensions();
    
    /**
     * @brief Get recommended device extensions for rendering
     */
    std::vector<const char*> GetRecommendedDeviceExtensions();
}

/**
 * @brief NVRHI integration utilities
 */
namespace nvrhi_integration {
    /**
     * @brief Create NVRHI device from existing Vulkan objects
     * This would be the main integration point with NVRHI
     */
    // nvrhi::DeviceHandle CreateNVRHIDevice(VkInstance instance, VkPhysicalDevice physical_device, 
    //                                       VkDevice device, VkQueue graphics_queue);
    
    /**
     * @brief Convert NVRHI format to Vulkan format
     */
    // VkFormat NVRHIFormatToVulkan(nvrhi::Format format);
    
    /**
     * @brief Convert Vulkan format to NVRHI format
     */
    // nvrhi::Format VulkanFormatToNVRHI(VkFormat format);
}

} // namespace vulkan_utils
} // namespace renderer
} // namespace helios