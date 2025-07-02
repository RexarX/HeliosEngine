#include "helios/renderer/pipeline/pipeline_cache.h"
#include "helios/renderer/pipeline/graphics_pipeline.h"
#include "helios/renderer/pipeline/compute_pipeline.h"

// Include HeliosEngine logging
#include <HeliosEngine/Log.h>

#include <fstream>
#include <functional>

namespace helios {
namespace renderer {

PipelineCache::PipelineCache() {
    CORE_INFO("PipelineCache initialized");
}

PipelineCache::~PipelineCache() {
    Clear();
    CORE_INFO("PipelineCache destroyed");
}

std::shared_ptr<GraphicsPipeline> PipelineCache::GetGraphicsPipeline(const GraphicsPipelineDesc& desc) {
    uint64_t hash = HashGraphicsPipelineDesc(desc);
    
    std::lock_guard<std::mutex> lock(graphics_pipelines_mutex_);
    
    auto it = graphics_pipelines_.find(hash);
    if (it != graphics_pipelines_.end()) {
        if (auto pipeline = it->second.lock()) {
            // Cache hit
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            ++stats_.cache_hits;
            return pipeline;
        } else {
            // Expired reference, remove it
            graphics_pipelines_.erase(it);
        }
    }
    
    // Cache miss - create new pipeline
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        ++stats_.cache_misses;
    }
    
    // TODO: Create actual graphics pipeline using device
    // For now, return nullptr to indicate not implemented
    CORE_WARN("Graphics pipeline creation not yet implemented");
    return nullptr;
}

std::shared_ptr<ComputePipeline> PipelineCache::GetComputePipeline(const ComputePipelineDesc& desc) {
    uint64_t hash = HashComputePipelineDesc(desc);
    
    std::lock_guard<std::mutex> lock(compute_pipelines_mutex_);
    
    auto it = compute_pipelines_.find(hash);
    if (it != compute_pipelines_.end()) {
        if (auto pipeline = it->second.lock()) {
            // Cache hit
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            ++stats_.cache_hits;
            return pipeline;
        } else {
            // Expired reference, remove it
            compute_pipelines_.erase(it);
        }
    }
    
    // Cache miss - create new pipeline
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        ++stats_.cache_misses;
    }
    
    // TODO: Create actual compute pipeline using device
    // For now, return nullptr to indicate not implemented
    CORE_WARN("Compute pipeline creation not yet implemented");
    return nullptr;
}

void PipelineCache::Clear() {
    {
        std::lock_guard<std::mutex> lock(graphics_pipelines_mutex_);
        graphics_pipelines_.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(compute_pipelines_mutex_);
        compute_pipelines_.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.graphics_pipeline_count = 0;
        stats_.compute_pipeline_count = 0;
    }
    
    CORE_INFO("Pipeline cache cleared");
}

PipelineCache::Statistics PipelineCache::GetStatistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    Statistics stats = stats_;
    
    // Update current counts
    {
        std::lock_guard<std::mutex> graphics_lock(graphics_pipelines_mutex_);
        stats.graphics_pipeline_count = static_cast<uint32_t>(graphics_pipelines_.size());
    }
    
    {
        std::lock_guard<std::mutex> compute_lock(compute_pipelines_mutex_);
        stats.compute_pipeline_count = static_cast<uint32_t>(compute_pipelines_.size());
    }
    
    return stats;
}

bool PipelineCache::LoadFromDisk(const char* cache_file_path) {
    if (!serialization_enabled_) {
        return false;
    }
    
    std::ifstream file(cache_file_path, std::ios::binary);
    if (!file.is_open()) {
        CORE_WARN("Failed to open pipeline cache file: {}", cache_file_path);
        return false;
    }
    
    // TODO: Implement pipeline cache deserialization
    // This would involve:
    // 1. Reading cache file format version
    // 2. Reading pipeline count
    // 3. For each pipeline, reading:
    //    - Pipeline description hash
    //    - Pipeline binary data
    //    - Creating pipeline from binary data
    
    CORE_INFO("Pipeline cache loading from disk not yet implemented");
    return false;
}

bool PipelineCache::SaveToDisk(const char* cache_file_path) const {
    if (!serialization_enabled_) {
        return false;
    }
    
    std::ofstream file(cache_file_path, std::ios::binary);
    if (!file.is_open()) {
        CORE_ERROR("Failed to create pipeline cache file: {}", cache_file_path);
        return false;
    }
    
    // TODO: Implement pipeline cache serialization
    // This would involve:
    // 1. Writing cache file format version
    // 2. Writing pipeline count
    // 3. For each pipeline, writing:
    //    - Pipeline description hash
    //    - Pipeline binary data
    
    CORE_INFO("Pipeline cache saving to disk not yet implemented");
    return false;
}

uint64_t PipelineCache::HashGraphicsPipelineDesc(const GraphicsPipelineDesc& desc) const {
    std::hash<std::string> hasher;
    std::string hash_string;
    
    // Hash shader stages
    if (desc.vertex_shader) {
        hash_string += "vs:";
        // In a real implementation, we'd hash the shader bytecode or ID
    }
    if (desc.fragment_shader) {
        hash_string += "fs:";
    }
    if (desc.geometry_shader) {
        hash_string += "gs:";
    }
    
    // Hash vertex input layout
    for (const auto& binding : desc.vertex_bindings) {
        hash_string += std::to_string(binding.binding) + ":" + 
                      std::to_string(binding.stride) + ":" +
                      std::to_string(binding.input_rate) + ",";
    }
    
    for (const auto& attribute : desc.vertex_attributes) {
        hash_string += std::to_string(attribute.location) + ":" +
                      std::to_string(attribute.format) + ":" +
                      std::to_string(attribute.offset) + ",";
    }
    
    // TODO: Hash other pipeline state (rasterization, depth-stencil, blend)
    
    return hasher(hash_string);
}

uint64_t PipelineCache::HashComputePipelineDesc(const ComputePipelineDesc& desc) const {
    std::hash<std::string> hasher;
    std::string hash_string;
    
    // Hash compute shader
    if (desc.compute_shader) {
        hash_string += "cs:";
        // In a real implementation, we'd hash the shader bytecode or ID
    }
    
    return hasher(hash_string);
}

} // namespace renderer
} // namespace helios