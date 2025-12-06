#include <doctest/doctest.h>

#include <helios/core/memory/n_frame_allocator.hpp>

#include <cstddef>
#include <cstdint>

namespace {

using namespace helios::memory;

TEST_SUITE("memory::NFrameAllocator") {
  TEST_CASE("Construction") {
    SUBCASE("Triple frame allocator") {
      constexpr size_t capacity_per_buffer = 1024;
      NFrameAllocator<3> allocator(capacity_per_buffer);

      CHECK_EQ(allocator.Capacity(), capacity_per_buffer * 3);
      CHECK_EQ(allocator.CurrentBufferIndex(), 0);
      CHECK_EQ(allocator.BufferCount(), 3);
    }

    SUBCASE("Quad frame allocator") {
      constexpr size_t capacity_per_buffer = 512;
      NFrameAllocator<4> allocator(capacity_per_buffer);

      CHECK_EQ(allocator.Capacity(), capacity_per_buffer * 4);
      CHECK_EQ(allocator.BufferCount(), 4);
    }

    SUBCASE("Single buffer (edge case)") {
      constexpr size_t capacity_per_buffer = 2048;
      NFrameAllocator<1> allocator(capacity_per_buffer);

      CHECK_EQ(allocator.Capacity(), capacity_per_buffer);
      CHECK_EQ(allocator.BufferCount(), 1);
    }

    SUBCASE("Large N value") {
      constexpr size_t capacity_per_buffer = 256;
      NFrameAllocator<8> allocator(capacity_per_buffer);

      CHECK_EQ(allocator.Capacity(), capacity_per_buffer * 8);
      CHECK_EQ(allocator.BufferCount(), 8);
    }

    SUBCASE("Buffer count constant") {
      constexpr size_t N = 5;
      CHECK_EQ(NFrameAllocator<N>::kBufferCount, N);
    }
  }

  TEST_CASE("Basic allocation") {
    constexpr size_t capacity_per_buffer = 2048;
    NFrameAllocator<3> allocator(capacity_per_buffer);

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

  TEST_CASE("Frame rotation") {
    constexpr size_t capacity_per_buffer = 2048;
    constexpr size_t N = 3;
    NFrameAllocator<N> allocator(capacity_per_buffer);

    SUBCASE("NextFrame cycles through all buffers") {
      CHECK_EQ(allocator.CurrentBufferIndex(), 0);

      allocator.NextFrame();
      CHECK_EQ(allocator.CurrentBufferIndex(), 1);

      allocator.NextFrame();
      CHECK_EQ(allocator.CurrentBufferIndex(), 2);

      allocator.NextFrame();
      CHECK_EQ(allocator.CurrentBufferIndex(), 0);
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

    SUBCASE("Previous N-1 frames data remains valid") {
      // Allocate in frame 0
      const auto result0 = allocator.Allocate(sizeof(int));
      auto* const data0 = static_cast<int*>(result0.ptr);
      *data0 = 100;

      allocator.NextFrame();  // To frame 1

      // Allocate in frame 1
      const auto result1 = allocator.Allocate(sizeof(int));
      auto* const data1 = static_cast<int*>(result1.ptr);
      *data1 = 200;

      allocator.NextFrame();  // To frame 2

      // Allocate in frame 2
      const auto result2 = allocator.Allocate(sizeof(int));
      auto* const data2 = static_cast<int*>(result2.ptr);
      *data2 = 300;

      // All three frames' data should be valid
      CHECK_EQ(*data0, 100);
      CHECK_EQ(*data1, 200);
      CHECK_EQ(*data2, 300);

      allocator.NextFrame();  // Back to frame 0 (overwrites old frame 0)

      // Frames 1 and 2 should still be valid
      CHECK_EQ(*data1, 200);
      CHECK_EQ(*data2, 300);
    }
  }

  TEST_CASE("Alignment") {
    constexpr size_t capacity_per_buffer = 4096;
    NFrameAllocator<3> allocator(capacity_per_buffer);

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

    SUBCASE("Custom alignment 64") {
      const auto result = allocator.Allocate(100, 64);
      CHECK_NE(result.ptr, nullptr);
      CHECK(IsAligned(result.ptr, 64));
    }

    SUBCASE("Alignment preserved across frames") {
      const auto result1 = allocator.Allocate(100, 64);
      CHECK(IsAligned(result1.ptr, 64));

      allocator.NextFrame();

      const auto result2 = allocator.Allocate(100, 64);
      CHECK(IsAligned(result2.ptr, 64));

      allocator.NextFrame();

      const auto result3 = allocator.Allocate(100, 64);
      CHECK(IsAligned(result3.ptr, 64));
    }
  }

  TEST_CASE("Capacity per buffer") {
    constexpr size_t capacity_per_buffer = 1024;
    NFrameAllocator<3> allocator(capacity_per_buffer);

    SUBCASE("Allocate full buffer capacity") {
      const auto result = allocator.Allocate(capacity_per_buffer);
      CHECK_NE(result.ptr, nullptr);
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

      const auto result3 = allocator.Allocate(100);
      CHECK_EQ(result3.ptr, nullptr);
    }

    SUBCASE("Full capacity across all frames") {
      // Fill frame 0
      const auto result0 = allocator.Allocate(capacity_per_buffer);
      CHECK_NE(result0.ptr, nullptr);

      allocator.NextFrame();

      // Fill frame 1
      const auto result1 = allocator.Allocate(capacity_per_buffer);
      CHECK_NE(result1.ptr, nullptr);

      allocator.NextFrame();

      // Fill frame 2
      const auto result2 = allocator.Allocate(capacity_per_buffer);
      CHECK_NE(result2.ptr, nullptr);

      // All frames should be full but accessible
      CHECK_NE(result0.ptr, nullptr);
      CHECK_NE(result1.ptr, nullptr);
      CHECK_NE(result2.ptr, nullptr);
    }
  }

  TEST_CASE("Reset") {
    constexpr size_t capacity_per_buffer = 2048;
    NFrameAllocator<3> allocator(capacity_per_buffer);

    SUBCASE("Reset clears all buffers") {
      auto result = allocator.Allocate(256);
      allocator.NextFrame();
      result = allocator.Allocate(512);
      allocator.NextFrame();
      result = allocator.Allocate(128);

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
      allocator.NextFrame();
      result = allocator.Allocate(128);

      allocator.Reset();

      result = allocator.Allocate(128);
      CHECK_NE(result.ptr, nullptr);
    }

    SUBCASE("Reset from any buffer index") {
      allocator.NextFrame();
      allocator.NextFrame();
      CHECK_EQ(allocator.CurrentBufferIndex(), 2);

      allocator.Reset();

      const auto result = allocator.Allocate(256);
      CHECK_NE(result.ptr, nullptr);
    }
  }

  TEST_CASE("Statistics") {
    constexpr size_t capacity_per_buffer = 4096;
    NFrameAllocator<3> allocator(capacity_per_buffer);

    SUBCASE("Current frame stats") {
      auto result = allocator.Allocate(256);
      result = allocator.Allocate(512);

      const auto stats = allocator.CurrentFrameStats();
      CHECK_EQ(stats.allocation_count, 2);
      CHECK_GT(stats.total_allocated, 0);
    }

    SUBCASE("Per-buffer stats") {
      auto result = allocator.Allocate(256);
      allocator.NextFrame();
      result = allocator.Allocate(512);
      allocator.NextFrame();
      result = allocator.Allocate(128);

      const auto stats0 = allocator.BufferStats(0);
      const auto stats1 = allocator.BufferStats(1);
      const auto stats2 = allocator.BufferStats(2);

      CHECK_EQ(stats0.allocation_count, 1);
      CHECK_EQ(stats1.allocation_count, 1);
      CHECK_EQ(stats2.allocation_count, 1);
    }

    SUBCASE("Combined stats") {
      auto result = allocator.Allocate(256);
      allocator.NextFrame();
      result = allocator.Allocate(512);
      allocator.NextFrame();
      result = allocator.Allocate(128);

      const auto stats = allocator.Stats();
      CHECK_EQ(stats.allocation_count, 3);
      CHECK_EQ(stats.total_allocations, 3);
    }

    SUBCASE("Peak usage tracking") {
      auto result = allocator.Allocate(512);
      allocator.NextFrame();
      result = allocator.Allocate(1024);
      allocator.NextFrame();
      result = allocator.Allocate(256);

      const auto stats = allocator.Stats();
      CHECK_GE(stats.peak_usage, 1024);
    }
  }

  TEST_CASE("Move semantics") {
    constexpr size_t capacity_per_buffer = 2048;

    SUBCASE("Move construction") {
      NFrameAllocator<3> allocator1(capacity_per_buffer);
      const auto result = allocator1.Allocate(128);

      NFrameAllocator<3> allocator2(std::move(allocator1));

      CHECK_EQ(allocator2.Capacity(), capacity_per_buffer * 3);
      const auto stats = allocator2.Stats();
      CHECK_GT(stats.allocation_count, 0);
    }

    SUBCASE("Move assignment") {
      NFrameAllocator<3> allocator1(capacity_per_buffer);
      const auto result = allocator1.Allocate(256);

      NFrameAllocator<3> allocator2(1024);

      allocator2 = std::move(allocator1);

      CHECK_EQ(allocator2.Capacity(), capacity_per_buffer * 3);
    }
  }

  TEST_CASE("Write and read allocated memory across frames") {
    constexpr size_t capacity_per_buffer = 4096;
    NFrameAllocator<4> allocator(capacity_per_buffer);

    struct TestData {
      int x = 0;
      float y = 0.0F;
      char z = '\0';
    };

    SUBCASE("Data persists for N-1 frames") {
      // Allocate in each frame
      const auto result0 = allocator.Allocate(sizeof(TestData));
      auto* const data0 = static_cast<TestData*>(result0.ptr);
      data0->x = 100;
      data0->y = 1.1F;
      data0->z = 'A';

      allocator.NextFrame();

      const auto result1 = allocator.Allocate(sizeof(TestData));
      auto* const data1 = static_cast<TestData*>(result1.ptr);
      data1->x = 200;
      data1->y = 2.2F;
      data1->z = 'B';

      allocator.NextFrame();

      const auto result2 = allocator.Allocate(sizeof(TestData));
      auto* const data2 = static_cast<TestData*>(result2.ptr);
      data2->x = 300;
      data2->y = 3.3F;
      data2->z = 'C';

      allocator.NextFrame();

      // Verify data in previous frames
      CHECK_EQ(data0->x, 100);
      CHECK_EQ(data0->y, doctest::Approx(1.1F));
      CHECK_EQ(data0->z, 'A');

      CHECK_EQ(data1->x, 200);
      CHECK_EQ(data1->y, doctest::Approx(2.2F));
      CHECK_EQ(data1->z, 'B');

      CHECK_EQ(data2->x, 300);
      CHECK_EQ(data2->y, doctest::Approx(3.3F));
      CHECK_EQ(data2->z, 'C');
    }
  }
}

}  // namespace
