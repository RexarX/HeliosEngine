#include <doctest/doctest.h>

#include <helios/core/memory/stack_allocator.hpp>

#include <cstddef>
#include <cstdint>

namespace {

using namespace helios::memory;

TEST_SUITE("memory::StackAllocator") {
  TEST_CASE("Construction") {
    SUBCASE("Valid capacity") {
      constexpr size_t capacity = 1024;
      StackAllocator allocator(capacity);

      CHECK_EQ(allocator.Capacity(), capacity);
      CHECK(allocator.Empty());
      CHECK_FALSE(allocator.Full());
      CHECK_EQ(allocator.CurrentOffset(), 0);
      CHECK_EQ(allocator.FreeSpace(), capacity);
    }

    SUBCASE("Large capacity") {
      constexpr size_t capacity = 1024 * 1024;
      StackAllocator allocator(capacity);

      CHECK_EQ(allocator.Capacity(), capacity);
      CHECK_EQ(allocator.FreeSpace(), capacity);
    }

    SUBCASE("Small capacity") {
      constexpr size_t capacity = 128;
      StackAllocator allocator(capacity);

      CHECK_EQ(allocator.Capacity(), capacity);
    }
  }

  TEST_CASE("Basic allocation") {
    constexpr size_t capacity = 4096;
    StackAllocator allocator(capacity);

    SUBCASE("Single allocation") {
      const auto result = allocator.Allocate(64);

      CHECK_NE(result.ptr, nullptr);
      CHECK_EQ(result.allocated_size, 64);
      CHECK_FALSE(allocator.Empty());
      CHECK_GT(allocator.CurrentOffset(), 64);
    }

    SUBCASE("Multiple sequential allocations") {
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

      // Pointers should be in order (later allocations have higher addresses)
      CHECK_LT(static_cast<uint8_t*>(result1.ptr), static_cast<uint8_t*>(result2.ptr));
      CHECK_LT(static_cast<uint8_t*>(result2.ptr), static_cast<uint8_t*>(result3.ptr));
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

      CHECK_NE(result1.ptr, nullptr);
      CHECK_NE(result2.ptr, nullptr);
      CHECK_NE(result3.ptr, nullptr);
    }
  }

  TEST_CASE("Alignment") {
    constexpr size_t capacity = 4096;

    SUBCASE("Default alignment") {
      StackAllocator allocator(capacity);
      const auto result = allocator.Allocate(64);

      CHECK_NE(result.ptr, nullptr);
      CHECK(IsAligned(result.ptr, kDefaultAlignment));
    }

    SUBCASE("Custom alignment 16") {
      StackAllocator allocator(capacity);
      const auto result = allocator.Allocate(64, 16);

      CHECK_NE(result.ptr, nullptr);
      CHECK(IsAligned(result.ptr, 16));
    }

    SUBCASE("Custom alignment 32") {
      StackAllocator allocator(capacity);
      const auto result = allocator.Allocate(64, 32);

      CHECK_NE(result.ptr, nullptr);
      CHECK(IsAligned(result.ptr, 32));
    }

    SUBCASE("Custom alignment 64") {
      StackAllocator allocator(capacity);
      const auto result = allocator.Allocate(64, 64);

      CHECK_NE(result.ptr, nullptr);
      CHECK(IsAligned(result.ptr, 64));
    }

    SUBCASE("Multiple allocations with different alignments") {
      StackAllocator allocator(capacity);
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
    constexpr size_t capacity = 512;
    StackAllocator allocator(capacity);

    SUBCASE("Allocate until full") {
      size_t allocated = 0;
      AllocationResult result;

      while ((result = allocator.Allocate(64)).ptr != nullptr) {
        ++allocated;
      }

      CHECK_GT(allocated, 0);
      CHECK_LT(allocator.FreeSpace(), 128);  // Some space left due to headers
    }

    SUBCASE("Allocation fails when insufficient space") {
      // Allocate most of the space
      const auto result1 = allocator.Allocate(400);
      CHECK_NE(result1.ptr, nullptr);

      // This should fail
      const auto result2 = allocator.Allocate(200);
      CHECK_EQ(result2.ptr, nullptr);
      CHECK_EQ(result2.allocated_size, 0);
    }

    SUBCASE("Exact capacity allocation") {
      // Try to allocate close to capacity
      const auto result = allocator.Allocate(capacity - 128);
      CHECK_NE(result.ptr, nullptr);

      // Small allocation should still fail
      const auto result2 = allocator.Allocate(256);
      CHECK_EQ(result2.ptr, nullptr);
    }
  }

  TEST_CASE("LIFO deallocation") {
    constexpr size_t capacity = 4096;
    StackAllocator allocator(capacity);

    SUBCASE("Single deallocation") {
      const auto result = allocator.Allocate(64);
      CHECK_NE(result.ptr, nullptr);

      auto offset_before = allocator.CurrentOffset();
      CHECK_FALSE(allocator.Empty());

      allocator.Deallocate(result.ptr, result.allocated_size);

      CHECK(allocator.Empty());
      CHECK_EQ(allocator.CurrentOffset(), 0);
    }

    SUBCASE("LIFO order deallocation") {
      const auto result1 = allocator.Allocate(64);
      const auto result2 = allocator.Allocate(128);
      const auto result3 = allocator.Allocate(256);

      auto offset_after_3 = allocator.CurrentOffset();

      // Deallocate in reverse order
      allocator.Deallocate(result3.ptr, result3.allocated_size);
      auto offset_after_2 = allocator.CurrentOffset();
      CHECK_LT(offset_after_2, offset_after_3);

      allocator.Deallocate(result2.ptr, result2.allocated_size);
      auto offset_after_1 = allocator.CurrentOffset();
      CHECK_LT(offset_after_1, offset_after_2);

      allocator.Deallocate(result1.ptr, result1.allocated_size);
      CHECK(allocator.Empty());
    }

    SUBCASE("Deallocate nullptr is no-op") {
      auto offset_before = allocator.CurrentOffset();
      allocator.Deallocate(nullptr, 0);
      auto offset_after = allocator.CurrentOffset();

      CHECK_EQ(offset_before, offset_after);
    }
  }

  TEST_CASE("Marker-based rewind") {
    constexpr size_t capacity = 4096;
    StackAllocator allocator(capacity);

    SUBCASE("Get marker and rewind") {
      auto marker1 = allocator.Marker();
      CHECK_EQ(marker1, 0);

      auto result = allocator.Allocate(64);
      result = allocator.Allocate(128);

      auto marker2 = allocator.Marker();
      CHECK_GT(marker2, marker1);

      result = allocator.Allocate(256);
      result = allocator.Allocate(512);

      auto marker3 = allocator.Marker();
      CHECK_GT(marker3, marker2);

      // Rewind to marker2
      allocator.RewindToMarker(marker2);
      CHECK_EQ(allocator.CurrentOffset(), marker2);

      // Rewind to beginning
      allocator.RewindToMarker(0);
      CHECK(allocator.Empty());
    }

    SUBCASE("Rewind and reallocate") {
      auto marker = allocator.Marker();

      auto result = allocator.Allocate(64);
      result = allocator.Allocate(128);
      result = allocator.Allocate(512, 64);

      CHECK_FALSE(allocator.Empty());

      allocator.Reset();
      CHECK(allocator.Empty());

      // Should be able to allocate again
      result = allocator.Allocate(64);
      CHECK_NE(result.ptr, nullptr);
    }

    SUBCASE("Partial rewind") {
      auto result = allocator.Allocate(64);
      auto marker = allocator.Marker();

      result = allocator.Allocate(128);
      result = allocator.Allocate(256);

      allocator.RewindToMarker(marker);

      // First allocation should still be "active"
      CHECK_FALSE(allocator.Empty());
      CHECK_EQ(allocator.CurrentOffset(), marker);
    }
  }

  TEST_CASE("Reset") {
    constexpr size_t capacity = 4096;
    StackAllocator allocator(capacity);

    SUBCASE("Reset after allocations") {
      auto result = allocator.Allocate(64);
      result = allocator.Allocate(128);
      result = allocator.Allocate(256);

      CHECK_FALSE(allocator.Empty());

      allocator.Reset();

      CHECK(allocator.Empty());
      CHECK_EQ(allocator.CurrentOffset(), 0);
      CHECK_EQ(allocator.FreeSpace(), capacity);
    }

    SUBCASE("Can allocate after reset") {
      // Fill the allocator
      for (int i = 0; i < 10; ++i) {
        const auto result = allocator.Allocate(256);
      }

      allocator.Reset();

      // Should be able to allocate again
      const auto result = allocator.Allocate(256);
      CHECK_NE(result.ptr, nullptr);
    }

    SUBCASE("Reset empty allocator") {
      CHECK(allocator.Empty());
      allocator.Reset();
      CHECK(allocator.Empty());
    }
  }

  TEST_CASE("Statistics") {
    constexpr size_t capacity = 4096;
    StackAllocator allocator(capacity);

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

      allocator.Deallocate(result2.ptr, result2.allocated_size);

      const auto stats = allocator.Stats();
      CHECK_EQ(stats.allocation_count, 1);
      CHECK_EQ(stats.total_allocations, 2);
      CHECK_EQ(stats.total_deallocations, 1);
    }

    SUBCASE("Peak usage tracking") {
      auto result = allocator.Allocate(64);
      result = allocator.Allocate(128);

      const auto stats1 = allocator.Stats();
      const auto peak1 = stats1.peak_usage;

      result = allocator.Allocate(256);
      result = allocator.Allocate(512);

      const auto stats2 = allocator.Stats();
      const auto peak2 = stats2.peak_usage;

      CHECK_GE(peak2, peak1);

      // Reset should not clear peak
      allocator.Reset();
      const auto stats3 = allocator.Stats();
      CHECK_EQ(stats3.peak_usage, peak2);
    }
  }

  TEST_CASE("Ownership checking") {
    constexpr size_t capacity = 4096;
    StackAllocator allocator(capacity);

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

      allocator.Deallocate(ptr, result.allocated_size);

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
    }
  }

