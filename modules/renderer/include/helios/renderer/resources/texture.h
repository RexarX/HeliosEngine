#pragma once

#include <memory>
#include <cstdint>

// Forward declarations for NVRHI types
namespace nvrhi {
    class ITexture;
}

namespace helios {
namespace renderer {

/**
 * @brief Texture wrapper that encapsulates NVRHI texture functionality
 * 
 * This class provides a RAII-based interface for GPU textures including
 * 2D textures, 3D textures, cube maps, and texture arrays. It ensures
 * proper resource lifetime management and thread-safe access.
 */
class Texture {
public:
    /**
     * @brief Texture types
     */
    enum Type : uint32_t {
        TEXTURE_2D,
        TEXTURE_3D,
        TEXTURE_CUBE,
        TEXTURE_2D_ARRAY,
        TEXTURE_CUBE_ARRAY
    };
    
    /**
     * @brief Texture usage flags
     */
    enum Usage : uint32_t {
        SHADER_RESOURCE = 1 << 0,
        RENDER_TARGET   = 1 << 1,
        DEPTH_STENCIL   = 1 << 2,
        UNORDERED_ACCESS = 1 << 3,
        TRANSFER_SRC    = 1 << 4,
        TRANSFER_DST    = 1 << 5
    };
    
    /**
     * @brief Common texture formats
     */
    enum Format : uint32_t {
        R8G8B8A8_UNORM,
        R8G8B8A8_SRGB,
        B8G8R8A8_UNORM,
        B8G8R8A8_SRGB,
        R16G16B16A16_FLOAT,
        R32G32B32A32_FLOAT,
        D32_FLOAT,
        D24_UNORM_S8_UINT,
        BC1_UNORM,
        BC3_UNORM,
        BC7_UNORM
    };
    
    virtual ~Texture() = default;
    
    /**
     * @brief Get the texture type
     */
    virtual Type GetType() const = 0;
    
    /**
     * @brief Get the texture format
     */
    virtual Format GetFormat() const = 0;
    
    /**
     * @brief Get texture width
     */
    virtual uint32_t GetWidth() const = 0;
    
    /**
     * @brief Get texture height
     */
    virtual uint32_t GetHeight() const = 0;
    
    /**
     * @brief Get texture depth (for 3D textures)
     */
    virtual uint32_t GetDepth() const = 0;
    
    /**
     * @brief Get number of mip levels
     */
    virtual uint32_t GetMipLevels() const = 0;
    
    /**
     * @brief Get number of array layers
     */
    virtual uint32_t GetArrayLayers() const = 0;
    
    /**
     * @brief Get the usage flags for this texture
     */
    virtual uint32_t GetUsage() const = 0;
    
    /**
     * @brief Update texture data
     * @param data Source data to copy
     * @param size Size of data to copy
     * @param mip_level Target mip level
     * @param array_layer Target array layer
     */
    virtual void UpdateData(const void* data, size_t size, uint32_t mip_level = 0, uint32_t array_layer = 0) = 0;
    
    /**
     * @brief Generate mip maps
     */
    virtual void GenerateMips() = 0;
    
    /**
     * @brief Get the underlying NVRHI texture (for internal use only)
     */
    virtual nvrhi::ITexture* GetNVRHITexture() const = 0;

protected:
    Texture() = default;
};

/**
 * @brief Texture creation parameters
 */
struct TextureDesc {
    Texture::Type type = Texture::TEXTURE_2D;
    Texture::Format format = Texture::R8G8B8A8_UNORM;
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    uint32_t mip_levels = 1;
    uint32_t array_layers = 1;
    uint32_t usage = 0;
    const char* debug_name = nullptr;
};

} // namespace renderer
} // namespace helios