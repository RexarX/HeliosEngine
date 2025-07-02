#include "helios/renderer/memory/resource_manager.h"
#include "helios/renderer/resources/buffer.h"
#include "helios/renderer/resources/texture.h"

// Include HeliosEngine logging
#include <HeliosEngine/Log.h>

#include <algorithm>

namespace helios {
namespace renderer {

ResourceManager::ResourceManager() {
    CORE_INFO("ResourceManager initialized");
}

ResourceManager::~ResourceManager() {
    ForceCleanup();
    CORE_INFO("ResourceManager destroyed");
}

ResourceManager::ResourceHandle<Buffer> ResourceManager::RegisterBuffer(std::shared_ptr<Buffer> buffer) {
    if (!buffer) {
        return ResourceHandle<Buffer>();
    }
    
    std::lock_guard<std::mutex> lock(buffers_mutex_);
    managed_buffers_.emplace_back(buffer);
    
    // Update memory usage
    memory_usage_.fetch_add(buffer->GetSize());
    
    return ResourceHandle<Buffer>(buffer);
}

ResourceManager::ResourceHandle<Texture> ResourceManager::RegisterTexture(std::shared_ptr<Texture> texture) {
    if (!texture) {
        return ResourceHandle<Texture>();
    }
    
    std::lock_guard<std::mutex> lock(textures_mutex_);
    managed_textures_.emplace_back(texture);
    
    // Estimate texture memory usage (simplified calculation)
    uint64_t estimated_size = static_cast<uint64_t>(texture->GetWidth()) * 
                             texture->GetHeight() * 
                             texture->GetDepth() * 
                             4; // Assume 4 bytes per pixel
    memory_usage_.fetch_add(estimated_size);
    
    return ResourceHandle<Texture>(texture);
}

void ResourceManager::ScheduleForDeletion(std::function<void()> deleter, uint32_t frames_to_wait) {
    std::lock_guard<std::mutex> lock(deletions_mutex_);
    pending_deletions_.push_back({std::move(deleter), frames_to_wait});
}

void ResourceManager::ProcessDeletions() {
    std::lock_guard<std::mutex> lock(deletions_mutex_);
    
    auto it = pending_deletions_.begin();
    while (it != pending_deletions_.end()) {
        if (it->frames_remaining == 0) {
            try {
                it->deleter();
            } catch (const std::exception& e) {
                CORE_ERROR("Exception during resource deletion: {}", e.what());
            }
            it = pending_deletions_.erase(it);
        } else {
            --it->frames_remaining;
            ++it;
        }
    }
    
    // Clean up expired references periodically
    static uint32_t frame_counter = 0;
    if (++frame_counter % 60 == 0) { // Every 60 frames
        CleanupExpiredReferences();
    }
}

void ResourceManager::ForceCleanup() {
    // Process all pending deletions immediately
    {
        std::lock_guard<std::mutex> lock(deletions_mutex_);
        for (auto& deletion : pending_deletions_) {
            try {
                deletion.deleter();
            } catch (const std::exception& e) {
                CORE_ERROR("Exception during forced resource deletion: {}", e.what());
            }
        }
        pending_deletions_.clear();
    }
    
    // Clear all managed resources
    {
        std::lock_guard<std::mutex> lock(buffers_mutex_);
        managed_buffers_.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(textures_mutex_);
        managed_textures_.clear();
    }
    
    memory_usage_.store(0);
}

ResourceManager::Statistics ResourceManager::GetStatistics() const {
    Statistics stats;
    
    {
        std::lock_guard<std::mutex> lock(buffers_mutex_);
        stats.buffer_count = static_cast<uint32_t>(managed_buffers_.size());
    }
    
    {
        std::lock_guard<std::mutex> lock(textures_mutex_);
        stats.texture_count = static_cast<uint32_t>(managed_textures_.size());
    }
    
    {
        std::lock_guard<std::mutex> lock(deletions_mutex_);
        stats.pending_deletions = static_cast<uint32_t>(pending_deletions_.size());
    }
    
    stats.total_memory_allocated = memory_usage_.load();
    
    return stats;
}

void ResourceManager::CleanupExpiredReferences() {
    // Clean up expired buffer references
    {
        std::lock_guard<std::mutex> lock(buffers_mutex_);
        managed_buffers_.erase(
            std::remove_if(managed_buffers_.begin(), managed_buffers_.end(),
                          [](const std::weak_ptr<Buffer>& weak_ptr) {
                              return weak_ptr.expired();
                          }),
            managed_buffers_.end()
        );
    }
    
    // Clean up expired texture references
    {
        std::lock_guard<std::mutex> lock(textures_mutex_);
        managed_textures_.erase(
            std::remove_if(managed_textures_.begin(), managed_textures_.end(),
                          [](const std::weak_ptr<Texture>& weak_ptr) {
                              return weak_ptr.expired();
                          }),
            managed_textures_.end()
        );
    }
}

} // namespace renderer
} // namespace helios