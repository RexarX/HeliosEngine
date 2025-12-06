#include <doctest/doctest.h>

#include <helios/core/logger.hpp>
#include <helios/core/memory/allocator_traits.hpp>
#include <helios/core/memory/double_frame_allocator.hpp>
#include <helios/core/memory/frame_allocator.hpp>
#include <helios/core/memory/free_list_allocator.hpp>
#include <helios/core/memory/n_frame_allocator.hpp>
#include <helios/core/memory/pool_allocator.hpp>
#include <helios/core/memory/stack_allocator.hpp>
#include <helios/core/timer.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <string_view>
#include <thread>
#include <vector>

namespace {

using namespace helios::memory;
using helios::Timer;

struct PerformanceStats {
  double avg_alloc_time_ns = 0.0;
  double avg_dealloc_time_ns = 0.0;
  double total_time_ms = 0.0;
  size_t successful_allocations = 0;
  size_t failed_allocations = 0;
  size_t total_bytes_allocated = 0;
  size_t peak_usage = 0;
  size_t thread_count = 1;

  void Print(std::string_view allocator_name, std::string_view test_name) const {
    HELIOS_INFO("=== {} - {} ===", allocator_name, test_name);
    HELIOS_INFO("  Threads: {}", thread_count);
    HELIOS_INFO("  Successful allocations: {}", successful_allocations);
    HELIOS_INFO("  Failed allocations: {}", failed_allocations);
    HELIOS_INFO("  Total bytes allocated: {} bytes", total_bytes_allocated);
    HELIOS_INFO("  Peak usage: {} bytes", peak_usage);
    HELIOS_INFO("  Total time: {} ms", total_time_ms);
    HELIOS_INFO("  Avg allocation time: {} ns", avg_alloc_time_ns);
    if (avg_dealloc_time_ns > 0.0) {
      HELIOS_INFO("  Avg deallocation time: {} ns", avg_dealloc_time_ns);
    }
    HELIOS_INFO("  Throughput: {} allocs/sec",
                successful_allocations > 0 ? (successful_allocations / (total_time_ms / 1000.0)) : 0.0);
  }
};

struct TestWorkload {
  size_t min_size = 16;
  size_t max_size = 1024;
  size_t iterations = 10000;
  size_t alignment = 64;
  bool random_sizes = true;

