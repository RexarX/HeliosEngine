#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <cstdint>

namespace helios {
namespace renderer {

class Buffer;
class Texture;

/**
 * @brief Resource manager for GPU resource lifetime management
 * 
 * This class provides RAII-based resource management with automatic cleanup,
 * reference counting, and garbage collection. It ensures thread-safe access
 * to resources and prevents resource leaks.
 */
class ResourceManager {
public:
    ResourceManager();
    virtual ~ResourceManager();
    
    /**
     * @brief Resource handle for weak references
     */
    template<typename T>
    class ResourceHandle {
    public:
        ResourceHandle() = default;
        explicit ResourceHandle(std::weak_ptr<T> resource) : resource_(resource) {}
        
        /**
         * @brief Get the resource if it's still valid
         */
        std::shared_ptr<T> Get() const { return resource_.lock(); }
        
        /**
         * @brief Check if the resource is still valid
         */
        bool IsValid() const { return !resource_.expired(); }
        
        /**
         * @brief Release the handle
         */
        void Reset() { resource_.reset(); }
        
    private:
        std::weak_ptr<T> resource_;
    };
    
    /**
     * @brief Register a buffer for management
     */
    ResourceHandle<Buffer> RegisterBuffer(std::shared_ptr<Buffer> buffer);
    
    /**
     * @brief Register a texture for management
     */
    ResourceHandle<Texture> RegisterTexture(std::shared_ptr<Texture> texture);
    
    /**
     * @brief Schedule a resource for deletion
     * @param deleter Function to call when the resource should be deleted
     * @param frames_to_wait Number of frames to wait before deletion
     */
    void ScheduleForDeletion(std::function<void()> deleter, uint32_t frames_to_wait = 2);
    
    /**
     * @brief Process pending deletions (call once per frame)
     */
    void ProcessDeletions();
    
    /**
     * @brief Force immediate cleanup of all resources
     */
    void ForceCleanup();
    
    /**
     * @brief Get resource statistics
     */
    struct Statistics {
        uint32_t buffer_count = 0;
        uint32_t texture_count = 0;
        uint32_t pending_deletions = 0;
        uint64_t total_memory_allocated = 0;
    };
    
    /**
     * @brief Get resource statistics
     */
    Statistics GetStatistics() const;
    
    /**
     * @brief Set memory budget for resource management
     * @param budget_bytes Maximum memory budget in bytes
     */
    void SetMemoryBudget(uint64_t budget_bytes) { memory_budget_ = budget_bytes; }
    
    /**
     * @brief Get current memory usage
     */
    uint64_t GetMemoryUsage() const { return memory_usage_.load(); }
    
    /**
     * @brief Check if memory budget is exceeded
     */
    bool IsMemoryBudgetExceeded() const { 
        return memory_usage_.load() > memory_budget_; 
    }

private:
    struct PendingDeletion {
        std::function<void()> deleter;
        uint32_t frames_remaining;
    };
    
    // Thread-safe containers
    mutable std::mutex buffers_mutex_;
    mutable std::mutex textures_mutex_;
    mutable std::mutex deletions_mutex_;
    
    std::vector<std::weak_ptr<Buffer>> managed_buffers_;
    std::vector<std::weak_ptr<Texture>> managed_textures_;
    std::vector<PendingDeletion> pending_deletions_;
    
    // Memory tracking
    std::atomic<uint64_t> memory_usage_{0};
    uint64_t memory_budget_ = UINT64_MAX;
    
    // Cleanup expired weak references
    void CleanupExpiredReferences();
};

} // namespace renderer
} // namespace helios