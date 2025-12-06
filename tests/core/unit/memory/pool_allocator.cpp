#include <doctest/doctest.h>

#include <helios/core/memory/pool_allocator.hpp>

#include <cstddef>
#include <cstdint>

namespace {

using namespace helios::memory;

TEST_SUITE("memory::PoolAllocator") {
  TEST_CASE("Construction") {
    SUBCASE("Valid capacity") {
      constexpr size_t block_size = 64;
      constexpr size_t block_count = 16;
      PoolAllocator allocator(block_size, block_count);

      CHECK_GE(allocator.BlockSize(), block_size);
      CHECK_EQ(allocator.BlockCount(), block_count);
      CHECK_EQ(allocator.Capacity(), allocator.BlockSize() * block_count);
      CHECK(allocator.Empty());
      CHECK_FALSE(allocator.Full());
    }

    SUBCASE("Large block size") {
      constexpr size_t block_size = 1024;
      constexpr size_t block_count = 100;
      PoolAllocator allocator(block_size, block_count);

      CHECK_GE(allocator.BlockSize(), block_size);
      CHECK_EQ(allocator.BlockCount(), block_count);
    }

    SUBCASE("Many blocks") {
      constexpr size_t block_size = 32;
      constexpr size_t block_count = 1000;
      PoolAllocator allocator(block_size, block_count);

      CHECK_EQ(allocator.FreeBlockCount(), block_count);
    }

    SUBCASE("Custom alignment") {
      constexpr size_t block_size = 128;
      constexpr size_t block_count = 10;
      constexpr size_t alignment = 64;
      PoolAllocator allocator(block_size, block_count, alignment);

      CHECK_GT(allocator.Capacity(), 0);
    }
  }

  TEST_CASE("Basic allocation") {
    constexpr size_t block_size = 128;
    constexpr size_t block_count = 10;
    PoolAllocator allocator(block_size, block_count);

    SUBCASE("Single allocation") {
      const auto result = allocator.Allocate(64);

      CHECK_NE(result.ptr, nullptr);
      CHECK_EQ(result.allocated_size, allocator.BlockSize());
      CHECK_FALSE(allocator.Empty());
      CHECK_EQ(allocator.FreeBlockCount(), block_count - 1);
    }

    SUBCASE("Multiple allocations") {
      const auto result1 = allocator.Allocate(64);
      const auto result2 = allocator.Allocate(64);
      const auto result3 = allocator.Allocate(64);

      CHECK_NE(result1.ptr, nullptr);
      CHECK_NE(result2.ptr, nullptr);
      CHECK_NE(result3.ptr, nullptr);

      // All pointers should be different
      CHECK_NE(result1.ptr, result2.ptr);
      CHECK_NE(result2.ptr, result3.ptr);
      CHECK_NE(result1.ptr, result3.ptr);

      CHECK_EQ(allocator.UsedBlockCount(), 3);
      CHECK_EQ(allocator.FreeBlockCount(), block_count - 3);
    }

    SUBCASE("Zero size allocation") {
      const auto result = allocator.Allocate(0);
      // Implementation may vary, but should handle gracefully
      CHECK(allocator.Empty());
    }

    SUBCASE("Allocate up to block size") {
      const auto result = allocator.Allocate(block_size);
      CHECK_NE(result.ptr, nullptr);
    }
  }

  TEST_CASE("Alignment") {
    constexpr size_t block_size = 128;
    constexpr size_t block_count = 10;

    SUBCASE("Default alignment") {
      PoolAllocator allocator(block_size, block_count);
      const auto result = allocator.Allocate(64);

      CHECK_NE(result.ptr, nullptr);
      CHECK(IsAligned(result.ptr, kDefaultAlignment));
    }

    SUBCASE("Custom alignment 16") {
      PoolAllocator allocator(block_size, block_count, 16);
      const auto result = allocator.Allocate(64);

      CHECK_NE(result.ptr, nullptr);
      CHECK(IsAligned(result.ptr, 16));
    }

    SUBCASE("Custom alignment 32") {
      PoolAllocator allocator(block_size, block_count, 32);
      const auto result = allocator.Allocate(64);

      CHECK_NE(result.ptr, nullptr);
      CHECK(IsAligned(result.ptr, 32));
    }

    SUBCASE("Custom alignment 64") {
      PoolAllocator allocator(block_size, block_count, 64);
      const auto result = allocator.Allocate(64);

      CHECK_NE(result.ptr, nullptr);
      CHECK(IsAligned(result.ptr, 64));
    }
  }

  TEST_CASE("Capacity exhaustion") {
    constexpr size_t block_size = 128;
    constexpr size_t block_count = 5;
    PoolAllocator allocator(block_size, block_count);

    SUBCASE("Allocate all blocks") {
      for (size_t i = 0; i < block_count; ++i) {
        const auto result = allocator.Allocate(32);
        CHECK_NE(result.ptr, nullptr);
      }

      CHECK(allocator.Full());
      CHECK_EQ(allocator.FreeBlockCount(), 0);
    }

    SUBCASE("Allocation fails when full") {
      for (size_t i = 0; i < block_count; ++i) {
        const auto result = allocator.Allocate(32);
        CHECK_NE(result.ptr, nullptr);
      }

      // Next allocation should fail
      const auto result = allocator.Allocate(32);
      CHECK_EQ(result.ptr, nullptr);
      CHECK_EQ(result.allocated_size, 0);
    }
  }

  TEST_CASE("Deallocation") {
    constexpr size_t block_size = 128;
    constexpr size_t block_count = 10;
    PoolAllocator allocator(block_size, block_count);

    SUBCASE("Single deallocation") {
      const auto result = allocator.Allocate(64);
      CHECK_NE(result.ptr, nullptr);
      CHECK_EQ(allocator.UsedBlockCount(), 1);

      allocator.Deallocate(result.ptr);

      CHECK_EQ(allocator.UsedBlockCount(), 0);
      CHECK(allocator.Empty());
    }

    SUBCASE("Multiple deallocations") {
      const auto result1 = allocator.Allocate(64);
      const auto result2 = allocator.Allocate(64);
      const auto result3 = allocator.Allocate(64);

      CHECK_EQ(allocator.UsedBlockCount(), 3);

      allocator.Deallocate(result1.ptr);
      CHECK_EQ(allocator.UsedBlockCount(), 2);

      allocator.Deallocate(result2.ptr);
      CHECK_EQ(allocator.UsedBlockCount(), 1);

      allocator.Deallocate(result3.ptr);
      CHECK_EQ(allocator.UsedBlockCount(), 0);
      CHECK(allocator.Empty());
    }

    SUBCASE("Deallocation order doesn't matter") {
      const auto result1 = allocator.Allocate(64);
      const auto result2 = allocator.Allocate(64);
      const auto result3 = allocator.Allocate(64);

      // Deallocate in different order
      allocator.Deallocate(result2.ptr);
      allocator.Deallocate(result3.ptr);
      allocator.Deallocate(result1.ptr);

      CHECK(allocator.Empty());
    }

    SUBCASE("Deallocate nullptr is no-op") {
      const auto count_before = allocator.UsedBlockCount();
      allocator.Deallocate(nullptr);
      const auto count_after = allocator.UsedBlockCount();

      CHECK_EQ(count_before, count_after);
    }
  }

  TEST_CASE("Reuse after deallocation") {
    constexpr size_t block_size = 128;
    constexpr size_t block_count = 10;
    PoolAllocator allocator(block_size, block_count);

    SUBCASE("Deallocated block can be reused") {
      const auto result1 = allocator.Allocate(64);
      void* const ptr1 = result1.ptr;
      CHECK_NE(ptr1, nullptr);

      allocator.Deallocate(ptr1);
      CHECK(allocator.Empty());

      const auto result2 = allocator.Allocate(64);
      void* const ptr2 = result2.ptr;
      CHECK_NE(ptr2, nullptr);

      // Memory should be reused (should get same pointer)
      CHECK_EQ(ptr1, ptr2);
      CHECK_EQ(allocator.UsedBlockCount(), 1);
    }

    SUBCASE("Multiple allocate-deallocate cycles") {
      void* ptrs[block_count] = {};

      for (int cycle = 0; cycle < 3; ++cycle) {
        // Allocate all blocks
        for (size_t i = 0; i < block_count; ++i) {
          const auto result = allocator.Allocate(64);
          CHECK_NE(result.ptr, nullptr);
          ptrs[i] = result.ptr;
        }

        CHECK(allocator.Full());

        // Deallocate all
        for (size_t i = 0; i < block_count; ++i) {
          allocator.Deallocate(ptrs[i]);
        }

        CHECK(allocator.Empty());
      }
    }

    SUBCASE("Partial deallocation and reuse") {
      void* ptrs[block_count] = {};

      for (size_t i = 0; i < block_count; ++i) {
        const auto result = allocator.Allocate(64);
        ptrs[i] = result.ptr;
      }

      // Deallocate half
      for (size_t i = 0; i < block_count / 2; ++i) {
        allocator.Deallocate(ptrs[i]);
      }

      // Should be able to allocate again
      const auto result = allocator.Allocate(64);
      CHECK_NE(result.ptr, nullptr);
    }
  }

  TEST_CASE("Reset") {
    constexpr size_t block_size = 128;
    constexpr size_t block_count = 10;
    PoolAllocator allocator(block_size, block_count);

    SUBCASE("Reset after allocations") {
      auto result = allocator.Allocate(64);
      result = allocator.Allocate(64);
      result = allocator.Allocate(64);

      CHECK_FALSE(allocator.Empty());

      allocator.Reset();

      CHECK(allocator.Empty());
      CHECK_EQ(allocator.UsedBlockCount(), 0);
      CHECK_EQ(allocator.FreeBlockCount(), block_count);
    }

    SUBCASE("Can allocate after reset") {
      for (size_t i = 0; i < block_count; ++i) {
        const auto result = allocator.Allocate(64);
        CHECK_NE(result.ptr, nullptr);
      }

      allocator.Reset();

      // Should be able to allocate again
      const auto result = allocator.Allocate(64);
      CHECK_NE(result.ptr, nullptr);
    }

    SUBCASE("Reset empty allocator") {
      CHECK(allocator.Empty());
      allocator.Reset();
      CHECK(allocator.Empty());
    }
  }

  TEST_CASE("Statistics") {
    constexpr size_t block_size = 128;
    constexpr size_t block_count = 10;
    PoolAllocator allocator(block_size, block_count);

    SUBCASE("Initial stats") {
      const auto stats = allocator.Stats();
      CHECK_EQ(stats.allocation_count, 0);
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
    }

    SUBCASE("Stats after allocations") {
      auto result = allocator.Allocate(64);
      result = allocator.Allocate(64);

      const auto stats = allocator.Stats();
      CHECK_EQ(stats.allocation_count, 2);
      CHECK_EQ(stats.total_allocations, 2);
    }

    SUBCASE("Stats after deallocations") {
      const auto result1 = allocator.Allocate(64);
      const auto result2 = allocator.Allocate(64);

      allocator.Deallocate(result2.ptr);

      const auto stats = allocator.Stats();
      CHECK_EQ(stats.allocation_count, 1);
      CHECK_EQ(stats.total_allocations, 2);
      CHECK_EQ(stats.total_deallocations, 1);
    }

    SUBCASE("Peak usage tracking") {
      auto result = allocator.Allocate(64);
      result = allocator.Allocate(64);

      const auto stats1 = allocator.Stats();
      const auto peak1 = stats1.peak_usage;

      result = allocator.Allocate(64);

      const auto stats2 = allocator.Stats();
      const auto peak2 = stats2.peak_usage;

      CHECK_GE(peak2, peak1);
    }
  }

  TEST_CASE("Ownership checking") {
    constexpr size_t block_size = 128;
    constexpr size_t block_count = 10;
    PoolAllocator allocator(block_size, block_count);

    SUBCASE("Owns allocated pointer") {
      const auto result = allocator.Allocate(64);
      CHECK_NE(result.ptr, nullptr);
      CHECK(allocator.Owns(result.ptr));
    }

    SUBCASE("Does not own external pointer") {
      const int external = 42;
      CHECK_FALSE(allocator.Owns(&external));
    }

    SUBCASE("Does not own nullptr") {
      CHECK_FALSE(allocator.Owns(nullptr));
    }

    SUBCASE("Owns pointer after deallocation") {
      const auto result = allocator.Allocate(64);
      void* const ptr = result.ptr;

      allocator.Deallocate(ptr);

      // Still owns the memory region
      CHECK(allocator.Owns(ptr));
    }
  }

  TEST_CASE("Move semantics") {
    constexpr size_t block_size = 128;
    constexpr size_t block_count = 10;

    SUBCASE("Move construction") {
      PoolAllocator allocator1(block_size, block_count);
      auto result = allocator1.Allocate(64);
      result = allocator1.Allocate(64);

      const auto used1 = allocator1.UsedBlockCount();

      PoolAllocator allocator2(std::move(allocator1));

      CHECK_EQ(allocator2.BlockCount(), block_count);
      CHECK_EQ(allocator2.UsedBlockCount(), used1);
      CHECK_FALSE(allocator2.Empty());
    }

    SUBCASE("Move assignment") {
      PoolAllocator allocator1(block_size, block_count);
      const auto result = allocator1.Allocate(64);

      PoolAllocator allocator2(64, 5);

      allocator2 = std::move(allocator1);

      CHECK_EQ(allocator2.BlockCount(), block_count);
    }
  }

  TEST_CASE("Write and read allocated memory") {
    constexpr size_t block_size = 128;
    constexpr size_t block_count = 10;
    PoolAllocator allocator(block_size, block_count);

    SUBCASE("Write and read int") {
      const auto result = allocator.Allocate(sizeof(int));
      CHECK_NE(result.ptr, nullptr);

      auto* const data = static_cast<int*>(result.ptr);
      *data = 42;

      CHECK_EQ(*data, 42);

      allocator.Deallocate(result.ptr);
    }

    SUBCASE("Write and read struct") {
      struct TestStruct {
        int x = 0;
        float y = 0.0F;
        char z = '\0';
      };

      const auto result = allocator.Allocate(sizeof(TestStruct));
      CHECK_NE(result.ptr, nullptr);

      auto* const data = static_cast<TestStruct*>(result.ptr);
      data->x = 100;
      data->y = 3.14F;
      data->z = 'X';

      CHECK_EQ(data->x, 100);
      CHECK_EQ(data->y, doctest::Approx(3.14F));
      CHECK_EQ(data->z, 'X');

      allocator.Deallocate(result.ptr);
    }

    SUBCASE("Multiple allocations with different data") {
      struct Data {
        int value = 0;
      };

      void* ptrs[5] = {};
      for (int i = 0; i < 5; ++i) {
        const auto result = allocator.Allocate(sizeof(Data));
        CHECK_NE(result.ptr, nullptr);
        ptrs[i] = result.ptr;

        auto* const data = static_cast<Data*>(result.ptr);
        data->value = i * 10;
      }

      // Verify all values
      for (int i = 0; i < 5; ++i) {
        auto* const data = static_cast<Data*>(ptrs[i]);
        CHECK_EQ(data->value, i * 10);
      }

      // Clean up
      for (int i = 0; i < 5; ++i) {
        allocator.Deallocate(ptrs[i]);
      }
    }
  }

  TEST_CASE("Boundary conditions") {
    SUBCASE("Minimum block count") {
      constexpr size_t block_size = 64;
      constexpr size_t block_count = 1;
      PoolAllocator allocator(block_size, block_count);

      const auto result = allocator.Allocate(32);
      CHECK_NE(result.ptr, nullptr);

      allocator.Deallocate(result.ptr);
    }

    SUBCASE("Small block size") {
      constexpr size_t block_size = 8;
      constexpr size_t block_count = 10;
      PoolAllocator allocator(block_size, block_count);

      const auto result = allocator.Allocate(4);
      CHECK_NE(result.ptr, nullptr);

      allocator.Deallocate(result.ptr);
    }

    SUBCASE("Large block size") {
      constexpr size_t block_size = 4096;
      constexpr size_t block_count = 10;
      PoolAllocator allocator(block_size, block_count);

      const auto result = allocator.Allocate(2048);
      CHECK_NE(result.ptr, nullptr);

      allocator.Deallocate(result.ptr);
    }
  }

  TEST_CASE("Stress test") {
    constexpr size_t block_size = 256;
    constexpr size_t block_count = 100;
    PoolAllocator allocator(block_size, block_count);

    SUBCASE("Random allocations and deallocations") {
      void* ptrs[block_count] = {};
      size_t allocated = 0;

      for (int cycle = 0; cycle < 1000; ++cycle) {
        if (allocated < block_count && (cycle % 3 != 0 || allocated == 0)) {
          // Allocate
          const auto result = allocator.Allocate(128);
          if (result.ptr) {
            ptrs[allocated++] = result.ptr;
          }
        } else if (allocated > 0) {
          // Deallocate
          allocator.Deallocate(ptrs[--allocated]);
        }
      }

      // Clean up remaining
      for (size_t i = 0; i < allocated; ++i) {
        allocator.Deallocate(ptrs[i]);
      }

      CHECK(allocator.Empty());
    }
  }
}

}  // namespace