  [[nodiscard]] size_t GetSize(size_t iteration, std::mt19937& rng) const {
    if (random_sizes) {
      std::uniform_int_distribution<size_t> dist(min_size, max_size);
      return dist(rng);
    }
    return min_size + (iteration % (max_size - min_size));
  }
};

TEST_SUITE("memory::AllocatorIntegration") {
  TEST_CASE("FrameAllocator: Sequential allocation patterns") {
    constexpr size_t capacity = 4 * 1024 * 1024;  // 4MB
    FrameAllocator allocator(capacity);

    TestWorkload workload{.min_size = 16, .max_size = 4096, .iterations = 50000, .alignment = 64, .random_sizes = true};

    PerformanceStats stats{};
    std::mt19937 rng(12345);

    Timer timer;  // measures total time

    for (size_t i = 0; i < workload.iterations; ++i) {
      const size_t size = workload.GetSize(i, rng);

      Timer alloc_timer;
      const auto result = allocator.Allocate(size);

      if (result.Valid()) {
        ++stats.successful_allocations;
        stats.total_bytes_allocated += result.allocated_size;

        const auto alloc_time = static_cast<double>(alloc_timer.ElapsedNanoSec());
        stats.avg_alloc_time_ns += alloc_time;

        // Write test pattern
        auto* const data = static_cast<uint8_t*>(result.ptr);
        data[0] = 0xAA;
        data[result.allocated_size - 1] = 0xBB;

        // Verify alignment
        CHECK(IsAligned(result.ptr, workload.alignment));
      } else {
        ++stats.failed_allocations;
      }

      // Reset every 1000 iterations to simulate frame boundaries
      if (i % 1000 == 999) {
        allocator.Reset();
      }

      // Track peak usage
      const size_t current = allocator.CurrentOffset();
      if (current > stats.peak_usage) {
        stats.peak_usage = current;
      }
    }

    stats.total_time_ms = timer.ElapsedMilliSec();
    if (stats.successful_allocations > 0) {
      stats.avg_alloc_time_ns /= stats.successful_allocations;
    }

    stats.Print("FrameAllocator", "Single-threaded Sequential");

    CHECK_GT(stats.successful_allocations, workload.iterations * 0.9);
  }

  TEST_CASE("PoolAllocator: Repeated alloc/dealloc cycles") {
    constexpr size_t block_size = 256;
    constexpr size_t block_count = 10000;
    PoolAllocator allocator(block_size, block_count);

    constexpr size_t iterations = 50000;
    PerformanceStats stats{};
    std::vector<void*> active_ptrs;
    active_ptrs.reserve(block_count);

    Timer timer;

    for (size_t i = 0; i < iterations; ++i) {
      Timer alloc_timer;
      const auto result = allocator.Allocate(block_size);

      if (result.Valid()) {
        ++stats.successful_allocations;
        stats.total_bytes_allocated += result.allocated_size;
        active_ptrs.push_back(result.ptr);

        const auto alloc_time = static_cast<double>(alloc_timer.ElapsedNanoSec());
        stats.avg_alloc_time_ns += alloc_time;

        // Write pattern
        auto* const data = static_cast<uint32_t*>(result.ptr);
        data[0] = static_cast<uint32_t>(i);
        data[block_size / sizeof(uint32_t) - 1] = ~static_cast<uint32_t>(i);
      } else {
        ++stats.failed_allocations;
      }

      // Deallocate some blocks periodically
      if (!active_ptrs.empty() && i % 10 == 0) {
        const size_t num_to_free = std::min(active_ptrs.size(), size_t(5));

        for (size_t j = 0; j < num_to_free; ++j) {
          Timer dealloc_timer;
          allocator.Deallocate(active_ptrs.back());
          const auto dealloc_time = static_cast<double>(dealloc_timer.ElapsedNanoSec());
          stats.avg_dealloc_time_ns += dealloc_time;

          active_ptrs.pop_back();
        }
      }

      stats.peak_usage = std::max(stats.peak_usage, allocator.UsedBlockCount() * block_size);
    }

    // Cleanup remaining allocations
    for (auto* ptr : active_ptrs) {
      allocator.Deallocate(ptr);
    }

    stats.total_time_ms = timer.ElapsedMilliSec();
    if (stats.successful_allocations > 0) {
      stats.avg_alloc_time_ns /= stats.successful_allocations;
    }

    const size_t dealloc_count = iterations / 10 * 5 + active_ptrs.size();
    if (dealloc_count > 0) {
      stats.avg_dealloc_time_ns /= dealloc_count;
    }

    stats.Print("PoolAllocator", "Single-threaded Alloc/Dealloc");

    CHECK_GT(stats.successful_allocations, 0);
    CHECK(allocator.Empty());
  }

  TEST_CASE("StackAllocator: LIFO pattern with markers") {
    constexpr size_t capacity = 2 * 1024 * 1024;  // 2MB
    StackAllocator allocator(capacity);

    constexpr size_t iterations = 10000;
    PerformanceStats stats{};

    Timer timer;

    for (size_t i = 0; i < iterations; ++i) {
      // Save marker
      const auto marker = allocator.Marker();

      // Allocate multiple blocks
      constexpr size_t blocks_per_iteration = 10;
      std::vector<AllocationResult> results;

      for (size_t j = 0; j < blocks_per_iteration; ++j) {
        const size_t size = 64 + j * 32;

        Timer alloc_timer;
        const auto result = allocator.Allocate(size, 16);

        if (result.Valid()) {
          ++stats.successful_allocations;
          stats.total_bytes_allocated += result.allocated_size;
          results.push_back(result);

          const auto alloc_time = static_cast<double>(alloc_timer.ElapsedNanoSec());
          stats.avg_alloc_time_ns += alloc_time;

          // Write data
          std::memset(result.ptr, static_cast<int>(j), result.allocated_size);
        }
      }

      stats.peak_usage = std::max(stats.peak_usage, allocator.CurrentOffset());

      // Rewind to marker (bulk deallocation)
      allocator.RewindToMarker(marker);
      CHECK_EQ(allocator.CurrentOffset(), marker);
    }

    stats.total_time_ms = timer.ElapsedMilliSec();
    if (stats.successful_allocations > 0) {
      stats.avg_alloc_time_ns /= stats.successful_allocations;
    }

    stats.Print("StackAllocator", "Single-threaded LIFO with Markers");

    CHECK_EQ(stats.successful_allocations, iterations * 10);
    CHECK(allocator.Empty());
  }

  TEST_CASE("FreeListAllocator: Variable-size allocations") {
    constexpr size_t capacity = 4 * 1024 * 1024;  // 4MB
    FreeListAllocator allocator(capacity);

    constexpr size_t iterations = 20000;
    PerformanceStats stats{};
    std::vector<std::pair<void*, size_t>> allocations;
    std::mt19937 rng(54321);

    Timer timer;

    for (size_t i = 0; i < iterations; ++i) {
      std::uniform_int_distribution<size_t> size_dist(32, 2048);
      const size_t size = size_dist(rng);

      Timer alloc_timer;
      const auto result = allocator.Allocate(size, 16);

      if (result.Valid()) {
        ++stats.successful_allocations;
        stats.total_bytes_allocated += result.allocated_size;
        allocations.emplace_back(result.ptr, size);

        const auto alloc_time = static_cast<double>(alloc_timer.ElapsedNanoSec());
        stats.avg_alloc_time_ns += alloc_time;
      } else {
        ++stats.failed_allocations;
      }

      // Randomly deallocate some blocks to create fragmentation
      if (!allocations.empty() && i % 5 == 0) {
        std::uniform_int_distribution<size_t> idx_dist(0, allocations.size() - 1);
        const size_t idx = idx_dist(rng);

        Timer dealloc_timer;
        allocator.Deallocate(allocations[idx].first);
        const auto dealloc_time = static_cast<double>(dealloc_timer.ElapsedNanoSec());
        stats.avg_dealloc_time_ns += dealloc_time;

        allocations.erase(allocations.begin() + idx);
      }

      stats.peak_usage = std::max(stats.peak_usage, allocator.UsedMemory());
    }

    // Cleanup
    const size_t deallocs = allocations.size();
    for (const auto& [ptr, size] : allocations) {
      allocator.Deallocate(ptr);
    }

    stats.total_time_ms = timer.ElapsedMilliSec();
    if (stats.successful_allocations > 0) {
      stats.avg_alloc_time_ns /= stats.successful_allocations;
    }

    const size_t total_deallocs = iterations / 5 + deallocs;
    if (total_deallocs > 0) {
      stats.avg_dealloc_time_ns /= total_deallocs;
    }

    stats.Print("FreeListAllocator", "Single-threaded Variable-size");

    CHECK_GT(stats.successful_allocations, 0);
    CHECK(allocator.Empty());
  }

  TEST_CASE("DoubleFrameAllocator: Frame swapping") {
    constexpr size_t capacity_per_frame = 1024 * 1024;  // 1MB per frame
    DoubleFrameAllocator allocator(capacity_per_frame);

    constexpr size_t num_frames = 1000;
    PerformanceStats stats{};

    Timer timer;

    for (size_t frame = 0; frame < num_frames; ++frame) {
      // Allocate for current frame
      for (size_t i = 0; i < 100; ++i) {
        const size_t size = 64 + i * 16;

        Timer alloc_timer;
        const auto result = allocator.Allocate(size);
        if (result.Valid()) {
          ++stats.successful_allocations;
          stats.total_bytes_allocated += result.allocated_size;

          // Write frame number
          auto* const data = static_cast<uint32_t*>(result.ptr);
          *data = static_cast<uint32_t>(frame);
        }
      }

      const auto current_stats = allocator.CurrentFrameStats();
      stats.peak_usage = std::max(stats.peak_usage, current_stats.total_allocated);

      // Advance to next frame
      allocator.NextFrame();
    }

    stats.total_time_ms = timer.ElapsedMilliSec();
    if (stats.successful_allocations > 0) {
      stats.avg_alloc_time_ns = (stats.total_time_ms * 1000000.0) / stats.successful_allocations;
    }

    stats.Print("DoubleFrameAllocator", "Single-threaded Frame Swapping");

    CHECK_EQ(stats.successful_allocations, num_frames * 100);
  }

  TEST_CASE("Single-threaded NFrameAllocator<4> - Pipeline simulation") {
    constexpr size_t capacity_per_frame = 512 * 1024;  // 512KB per frame
    NFrameAllocator<4> allocator(capacity_per_frame);

    constexpr size_t num_frames = 1000;
    PerformanceStats stats{};

    Timer timer;

    for (size_t frame = 0; frame < num_frames; ++frame) {
      // Simulate pipeline stages
      for (size_t stage = 0; stage < 4; ++stage) {
        for (size_t i = 0; i < 25; ++i) {
          const size_t size = 128 + stage * 64;

          Timer alloc_timer;
          const auto result = allocator.Allocate(size);
          if (result.Valid()) {
            ++stats.successful_allocations;
            stats.total_bytes_allocated += result.allocated_size;
          }
        }
      }

      const auto current_stats = allocator.CurrentFrameStats();
      stats.peak_usage = std::max(stats.peak_usage, current_stats.total_allocated);

      allocator.NextFrame();
    }

    stats.total_time_ms = timer.ElapsedMilliSec();
    if (stats.successful_allocations > 0) {
      stats.avg_alloc_time_ns = (stats.total_time_ms * 1000000.0) / stats.successful_allocations;
    }

    stats.Print("NFrameAllocator<4>", "Single-threaded Pipeline");

    CHECK_EQ(stats.successful_allocations, num_frames * 100);
  }

  TEST_CASE("FrameAllocator: multi-threaded Concurrent allocations") {
    constexpr size_t capacity = 4 * 1024 * 1024;  // 4MB
    FrameAllocator allocator(capacity);

    constexpr size_t num_threads = 8;
    constexpr size_t iterations_per_thread = 5000;

    std::atomic<size_t> successful_allocations{0};
    std::atomic<size_t> failed_allocations{0};
    std::atomic<size_t> total_bytes{0};
    std::atomic<double> total_alloc_time_ns{0.0};

    Timer timer;

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (size_t thread_id = 0; thread_id < num_threads; ++thread_id) {
      threads.emplace_back([&, thread_id, iterations_per_thread]() {
        std::mt19937 rng(static_cast<unsigned>(thread_id));
        std::uniform_int_distribution<size_t> size_dist(16, 1024);

        for (size_t i = 0; i < iterations_per_thread; ++i) {
          Timer alloc_timer;
          const size_t size = size_dist(rng);
          const auto result = allocator.Allocate(size);

          if (result.Valid()) {
            successful_allocations.fetch_add(1, std::memory_order_relaxed);
            total_bytes.fetch_add(result.allocated_size, std::memory_order_relaxed);

            // Write thread ID
            auto* const data = static_cast<uint8_t*>(result.ptr);
            data[0] = static_cast<uint8_t>(thread_id);

            // Verify
            CHECK_EQ(data[0], static_cast<uint8_t>(thread_id));
            CHECK(IsAligned(result.ptr, kDefaultAlignment));

            const double alloc_time = static_cast<double>(alloc_timer.ElapsedNanoSec());

            // Use atomic add for double (not perfect but good enough for stats)
            double expected = total_alloc_time_ns.load(std::memory_order_relaxed);
            while (!total_alloc_time_ns.compare_exchange_weak(expected, expected + alloc_time,
                                                              std::memory_order_relaxed)) {
            }
          } else {
            failed_allocations.fetch_add(1, std::memory_order_relaxed);
          }
        }
      });
    }

    for (auto& thread : threads) {
      thread.join();
    }

    PerformanceStats stats{.total_time_ms = timer.ElapsedMilliSec(),
                           .successful_allocations = successful_allocations.load(),
                           .failed_allocations = failed_allocations.load(),
                           .total_bytes_allocated = total_bytes.load(),
                           .peak_usage = allocator.Stats().peak_usage,
                           .thread_count = num_threads};

    if (stats.successful_allocations > 0) {
      stats.avg_alloc_time_ns = total_alloc_time_ns.load() / stats.successful_allocations;
    }

    stats.Print("FrameAllocator", "Multi-threaded Concurrent");

    CHECK_GT(stats.successful_allocations, 0);
  }

  TEST_CASE("PoolAllocator: multi-threaded Concurrent alloc/dealloc") {
    constexpr size_t block_size = 256;
    constexpr size_t block_count = 20000;
    PoolAllocator allocator(block_size, block_count);

    constexpr size_t num_threads = 8;
    constexpr size_t iterations_per_thread = 2000;

    std::atomic<size_t> successful_allocations{0};
    std::atomic<size_t> total_deallocations{0};

    Timer timer;

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (size_t t = 0; t < num_threads; ++t) {
      threads.emplace_back([&, thread_id = t]() {
        std::vector<void*> local_ptrs;
        local_ptrs.reserve(iterations_per_thread);

        // Allocate
        for (size_t i = 0; i < iterations_per_thread; ++i) {
          const auto result = allocator.Allocate(block_size);
          if (result.Valid()) {
            successful_allocations.fetch_add(1, std::memory_order_relaxed);
            local_ptrs.push_back(result.ptr);

            // Write pattern
            auto* const data = static_cast<uint64_t*>(result.ptr);
            data[0] = (static_cast<uint64_t>(thread_id) << 32) | i;

            // Verify
            CHECK_EQ(data[0], ((static_cast<uint64_t>(thread_id) << 32) | i));
          }
        }

        // Deallocate half
        const size_t half = local_ptrs.size() / 2;
        for (size_t i = 0; i < half; ++i) {
          allocator.Deallocate(local_ptrs[i]);
          total_deallocations.fetch_add(1, std::memory_order_relaxed);
        }

        // Allocate more (reusing freed blocks)
        for (size_t i = 0; i < half; ++i) {
          const auto result = allocator.Allocate(block_size);
          if (result.Valid()) {
            successful_allocations.fetch_add(1, std::memory_order_relaxed);
            local_ptrs.push_back(result.ptr);
          }
        }

        // Cleanup remaining
        for (size_t i = half; i < local_ptrs.size(); ++i) {
          allocator.Deallocate(local_ptrs[i]);
          total_deallocations.fetch_add(1, std::memory_order_relaxed);
        }
      });
    }

    for (auto& thread : threads) {
      thread.join();
    }

    PerformanceStats stats{.total_time_ms = timer.ElapsedMilliSec(),
                           .successful_allocations = successful_allocations.load(),
                           .total_bytes_allocated = successful_allocations.load() * block_size,
                           .peak_usage = allocator.Stats().peak_usage,
                           .thread_count = num_threads};

    stats.Print("PoolAllocator", "Multi-threaded Concurrent Alloc/Dealloc");

    CHECK(allocator.Empty());
    CHECK_EQ(total_deallocations.load(), successful_allocations.load());
  }

  TEST_CASE("StackAllocator: multi-threaded Per-thread stacks") {
    constexpr size_t num_threads = 8;
    constexpr size_t capacity = 1024 * 1024;  // 1MB per thread
    constexpr size_t iterations_per_thread = 1000;

    std::atomic<size_t> total_allocations{0};
    std::atomic<size_t> total_bytes{0};

    Timer timer;

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (size_t t = 0; t < num_threads; ++t) {
      threads.emplace_back([&, iterations_per_thread, thread_id = t]() {
        // Each thread has its own stack
        StackAllocator allocator(capacity);

        for (size_t i = 0; i < iterations_per_thread; ++i) {
          const auto marker = allocator.Marker();

          // Nested allocations
          for (size_t j = 0; j < 10; ++j) {
            const size_t size = 64 + j * 32;
            const auto result = allocator.Allocate(size);

            if (result.Valid()) {
              total_allocations.fetch_add(1, std::memory_order_relaxed);
              total_bytes.fetch_add(result.allocated_size, std::memory_order_relaxed);

              // Write thread and iteration info
              auto* const data = static_cast<uint32_t*>(result.ptr);
              data[0] = static_cast<uint32_t>(thread_id);
              data[1] = static_cast<uint32_t>(i);

              CHECK_EQ(data[0], static_cast<uint32_t>(thread_id));
            }
          }

          // Rewind
          allocator.RewindToMarker(marker);
        }

        CHECK(allocator.Empty());
      });
    }

    for (auto& thread : threads) {
      thread.join();
    }

    PerformanceStats stats{.total_time_ms = timer.ElapsedMilliSec(),
                           .successful_allocations = total_allocations.load(),
                           .total_bytes_allocated = total_bytes.load(),
                           .thread_count = num_threads};

    stats.Print("StackAllocator", "Multi-threaded Per-thread Stacks");

    CHECK_EQ(stats.successful_allocations, num_threads * iterations_per_thread * 10);
  }

  TEST_CASE("FreeListAllocator: multi-threaded Shared allocator stress test") {
    constexpr size_t capacity = 16 * 1024 * 1024;  // 16MB
    FreeListAllocator allocator(capacity);

    constexpr size_t num_threads = 8;
    constexpr size_t iterations_per_thread = 1000;

    std::atomic<size_t> successful_allocations{0};
    std::atomic<size_t> total_deallocations{0};

    Timer timer;

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (size_t t = 0; t < num_threads; ++t) {
      threads.emplace_back([&, iterations_per_thread, thread_id = t]() {
        std::mt19937 rng(static_cast<unsigned>(thread_id * 1000));
        std::uniform_int_distribution<size_t> size_dist(32, 4096);
        std::vector<std::pair<void*, size_t>> local_allocations;

        for (size_t i = 0; i < iterations_per_thread; ++i) {
          const size_t size = size_dist(rng);

          const auto result = allocator.Allocate(size);
          if (result.Valid()) {
            successful_allocations.fetch_add(1, std::memory_order_relaxed);
            local_allocations.emplace_back(result.ptr, size);

            // Write pattern
            std::memset(result.ptr, static_cast<int>(thread_id), result.allocated_size);
          }

          // Randomly deallocate
          if (!local_allocations.empty() && i % 3 == 0) {
            std::uniform_int_distribution<size_t> idx_dist(0, local_allocations.size() - 1);
            const size_t idx = idx_dist(rng);

            allocator.Deallocate(local_allocations[idx].first);
            total_deallocations.fetch_add(1, std::memory_order_relaxed);

            local_allocations.erase(local_allocations.begin() + idx);
          }
        }

        // Cleanup
        for (const auto& [ptr, size] : local_allocations) {
          allocator.Deallocate(ptr);
          total_deallocations.fetch_add(1, std::memory_order_relaxed);
        }
      });
    }

    for (auto& thread : threads) {
      thread.join();
    }

    PerformanceStats stats{.total_time_ms = timer.ElapsedMilliSec(),
                           .successful_allocations = successful_allocations.load(),
                           .total_bytes_allocated = allocator.Stats().total_freed,
                           .peak_usage = allocator.Stats().peak_usage,
                           .thread_count = num_threads};

    stats.Print("FreeListAllocator", "Multi-threaded Stress Test");

    CHECK(allocator.Empty());
    CHECK_EQ(total_deallocations.load(), successful_allocations.load());
  }

  TEST_CASE("DoubleFrameAllocator: multi-threaded Frame sync simulation") {
    constexpr size_t capacity_per_frame = 2 * 1024 * 1024;  // 2MB per frame
    DoubleFrameAllocator allocator(capacity_per_frame);

    constexpr size_t num_frames = 100;
    constexpr size_t num_worker_threads = 4;

    std::atomic<size_t> total_allocations{0};
    std::atomic<bool> should_stop{false};

    Timer timer;

    // Main thread controls frame advancement
    std::thread frame_controller([&]() {
      for (size_t frame = 0; frame < num_frames; ++frame) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        allocator.NextFrame();
      }
      should_stop.store(true, std::memory_order_release);
    });

    // Worker threads allocate continuously
    std::vector<std::thread> workers;
    workers.reserve(num_worker_threads);

    for (size_t w = 0; w < num_worker_threads; ++w) {
      workers.emplace_back([&, worker_id = w]() {
        std::mt19937 rng(static_cast<unsigned>(worker_id));
        std::uniform_int_distribution<size_t> size_dist(64, 2048);

        while (!should_stop.load(std::memory_order_acquire)) {
          const size_t size = size_dist(rng);
          const auto result = allocator.Allocate(size);

          if (result.Valid()) {
            total_allocations.fetch_add(1, std::memory_order_relaxed);

            // Write worker ID
            auto* const data = static_cast<uint8_t*>(result.ptr);
            data[0] = static_cast<uint8_t>(worker_id);
          }
        }
      });
    }

    frame_controller.join();
    for (auto& worker : workers) {
      worker.join();
    }

    PerformanceStats stats{.total_time_ms = timer.ElapsedMilliSec(),
                           .successful_allocations = total_allocations.load(),
                           .peak_usage = allocator.Stats().peak_usage,
                           .thread_count = num_worker_threads + 1};

    stats.Print("DoubleFrameAllocator", "Multi-threaded Frame Sync");

    CHECK_GT(stats.successful_allocations, 0);
    HELIOS_INFO("  Frames processed: {}", num_frames);
  }

