#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace helios {
namespace renderer {

class Buffer;

/**
 * @brief Memory allocation strategies for GPU resources
 * 
 * This class provides different allocation strategies for GPU memory including
 * pool-based allocation, linear allocation, and buddy allocation. It supports
 * memory defragmentation and provides statistics for memory usage analysis.
 */
class MemoryAllocator {
public:
    /**
     * @brief Allocation strategy types
     */
    enum class Strategy {
        POOL,      // Pool-based allocation for fixed-size objects
        LINEAR,    // Linear allocation for temporary objects
        BUDDY,     // Buddy allocation for variable-size objects
        DEFAULT    // Use the most appropriate strategy
    };
    
    /**
     * @brief Memory allocation info
     */
    struct AllocationInfo {
        uint64_t offset = 0;
        uint64_t size = 0;
        uint64_t alignment = 0;
        void* mapped_ptr = nullptr;
        uint32_t memory_type_index = 0;
    };
    
    /**
     * @brief Memory pool for specific allocation sizes
     */
    class MemoryPool {
    public:
        MemoryPool(uint64_t block_size, uint32_t initial_block_count = 16);
        virtual ~MemoryPool();
        
        /**
         * @brief Allocate a block from the pool
         */
        AllocationInfo Allocate();
        
        /**
         * @brief Free a block back to the pool
         */
        void Deallocate(const AllocationInfo& allocation);
        
        /**
         * @brief Get pool statistics
         */
        struct Statistics {
            uint64_t block_size = 0;
            uint32_t total_blocks = 0;
            uint32_t free_blocks = 0;
            uint32_t used_blocks = 0;
        };
        
        Statistics GetStatistics() const;
        
    private:
        uint64_t block_size_;
        std::vector<AllocationInfo> free_blocks_;
        std::vector<std::unique_ptr<Buffer>> memory_blocks_;
        mutable std::mutex mutex_;
    };
    
    MemoryAllocator();
    virtual ~MemoryAllocator();
    
    /**
     * @brief Allocate memory with the specified strategy
     */
    AllocationInfo Allocate(uint64_t size, uint64_t alignment, uint32_t usage_flags, 
                           Strategy strategy = Strategy::DEFAULT);
    
    /**
     * @brief Free allocated memory
     */
    void Deallocate(const AllocationInfo& allocation);
    
    /**
     * @brief Get a memory pool for the specified block size
     */
    MemoryPool& GetPool(uint64_t block_size);
    
    /**
     * @brief Perform memory defragmentation
     * @return Number of allocations moved during defragmentation
     */
    uint32_t Defragment();
    
    /**
     * @brief Set allocation alignment requirements
     */
    void SetAlignment(uint32_t buffer_alignment, uint32_t texture_alignment) {
        buffer_alignment_ = buffer_alignment;
        texture_alignment_ = texture_alignment;
    }
    
    /**
     * @brief Memory allocator statistics
     */
    struct Statistics {
        uint64_t total_allocated = 0;
        uint64_t total_freed = 0;
        uint64_t current_usage = 0;
        uint32_t allocation_count = 0;
        uint32_t pool_count = 0;
        double fragmentation_ratio = 0.0;
    };
    
    /**
     * @brief Get allocator statistics
     */
    Statistics GetStatistics() const;
    
    /**
     * @brief Enable/disable memory tracking
     */
    void SetTrackingEnabled(bool enabled) { tracking_enabled_ = enabled; }

private:
    // Memory pools for different block sizes
    std::unordered_map<uint64_t, std::unique_ptr<MemoryPool>> memory_pools_;
    mutable std::mutex pools_mutex_;
    
    // Linear allocator for temporary allocations
    class LinearAllocator;
    std::unique_ptr<LinearAllocator> linear_allocator_;
    
    // Buddy allocator for variable-size allocations
    class BuddyAllocator;
    std::unique_ptr<BuddyAllocator> buddy_allocator_;
    
    // Allocation tracking
    mutable std::mutex stats_mutex_;
    mutable Statistics stats_;
    bool tracking_enabled_ = true;
    
    // Alignment requirements
    uint32_t buffer_alignment_ = 256;
    uint32_t texture_alignment_ = 1024;
    
    // Choose the best allocation strategy
    Strategy ChooseStrategy(uint64_t size, uint32_t usage_flags) const;
};

} // namespace renderer
} // namespace helios