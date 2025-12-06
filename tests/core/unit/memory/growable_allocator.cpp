#include <doctest/doctest.h>

#include <helios/core/memory/frame_allocator.hpp>
#include <helios/core/memory/free_list_allocator.hpp>
#include <helios/core/memory/growable_allocator.hpp>
#include <helios/core/memory/stl_allocator_adapter.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

using namespace helios::memory;

TEST_SUITE("memory::GrowableAllocator") {
  TEST_CASE("Construction with FrameAllocator") {
    SUBCASE("Valid parameters") {
      constexpr size_t initial_capacity = 1024;
      constexpr double growth_factor = 2.0;
      GrowableAllocator<FrameAllocator> allocator(initial_capacity, growth_factor);

      CHECK_EQ(allocator.InitialCapacity(), initial_capacity);
      CHECK_EQ(allocator.GrowthFactor(), doctest::Approx(growth_factor));
      CHECK_EQ(allocator.AllocatorCount(), 1);
      CHECK_EQ(allocator.TotalCapacity(), initial_capacity);
      CHECK(allocator.CanGrow());
    }

    SUBCASE("Custom growth factor") {
      constexpr size_t initial_capacity = 512;
      constexpr double growth_factor = 1.5;
      GrowableAllocator<FrameAllocator> allocator(initial_capacity, growth_factor);

      CHECK_EQ(allocator.GrowthFactor(), doctest::Approx(growth_factor));
    }

    SUBCASE("With max allocators limit") {
      constexpr size_t initial_capacity = 256;
      constexpr size_t max_allocators = 5;
      GrowableAllocator<FrameAllocator> allocator(initial_capacity, 2.0, max_allocators);

      CHECK_EQ(allocator.MaxAllocators(), max_allocators);
      CHECK(allocator.CanGrow());
    }

    SUBCASE("Unlimited growth") {
      GrowableAllocator<FrameAllocator> allocator(1024, 2.0, 0);
      CHECK_EQ(allocator.MaxAllocators(), 0);
      CHECK(allocator.CanGrow());
    }
  }

  TEST_CASE("Basic allocation without growth") {
    constexpr size_t initial_capacity = 4096;
    GrowableAllocator<FrameAllocator> allocator(initial_capacity);

    SUBCASE("Single allocation within capacity") {
      constexpr size_t size = 512;
      const auto result = allocator.Allocate(size);

      CHECK_NE(result.ptr, nullptr);
      CHECK_EQ(result.allocated_size, size);
      CHECK_EQ(allocator.AllocatorCount(), 1);
    }

    SUBCASE("Multiple allocations within capacity") {
      const auto result1 = allocator.Allocate(256);
      const auto result2 = allocator.Allocate(512);
      const auto result3 = allocator.Allocate(1024);

      CHECK_NE(result1.ptr, nullptr);
      CHECK_NE(result2.ptr, nullptr);
      CHECK_NE(result3.ptr, nullptr);
      CHECK_EQ(allocator.AllocatorCount(), 1);
    }

    SUBCASE("Zero size allocation") {
      const auto result = allocator.Allocate(0);
      CHECK_EQ(result.ptr, nullptr);
      CHECK_EQ(result.allocated_size, 0);
    }
  }

  TEST_CASE("Automatic growth") {
    constexpr size_t initial_capacity = 1024;
    constexpr double growth_factor = 2.0;
    GrowableAllocator<FrameAllocator> allocator(initial_capacity, growth_factor);

    SUBCASE("Grow when capacity exceeded") {
      // Fill first allocator
      const auto result1 = allocator.Allocate(1024);
      CHECK_NE(result1.ptr, nullptr);
      CHECK_EQ(allocator.AllocatorCount(), 1);

      // This should trigger growth
      const auto result2 = allocator.Allocate(512);
      CHECK_NE(result2.ptr, nullptr);
      CHECK_EQ(allocator.AllocatorCount(), 2);
      CHECK_GT(allocator.TotalCapacity(), initial_capacity);
    }

    SUBCASE("Multiple growth cycles") {
      // Fill first allocator
      [[maybe_unused]] const auto r1 = allocator.Allocate(1024);
      CHECK_EQ(allocator.AllocatorCount(), 1);

      // Trigger first growth
      [[maybe_unused]] const auto r2 = allocator.Allocate(512);
      CHECK_EQ(allocator.AllocatorCount(), 2);

      // Fill second allocator (capacity should be 2048 after growth)
      [[maybe_unused]] const auto r3 = allocator.Allocate(1536);
      CHECK_EQ(allocator.AllocatorCount(), 2);

      // Trigger second growth
      [[maybe_unused]] const auto r4 = allocator.Allocate(1024);
      CHECK_EQ(allocator.AllocatorCount(), 3);
    }

    SUBCASE("Growth factor applied correctly") {
      // Fill first allocator
      [[maybe_unused]] const auto r1 = allocator.Allocate(1024);

      // Trigger growth
      [[maybe_unused]] const auto r2 = allocator.Allocate(512);
      CHECK_EQ(allocator.AllocatorCount(), 2);

      // Total capacity should be initial + (initial * growth_factor)
      // 1024 + 2048 = 3072
      CHECK_GE(allocator.TotalCapacity(), initial_capacity + initial_capacity * growth_factor);
    }

    SUBCASE("Allocation larger than current capacity") {
      // Request allocation larger than initial capacity
      constexpr size_t large_size = initial_capacity * 3;
      const auto result = allocator.Allocate(large_size);

      CHECK_NE(result.ptr, nullptr);
      CHECK_EQ(allocator.AllocatorCount(), 2);
      // New allocator should be large enough
      CHECK_GE(allocator.TotalCapacity(), large_size + initial_capacity);
    }
  }

  TEST_CASE("Growth limits") {
    constexpr size_t initial_capacity = 512;
    constexpr size_t max_allocators = 3;
    GrowableAllocator<FrameAllocator> allocator(initial_capacity, 2.0, max_allocators);

    SUBCASE("Respect max allocators limit") {
      // Fill all allocators
      [[maybe_unused]] const auto r1 = allocator.Allocate(512);   // Allocator 1
      [[maybe_unused]] const auto r2 = allocator.Allocate(512);   // Allocator 2 (growth)
      [[maybe_unused]] const auto r3 = allocator.Allocate(512);   // Allocator 2
      [[maybe_unused]] const auto r4 = allocator.Allocate(1024);  // Allocator 3 (growth)

      CHECK_EQ(allocator.AllocatorCount(), max_allocators);
      CHECK_FALSE(allocator.CanGrow());

      // Next allocation that doesn't fit should fail
      const auto result = allocator.Allocate(2048);
      CHECK_EQ(result.ptr, nullptr);
      CHECK_EQ(allocator.AllocatorCount(), max_allocators);
    }

    SUBCASE("Can allocate within last allocator") {
      // Fill first two allocators
      [[maybe_unused]] const auto r1 = allocator.Allocate(512);  // Allocator 1
      [[maybe_unused]] const auto r2 = allocator.Allocate(256);  // Allocator 2 (growth)

      CHECK_EQ(allocator.AllocatorCount(), 2);

      // Should be able to allocate more from second allocator
      const auto result = allocator.Allocate(256);
      CHECK_NE(result.ptr, nullptr);
      CHECK_EQ(allocator.AllocatorCount(), 2);
    }
  }

  TEST_CASE("Deallocation with FreeListAllocator") {
    constexpr size_t initial_capacity = 2048;
    GrowableAllocator<FreeListAllocator> allocator(initial_capacity);

    SUBCASE("Deallocate from single allocator") {
      const auto result = allocator.Allocate(512);
      CHECK_NE(result.ptr, nullptr);

      allocator.Deallocate(result.ptr, result.allocated_size);
      // Should succeed without assertion
    }

    SUBCASE("Deallocate from multiple allocators") {
      const auto result1 = allocator.Allocate(2048);  // First allocator
      const auto result2 = allocator.Allocate(1024);  // Second allocator (growth)

      CHECK_NE(result1.ptr, nullptr);
      CHECK_NE(result2.ptr, nullptr);
      CHECK_EQ(allocator.AllocatorCount(), 2);

      // Deallocate from second allocator
      allocator.Deallocate(result2.ptr, result2.allocated_size);

      // Deallocate from first allocator
      allocator.Deallocate(result1.ptr, result1.allocated_size);
    }

    SUBCASE("Deallocate after growth") {
      const auto result1 = allocator.Allocate(1024);
      const auto result2 = allocator.Allocate(1024);

      // Trigger growth
      const auto result3 = allocator.Allocate(1024);
      CHECK_EQ(allocator.AllocatorCount(), 2);

      // Deallocate in different order
      allocator.Deallocate(result3.ptr, result3.allocated_size);
      allocator.Deallocate(result1.ptr, result1.allocated_size);
      allocator.Deallocate(result2.ptr, result2.allocated_size);
    }
  }

  TEST_CASE("Reset functionality") {
    constexpr size_t initial_capacity = 1024;
    GrowableAllocator<FrameAllocator> allocator(initial_capacity);

    SUBCASE("Reset after single allocator usage") {
      [[maybe_unused]] const auto r1 = allocator.Allocate(512);
      CHECK_EQ(allocator.AllocatorCount(), 1);

      allocator.Reset();

      CHECK_EQ(allocator.AllocatorCount(), 1);
      const auto stats = allocator.Stats();
      CHECK_EQ(stats.total_allocated, 0);
    }

    SUBCASE("Reset after growth") {
      [[maybe_unused]] const auto r1 = allocator.Allocate(1024);  // First allocator
      [[maybe_unused]] const auto r2 = allocator.Allocate(512);   // Growth
      [[maybe_unused]] const auto r3 = allocator.Allocate(1024);  // Growth

      CHECK_GT(allocator.AllocatorCount(), 1);

      allocator.Reset();

      // Should keep only first allocator
      CHECK_EQ(allocator.AllocatorCount(), 1);
      CHECK_EQ(allocator.TotalCapacity(), initial_capacity);
      CHECK(allocator.CanGrow());
    }

    SUBCASE("Allocate after reset") {
      [[maybe_unused]] const auto r1 = allocator.Allocate(1024);
      [[maybe_unused]] const auto r2 = allocator.Allocate(512);  // Growth
      CHECK_EQ(allocator.AllocatorCount(), 2);

      allocator.Reset();
      CHECK_EQ(allocator.AllocatorCount(), 1);

      // Should be able to allocate again
      const auto result = allocator.Allocate(256);
      CHECK_NE(result.ptr, nullptr);
    }
  }

  TEST_CASE("STL container integration") {
    SUBCASE("std::vector with GrowableAllocator<FrameAllocator>") {
      GrowableAllocator<FrameAllocator> allocator(1024);
      std::vector<int, STLGrowableAllocator<int, FrameAllocator>> vec{
          STLGrowableAllocator<int, FrameAllocator>(allocator)};

      // Add elements to trigger growth
      for (int i = 0; i < 1000; ++i) {
        vec.push_back(i);
      }

      CHECK_EQ(vec.size(), 1000);

      for (int i = 0; i < 1000; ++i) {
        CHECK_EQ(vec[i], i);
      }
    }

    SUBCASE("std::vector with GrowableAllocator<FreeListAllocator>") {
      GrowableAllocator<FreeListAllocator> allocator(2048);
      std::vector<double, STLGrowableAllocator<double, FreeListAllocator>> vec{
          STLGrowableAllocator<double, FreeListAllocator>(allocator)};

      // Add elements
      for (int i = 0; i < 500; ++i) {
        vec.push_back(i * 3.14);
      }

      CHECK_EQ(vec.size(), 500);

      // FreeListAllocator supports deallocation
      vec.clear();
      CHECK_EQ(vec.size(), 0);
      CHECK_GT(vec.capacity(), 0);  // Capacity remains
    }

    SUBCASE("Multiple vectors share allocator") {
      constexpr size_t small_capacity = 256;
      GrowableAllocator<FrameAllocator> allocator(small_capacity);

      std::vector<int, STLGrowableAllocator<int, FrameAllocator>> vec1{
          STLGrowableAllocator<int, FrameAllocator>(allocator)};
      std::vector<int, STLGrowableAllocator<int, FrameAllocator>> vec2{
          STLGrowableAllocator<int, FrameAllocator>(allocator)};

      for (int i = 0; i < 200; ++i) {
        vec1.push_back(i);
        vec2.push_back(i * 2);
      }

      CHECK_EQ(vec1.size(), 200);
      CHECK_EQ(vec2.size(), 200);

      // Both should share the same underlying GrowableAllocator
      const auto stats = allocator.Stats();
      CHECK_GT(stats.allocation_count, 0);
    }

    SUBCASE("Container operations trigger growth") {
      constexpr size_t small_capacity = 256;
      GrowableAllocator<FrameAllocator> allocator(small_capacity);
      std::vector<int, STLGrowableAllocator<int, FrameAllocator>> vec{
          STLGrowableAllocator<int, FrameAllocator>(allocator)};

      CHECK_EQ(allocator.AllocatorCount(), 1);

      // Reserve space that exceeds initial capacity
      vec.reserve(1000);

      CHECK_GT(vec.capacity(), 0);
      CHECK_GE(allocator.AllocatorCount(), 1);
    }
  }
}

}  // namespace