  TEST_CASE("Comparative performance - All allocators") {
    constexpr size_t test_iterations = 10000;
    constexpr size_t allocation_size = 256;

    HELIOS_INFO("\n=== Comparative Performance ({} iterations, {} bytes) ===\n", test_iterations, allocation_size);

    // FrameAllocator
    {
      FrameAllocator allocator(test_iterations * allocation_size * 2);
      Timer timer;

      for (size_t i = 0; i < test_iterations; ++i) {
        const auto result = allocator.Allocate(allocation_size);
        CHECK(result.Valid());
      }

      const double time_ms = timer.ElapsedMilliSec();
      const double avg_ns = (time_ms * 1000000.0) / test_iterations;

      HELIOS_INFO("FrameAllocator:     {} ms ({} ns/alloc)", time_ms, avg_ns);
    }

    // PoolAllocator
    {
      PoolAllocator allocator(allocation_size, test_iterations);
      Timer timer;

      std::vector<void*> ptrs;
      for (size_t i = 0; i < test_iterations; ++i) {
        const auto result = allocator.Allocate(allocation_size);
        CHECK(result.Valid());
        ptrs.push_back(result.ptr);
      }

      for (auto* ptr : ptrs) {
        allocator.Deallocate(ptr);
      }

      const double time_ms = timer.ElapsedMilliSec();
      const double avg_ns = (time_ms * 1000000.0) / (test_iterations * 2);

      HELIOS_INFO("PoolAllocator:      {} ms ({} ns/op)", time_ms, avg_ns);
    }

    // StackAllocator
    {
      StackAllocator allocator(test_iterations * allocation_size * 2);
      Timer timer;

      for (size_t i = 0; i < test_iterations; ++i) {
        const auto marker = allocator.Marker();
        const auto result = allocator.Allocate(allocation_size);
        CHECK(result.Valid());
        allocator.RewindToMarker(marker);
      }

      const double time_ms = timer.ElapsedMilliSec();
      const double avg_ns = (time_ms * 1000000.0) / test_iterations;

      HELIOS_INFO("StackAllocator:     {} ms ({} ns/alloc+rewind)", time_ms, avg_ns);
    }

    // FreeListAllocator
    {
      FreeListAllocator allocator(test_iterations * allocation_size * 2);
      Timer timer;

      std::vector<void*> ptrs;
      for (size_t i = 0; i < test_iterations; ++i) {
        const auto result = allocator.Allocate(allocation_size);
        CHECK(result.Valid());
        ptrs.push_back(result.ptr);
      }

      for (auto* ptr : ptrs) {
        allocator.Deallocate(ptr);
      }

      const double time_ms = timer.ElapsedMilliSec();
      const double avg_ns = (time_ms * 1000000.0) / (test_iterations * 2);

      HELIOS_INFO("FreeListAllocator:  {} ms ({} ns/op)", time_ms, avg_ns);
    }
  }
}

}  // namespace
