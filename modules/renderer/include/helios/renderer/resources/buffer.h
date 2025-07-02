#pragma once

#include <memory>
#include <cstdint>

// Forward declarations for NVRHI types
namespace nvrhi {
    class IBuffer;
}

namespace helios {
namespace renderer {

/**
 * @brief Buffer wrapper that encapsulates NVRHI buffer functionality
 * 
 * This class provides a RAII-based interface for GPU buffers including
 * vertex buffers, index buffers, uniform buffers, and storage buffers.
 * It ensures proper resource lifetime management and thread-safe access.
 */
class Buffer {
public:
    /**
     * @brief Buffer usage flags
     */
    enum Usage : uint32_t {
        VERTEX_BUFFER   = 1 << 0,
        INDEX_BUFFER    = 1 << 1,
        UNIFORM_BUFFER  = 1 << 2,
        STORAGE_BUFFER  = 1 << 3,
        TRANSFER_SRC    = 1 << 4,
        TRANSFER_DST    = 1 << 5
    };
    
    virtual ~Buffer() = default;
    
    /**
     * @brief Get the size of the buffer in bytes
     */
    virtual size_t GetSize() const = 0;
    
    /**
     * @brief Get the usage flags for this buffer
     */
    virtual uint32_t GetUsage() const = 0;
    
    /**
     * @brief Check if the buffer is host-visible (CPU accessible)
     */
    virtual bool IsHostVisible() const = 0;
    
    /**
     * @brief Map the buffer memory for CPU access
     * @return Pointer to mapped memory, nullptr on failure
     */
    virtual void* Map() = 0;
    
    /**
     * @brief Unmap the buffer memory
     */
    virtual void Unmap() = 0;
    
    /**
     * @brief Update buffer data (for host-visible buffers)
     * @param data Source data to copy
     * @param size Size of data to copy
     * @param offset Offset in the buffer to start copying to
     */
    virtual void UpdateData(const void* data, size_t size, size_t offset = 0) = 0;
    
    /**
     * @brief Get the underlying NVRHI buffer (for internal use only)
     */
    virtual nvrhi::IBuffer* GetNVRHIBuffer() const = 0;

protected:
    Buffer() = default;
};

/**
 * @brief Buffer creation parameters
 */
struct BufferDesc {
    size_t size = 0;
    uint32_t usage = 0;
    bool host_visible = false;
    const char* debug_name = nullptr;
};

} // namespace renderer
} // namespace helios