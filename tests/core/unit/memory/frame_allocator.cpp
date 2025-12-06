#include <doctest/doctest.h>

#include <helios/core/memory/frame_allocator.hpp>

#include <cstddef>
#include <cstdint>

namespace {

using namespace helios::memory;

TEST_SUITE("memory::FrameAllocator") {
  TEST_CASE("FrameAllocator::ctor: construction") {
    SUBCASE("Valid capacity") {
      constexpr size_t capacity = 1024;
      FrameAllocator allocator(capacity);

      CHECK_EQ(allocator.Capacity(), capacity);
      CHECK_EQ(allocator.CurrentOffset(), 0);
      CHECK_EQ(allocator.FreeSpace(), capacity);
      CHECK(allocator.Empty());
      CHECK_FALSE(allocator.Full());
    }

    SUBCASE("Large capacity") {
      constexpr size_t capacity = 1024 * 1024;
      FrameAllocator allocator(capacity);

      CHECK_EQ(allocator.Capacity(), capacity);
      CHECK(allocator.Empty());
    }
  }

  TEST_CASE("FrameAllocator::Allocate: basic allocation") {
    constexpr size_t capacity = 4096;
    FrameAllocator allocator(capacity);

    SUBCASE("Single allocation") {
      constexpr size_t size = 64;
      const auto result = allocator.Allocate(size);

      CHECK_NE(result.ptr, nullptr);
      CHECK_EQ(result.allocated_size, size);
      CHECK_FALSE(allocator.Empty());
      CHECK_GT(allocator.CurrentOffset(), 0);
      CHECK_LT(allocator.FreeSpace(), capacity);
    }

    SUBCASE("Multiple allocations") {
      const auto result1 = allocator.Allocate(128);
      const auto result2 = allocator.Allocate(256);
      const auto result3 = allocator.Allocate(512);

      CHECK_NE(result1.ptr, nullptr);
      CHECK_NE(result2.ptr, nullptr);
      CHECK_NE(result3.ptr, nullptr);

      // Pointers should be different
      CHECK_NE(result1.ptr, result2.ptr);
      CHECK_NE(result2.ptr, result3.ptr);
      CHECK_NE(result1.ptr, result3.ptr);

      CHECK_FALSE(allocator.Empty());
    }

    SUBCASE("Zero size allocation") {
      const auto result = allocator.Allocate(0);
      CHECK_EQ(result.ptr, nullptr);
      CHECK_EQ(result.allocated_size, 0);
      CHECK(allocator.Empty());
    }
  }

  TEST_CASE("FrameAllocator::Allocate: alignment") {
    constexpr size_t capacity = 4096;
    FrameAllocator allocator(capacity);

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

    SUBCASE("Custom alignment 64") {
      const auto result = allocator.Allocate(100, 64);
      CHECK_NE(result.ptr, nullptr);
      CHECK(IsAligned(result.ptr, 64));
    }

    SUBCASE("Multiple allocations with alignment") {
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

  TEST_CASE("FrameAllocator::Allocate: capacity exhaustion") {
    constexpr size_t capacity = 1024;
    FrameAllocator allocator(capacity);

    SUBCASE("Allocate entire capacity") {
      const auto result = allocator.Allocate(capacity);
      CHECK_NE(result.ptr, nullptr);
      CHECK_EQ(allocator.FreeSpace(), 0);
      CHECK(allocator.Full());
    }

    SUBCASE("Allocate beyond capacity") {
      const auto result = allocator.Allocate(capacity + 1);
      CHECK_EQ(result.ptr, nullptr);
      CHECK_EQ(result.allocated_size, 0);
    }

    SUBCASE("Multiple allocations exhausting capacity") {
      const auto result1 = allocator.Allocate(512);
      CHECK_NE(result1.ptr, nullptr);

      const auto result2 = allocator.Allocate(512);
      CHECK_NE(result2.ptr, nullptr);

      // Should have little to no space left (accounting for alignment)
      const auto result3 = allocator.Allocate(100);
      CHECK_EQ(result3.ptr, nullptr);
    }
  }

  TEST_CASE("FrameAllocator::Reset: clears state") {
    constexpr size_t capacity = 2048;
    FrameAllocator allocator(capacity);

    SUBCASE("Reset after single allocation") {
      const auto result = allocator.Allocate(128);
      CHECK_NE(result.ptr, nullptr);
      CHECK_FALSE(allocator.Empty());

      allocator.Reset();

      CHECK(allocator.Empty());
      CHECK_EQ(allocator.CurrentOffset(), 0);
      CHECK_EQ(allocator.FreeSpace(), capacity);
    }

    SUBCASE("Reset after multiple allocations") {
      auto result = allocator.Allocate(256);
      result = allocator.Allocate(512);
      result = allocator.Allocate(128);

      CHECK_FALSE(allocator.Empty());

      allocator.Reset();

      CHECK(allocator.Empty());
      CHECK_EQ(allocator.CurrentOffset(), 0);
    }

    SUBCASE("Allocate after reset") {
      const auto result1 = allocator.Allocate(512);
      CHECK_NE(result1.ptr, nullptr);

      allocator.Reset();

      const auto result2 = allocator.Allocate(512);
      CHECK_NE(result2.ptr, nullptr);
      // After reset, should get the same memory region
      CHECK_EQ(result1.ptr, result2.ptr);
    }
  }

  TEST_CASE("FrameAllocator::Stats: statistics tracking") {
    constexpr size_t capacity = 4096;
    FrameAllocator allocator(capacity);

    SUBCASE("Initial stats") {
      const auto stats = allocator.Stats();
      CHECK_EQ(stats.total_allocated, 0);
      CHECK_EQ(stats.allocation_count, 0);
      CHECK_EQ(stats.peak_usage, 0);
      CHECK_EQ(stats.total_allocations, 0);
    }

    SUBCASE("Stats after allocation") {
      const auto result = allocator.Allocate(256);
      const auto stats = allocator.Stats();

      CHECK_GT(stats.total_allocated, 0);
      CHECK_EQ(stats.allocation_count, 1);
      CHECK_EQ(stats.total_allocations, 1);
      CHECK_GT(stats.peak_usage, 0);
    }

    SUBCASE("Stats track peak usage") {
      auto result = allocator.Allocate(512);
      const auto stats1 = allocator.Stats();
      const auto peak1 = stats1.peak_usage;

      result = allocator.Allocate(1024);
      const auto stats2 = allocator.Stats();
      const auto peak2 = stats2.peak_usage;

      CHECK_GT(peak2, peak1);
      CHECK_EQ(stats2.allocation_count, 2);
    }

    SUBCASE("Stats after reset") {
      auto result = allocator.Allocate(512);
      result = allocator.Allocate(256);

      const auto stats_before = allocator.Stats();
      CHECK_EQ(stats_before.allocation_count, 2);

      allocator.Reset();

      const auto stats_after = allocator.Stats();
      CHECK_EQ(stats_after.total_allocated, 0);
      CHECK_EQ(stats_after.allocation_count, 0);
      // Peak should persist
      CHECK_EQ(stats_after.peak_usage, stats_before.peak_usage);
    }
  }

  TEST_CASE("FrameAllocator::ctor: move semantics") {
    constexpr size_t capacity = 2048;

    SUBCASE("Move construction") {
      FrameAllocator allocator1(capacity);
      const auto result = allocator1.Allocate(128);

      const auto stats1 = allocator1.Stats();

      FrameAllocator allocator2(std::move(allocator1));

      CHECK_EQ(allocator2.Capacity(), capacity);
      const auto stats2 = allocator2.Stats();
      CHECK_EQ(stats2.allocation_count, stats1.allocation_count);
    }

    SUBCASE("Move assignment") {
      FrameAllocator allocator1(capacity);
      const auto result = allocator1.Allocate(256);

      FrameAllocator allocator2(1024);

      allocator2 = std::move(allocator1);

      CHECK_EQ(allocator2.Capacity(), capacity);
      CHECK_FALSE(allocator2.Empty());
    }
  }

  TEST_CASE("FrameAllocator::Allocate: write and read memory") {
    constexpr size_t capacity = 4096;
    FrameAllocator allocator(capacity);

    SUBCASE("Write int values") {
      const auto result = allocator.Allocate(sizeof(int) * 10);
      CHECK_NE(result.ptr, nullptr);

      auto* const data = static_cast<int*>(result.ptr);
      for (int i = 0; i < 10; ++i) {
        data[i] = i * 10;
      }

      // Verify written values
      for (int i = 0; i < 10; ++i) {
        CHECK_EQ(data[i], i * 10);
      }
    }

    SUBCASE("Write struct values") {
      struct TestStruct {
        int x = 0;
        float y = 0.0F;
        char z = '\0';
      };

      const auto result = allocator.Allocate(sizeof(TestStruct));
      CHECK_NE(result.ptr, nullptr);

      auto* const data = static_cast<TestStruct*>(result.ptr);
      data->x = 42;
      data->y = 3.14F;
      data->z = 'A';

      CHECK_EQ(data->x, 42);
      CHECK_EQ(data->y, doctest::Approx(3.14F));
      CHECK_EQ(data->z, 'A');
    }
  }

  TEST_CASE("FrameAllocator::Allocate: large number of allocations") {
    constexpr size_t capacity = 1024 * 1024;  // 1 MB
    FrameAllocator allocator(capacity);

    constexpr size_t allocation_size = 64;
    constexpr size_t num_allocations = 1000;

    size_t successful_allocations = 0;

    for (size_t i = 0; i < num_allocations; ++i) {
      const auto result = allocator.Allocate(allocation_size);
      if (result.ptr != nullptr) {
        ++successful_allocations;
      } else {
        break;
      }
    }

    CHECK_GT(successful_allocations, 0);
    CHECK_FALSE(allocator.Empty());
  }

  TEST_CASE("FrameAllocator::Stats: alignment waste tracking") {
    constexpr size_t capacity = 4096;
    FrameAllocator allocator(capacity);

    SUBCASE("Misaligned sizes with high alignment") {
      // Allocate sizes that aren't naturally aligned
      auto result = allocator.Allocate(10, 64);
      result = allocator.Allocate(15, 64);
      result = allocator.Allocate(20, 64);

      const auto stats = allocator.Stats();
      // Should have some alignment waste
      CHECK_GT(stats.alignment_waste, 0);
    }
  }

  TEST_CASE("FrameAllocator::Allocate: boundary conditions") {
    SUBCASE("Minimum capacity") {
      constexpr size_t capacity = 64;
      FrameAllocator allocator(capacity);

      const auto result = allocator.Allocate(32);
      CHECK_NE(result.ptr, nullptr);
    }

    SUBCASE("Single byte allocation") {
      constexpr size_t capacity = 1024;
      FrameAllocator allocator(capacity);

      const auto result = allocator.Allocate(1);
      CHECK_NE(result.ptr, nullptr);
      CHECK_EQ(result.allocated_size, 1);
    }

    SUBCASE("Exact capacity allocation") {
      constexpr size_t capacity = 512;
      FrameAllocator allocator(capacity);

      const auto result = allocator.Allocate(capacity);
      CHECK_NE(result.ptr, nullptr);
      CHECK(allocator.Full());

      // Next allocation should fail
      const auto result2 = allocator.Allocate(1);
      CHECK_EQ(result2.ptr, nullptr);
    }
  }

  TEST_CASE("FrameAllocator::Allocate<T>: templated single allocation") {
    constexpr size_t capacity = 4096;
    FrameAllocator allocator(capacity);

    SUBCASE("Allocate single int") {
      int* ptr = allocator.Allocate<int>();
      CHECK_NE(ptr, nullptr);
      CHECK(IsAligned(ptr, alignof(int)));

      // Write to verify memory is usable
      new (ptr) int(42);
      CHECK_EQ(*ptr, 42);
    }

    SUBCASE("Allocate single double") {
      double* ptr = allocator.Allocate<double>();
      CHECK_NE(ptr, nullptr);
      CHECK(IsAligned(ptr, alignof(double)));

      new (ptr) double(3.14159);
      CHECK_EQ(*ptr, doctest::Approx(3.14159));
    }

    SUBCASE("Allocate struct") {
      struct TestStruct {
        int a;
        double b;
        char c;
      };

      TestStruct* ptr = allocator.Allocate<TestStruct>();
      CHECK_NE(ptr, nullptr);
      CHECK(IsAligned(ptr, alignof(TestStruct)));

      new (ptr) TestStruct{10, 2.5, 'x'};
      CHECK_EQ(ptr->a, 10);
      CHECK_EQ(ptr->b, doctest::Approx(2.5));
      CHECK_EQ(ptr->c, 'x');
    }

    SUBCASE("Multiple templated allocations") {
      int* p1 = allocator.Allocate<int>();
      double* p2 = allocator.Allocate<double>();
      char* p3 = allocator.Allocate<char>();

      CHECK_NE(p1, nullptr);
      CHECK_NE(p2, nullptr);
      CHECK_NE(p3, nullptr);

      // All should be different
      CHECK_NE(static_cast<void*>(p1), static_cast<void*>(p2));
      CHECK_NE(static_cast<void*>(p2), static_cast<void*>(p3));
    }
  }

  TEST_CASE("FrameAllocator::Allocate<T>(count): templated array allocation") {
    constexpr size_t capacity = 4096;
    FrameAllocator allocator(capacity);

    SUBCASE("Allocate array of ints") {
      constexpr size_t count = 10;
      int* ptr = allocator.Allocate<int>(count);
      CHECK_NE(ptr, nullptr);
      CHECK(IsAligned(ptr, alignof(int)));

      // Write to all elements
      for (size_t i = 0; i < count; ++i) {
        new (&ptr[i]) int(static_cast<int>(i * 2));
      }

      // Verify values
      for (size_t i = 0; i < count; ++i) {
        CHECK_EQ(ptr[i], static_cast<int>(i * 2));
      }
    }

    SUBCASE("Allocate array of doubles") {
      constexpr size_t count = 5;
      double* ptr = allocator.Allocate<double>(count);
      CHECK_NE(ptr, nullptr);
      CHECK(IsAligned(ptr, alignof(double)));

      for (size_t i = 0; i < count; ++i) {
        new (&ptr[i]) double(static_cast<double>(i) * 1.5);
      }

      for (size_t i = 0; i < count; ++i) {
        CHECK_EQ(ptr[i], doctest::Approx(static_cast<double>(i) * 1.5));
      }
    }

    SUBCASE("Zero count returns nullptr") {
      int* ptr = allocator.Allocate<int>(0);
      CHECK_EQ(ptr, nullptr);
      CHECK(allocator.Empty());
    }

    SUBCASE("Large array allocation") {
      constexpr size_t count = 100;
      int* ptr = allocator.Allocate<int>(count);
      CHECK_NE(ptr, nullptr);

      // Verify all memory is accessible
      for (size_t i = 0; i < count; ++i) {
        new (&ptr[i]) int(static_cast<int>(i));
      }

      for (size_t i = 0; i < count; ++i) {
        CHECK_EQ(ptr[i], static_cast<int>(i));
      }
    }
  }

  TEST_CASE("FrameAllocator::AllocateAndConstruct: templated allocation with construction") {
    constexpr size_t capacity = 4096;
    FrameAllocator allocator(capacity);

    SUBCASE("Construct int with value") {
      int* ptr = allocator.AllocateAndConstruct<int>(42);
      CHECK_NE(ptr, nullptr);
      CHECK_EQ(*ptr, 42);
    }

    SUBCASE("Construct double with value") {
      double* ptr = allocator.AllocateAndConstruct<double>(3.14159);
      CHECK_NE(ptr, nullptr);
      CHECK_EQ(*ptr, doctest::Approx(3.14159));
    }

    SUBCASE("Construct struct with multiple args") {
      struct Point {
        float x;
        float y;
        float z;

        Point(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
      };

      Point* ptr = allocator.AllocateAndConstruct<Point>(1.0F, 2.0F, 3.0F);
      CHECK_NE(ptr, nullptr);
      CHECK_EQ(ptr->x, doctest::Approx(1.0F));
      CHECK_EQ(ptr->y, doctest::Approx(2.0F));
      CHECK_EQ(ptr->z, doctest::Approx(3.0F));
    }

    SUBCASE("Construct with default constructor") {
      struct DefaultInit {
        int value = 100;
      };

      DefaultInit* ptr = allocator.AllocateAndConstruct<DefaultInit>();
      CHECK_NE(ptr, nullptr);
      CHECK_EQ(ptr->value, 100);
    }
  }

  TEST_CASE("FrameAllocator::AllocateAndConstructArray: templated array with construction") {
    constexpr size_t capacity = 4096;
    FrameAllocator allocator(capacity);

    SUBCASE("Construct array of default-initialized ints") {
      constexpr size_t count = 10;
      int* ptr = allocator.AllocateAndConstructArray<int>(count);
      CHECK_NE(ptr, nullptr);

      // Default-initialized ints should be 0
      for (size_t i = 0; i < count; ++i) {
        CHECK_EQ(ptr[i], 0);
      }
    }

    SUBCASE("Construct array of structs with default values") {
      struct Counter {
        int value = 42;
      };

      constexpr size_t count = 5;
      Counter* ptr = allocator.AllocateAndConstructArray<Counter>(count);
      CHECK_NE(ptr, nullptr);

      for (size_t i = 0; i < count; ++i) {
        CHECK_EQ(ptr[i].value, 42);
      }
    }

    SUBCASE("Zero count returns nullptr") {
      int* ptr = allocator.AllocateAndConstructArray<int>(0);
      CHECK_EQ(ptr, nullptr);
    }
  }
}

}  // namespace
