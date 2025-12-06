#include <doctest/doctest.h>

#include <helios/core/memory/double_frame_allocator.hpp>

#include <cstddef>
#include <cstdint>

namespace {

using namespace helios::memory;

TEST_SUITE("memory::DoubleFrameAllocator") {
  TEST_CASE("Construction") {
    SUBCASE("Valid capacity") {
      constexpr size_t capacity_per_buffer = 1024;
      DoubleFrameAllocator allocator(capacity_per_buffer);

      CHECK_EQ(allocator.Capacity(), capacity_per_buffer * 2);
      CHECK_EQ(allocator.CurrentBufferIndex(), 0);
      CHECK_EQ(allocator.PreviousBufferIndex(), 1);
    }

    SUBCASE("Large capacity") {
      constexpr size_t capacity_per_buffer = 1024 * 1024;
      DoubleFrameAllocator allocator(capacity_per_buffer);

      CHECK_EQ(allocator.Capacity(), capacity_per_buffer * 2);
    }

    SUBCASE("Buffer count constant") {
      CHECK_EQ(DoubleFrameAllocator::kBufferCount, 2);
    }
  }

  TEST_CASE("Basic allocation") {
    constexpr size_t capacity_per_buffer = 2048;
    DoubleFrameAllocator allocator(capacity_per_buffer);

    SUBCASE("Single allocation") {
      constexpr size_t size = 64;
      const auto result = allocator.Allocate(size);

      CHECK_NE(result.ptr, nullptr);
      CHECK_EQ(result.allocated_size, size);
    }

    SUBCASE("Multiple allocations in same frame") {
      const auto result1 = allocator.Allocate(128);
      const auto result2 = allocator.Allocate(256);
      const auto result3 = allocator.Allocate(512);

      CHECK_NE(result1.ptr, nullptr);
      CHECK_NE(result2.ptr, nullptr);
      CHECK_NE(result3.ptr, nullptr);

      CHECK_NE(result1.ptr, result2.ptr);
      CHECK_NE(result2.ptr, result3.ptr);
      CHECK_NE(result1.ptr, result3.ptr);
    }

    SUBCASE("Zero size allocation") {
      const auto result = allocator.Allocate(0);
      CHECK_EQ(result.ptr, nullptr);
      CHECK_EQ(result.allocated_size, 0);
    }
  }

  TEST_CASE("Frame switching") {
    constexpr size_t capacity_per_buffer = 2048;
    DoubleFrameAllocator allocator(capacity_per_buffer);

    SUBCASE("NextFrame switches buffers") {
      CHECK_EQ(allocator.CurrentBufferIndex(), 0);
      CHECK_EQ(allocator.PreviousBufferIndex(), 1);

      allocator.NextFrame();

      CHECK_EQ(allocator.CurrentBufferIndex(), 1);
      CHECK_EQ(allocator.PreviousBufferIndex(), 0);

      allocator.NextFrame();

      CHECK_EQ(allocator.CurrentBufferIndex(), 0);
      CHECK_EQ(allocator.PreviousBufferIndex(), 1);
    }

    SUBCASE("NextFrame resets current buffer") {
      const auto result1 = allocator.Allocate(256);
      CHECK_NE(result1.ptr, nullptr);

      const auto stats_before = allocator.CurrentFrameStats();
      CHECK_GT(stats_before.allocation_count, 0);

      allocator.NextFrame();

      const auto stats_after = allocator.CurrentFrameStats();
      CHECK_EQ(stats_after.allocation_count, 0);
      CHECK_EQ(stats_after.total_allocated, 0);
    }

    SUBCASE("Previous frame data remains valid") {
      // Frame 0: Allocate and write data
      const auto result1 = allocator.Allocate(sizeof(int));
      CHECK_NE(result1.ptr, nullptr);
      auto* const data1 = static_cast<int*>(result1.ptr);
      *data1 = 42;

      // Switch to frame 1
      allocator.NextFrame();

      // Frame 1: Allocate and write different data
      const auto result2 = allocator.Allocate(sizeof(int));
      CHECK_NE(result2.ptr, nullptr);
      auto* const data2 = static_cast<int*>(result2.ptr);
      *data2 = 100;

      // Both frames' data should still be valid
      CHECK_EQ(*data1, 42);
      CHECK_EQ(*data2, 100);

      // Switch to frame 0 (overwrites old frame 0)
      allocator.NextFrame();

      // Frame 1's data should still be valid
      CHECK_EQ(*data2, 100);
    }
  }

  TEST_CASE("Alignment") {
    constexpr size_t capacity_per_buffer = 4096;
    DoubleFrameAllocator allocator(capacity_per_buffer);

    SUBCASE("Default alignment") {
      const auto result = allocator.Allocate(100);
      CHECK_NE(result.ptr, nullptr);
      CHECK(IsAligned(result.ptr, kDefaultAlignment));
    }

    SUBCASE("Custom alignment 16") {
      const auto result = allocator.Allocate(100, 16);
      CHECK_NE(result.ptr, nullptr);
      CHECK(IsAligned(result.ptr, 16));
    }

    SUBCASE("Custom alignment 32") {
      const auto result = allocator.Allocate(100, 32);
      CHECK_NE(result.ptr, nullptr);
      CHECK(IsAligned(result.ptr, 32));
    }

    SUBCASE("Alignment preserved across frames") {
      const auto result1 = allocator.Allocate(100, 64);
      CHECK_NE(result1.ptr, nullptr);
      CHECK(IsAligned(result1.ptr, 64));

      allocator.NextFrame();

      const auto result2 = allocator.Allocate(100, 64);
      CHECK_NE(result2.ptr, nullptr);
      CHECK(IsAligned(result2.ptr, 64));
    }
  }

  TEST_CASE("Capacity per buffer") {
    constexpr size_t capacity_per_buffer = 1024;
    DoubleFrameAllocator allocator(capacity_per_buffer);

    SUBCASE("Allocate full buffer capacity") {
      const auto result = allocator.Allocate(capacity_per_buffer);
      CHECK_NE(result.ptr, nullptr);

      const auto stats = allocator.CurrentFrameStats();
      CHECK_GT(stats.allocation_count, 0);
    }

    SUBCASE("Cannot exceed single buffer capacity") {
      const auto result = allocator.Allocate(capacity_per_buffer + 1);
      CHECK_EQ(result.ptr, nullptr);
    }

    SUBCASE("Multiple allocations in one frame") {
      const auto result1 = allocator.Allocate(512);
      CHECK_NE(result1.ptr, nullptr);

      const auto result2 = allocator.Allocate(512);
      CHECK_NE(result2.ptr, nullptr);

      // Should have little space left
      const auto result3 = allocator.Allocate(100);
      CHECK_EQ(result3.ptr, nullptr);
    }

    SUBCASE("Full capacity across both frames") {
      // Fill frame 0
      const auto result1 = allocator.Allocate(capacity_per_buffer);
      CHECK_NE(result1.ptr, nullptr);

      allocator.NextFrame();

      // Fill frame 1
      const auto result2 = allocator.Allocate(capacity_per_buffer);
      CHECK_NE(result2.ptr, nullptr);

      // Both frames should be full but accessible
      CHECK_NE(result1.ptr, nullptr);
      CHECK_NE(result2.ptr, nullptr);
    }
  }

  TEST_CASE("Reset") {
    constexpr size_t capacity_per_buffer = 2048;
    DoubleFrameAllocator allocator(capacity_per_buffer);

    SUBCASE("Reset clears both buffers") {
      auto result = allocator.Allocate(256);
      allocator.NextFrame();
      result = allocator.Allocate(512);

      const auto stats_before = allocator.Stats();
      CHECK_GT(stats_before.allocation_count, 0);

      allocator.Reset();

      const auto stats_after = allocator.Stats();
      CHECK_EQ(stats_after.total_allocated, 0);
      CHECK_EQ(stats_after.allocation_count, 0);
    }

    SUBCASE("Can allocate after reset") {
      auto result = allocator.Allocate(512);
      allocator.NextFrame();
      result = allocator.Allocate(256);

      allocator.Reset();

      result = allocator.Allocate(128);
      CHECK_NE(result.ptr, nullptr);
    }
  }

  TEST_CASE("Statistics") {
    constexpr size_t capacity_per_buffer = 4096;
    DoubleFrameAllocator allocator(capacity_per_buffer);

    SUBCASE("Current frame stats") {
      auto result = allocator.Allocate(256);
      result = allocator.Allocate(512);

      const auto stats = allocator.CurrentFrameStats();
      CHECK_EQ(stats.allocation_count, 2);
      CHECK_GT(stats.total_allocated, 0);
    }

    SUBCASE("Previous frame stats") {
      auto result = allocator.Allocate(256);

      allocator.NextFrame();

      result = allocator.Allocate(512);

      const auto current_stats = allocator.CurrentFrameStats();
      const auto previous_stats = allocator.PreviousFrameStats();

      CHECK_EQ(current_stats.allocation_count, 1);
      CHECK_EQ(previous_stats.allocation_count, 1);
    }

    SUBCASE("Combined stats") {
      auto result = allocator.Allocate(256);

      allocator.NextFrame();

      result = allocator.Allocate(512);

      const auto stats = allocator.Stats();
      CHECK_EQ(stats.allocation_count, 2);
      CHECK_EQ(stats.total_allocations, 2);
    }

    SUBCASE("Peak usage tracking") {
      auto result = allocator.Allocate(512);

      allocator.NextFrame();

      result = allocator.Allocate(1024);

      const auto stats = allocator.Stats();
      // Peak should be from the larger allocation
      CHECK_GE(stats.peak_usage, 1024);
    }
  }

  TEST_CASE("Move semantics") {
    constexpr size_t capacity_per_buffer = 2048;

    SUBCASE("Move construction") {
      DoubleFrameAllocator allocator1(capacity_per_buffer);
      const auto result = allocator1.Allocate(128);

      DoubleFrameAllocator allocator2(std::move(allocator1));

      CHECK_EQ(allocator2.Capacity(), capacity_per_buffer * 2);
      const auto stats = allocator2.Stats();
      CHECK_GT(stats.allocation_count, 0);
    }

    SUBCASE("Move assignment") {
      DoubleFrameAllocator allocator1(capacity_per_buffer);
      const auto result = allocator1.Allocate(256);

      DoubleFrameAllocator allocator2(1024);

      allocator2 = std::move(allocator1);

      CHECK_EQ(allocator2.Capacity(), capacity_per_buffer * 2);
    }
  }

  TEST_CASE("Write and read allocated memory") {
    constexpr size_t capacity_per_buffer = 4096;
    DoubleFrameAllocator allocator(capacity_per_buffer);

    SUBCASE("Write and read across frames") {
      struct TestData {
        int x = 0;
        float y = 0.0F;
        char z = '\0';
      };

      // Frame 0
      const auto result1 = allocator.Allocate(sizeof(TestData));
      auto* const data1 = static_cast<TestData*>(result1.ptr);
      data1->x = 42;
      data1->y = 3.14F;
      data1->z = 'A';

      allocator.NextFrame();

      // Frame 1
      const auto result2 = allocator.Allocate(sizeof(TestData));
      auto* const data2 = static_cast<TestData*>(result2.ptr);
      data2->x = 100;
      data2->y = 2.71F;
      data2->z = 'B';

      // Verify both frames
      CHECK_EQ(data1->x, 42);
      CHECK_EQ(data1->y, doctest::Approx(3.14F));
      CHECK_EQ(data1->z, 'A');

      CHECK_EQ(data2->x, 100);
      CHECK_EQ(data2->y, doctest::Approx(2.71F));
      CHECK_EQ(data2->z, 'B');
    }

    SUBCASE("Array data across frames") {
      constexpr int array_size = 10;

      // Frame 0
      const auto result1 = allocator.Allocate(sizeof(int) * array_size);
      auto* const array1 = static_cast<int*>(result1.ptr);
      for (int i = 0; i < array_size; ++i) {
        array1[i] = i;
      }

      allocator.NextFrame();

      // Frame 1
      const auto result2 = allocator.Allocate(sizeof(int) * array_size);
      auto* const array2 = static_cast<int*>(result2.ptr);
      for (int i = 0; i < array_size; ++i) {
        array2[i] = i * 10;
      }

      // Verify frame 0 data
      for (int i = 0; i < array_size; ++i) {
        CHECK_EQ(array1[i], i);
      }

      // Verify frame 1 data
      for (int i = 0; i < array_size; ++i) {
        CHECK_EQ(array2[i], i * 10);
      }
    }
  }

  TEST_CASE("Free space tracking") {
    constexpr size_t capacity_per_buffer = 2048;
    DoubleFrameAllocator allocator(capacity_per_buffer);

    SUBCASE("Initial free space") {
      const auto free_space = allocator.FreeSpace();
      CHECK_EQ(free_space, capacity_per_buffer);
    }

    SUBCASE("Free space decreases with allocations") {
      const auto free_before = allocator.FreeSpace();

      const auto result = allocator.Allocate(256);

      const auto free_after = allocator.FreeSpace();
      CHECK_LT(free_after, free_before);
    }

    SUBCASE("Free space resets on frame switch") {
      const auto result = allocator.Allocate(512);

      const auto free_before = allocator.FreeSpace();
      CHECK_LT(free_before, capacity_per_buffer);

      allocator.NextFrame();

      const auto free_after = allocator.FreeSpace();
      CHECK_EQ(free_after, capacity_per_buffer);
    }
  }

  TEST_CASE("Multiple frame cycles") {
    constexpr size_t capacity_per_buffer = 1024;
    DoubleFrameAllocator allocator(capacity_per_buffer);

    SUBCASE("Alternating frames maintain data") {
      // Create pattern: allocate, switch, allocate, switch, verify
      for (int cycle = 0; cycle < 5; ++cycle) {
        const auto result = allocator.Allocate(sizeof(int));
        auto* const data = static_cast<int*>(result.ptr);
        *data = cycle * 100;

        allocator.NextFrame();

        // Previous frame data should be accessible for one more frame
        CHECK_EQ(*data, cycle * 100);
      }
    }

    SUBCASE("Buffer indices cycle correctly") {
      for (int i = 0; i < 10; ++i) {
        const size_t expected_index = i % 2;
        CHECK_EQ(allocator.CurrentBufferIndex(), expected_index);
        allocator.NextFrame();
      }
    }
  }

  TEST_CASE("Boundary conditions") {
    SUBCASE("Minimum capacity") {
      constexpr size_t capacity_per_buffer = 64;
      DoubleFrameAllocator allocator(capacity_per_buffer);

      const auto result = allocator.Allocate(32);
      CHECK_NE(result.ptr, nullptr);
    }

    SUBCASE("Single byte allocation") {
      constexpr size_t capacity_per_buffer = 1024;
      DoubleFrameAllocator allocator(capacity_per_buffer);

      const auto result = allocator.Allocate(1);
      CHECK_NE(result.ptr, nullptr);
      CHECK_EQ(result.allocated_size, 1);
    }

    SUBCASE("Exact capacity allocation") {
      constexpr size_t capacity_per_buffer = 512;
      DoubleFrameAllocator allocator(capacity_per_buffer);

      const auto result = allocator.Allocate(capacity_per_buffer);
      CHECK_NE(result.ptr, nullptr);

      // Next allocation should fail
      const auto result2 = allocator.Allocate(1);
      CHECK_EQ(result2.ptr, nullptr);
    }
  }
}

}  // namespace