  TEST_CASE("Move semantics") {
    constexpr size_t capacity = 4096;

    SUBCASE("Move construction") {
      StackAllocator allocator1(capacity);
      auto result = allocator1.Allocate(64);
      result = allocator1.Allocate(128);

      auto offset1 = allocator1.CurrentOffset();

      StackAllocator allocator2(std::move(allocator1));

      CHECK_EQ(allocator2.Capacity(), capacity);
      CHECK_EQ(allocator2.CurrentOffset(), offset1);
      CHECK_FALSE(allocator2.Empty());
    }

    SUBCASE("Move assignment") {
      StackAllocator allocator1(capacity);
      const auto result = allocator1.Allocate(64);

      StackAllocator allocator2(512);

      allocator2 = std::move(allocator1);

      CHECK_EQ(allocator2.Capacity(), capacity);
    }

    SUBCASE("Self move assignment") {
      StackAllocator allocator(capacity);
      const auto result = allocator.Allocate(64);

      auto* ptr = &allocator;
      allocator = std::move(*ptr);

      // Should still be valid
      CHECK(allocator.Capacity() == capacity);
    }
  }

  TEST_CASE("Write and read allocated memory") {
    constexpr size_t capacity = 4096;
    StackAllocator allocator(capacity);

    SUBCASE("Write and read int") {
      const auto result = allocator.Allocate(sizeof(int));
      CHECK_NE(result.ptr, nullptr);

      auto* const data = static_cast<int*>(result.ptr);
      *data = 42;

      CHECK_EQ(*data, 42);
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
    }

    SUBCASE("Multiple allocations with different data") {
      struct Data {
        int value = 0;
      };

      void* ptrs[5];
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
    }

    SUBCASE("Write array of data") {
      constexpr size_t array_size = 100;
      const auto result = allocator.Allocate(sizeof(int) * array_size);
      CHECK_NE(result.ptr, nullptr);

      auto* array = static_cast<int*>(result.ptr);
      for (size_t i = 0; i < array_size; ++i) {
        array[i] = static_cast<int>(i);
      }

      // Verify all values
      for (size_t i = 0; i < array_size; ++i) {
        CHECK_EQ(array[i], static_cast<int>(i));
      }
    }
  }

