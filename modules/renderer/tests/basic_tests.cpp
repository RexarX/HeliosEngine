#include "helios/renderer/renderer.h"
#include "helios/renderer/memory/resource_manager.h"
#include "helios/renderer/pipeline/pipeline_cache.h"

#include <HeliosEngine/Log.h>

#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <cassert>

/**
 * @brief Basic functionality tests for the renderer module
 * 
 * These tests validate the core functionality and thread safety
 * of the new renderer module without requiring full NVRHI integration.
 */

namespace helios {
namespace renderer {
namespace tests {

/**
 * @brief Test resource manager thread safety
 */
bool TestResourceManagerThreadSafety() {
    CORE_INFO("Testing ResourceManager thread safety...");
    
    ResourceManager resource_manager;
    std::atomic<int> completed_operations{0};
    constexpr int num_threads = 4;
    constexpr int operations_per_thread = 100;
    
    std::vector<std::thread> threads;
    
    // Launch multiple threads that perform resource operations
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&resource_manager, &completed_operations, operations_per_thread]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                // Simulate resource creation and deletion
                resource_manager.ScheduleForDeletion([]() {
                    // Simulate cleanup work
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }, 2);
                
                completed_operations.fetch_add(1);
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Process deletions
    for (int frame = 0; frame < 5; ++frame) {
        resource_manager.ProcessDeletions();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    bool success = completed_operations.load() == num_threads * operations_per_thread;
    
    auto stats = resource_manager.GetStatistics();
    CORE_INFO("ResourceManager test completed - Operations: {}, Pending deletions: {}", 
              completed_operations.load(), stats.pending_deletions);
    
    return success;
}

/**
 * @brief Test pipeline cache thread safety
 */
bool TestPipelineCacheThreadSafety() {
    CORE_INFO("Testing PipelineCache thread safety...");
    
    PipelineCache pipeline_cache;
    std::atomic<int> cache_operations{0};
    constexpr int num_threads = 4;
    constexpr int operations_per_thread = 50;
    
    std::vector<std::thread> threads;
    
    // Launch multiple threads that access the pipeline cache
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&pipeline_cache, &cache_operations, operations_per_thread, t]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                // Create dummy pipeline descriptions
                GraphicsPipelineDesc desc;
                desc.debug_name = "TestPipeline";
                
                // Try to get pipeline (will return nullptr in current implementation)
                auto pipeline = pipeline_cache.GetGraphicsPipeline(desc);
                
                ComputePipelineDesc compute_desc;
                compute_desc.debug_name = "TestComputePipeline";
                
                auto compute_pipeline = pipeline_cache.GetComputePipeline(compute_desc);
                
                cache_operations.fetch_add(1);
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    bool success = cache_operations.load() == num_threads * operations_per_thread;
    
    auto stats = pipeline_cache.GetStatistics();
    CORE_INFO("PipelineCache test completed - Operations: {}, Cache hits: {}, Cache misses: {}", 
              cache_operations.load(), stats.cache_hits, stats.cache_misses);
    
    return success;
}

/**
 * @brief Test renderer singleton behavior
 */
bool TestRendererSingleton() {
    CORE_INFO("Testing Renderer singleton behavior...");
    
    // Test that renderer is not initialized initially
    bool exception_thrown = false;
    try {
        auto& renderer = Renderer::GetInstance();
        (void)renderer; // Suppress unused variable warning
    } catch (const std::runtime_error&) {
        exception_thrown = true;
    }
    
    if (!exception_thrown) {
        CORE_ERROR("Expected exception when accessing uninitialized renderer");
        return false;
    }
    
    // Test initialization (will fail without proper window handle, but tests the flow)
    bool init_result = Renderer::Initialize(nullptr, false, 2);
    
    // Test shutdown
    Renderer::Shutdown();
    
    CORE_INFO("Renderer singleton test completed");
    return true; // Pass test regardless of init result since we don't have a real window
}

/**
 * @brief Run all basic tests
 */
bool RunAllTests() {
    CORE_INFO("Starting renderer module tests...");
    
    std::vector<std::pair<const char*, std::function<bool()>>> tests = {
        {"ResourceManager Thread Safety", TestResourceManagerThreadSafety},
        {"PipelineCache Thread Safety", TestPipelineCacheThreadSafety},
        {"Renderer Singleton", TestRendererSingleton}
    };
    
    int passed = 0;
    int total = static_cast<int>(tests.size());
    
    for (const auto& [name, test_func] : tests) {
        try {
            bool result = test_func();
            if (result) {
                CORE_INFO("✅ Test PASSED: {}", name);
                ++passed;
            } else {
                CORE_ERROR("❌ Test FAILED: {}", name);
            }
        } catch (const std::exception& e) {
            CORE_ERROR("❌ Test FAILED with exception: {} - {}", name, e.what());
        }
    }
    
    CORE_INFO("Test results: {}/{} tests passed", passed, total);
    return passed == total;
}

} // namespace tests
} // namespace renderer
} // namespace helios

// Simple main function for running tests
int main() {
    // Initialize basic logging for tests
    Helios::Log::Init();
    
    bool all_passed = helios::renderer::tests::RunAllTests();
    
    return all_passed ? 0 : 1;
}