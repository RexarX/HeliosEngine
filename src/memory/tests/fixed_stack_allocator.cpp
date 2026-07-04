#include <doctest/doctest.h>

#include <tuple>

#include <helios/memory/fixed_stack_allocator.hpp>

#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <vector>

using namespace helios::mem;

namespace {

constexpr size_t kMinStackCapacity = sizeof(size_t) * 2 + 1;
constexpr size_t kCapacity = 4096;
constexpr size_t kSmallCapacity = 512;
constexpr size_t kAlign = alignof(std::max_align_t);
constexpr size_t kHeaderSize = sizeof(size_t) * 2;

}  // namespace

TEST_SUITE("helios::mem::FixedStackAllocator") {
  TEST_CASE("mem::FixedStackAllocator::ctor()") {
    SUBCASE("InitialCapacity equals configured capacity") {
      const FixedStackAllocator stack(kCapacity);
      CHECK_EQ(stack.InitialCapacity(), kCapacity);
    }

    SUBCASE("TotalCapacity equals configured capacity") {
      const FixedStackAllocator stack(kCapacity);
      CHECK_EQ(stack.TotalCapacity(), kCapacity);
    }

    SUBCASE("Stack is empty on construction") {
      const FixedStackAllocator stack(kCapacity);
      CHECK(stack.Empty());
    }

    SUBCASE("GetMarker returns zero offset on construction") {
      const FixedStackAllocator stack(kCapacity);
      CHECK_EQ(stack.GetMarker().offset, 0);
    }

    SUBCASE("Stats counters are zero on construction") {
      const FixedStackAllocator stack(kCapacity);
      const AllocatorStats stats = stack.Stats();
      CHECK_EQ(stats.total_allocated, 0);
      CHECK_EQ(stats.peak_usage, 0);
      CHECK_EQ(stats.allocation_count, 0);
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
      CHECK_EQ(stats.alignment_waste, 0);
    }
  }

  TEST_CASE("mem::FixedStackAllocator::ctor(FixedStackAllocator&&)") {
    SUBCASE("Moved-into stack has same InitialCapacity") {
      FixedStackAllocator source(kCapacity);
      const FixedStackAllocator moved(std::move(source));
      CHECK_EQ(moved.InitialCapacity(), kCapacity);
    }

    SUBCASE("Moved-into stack carries existing allocation state") {
      FixedStackAllocator source(kCapacity);
      void* const ptr = source.allocate(64, kAlign);
      const FixedStackAllocator moved(std::move(source));
      CHECK_FALSE(moved.Empty());
      CHECK(moved.Owns(ptr));
      CHECK_EQ(moved.Stats().allocation_count, 1);
    }

    SUBCASE("Source is empty after move") {
      FixedStackAllocator source(kCapacity);
      std::ignore = source.allocate(32, kAlign);
      const FixedStackAllocator moved(std::move(source));
      CHECK(source.Empty());
    }

    SUBCASE("Source stats are zeroed after move") {
      FixedStackAllocator source(kCapacity);
      std::ignore = source.allocate(32, kAlign);
      const FixedStackAllocator moved(std::move(source));
      CHECK_EQ(source.Stats().allocation_count, 0);
      CHECK_EQ(source.Stats().total_allocated, 0);
    }

    SUBCASE("Moved-into stack TotalCapacity is unchanged") {
      FixedStackAllocator source(kCapacity);
      const FixedStackAllocator moved(std::move(source));
      CHECK_EQ(moved.TotalCapacity(), kCapacity);
    }
  }

  TEST_CASE("mem::FixedStackAllocator::operator=(FixedStackAllocator&&)") {
    SUBCASE("Target acquires source allocation state") {
      FixedStackAllocator source(kCapacity);
      std::ignore = source.allocate(32, kAlign);
      FixedStackAllocator target(kCapacity);

      target = std::move(source);

      CHECK_FALSE(target.Empty());
      CHECK_EQ(target.Stats().allocation_count, 1);
    }

    SUBCASE("Source is empty after move assign") {
      FixedStackAllocator source(kCapacity);
      std::ignore = source.allocate(32, kAlign);
      FixedStackAllocator target(kCapacity);

      target = std::move(source);

      CHECK(source.Empty());
      CHECK_EQ(source.Stats().allocation_count, 0);
    }

    SUBCASE("Self move assignment does not corrupt state") {
      FixedStackAllocator stack(kCapacity);
      std::ignore = stack.allocate(16, kAlign);
      FixedStackAllocator& ref = stack;

      ref = std::move(stack);  // NOLINT(clang-diagnostic-self-move)

      CHECK_FALSE(ref.Empty());
    }

    SUBCASE("Target releases prior backing storage on move assign") {
      FixedStackAllocator source(kCapacity);
      void* const source_ptr = source.allocate(64, kAlign);
      FixedStackAllocator target(kCapacity);
      void* const target_ptr = target.allocate(64, kAlign);

      target = std::move(source);

      CHECK(target.Owns(source_ptr));
      CHECK_FALSE(target.Owns(target_ptr));
    }
  }

  TEST_CASE("mem::FixedStackAllocator::RewindToMarker") {
    SUBCASE("Rewinds offset to captured marker") {
      FixedStackAllocator stack(kCapacity);
      const auto marker = stack.GetMarker();
      std::ignore = stack.allocate(64, kAlign);
      stack.RewindToMarker(marker);
      CHECK_EQ(stack.GetMarker().offset, marker.offset);
      CHECK(stack.Empty());
    }

    SUBCASE("Clears allocation_count after rewind") {
      FixedStackAllocator stack(kCapacity);
      const auto marker = stack.GetMarker();
      std::ignore = stack.allocate(64, kAlign);
      stack.RewindToMarker(marker);
      CHECK_EQ(stack.Stats().allocation_count, 0);
    }

    SUBCASE("Allows reuse of rewound space") {
      FixedStackAllocator stack(kCapacity);
      const auto marker = stack.GetMarker();
      void* const first = stack.allocate(64, kAlign);
      stack.RewindToMarker(marker);
      void* const second = stack.allocate(64, kAlign);
      CHECK_NE(second, nullptr);
      CHECK(stack.Owns(first));
      CHECK(stack.Owns(second));
    }

    SUBCASE("Partial rewind preserves intermediate allocations") {
      FixedStackAllocator stack(kCapacity);
      const auto base = stack.GetMarker();
      void* const first = stack.allocate(32, kAlign);
      const auto mid = stack.GetMarker();
      std::ignore = stack.allocate(32, kAlign);
      stack.RewindToMarker(mid);
      CHECK(stack.Owns(first));
      CHECK_EQ(stack.Stats().allocation_count, 0);
      CHECK_EQ(stack.GetMarker().offset, mid.offset);
      CHECK_NE(stack.GetMarker().offset, base.offset);
    }
  }

  TEST_CASE("mem::FixedStackAllocator::Reset") {
    SUBCASE("Empty returns true after Reset") {
      FixedStackAllocator stack(kCapacity);
      std::ignore = stack.allocate(128, kAlign);
      stack.Reset();
      CHECK(stack.Empty());
    }

    SUBCASE("Stats counters are zeroed after Reset") {
      FixedStackAllocator stack(kCapacity);
      std::ignore = stack.allocate(64, kAlign);
      stack.Reset();
      const AllocatorStats stats = stack.Stats();
      CHECK_EQ(stats.total_allocated, 0);
      CHECK_EQ(stats.peak_usage, 0);
      CHECK_EQ(stats.allocation_count, 0);
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
      CHECK_EQ(stats.alignment_waste, 0);
    }

    SUBCASE("GetMarker returns zero offset after Reset") {
      FixedStackAllocator stack(kCapacity);
      std::ignore = stack.allocate(64, kAlign);
      stack.Reset();
      CHECK_EQ(stack.GetMarker().offset, 0);
    }

    SUBCASE("Allocations succeed after Reset") {
      FixedStackAllocator stack(kCapacity);
      std::ignore = stack.allocate(512, kAlign);
      stack.Reset();
      void* const ptr = stack.allocate(64, kAlign);
      CHECK_NE(ptr, nullptr);
    }
  }

  TEST_CASE("mem::FixedStackAllocator::Empty") {
    SUBCASE("Returns true on fresh stack") {
      const FixedStackAllocator stack(kCapacity);
      CHECK(stack.Empty());
    }

    SUBCASE("Returns false after one allocation") {
      FixedStackAllocator stack(kCapacity);
      std::ignore = stack.allocate(8, kAlign);
      CHECK_FALSE(stack.Empty());
    }

    SUBCASE("Returns true after Reset") {
      FixedStackAllocator stack(kCapacity);
      std::ignore = stack.allocate(8, kAlign);
      stack.Reset();
      CHECK(stack.Empty());
    }

    SUBCASE("Returns true after LIFO deallocate of sole allocation") {
      FixedStackAllocator stack(kCapacity);
      void* const ptr = stack.allocate(8, kAlign);
      stack.deallocate(ptr, 8, kAlign);
      CHECK(stack.Empty());
    }

    SUBCASE("Returns false after non-LIFO deallocate") {
      FixedStackAllocator stack(kCapacity);
      void* const first = stack.allocate(8, kAlign);
      std::ignore = stack.allocate(8, kAlign);
      stack.deallocate(first, 8, kAlign);
      CHECK_FALSE(stack.Empty());
    }
  }

  TEST_CASE("mem::FixedStackAllocator::Owns") {
    SUBCASE("Returns true for pointer from this stack") {
      FixedStackAllocator stack(kCapacity);
      void* const ptr = stack.allocate(64, kAlign);
      CHECK(stack.Owns(ptr));
    }

    SUBCASE("Returns false for nullptr") {
      const FixedStackAllocator stack(kCapacity);
      CHECK_FALSE(stack.Owns(nullptr));
    }

    SUBCASE("Returns false for stack variable pointer") {
      const FixedStackAllocator stack(kCapacity);
      int local = 0;
      CHECK_FALSE(stack.Owns(&local));
    }

    SUBCASE("Returns false for pointer from a different stack") {
      FixedStackAllocator first(kCapacity);
      FixedStackAllocator second(kCapacity);
      void* const ptr = second.allocate(64, kAlign);
      CHECK_FALSE(first.Owns(ptr));
    }

    SUBCASE("Returns true for pointer after LIFO deallocate") {
      FixedStackAllocator stack(kCapacity);
      void* const ptr = stack.allocate(64, kAlign);
      stack.deallocate(ptr, 64, kAlign);
      CHECK(stack.Owns(ptr));
    }

    SUBCASE("Returns false for moved-from stack pointer") {
      FixedStackAllocator source(kCapacity);
      void* const ptr = source.allocate(64, kAlign);
      FixedStackAllocator moved(std::move(source));
      CHECK(moved.Owns(ptr));
      CHECK_FALSE(source.Owns(ptr));
    }
  }

  TEST_CASE("mem::FixedStackAllocator::GetMarker") {
    SUBCASE("Returns zero offset on fresh stack") {
      const FixedStackAllocator stack(kCapacity);
      CHECK_EQ(stack.GetMarker().offset, 0);
    }

    SUBCASE("Captures current bump offset after allocations") {
      FixedStackAllocator stack(kCapacity);
      std::ignore = stack.allocate(64, kAlign);
      const size_t offset_after_alloc = stack.Stats().total_allocated;
      CHECK_EQ(stack.GetMarker().offset, offset_after_alloc);
    }

    SUBCASE("Marker is unchanged by non-LIFO deallocate") {
      FixedStackAllocator stack(kCapacity);
      void* const first = stack.allocate(64, kAlign);
      std::ignore = stack.allocate(64, kAlign);
      const auto marker = stack.GetMarker();
      stack.deallocate(first, 64, kAlign);
      CHECK_EQ(stack.GetMarker().offset, marker.offset);
    }

    SUBCASE("Marker rewinds after LIFO deallocate") {
      FixedStackAllocator stack(kCapacity);
      std::ignore = stack.allocate(64, kAlign);
      void* const second = stack.allocate(64, kAlign);
      const size_t offset_before = stack.GetMarker().offset;
      stack.deallocate(second, 64, kAlign);
      CHECK_LT(stack.GetMarker().offset, offset_before);
    }
  }

  TEST_CASE("mem::FixedStackAllocator::Stats") {
    SUBCASE("All fields are zero on fresh stack") {
      const FixedStackAllocator stack(kCapacity);
      const AllocatorStats stats = stack.Stats();
      CHECK_EQ(stats.total_allocated, 0);
      CHECK_EQ(stats.peak_usage, 0);
      CHECK_EQ(stats.allocation_count, 0);
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
      CHECK_EQ(stats.alignment_waste, 0);
    }

    SUBCASE("total_allocated reflects bump offset after allocations") {
      FixedStackAllocator stack(kCapacity);
      std::ignore = stack.allocate(64, kAlign);
      std::ignore = stack.allocate(128, kAlign);
      CHECK_GE(stack.Stats().total_allocated, 192);
    }

    SUBCASE("allocation_count tracks live allocations") {
      FixedStackAllocator stack(kCapacity);
      std::ignore = stack.allocate(32, kAlign);
      std::ignore = stack.allocate(32, kAlign);
      CHECK_EQ(stack.Stats().allocation_count, 2);
    }

    SUBCASE("total_allocations counts every allocate call") {
      FixedStackAllocator stack(kCapacity);
      std::ignore = stack.allocate(16, kAlign);
      std::ignore = stack.allocate(16, kAlign);
      std::ignore = stack.allocate(16, kAlign);
      CHECK_EQ(stack.Stats().total_allocations, 3);
    }

    SUBCASE("total_deallocations increments only on successful LIFO free") {
      FixedStackAllocator stack(kCapacity);
      void* const first = stack.allocate(32, kAlign);
      void* const second = stack.allocate(32, kAlign);
      stack.deallocate(first, 32, kAlign);
      CHECK_EQ(stack.Stats().total_deallocations, 0);
      stack.deallocate(second, 32, kAlign);
      CHECK_EQ(stack.Stats().total_deallocations, 1);
    }

    SUBCASE("peak_usage is at least total bytes allocated at high watermark") {
      FixedStackAllocator stack(kCapacity);
      std::ignore = stack.allocate(256, kAlign);
      std::ignore = stack.allocate(128, kAlign);
      CHECK_GE(stack.Stats().peak_usage, 256);
    }

    SUBCASE("alignment_waste is non-negative with large alignment request") {
      FixedStackAllocator stack(kCapacity);
      std::ignore = stack.allocate(1, 64);
      CHECK_GE(stack.Stats().alignment_waste, 0);
    }
  }

  TEST_CASE("mem::FixedStackAllocator::InitialCapacity") {
    SUBCASE("Returns configured capacity template argument") {
      const FixedStackAllocator stack(1024);
      CHECK_EQ(stack.InitialCapacity(), 1024);
    }

    SUBCASE("Unchanged by allocations") {
      FixedStackAllocator stack(kCapacity);
      std::ignore = stack.allocate(512, kAlign);
      CHECK_EQ(stack.InitialCapacity(), kCapacity);
    }

    SUBCASE("Unchanged after Reset") {
      FixedStackAllocator stack(kCapacity);
      std::ignore = stack.allocate(512, kAlign);
      stack.Reset();
      CHECK_EQ(stack.InitialCapacity(), kCapacity);
    }
  }

  TEST_CASE("mem::FixedStackAllocator::TotalCapacity") {
    SUBCASE("Equals configured capacity immediately after construction") {
      const FixedStackAllocator stack(kCapacity);
      CHECK_EQ(stack.TotalCapacity(), kCapacity);
    }

    SUBCASE("Unchanged by allocations") {
      FixedStackAllocator stack(kCapacity);
      std::ignore = stack.allocate(512, kAlign);
      CHECK_EQ(stack.TotalCapacity(), kCapacity);
    }

    SUBCASE("Unchanged after Reset") {
      FixedStackAllocator stack(kCapacity);
      std::ignore = stack.allocate(512, kAlign);
      stack.Reset();
      CHECK_EQ(stack.TotalCapacity(), kCapacity);
    }
  }

  TEST_CASE("mem::FixedStackAllocator::allocate") {
    SUBCASE("Returns non-null pointer") {
      FixedStackAllocator stack(kCapacity);
      CHECK_NE(stack.allocate(32, kAlign), nullptr);
    }

    SUBCASE(
        "Consecutive allocations return distinct non-overlapping pointers") {
      FixedStackAllocator stack(kCapacity);
      void* const first = stack.allocate(64, kAlign);
      void* const second = stack.allocate(64, kAlign);
      CHECK_NE(first, second);
      const auto ua = reinterpret_cast<uintptr_t>(first);
      const auto ub = reinterpret_cast<uintptr_t>(second);
      const bool non_overlapping = ub >= ua + 64 || ua >= ub + 64;
      CHECK(non_overlapping);
    }

    SUBCASE("Pointer satisfies the requested alignment") {
      FixedStackAllocator stack(kCapacity);
      for (const size_t al : {1UZ, 4UZ, 8UZ, 16UZ, 32UZ, 64UZ}) {
        void* const ptr = stack.allocate(4, al);
        CHECK_EQ(reinterpret_cast<uintptr_t>(ptr) % al, 0);
      }
    }

    SUBCASE("Allocates until capacity is exhausted") {
      constexpr size_t kBytes = 32;
      constexpr size_t kBytesAlign = 16;
      FixedStackAllocator stack(kSmallCapacity);
      size_t allocation_count = 0;
      while (stack.Stats().total_allocated + kHeaderSize + kBytesAlign +
                 kBytes <=
             kSmallCapacity) {
        CHECK_NE(stack.allocate(kBytes, kBytesAlign), nullptr);
        ++allocation_count;
      }
      CHECK_GT(allocation_count, 0);
      CHECK_LE(stack.Stats().total_allocated, kSmallCapacity);
    }

    SUBCASE("Usable as PMR upstream for std::pmr::vector") {
      FixedStackAllocator stack(kCapacity);
      std::pmr::vector<int> vec(&stack);
      vec.push_back(1);
      vec.push_back(2);
      CHECK_EQ(vec.size(), 2);
    }
  }

  TEST_CASE("mem::FixedStackAllocator::deallocate") {
    SUBCASE("LIFO deallocate decrements allocation_count") {
      FixedStackAllocator stack(kCapacity);
      std::ignore = stack.allocate(64, kAlign);
      void* const second = stack.allocate(64, kAlign);
      stack.deallocate(second, 64, kAlign);
      CHECK_EQ(stack.Stats().allocation_count, 1);
    }

    SUBCASE("LIFO deallocate reduces total_allocated") {
      FixedStackAllocator stack(kCapacity);
      std::ignore = stack.allocate(64, kAlign);
      void* const second = stack.allocate(64, kAlign);
      const size_t allocated_before = stack.Stats().total_allocated;
      stack.deallocate(second, 64, kAlign);
      CHECK_LT(stack.Stats().total_allocated, allocated_before);
    }

    SUBCASE("LIFO deallocate increments total_deallocations") {
      FixedStackAllocator stack(kCapacity);
      void* const ptr = stack.allocate(64, kAlign);
      stack.deallocate(ptr, 64, kAlign);
      CHECK_EQ(stack.Stats().total_deallocations, 1);
    }

    SUBCASE("Non-LIFO deallocate is a no-op for allocation_count") {
      FixedStackAllocator stack(kCapacity);
      void* const first = stack.allocate(64, kAlign);
      std::ignore = stack.allocate(64, kAlign);
      stack.deallocate(first, 64, kAlign);
      CHECK_EQ(stack.Stats().allocation_count, 2);
    }

    SUBCASE("Non-LIFO deallocate is a no-op for total_allocated") {
      FixedStackAllocator stack(kCapacity);
      void* const first = stack.allocate(64, kAlign);
      std::ignore = stack.allocate(64, kAlign);
      const size_t allocated_before = stack.Stats().total_allocated;
      stack.deallocate(first, 64, kAlign);
      CHECK_EQ(stack.Stats().total_allocated, allocated_before);
    }

    SUBCASE("Non-LIFO deallocate does not increment total_deallocations") {
      FixedStackAllocator stack(kCapacity);
      void* const first = stack.allocate(64, kAlign);
      std::ignore = stack.allocate(64, kAlign);
      stack.deallocate(first, 64, kAlign);
      CHECK_EQ(stack.Stats().total_deallocations, 0);
    }

    SUBCASE("LIFO order frees nested allocations correctly") {
      FixedStackAllocator stack(kCapacity);
      void* const first = stack.allocate(32, kAlign);
      void* const second = stack.allocate(32, kAlign);
      void* const third = stack.allocate(32, kAlign);
      stack.deallocate(third, 32, kAlign);
      CHECK_EQ(stack.Stats().allocation_count, 2);
      stack.deallocate(second, 32, kAlign);
      CHECK_EQ(stack.Stats().allocation_count, 1);
      stack.deallocate(first, 32, kAlign);
      CHECK(stack.Empty());
    }
  }
}  // TEST_SUITE