  TEST_CASE("Boundary conditions") {
    SUBCASE("Minimum capacity") {
      constexpr size_t capacity = 256;
      StackAllocator allocator(capacity);

      const auto result = allocator.Allocate(32);
      CHECK_NE(result.ptr, nullptr);
    }

    SUBCASE("Single byte allocation") {
      constexpr size_t capacity = 1024;
      StackAllocator allocator(capacity);

      const auto result = allocator.Allocate(1);
      CHECK_NE(result.ptr, nullptr);
      CHECK_EQ(result.allocated_size, 1);
    }

    SUBCASE("Maximum size allocation") {
      constexpr size_t capacity = 1024;
      StackAllocator allocator(capacity);

      // Account for header overhead
      const auto result = allocator.Allocate(capacity - 128);
      CHECK_NE(result.ptr, nullptr);
    }
  }

  TEST_CASE("Allocation patterns") {
    constexpr size_t capacity = 4096;
    StackAllocator allocator(capacity);

    SUBCASE("Allocate, deallocate, allocate pattern") {
      const auto result1 = allocator.Allocate(256);
      CHECK_NE(result1.ptr, nullptr);

      allocator.Deallocate(result1.ptr, result1.allocated_size);
      CHECK(allocator.Empty());

      const auto result2 = allocator.Allocate(512);
      CHECK_NE(result2.ptr, nullptr);
    }

    SUBCASE("Multiple small allocations") {
      constexpr int count = 50;
      for (int i = 0; i < count; ++i) {
        const auto result = allocator.Allocate(16);
        if (result.ptr == nullptr) {
          // Ran out of space due to header overhead - acceptable
          break;
        }
        CHECK_NE(result.ptr, nullptr);
      }
    }

    SUBCASE("Alternating sizes") {
      for (int i = 0; i < 10; ++i) {
        const size_t size = (i % 2 == 0) ? 64 : 128;
        const auto result = allocator.Allocate(size);
        CHECK_NE(result.ptr, nullptr);
      }
    }
  }

  TEST_CASE("Stress test") {
    constexpr size_t capacity = 65536;  // 64KB
    StackAllocator allocator(capacity);

    SUBCASE("Many allocations and deallocations") {
      for (int cycle = 0; cycle < 100; ++cycle) {
        // Allocate
        void* ptrs[10];
        size_t sizes[10];
        for (int i = 0; i < 10; ++i) {
          const auto result = allocator.Allocate(64);
          if (result.ptr) {
            ptrs[i] = result.ptr;
            sizes[i] = result.allocated_size;
          }
        }

        // Deallocate in reverse order
        for (int i = 9; i >= 0; --i) {
          if (ptrs[i]) {
            allocator.Deallocate(ptrs[i], sizes[i]);
          }
        }

        CHECK(allocator.Empty());
      }
    }

    SUBCASE("Marker-based bulk operations") {
      for (int cycle = 0; cycle < 50; ++cycle) {
        auto marker = allocator.Marker();

        // Allocate many blocks
        for (int i = 0; i < 20; ++i) {
          const auto result = allocator.Allocate(128);
        }

        // Bulk deallocate
        allocator.RewindToMarker(marker);
        CHECK_EQ(allocator.CurrentOffset(), marker);
      }
    }
  }
}

}  // namespace
