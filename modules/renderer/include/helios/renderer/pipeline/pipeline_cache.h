#pragma once

#include <memory>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace helios {
namespace renderer {

class GraphicsPipeline;
class ComputePipeline;
struct GraphicsPipelineDesc;
struct ComputePipelineDesc;

/**
 * @brief Pipeline caching system for efficient pipeline state management
 * 
 * This class provides thread-safe caching of graphics and compute pipelines
 * to avoid expensive pipeline creation overhead. It manages pipeline lifetime
 * and provides automatic cleanup of unused pipelines.
 */
class PipelineCache {
public:
    PipelineCache();
    virtual ~PipelineCache();
    
    /**
     * @brief Get or create a graphics pipeline
     * @param desc Pipeline description
     * @return Cached or newly created pipeline
     */
    std::shared_ptr<GraphicsPipeline> GetGraphicsPipeline(const GraphicsPipelineDesc& desc);
    
    /**
     * @brief Get or create a compute pipeline
     * @param desc Pipeline description
     * @return Cached or newly created pipeline
     */
    std::shared_ptr<ComputePipeline> GetComputePipeline(const ComputePipelineDesc& desc);
    
    /**
     * @brief Clear all cached pipelines
     */
    void Clear();
    
    /**
     * @brief Get cache statistics
     */
    struct Statistics {
        uint32_t graphics_pipeline_count = 0;
        uint32_t compute_pipeline_count = 0;
        uint32_t cache_hits = 0;
        uint32_t cache_misses = 0;
    };
    
    /**
     * @brief Get cache statistics
     */
    Statistics GetStatistics() const;
    
    /**
     * @brief Enable/disable pipeline serialization for persistent caching
     */
    void SetSerializationEnabled(bool enabled) { serialization_enabled_ = enabled; }
    
    /**
     * @brief Load cached pipelines from disk
     */
    bool LoadFromDisk(const char* cache_file_path);
    
    /**
     * @brief Save cached pipelines to disk
     */
    bool SaveToDisk(const char* cache_file_path) const;

private:
    // Hash function for pipeline descriptions
    uint64_t HashGraphicsPipelineDesc(const GraphicsPipelineDesc& desc) const;
    uint64_t HashComputePipelineDesc(const ComputePipelineDesc& desc) const;
    
    // Thread-safe containers for cached pipelines
    mutable std::mutex graphics_pipelines_mutex_;
    mutable std::mutex compute_pipelines_mutex_;
    
    std::unordered_map<uint64_t, std::weak_ptr<GraphicsPipeline>> graphics_pipelines_;
    std::unordered_map<uint64_t, std::weak_ptr<ComputePipeline>> compute_pipelines_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    mutable Statistics stats_;
    
    bool serialization_enabled_ = true;
};

} // namespace renderer
} // namespace helios