#include <doctest/doctest.h>

#include <helios/core/memory/free_list_allocator.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

using namespace helios::memory;

TEST_SUITE("memory::FreeListAllocator") {
  TEST_CASE("Construction") {
    SUBCASE("Valid capacity") {
      constexpr size_t capacity = 4096;
      FreeListAllocator allocator(capacity);

      CHECK_EQ(allocator.Capacity(), capacity);
      CHECK(allocator.Empty());
      CHECK_EQ(allocator.UsedMemory(), 0);
      CHECK_EQ(allocator.FreeMemory(), capacity);
      CHECK_EQ(allocator.AllocationCount(), 0);
    }

    SUBCASE("Large capacity") {
      constexpr size_t capacity = 1024 * 1024;
      FreeListAllocator allocator(capacity);

      CHECK_EQ(allocator.Capacity(), capacity);
      CHECK_EQ(allocator.FreeMemory(), capacity);
    }

    SUBCASE("Small capacity") {
      constexpr size_t capacity = 512;
      FreeListAllocator allocator(capacity);

      CHECK_EQ(allocator.Capacity(), capacity);
    }
  }

  TEST_CASE("Basic allocation") {
    constexpr size_t capacity = 8192;
    FreeListAllocator allocator(capacity);

    SUBCASE("Single allocation") {
      const auto result = allocator.Allocate(64);

      CHECK_NE(result.ptr, nullptr);
      CHECK_EQ(result.allocated_size, 64);
      CHECK_FALSE(allocator.Empty());
      CHECK_EQ(allocator.AllocationCount(), 1);
      CHECK_GT(allocator.UsedMemory(), 0);
    }

    SUBCASE("Multiple allocations") {
      const auto result1 = allocator.Allocate(64);
      const auto result2 = allocator.Allocate(128);
      const auto result3 = allocator.Allocate(256);

      CHECK_NE(result1.ptr, nullptr);
      CHECK_NE(result2.ptr, nullptr);
      CHECK_NE(result3.ptr, nullptr);

      // All pointers should be different
      CHECK_NE(result1.ptr, result2.ptr);
      CHECK_NE(result2.ptr, result3.ptr);
      CHECK_NE(result1.ptr, result3.ptr);

      CHECK_EQ(allocator.AllocationCount(), 3);
    }

    SUBCASE("Zero size allocation") {
      const auto result = allocator.Allocate(0);

      CHECK_EQ(result.ptr, nullptr);
      CHECK_EQ(result.allocated_size, 0);
      CHECK(allocator.Empty());
    }

    SUBCASE("Variable size allocations") {
      const auto result1 = allocator.Allocate(16);
      const auto result2 = allocator.Allocate(1024);
      const auto result3 = allocator.Allocate(8);
      const auto result4 = allocator.Allocate(512);

      CHECK_NE(result1.ptr, nullptr);
      CHECK_NE(result2.ptr, nullptr);
      CHECK_NE(result3.ptr, nullptr);
      CHECK_NE(result4.ptr, nullptr);
    }
  }

  TEST_CASE("Alignment") {
    constexpr size_t capacity = 8192;

    SUBCASE("Default alignment") {
      FreeListAllocator allocator(capacity);
      const auto result = allocator.Allocate(64);

      CHECK_NE(result.ptr, nullptr);
      CHECK(IsAligned(result.ptr, kDefaultAlignment));
    }

    SUBCASE("Custom alignment 16") {
      FreeListAllocator allocator(capacity);
      const auto result = allocator.Allocate(64, 16);

      CHECK_NE(result.ptr, nullptr);
      CHECK(IsAligned(result.ptr, 16));
    }

    SUBCASE("Custom alignment 32") {
      FreeListAllocator allocator(capacity);
      const auto result = allocator.Allocate(64, 32);

      CHECK_NE(result.ptr, nullptr);
      CHECK(IsAligned(result.ptr, 32));
    }

    SUBCASE("Custom alignment 64") {
      FreeListAllocator allocator(capacity);
      const auto result = allocator.Allocate(64, 64);

      CHECK_NE(result.ptr, nullptr);
      CHECK(IsAligned(result.ptr, 64));
    }

    SUBCASE("Multiple allocations with different alignments") {
      FreeListAllocator allocator(capacity);
      const auto result1 = allocator.Allocate(10, 16);
      const auto result2 = allocator.Allocate(20, 32);
      const auto result3 = allocator.Allocate(30, 64);

      CHECK_NE(result1.ptr, nullptr);
      CHECK_NE(result2.ptr, nullptr);
      CHECK_NE(result3.ptr, nullptr);

      CHECK(IsAligned(result1.ptr, 16));
      CHECK(IsAligned(result2.ptr, 32));
      CHECK(IsAligned(result3.ptr, 64));
    }
  }

  TEST_CASE("Capacity exhaustion") {
    constexpr size_t capacity = 2048;
    FreeListAllocator allocator(capacity);

    SUBCASE("Allocate until full") {
      size_t allocated = 0;
      AllocationResult result;

      while ((result = allocator.Allocate(64)).ptr != nullptr) {
        ++allocated;
      }

      CHECK_GT(allocated, 0);
      CHECK_LT(allocator.FreeMemory(), 200);  // Some space left due to headers
    }

    SUBCASE("Allocation fails when insufficient space") {
      // Allocate most of the space
      const auto result1 = allocator.Allocate(1800);
      CHECK_NE(result1.ptr, nullptr);

      // This should fail
      const auto result2 = allocator.Allocate(500);
      CHECK_EQ(result2.ptr, nullptr);
      CHECK_EQ(result2.allocated_size, 0);
    }

    SUBCASE("Large allocation") {
      const auto result = allocator.Allocate(capacity - 200);
      CHECK_NE(result.ptr, nullptr);

      // Small allocation should fail
      const auto result2 = allocator.Allocate(300);
      CHECK_EQ(result2.ptr, nullptr);
    }
  }

  TEST_CASE("Deallocation") {
    constexpr size_t capacity = 8192;
    FreeListAllocator allocator(capacity);

    SUBCASE("Single deallocation") {
      const auto result = allocator.Allocate(64);
      CHECK_NE(result.ptr, nullptr);
      CHECK_EQ(allocator.AllocationCount(), 1);

      allocator.Deallocate(result.ptr);

      CHECK_EQ(allocator.AllocationCount(), 0);
      CHECK(allocator.Empty());
    }

    SUBCASE("Multiple deallocations") {
      const auto result1 = allocator.Allocate(64);
      const auto result2 = allocator.Allocate(128);
      const auto result3 = allocator.Allocate(256);

      CHECK_EQ(allocator.AllocationCount(), 3);

      allocator.Deallocate(result1.ptr);
      CHECK_EQ(allocator.AllocationCount(), 2);

      allocator.Deallocate(result2.ptr);
      CHECK_EQ(allocator.AllocationCount(), 1);

      allocator.Deallocate(result3.ptr);
      CHECK_EQ(allocator.AllocationCount(), 0);
      CHECK(allocator.Empty());
    }

    SUBCASE("Deallocation in reverse order") {
      const auto result1 = allocator.Allocate(64);
      const auto result2 = allocator.Allocate(128);
      const auto result3 = allocator.Allocate(256);

      // Deallocate in reverse order
      allocator.Deallocate(result3.ptr);
      allocator.Deallocate(result2.ptr);
      allocator.Deallocate(result1.ptr);

      CHECK(allocator.Empty());
    }

    SUBCASE("Deallocation in random order") {
      const auto result1 = allocator.Allocate(64);
      const auto result2 = allocator.Allocate(128);
      const auto result3 = allocator.Allocate(256);
      const auto result4 = allocator.Allocate(512);

      // Deallocate in random order
      allocator.Deallocate(result2.ptr);
      allocator.Deallocate(result4.ptr);
      allocator.Deallocate(result1.ptr);
      allocator.Deallocate(result3.ptr);

      CHECK(allocator.Empty());
    }

    SUBCASE("Deallocate nullptr is no-op") {
      const auto count_before = allocator.AllocationCount();
      allocator.Deallocate(nullptr);
      const auto count_after = allocator.AllocationCount();

      CHECK_EQ(count_before, count_after);
    }
  }

  TEST_CASE("Reuse after deallocation") {
    constexpr size_t capacity = 8192;
    FreeListAllocator allocator(capacity);

    SUBCASE("Deallocated memory can be reused") {
      const auto result1 = allocator.Allocate(128);
      void* const ptr1 = result1.ptr;
      CHECK_NE(ptr1, nullptr);

      allocator.Deallocate(ptr1);
      CHECK(allocator.Empty());

      const auto result2 = allocator.Allocate(128);
      void* const ptr2 = result2.ptr;
      CHECK_NE(ptr2, nullptr);

      // Memory should be reused (though not necessarily same address)
      CHECK_EQ(allocator.AllocationCount(), 1);
    }

    SUBCASE("Multiple allocate-deallocate cycles") {
      for (int cycle = 0; cycle < 3; ++cycle) {
        std::vector<void*> ptrs;

        // Allocate multiple blocks
        for (int i = 0; i < 3; ++i) {
          const auto result = allocator.Allocate(128);
          CHECK_NE(result.ptr, nullptr);
          ptrs.push_back(result.ptr);
        }

        CHECK(allocator.AllocationCount() == 3);

        // Deallocate all
        for (void* const ptr : ptrs) {
          allocator.Deallocate(ptr);
        }

        CHECK(allocator.Empty());
      }
    }

    SUBCASE("Best-fit allocation after fragmentation") {
      // Create fragmentation
      const auto result1 = allocator.Allocate(64);
      const auto result2 = allocator.Allocate(128);
      const auto result3 = allocator.Allocate(256);

      // Free middle block
      allocator.Deallocate(result2.ptr);

      // Allocate something that fits in the freed space
      const auto result4 = allocator.Allocate(64);
      CHECK_NE(result4.ptr, nullptr);

      allocator.Deallocate(result1.ptr);
      allocator.Deallocate(result3.ptr);
      allocator.Deallocate(result4.ptr);
    }
  }

  TEST_CASE("Coalescing") {
    constexpr size_t capacity = 8192;
    FreeListAllocator allocator(capacity);

    SUBCASE("Adjacent blocks coalesce") {
      // Allocate three adjacent blocks
      const auto result1 = allocator.Allocate(128);
      const auto result2 = allocator.Allocate(128);
      const auto result3 = allocator.Allocate(128);

      CHECK_EQ(allocator.AllocationCount(), 3);

      // Free them - they should coalesce
      allocator.Deallocate(result1.ptr);
      allocator.Deallocate(result2.ptr);
      allocator.Deallocate(result3.ptr);

      CHECK(allocator.Empty());

      // Should be able to allocate larger block now
      const auto result4 = allocator.Allocate(384);
      CHECK_NE(result4.ptr, nullptr);

      allocator.Deallocate(result4.ptr);
    }

    SUBCASE("Non-adjacent blocks don't coalesce incorrectly") {
      const auto result1 = allocator.Allocate(128);
      const auto result2 = allocator.Allocate(128);
      const auto result3 = allocator.Allocate(128);

      // Free first and third, keep second
      allocator.Deallocate(result1.ptr);
      allocator.Deallocate(result3.ptr);

      CHECK_EQ(allocator.AllocationCount(), 1);

      // Clean up
      allocator.Deallocate(result2.ptr);
    }
  }

  TEST_CASE("Reset") {
    constexpr size_t capacity = 8192;
    FreeListAllocator allocator(capacity);

    SUBCASE("Reset after allocations") {
      auto result = allocator.Allocate(64);
      result = allocator.Allocate(128);
      result = allocator.Allocate(256);

      CHECK_FALSE(allocator.Empty());

      allocator.Reset();

      CHECK(allocator.Empty());
      CHECK_EQ(allocator.UsedMemory(), 0);
      CHECK_EQ(allocator.FreeMemory(), capacity);
      CHECK_EQ(allocator.AllocationCount(), 0);
    }

    SUBCASE("Can allocate after reset") {
      // Fill some space
      for (int i = 0; i < 10; ++i) {
        const auto result = allocator.Allocate(256);
      }

      allocator.Reset();

      // Should be able to allocate full capacity
      const auto result = allocator.Allocate(capacity - 200);
      CHECK_NE(result.ptr, nullptr);
    }

    SUBCASE("Reset empty allocator") {
      CHECK(allocator.Empty());
      allocator.Reset();
      CHECK(allocator.Empty());
    }
  }

  TEST_CASE("Statistics") {
    constexpr size_t capacity = 8192;
    FreeListAllocator allocator(capacity);

    SUBCASE("Initial stats") {
      const auto stats = allocator.Stats();
      CHECK_EQ(stats.allocation_count, 0);
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
      CHECK_EQ(stats.total_allocated, 0);
    }

    SUBCASE("Stats after allocations") {
      auto result = allocator.Allocate(64);
      result = allocator.Allocate(128);

      const auto stats = allocator.Stats();
      CHECK_EQ(stats.allocation_count, 2);
      CHECK_EQ(stats.total_allocations, 2);
      CHECK_GT(stats.total_allocated, 0);
    }

    SUBCASE("Stats after deallocations") {
      const auto result1 = allocator.Allocate(64);
      const auto result2 = allocator.Allocate(128);

      allocator.Deallocate(result2.ptr);

      const auto stats = allocator.Stats();
      CHECK_EQ(stats.allocation_count, 1);
      CHECK_EQ(stats.total_allocations, 2);
      CHECK_EQ(stats.total_deallocations, 1);
    }

    SUBCASE("Peak usage tracking") {
      [[maybe_unused]] const auto r1 = allocator.Allocate(256);
      [[maybe_unused]] const auto r2 = allocator.Allocate(512);

      const auto stats1 = allocator.Stats();
      const auto peak1 = stats1.peak_usage;

      [[maybe_unused]] const auto r3 = allocator.Allocate(1024);

      const auto stats2 = allocator.Stats();
      const auto peak2 = stats2.peak_usage;

      CHECK_GE(peak2, peak1);

      // Reset should not clear peak
      allocator.Reset();
      const auto stats3 = allocator.Stats();
      CHECK_EQ(stats3.peak_usage, peak2);
    }

    SUBCASE("Free block count tracking") {
      CHECK_EQ(allocator.FreeBlockCount(), 1);  // Initially one large block

      const auto result1 = allocator.Allocate(128);
      const auto result2 = allocator.Allocate(128);

      // Deallocate creating fragmentation
      allocator.Deallocate(result1.ptr);

      CHECK_GE(allocator.FreeBlockCount(), 1);

      // Clean up
      allocator.Deallocate(result2.ptr);
    }
  }

  TEST_CASE("Ownership checking") {
    constexpr size_t capacity = 8192;
    FreeListAllocator allocator(capacity);

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

    SUBCASE("Owns multiple allocations") {
      const auto result1 = allocator.Allocate(64);
      const auto result2 = allocator.Allocate(128);
      const auto result3 = allocator.Allocate(256);

      CHECK(allocator.Owns(result1.ptr));
      CHECK(allocator.Owns(result2.ptr));
      CHECK(allocator.Owns(result3.ptr));

      allocator.Deallocate(result1.ptr);
      allocator.Deallocate(result2.ptr);
      allocator.Deallocate(result3.ptr);
    }
  }

  TEST_CASE("Move semantics") {
    constexpr size_t capacity = 8192;

    SUBCASE("Move construction") {
      FreeListAllocator allocator1(capacity);
      [[maybe_unused]] const auto r1 = allocator1.Allocate(64);
      [[maybe_unused]] const auto r2 = allocator1.Allocate(128);

      const auto used1 = allocator1.UsedMemory();

      FreeListAllocator allocator2(std::move(allocator1));

      CHECK_EQ(allocator2.Capacity(), capacity);
      CHECK_EQ(allocator2.UsedMemory(), used1);
      CHECK_FALSE(allocator2.Empty());
    }

    SUBCASE("Move assignment") {
      FreeListAllocator allocator1(capacity);
      [[maybe_unused]] const auto r1 = allocator1.Allocate(64);

      FreeListAllocator allocator2(1024);

      allocator2 = std::move(allocator1);

      CHECK_EQ(allocator2.Capacity(), capacity);
    }

    SUBCASE("Self move assignment") {
      FreeListAllocator allocator(capacity);
      [[maybe_unused]] const auto r1 = allocator.Allocate(64);

      auto* const ptr = &allocator;
      allocator = std::move(*ptr);

      // Should still be valid
      CHECK_EQ(allocator.Capacity(), capacity);
    }
  }

  TEST_CASE("Write and read allocated memory") {
    constexpr size_t capacity = 8192;
    FreeListAllocator allocator(capacity);

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

      std::vector<void*> ptrs;
      for (int i = 0; i < 5; ++i) {
        const auto result = allocator.Allocate(sizeof(Data));
        CHECK_NE(result.ptr, nullptr);
        ptrs.push_back(result.ptr);

        auto* const data = static_cast<Data*>(result.ptr);
        data->value = i * 10;
      }

      // Verify all values
      for (int i = 0; i < 5; ++i) {
        auto* const data = static_cast<Data*>(ptrs[i]);
        CHECK_EQ(data->value, i * 10);
      }

      // Clean up
      for (void* const ptr : ptrs) {
        allocator.Deallocate(ptr);
      }
    }

    SUBCASE("Write array of data") {
      constexpr size_t array_size = 100;
      const auto result = allocator.Allocate(sizeof(int) * array_size);
      CHECK_NE(result.ptr, nullptr);

      auto* const array = static_cast<int*>(result.ptr);
      for (size_t i = 0; i < array_size; ++i) {
        array[i] = static_cast<int>(i);
      }

      // Verify all values
      for (size_t i = 0; i < array_size; ++i) {
        CHECK_EQ(array[i], static_cast<int>(i));
      }

      allocator.Deallocate(result.ptr);
    }
  }

  TEST_CASE("Boundary conditions") {
    SUBCASE("Minimum capacity") {
      constexpr size_t capacity = 256;
      FreeListAllocator allocator(capacity);

      const auto result = allocator.Allocate(64);
      CHECK_NE(result.ptr, nullptr);

      allocator.Deallocate(result.ptr);
    }

    SUBCASE("Single byte allocation") {
      constexpr size_t capacity = 1024;
      FreeListAllocator allocator(capacity);

      const auto result = allocator.Allocate(1);
      CHECK_NE(result.ptr, nullptr);
      CHECK_EQ(result.allocated_size, 1);

      allocator.Deallocate(result.ptr);
    }

    SUBCASE("Large allocation") {
      constexpr size_t capacity = 4096;
      FreeListAllocator allocator(capacity);

      const auto result = allocator.Allocate(capacity - 200);
      CHECK_NE(result.ptr, nullptr);

      allocator.Deallocate(result.ptr);
    }
  }

  TEST_CASE("Fragmentation scenarios") {
    constexpr size_t capacity = 8192;
    FreeListAllocator allocator(capacity);

    SUBCASE("Checkerboard fragmentation") {
      std::vector<void*> keep_ptrs;
      std::vector<void*> free_ptrs;

      // Allocate many blocks
      for (int i = 0; i < 20; ++i) {
        const auto result = allocator.Allocate(128);
        if (result.ptr) {
          if (i % 2 == 0) {
            keep_ptrs.push_back(result.ptr);
          } else {
            free_ptrs.push_back(result.ptr);
          }
        }
      }

      // Free every other block
      for (void* const ptr : free_ptrs) {
        allocator.Deallocate(ptr);
      }

      // Should still be able to allocate small blocks
      const auto result = allocator.Allocate(64);
      CHECK_NE(result.ptr, nullptr);

      // Clean up
      if (result.ptr) {
        allocator.Deallocate(result.ptr);
      }
      for (void* const ptr : keep_ptrs) {
        allocator.Deallocate(ptr);
      }
    }

    SUBCASE("Worst-case fragmentation") {
      std::vector<void*> ptrs;

      // Allocate many small blocks
      for (int i = 0; i < 10; ++i) {
        const auto result = allocator.Allocate(64);
        if (result.ptr) {
          ptrs.push_back(result.ptr);
        }
      }

      // Free all but one in the middle
      for (size_t i = 0; i < ptrs.size(); ++i) {
        if (i != ptrs.size() / 2) {
          allocator.Deallocate(ptrs[i]);
        }
      }

      // Clean up remaining
      allocator.Deallocate(ptrs[ptrs.size() / 2]);
    }
  }

  TEST_CASE("Stress test") {
    constexpr size_t capacity = 65536;  // 64KB
    FreeListAllocator allocator(capacity);

    SUBCASE("Many random allocations and deallocations") {
      std::vector<std::pair<void*, size_t>> allocations;

      for (int cycle = 0; cycle < 100; ++cycle) {
        // Random allocation size
        const size_t size = 16 + (cycle % 7) * 16;
        const auto result = allocator.Allocate(size);

        if (result.ptr) {
          allocations.push_back({result.ptr, size});
        }

        // Randomly deallocate some
        if (!allocations.empty() && cycle % 3 == 0) {
          const auto [ptr, alloc_size] = allocations.back();
          allocator.Deallocate(ptr);
          allocations.pop_back();
        }
      }

      // Clean up remaining
      for (const auto [ptr, size] : allocations) {
        allocator.Deallocate(ptr);
      }

      CHECK(allocator.Empty());
    }

    SUBCASE("Repeated full utilization") {
      for (int cycle = 0; cycle < 10; ++cycle) {
        std::vector<void*> ptrs;

        // Fill allocator
        while (true) {
          const auto result = allocator.Allocate(256);
          if (result.ptr == nullptr) {
            break;
          }
          ptrs.push_back(result.ptr);
        }

        CHECK(ptrs.size() > 0);

        // Free all
        for (void* const ptr : ptrs) {
          allocator.Deallocate(ptr);
        }

        CHECK(allocator.Empty());
      }
    }
  }
}

}  // namespace
