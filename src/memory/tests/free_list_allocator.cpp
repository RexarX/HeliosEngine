#include <doctest/doctest.h>

#include <helios/memory/free_list_allocator.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <thread>
#include <vector>

using namespace helios::mem;

namespace {

constexpr size_t kAlign = alignof(std::max_align_t);
// Minimum capacity that satisfies the FreeBlockHeader size assertion.
constexpr size_t kMinCap = 256;

constexpr FreeListAllocatorOptions GrowingOptions(size_t cap = kMinCap) {
  return FreeListAllocatorOptions{
      .initial_capacity = cap,
      .growth = GrowthPolicy::Linear(cap, std::numeric_limits<size_t>::max())};
}

}  // namespace

TEST_SUITE("helios::mem::FreeListAllocator") {
  TEST_CASE("mem::FreeListAllocator::ctor(FreeListAllocatorOptions)") {
    SUBCASE("Capacity is at least initial_capacity") {
      const FreeListAllocator alloc(
          FreeListAllocatorOptions{.initial_capacity = kMinCap});
      CHECK_GE(alloc.Capacity(), kMinCap);
    }

    SUBCASE("Allocator is empty on construction") {
      const FreeListAllocator alloc(
          FreeListAllocatorOptions{.initial_capacity = kMinCap});
      CHECK(alloc.Empty());
    }

    SUBCASE("AllocationCount is zero on construction") {
      const FreeListAllocator alloc(
          FreeListAllocatorOptions{.initial_capacity = kMinCap});
      CHECK_EQ(alloc.AllocationCount(), 0);
    }

    SUBCASE("UsedMemory is zero on construction") {
      const FreeListAllocator alloc(
          FreeListAllocatorOptions{.initial_capacity = kMinCap});
      CHECK_EQ(alloc.UsedMemory(), 0);
    }

    SUBCASE("Growth policy is stored correctly") {
      const auto policy = GrowthPolicy::Geometric();
      const FreeListAllocator alloc(FreeListAllocatorOptions{
          .initial_capacity = kMinCap, .growth = policy});
      CHECK_EQ(alloc.Growth().max_capacity, policy.max_capacity);
    }
  }

  TEST_CASE("mem::FreeListAllocator::ctor(size_t)") {
    SUBCASE("Capacity is at least the requested size") {
      const FreeListAllocator alloc(kMinCap);
      CHECK_GE(alloc.Capacity(), kMinCap);
    }

    SUBCASE("Uses geometric growth policy") {
      const FreeListAllocator alloc(kMinCap);
      CHECK_EQ(alloc.Growth().max_capacity,
               GrowthPolicy::Geometric().max_capacity);
    }

    SUBCASE("Allocator is empty on construction") {
      const FreeListAllocator alloc(kMinCap);
      CHECK(alloc.Empty());
    }
  }

  TEST_CASE("mem::FreeListAllocator::ctor(FreeListAllocator&&)") {
    SUBCASE("Moved-into allocator carries allocation state") {
      FreeListAllocator source(kMinCap);
      [[maybe_unused]] const void* _ = source.allocate(32, kAlign);

      FreeListAllocator moved(std::move(source));

      CHECK_FALSE(moved.Empty());
    }

    SUBCASE("Source capacity is zeroed after move") {
      FreeListAllocator source(kMinCap);
      FreeListAllocator moved(std::move(source));
      CHECK_EQ(source.Capacity(), 0);
    }

    SUBCASE("Source is empty after move") {
      FreeListAllocator source(kMinCap);
      [[maybe_unused]] const void* _ = source.allocate(32, kAlign);

      FreeListAllocator moved(std::move(source));

      CHECK(source.Empty());
    }
  }

  TEST_CASE("mem::FreeListAllocator::operator=(FreeListAllocator&&)") {
    SUBCASE("Target acquires source state") {
      FreeListAllocator source(kMinCap);
      [[maybe_unused]] const void* _ = source.allocate(32, kAlign);
      FreeListAllocator target(kMinCap);

      target = std::move(source);

      CHECK_FALSE(target.Empty());
    }

    SUBCASE("Source capacity is zeroed after move assign") {
      FreeListAllocator source(kMinCap);
      FreeListAllocator target(kMinCap);

      target = std::move(source);

      CHECK_EQ(source.Capacity(), 0);
    }

    SUBCASE("Self move assignment does not corrupt state") {
      FreeListAllocator alloc(kMinCap);
      [[maybe_unused]] const void* _ = alloc.allocate(16, kAlign);
      FreeListAllocator& ref = alloc;

      ref = std::move(alloc);  // NOLINT(clang-diagnostic-self-move)

      CHECK_FALSE(ref.Empty());
    }
  }

  TEST_CASE("mem::FreeListAllocator::Reset") {
    SUBCASE("Empty returns true after Reset") {
      FreeListAllocator alloc(kMinCap);
      [[maybe_unused]] const void* _ = alloc.allocate(32, kAlign);

      alloc.Reset();

      CHECK(alloc.Empty());
    }

    SUBCASE("AllocationCount is zero after Reset") {
      FreeListAllocator alloc(kMinCap);
      [[maybe_unused]] const void* _ = alloc.allocate(32, kAlign);

      alloc.Reset();

      CHECK_EQ(alloc.AllocationCount(), 0);
    }

    SUBCASE("Stats counters are zeroed after Reset") {
      FreeListAllocator alloc(kMinCap);
      [[maybe_unused]] const void* _ = alloc.allocate(32, kAlign);

      alloc.Reset();

      const AllocatorStats stats = alloc.Stats();
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
      CHECK_EQ(stats.allocation_count, 0);
    }

    SUBCASE("UsedMemory is zero after Reset") {
      FreeListAllocator alloc(kMinCap);
      [[maybe_unused]] const void* _ = alloc.allocate(32, kAlign);
      alloc.Reset();
      CHECK_EQ(alloc.UsedMemory(), 0);
    }

    SUBCASE("Allocation succeeds after Reset") {
      FreeListAllocator alloc(kMinCap);
      [[maybe_unused]] const void* _ = alloc.allocate(64, kAlign);
      alloc.Reset();
      void* const ptr = alloc.allocate(32, kAlign);
      CHECK_NE(ptr, nullptr);
    }

    SUBCASE("Capacity is preserved across Reset (regions not freed)") {
      FreeListAllocator alloc(GrowingOptions());
      [[maybe_unused]] const void* _ = alloc.allocate(32, kAlign);
      const size_t cap_before = alloc.Capacity();
      alloc.Reset();
      CHECK_EQ(alloc.Capacity(), cap_before);
    }
  }

  TEST_CASE("mem::FreeListAllocator::Empty") {
    SUBCASE("Returns true on fresh allocator") {
      const FreeListAllocator alloc(kMinCap);
      CHECK(alloc.Empty());
    }

    SUBCASE("Returns false after allocation") {
      FreeListAllocator alloc(kMinCap);
      [[maybe_unused]] const void* _ = alloc.allocate(16, kAlign);
      CHECK_FALSE(alloc.Empty());
    }

    SUBCASE("Returns true after every allocation is freed") {
      FreeListAllocator alloc(kMinCap);
      void* const first = alloc.allocate(16, kAlign);
      void* const second = alloc.allocate(16, kAlign);
      alloc.deallocate(first, 16, kAlign);
      alloc.deallocate(second, 16, kAlign);
      CHECK(alloc.Empty());
    }

    SUBCASE("Returns true after Reset") {
      FreeListAllocator alloc(kMinCap);
      [[maybe_unused]] const void* _ = alloc.allocate(32, kAlign);
      alloc.Reset();
      CHECK(alloc.Empty());
    }
  }

  TEST_CASE("mem::FreeListAllocator::Owns") {
    SUBCASE("Returns true for pointer allocated from this allocator") {
      FreeListAllocator alloc(kMinCap);
      void* const ptr = alloc.allocate(32, kAlign);
      CHECK(alloc.Owns(ptr));
    }

    SUBCASE("Returns false for nullptr") {
      const FreeListAllocator alloc(kMinCap);
      CHECK_FALSE(alloc.Owns(nullptr));
    }

    SUBCASE("Returns false for arbitrary stack pointer") {
      const FreeListAllocator alloc(kMinCap);
      int local = 0;
      CHECK_FALSE(alloc.Owns(&local));
    }

    SUBCASE("Returns false for pointer from first different allocator") {
      FreeListAllocator first(kMinCap);
      FreeListAllocator second(kMinCap);
      void* const ptr = second.allocate(32, kAlign);
      CHECK_FALSE(first.Owns(ptr));
    }

    SUBCASE("Returns true for pointer after deallocation (still in region)") {
      FreeListAllocator alloc(kMinCap);
      void* const ptr = alloc.allocate(32, kAlign);
      alloc.deallocate(ptr, 32, kAlign);
      // The region memory is not freed; the pointer still belongs to this
      // alloc.
      CHECK(alloc.Owns(ptr));
    }
  }

  TEST_CASE("mem::FreeListAllocator::Stats") {
    SUBCASE("All fields are zero on fresh allocator") {
      const FreeListAllocator alloc(kMinCap);

      const AllocatorStats stats = alloc.Stats();
      CHECK_EQ(stats.total_allocated, 0);
      CHECK_EQ(stats.peak_usage, 0);
      CHECK_EQ(stats.allocation_count, 0);
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
    }

    SUBCASE("total_allocations counts every allocate call") {
      FreeListAllocator alloc(kMinCap);

      const void* ptr = alloc.allocate(16, kAlign);
      ptr = alloc.allocate(16, kAlign);

      CHECK_EQ(alloc.Stats().total_allocations, 2);
    }

    SUBCASE("allocation_count tracks live allocations") {
      FreeListAllocator alloc(kMinCap);

      void* const first = alloc.allocate(16, kAlign);
      [[maybe_unused]] const void* _ = alloc.allocate(16, kAlign);

      CHECK_EQ(alloc.Stats().allocation_count, 2);
      alloc.deallocate(first, 16, kAlign);
      CHECK_EQ(alloc.Stats().allocation_count, 1);
    }

    SUBCASE("total_deallocations counts every deallocate call") {
      FreeListAllocator alloc(kMinCap);

      void* const first = alloc.allocate(16, kAlign);
      void* const second = alloc.allocate(16, kAlign);
      alloc.deallocate(first, 16, kAlign);
      alloc.deallocate(second, 16, kAlign);

      CHECK_EQ(alloc.Stats().total_deallocations, 2);
    }

    SUBCASE("peak_usage is at least the highest UsedMemory observed") {
      FreeListAllocator alloc(kMinCap * 4);

      void* const first = alloc.allocate(64, kAlign);
      void* const second = alloc.allocate(64, kAlign);
      const size_t peak_candidate = alloc.UsedMemory();
      alloc.deallocate(first, 64, kAlign);
      alloc.deallocate(second, 64, kAlign);

      CHECK_GE(alloc.Stats().peak_usage, peak_candidate);
    }
  }

  TEST_CASE("mem::FreeListAllocator::Capacity") {
    SUBCASE("Is at least initial_capacity on construction") {
      const FreeListAllocator alloc(
          FreeListAllocatorOptions{.initial_capacity = kMinCap});
      CHECK_GE(alloc.Capacity(), kMinCap);
    }

    SUBCASE("Increases when allocator grows into first new region") {
      FreeListAllocator alloc(FreeListAllocatorOptions{
          .initial_capacity = kMinCap,
          .growth = GrowthPolicy::Linear(kMinCap, kMinCap * 2)});

      const size_t before = alloc.Capacity();
      for (size_t i = 0; i < 64 && alloc.Capacity() == before; ++i) {
        void* const ptr = alloc.allocate(32, kAlign);
        REQUIRE(ptr != nullptr);
      }

      CHECK_GT(alloc.Capacity(), before);
    }
  }

  TEST_CASE("mem::FreeListAllocator::UsedMemory") {
    SUBCASE("Is zero on fresh allocator") {
      const FreeListAllocator alloc(kMinCap);
      CHECK_EQ(alloc.UsedMemory(), 0);
    }

    SUBCASE("Increases after allocation") {
      FreeListAllocator alloc(kMinCap);
      [[maybe_unused]] const void* _ = alloc.allocate(64, kAlign);
      CHECK_GT(alloc.UsedMemory(), 0);
    }

    SUBCASE("Decreases after deallocation") {
      FreeListAllocator alloc(kMinCap);

      void* const ptr = alloc.allocate(64, kAlign);
      const size_t used = alloc.UsedMemory();
      alloc.deallocate(ptr, 64, kAlign);

      CHECK_LT(alloc.UsedMemory(), used);
    }

    SUBCASE("Is zero after all allocations are freed") {
      FreeListAllocator alloc(kMinCap);

      void* const first = alloc.allocate(32, kAlign);
      void* const second = alloc.allocate(32, kAlign);
      alloc.deallocate(first, 32, kAlign);
      alloc.deallocate(second, 32, kAlign);

      CHECK_EQ(alloc.UsedMemory(), 0);
    }
  }

  TEST_CASE("mem::FreeListAllocator::FreeMemory") {
    SUBCASE("Is positive on fresh allocator") {
      const FreeListAllocator alloc(kMinCap);
      CHECK_GT(alloc.FreeMemory(), 0);
    }

    SUBCASE("Equals Capacity on fresh allocator (nothing used)") {
      const FreeListAllocator alloc(kMinCap);
      CHECK_EQ(alloc.FreeMemory(), alloc.Capacity());
    }

    SUBCASE("Decreases after allocation") {
      FreeListAllocator alloc(kMinCap);

      const size_t free_before = alloc.FreeMemory();
      [[maybe_unused]] const void* _ = alloc.allocate(64, kAlign);

      CHECK_LT(alloc.FreeMemory(), free_before);
    }

    SUBCASE("Increases after deallocation") {
      FreeListAllocator alloc(kMinCap);

      void* const ptr = alloc.allocate(64, kAlign);
      const size_t free_after_alloc = alloc.FreeMemory();
      alloc.deallocate(ptr, 64, kAlign);

      CHECK_GT(alloc.FreeMemory(), free_after_alloc);
    }
  }

  TEST_CASE("mem::FreeListAllocator::FreeBlockCount") {
    SUBCASE("Is 1 on fresh allocator (one contiguous free region)") {
      const FreeListAllocator alloc(kMinCap);
      CHECK_EQ(alloc.FreeBlockCount(), 1);
    }

    SUBCASE("Freeing first middle block creates extra free block") {
      FreeListAllocator alloc(kMinCap * 4);

      void* const first = alloc.allocate(32, kAlign);
      void* const second = alloc.allocate(32, kAlign);
      void* const third = alloc.allocate(32, kAlign);
      // Free 'second' leaving 'first' and 'third' alive — prevents full
      // coalescing.
      alloc.deallocate(second, 32, kAlign);
      CHECK_GE(alloc.FreeBlockCount(), 1);
      // Clean up.
      alloc.deallocate(first, 32, kAlign);
      alloc.deallocate(third, 32, kAlign);
    }

    SUBCASE("Freeing adjacent blocks in order causes coalescing back to 1") {
      FreeListAllocator alloc(kMinCap * 2);

      void* const first = alloc.allocate(32, kAlign);
      void* const second = alloc.allocate(32, kAlign);
      alloc.deallocate(first, 32, kAlign);
      alloc.deallocate(second, 32, kAlign);

      // After coalescing there should be exactly one free block again.
      CHECK_EQ(alloc.FreeBlockCount(), 1);
    }
  }

  TEST_CASE("mem::FreeListAllocator::AllocationCount") {
    SUBCASE("Is zero on fresh allocator") {
      const FreeListAllocator alloc(kMinCap);
      CHECK_EQ(alloc.AllocationCount(), 0);
    }

    SUBCASE("Increases by one per allocate call") {
      FreeListAllocator alloc(kMinCap);

      const void* ptr = alloc.allocate(16, kAlign);
      CHECK_EQ(alloc.AllocationCount(), 1);
      ptr = alloc.allocate(16, kAlign);
      CHECK_EQ(alloc.AllocationCount(), 2);
    }

    SUBCASE("Decreases by one per deallocate call") {
      FreeListAllocator alloc(kMinCap);

      void* const first = alloc.allocate(16, kAlign);
      void* const second = alloc.allocate(16, kAlign);
      alloc.deallocate(first, 16, kAlign);
      CHECK_EQ(alloc.AllocationCount(), 1);
      alloc.deallocate(second, 16, kAlign);
      CHECK_EQ(alloc.AllocationCount(), 0);
    }
  }

  TEST_CASE("mem::FreeListAllocator::Growth") {
    SUBCASE("Returns geometric policy for size_t ctor") {
      const FreeListAllocator alloc(kMinCap);
      CHECK_EQ(alloc.Growth().max_capacity,
               GrowthPolicy::Geometric().max_capacity);
    }

    SUBCASE("Returns policy supplied via options") {
      const GrowthPolicy policy =
          GrowthPolicy::Linear(kMinCap, std::numeric_limits<size_t>::max());
      const FreeListAllocator alloc(FreeListAllocatorOptions{
          .initial_capacity = kMinCap, .growth = policy});
      CHECK_EQ(alloc.Growth().max_capacity, policy.max_capacity);
    }

    SUBCASE("Unchanged by allocations") {
      FreeListAllocator alloc(kMinCap);
      [[maybe_unused]] const void* _ = alloc.allocate(32, kAlign);
      CHECK_EQ(alloc.Growth().max_capacity,
               GrowthPolicy::Geometric().max_capacity);
    }
  }

  TEST_CASE("mem::FreeListAllocator::allocate") {
    SUBCASE("Returns non-null pointer") {
      FreeListAllocator alloc(kMinCap);
      CHECK_NE(alloc.allocate(32, kAlign), nullptr);
    }

    SUBCASE("Multiple allocations return distinct pointers") {
      FreeListAllocator alloc(kMinCap * 4);

      void* const first = alloc.allocate(32, kAlign);
      void* const second = alloc.allocate(32, kAlign);

      CHECK_NE(first, second);
    }

    SUBCASE("Pointer satisfies the requested alignment") {
      FreeListAllocator alloc(kMinCap * 4);
      for (const size_t al : {1UZ, 4UZ, 8UZ, 16UZ, 32UZ}) {
        void* const ptr = alloc.allocate(4, al);
        CHECK_EQ(reinterpret_cast<uintptr_t>(ptr) % al, 0);
      }
    }

    SUBCASE("Allocation triggers growth when capacity is exhausted") {
      FreeListAllocator alloc(FreeListAllocatorOptions{
          .initial_capacity = kMinCap,
          .growth = GrowthPolicy::Linear(kMinCap, kMinCap * 2)});

      const size_t cap_before = alloc.Capacity();
      for (size_t i = 0; i < 64 && alloc.Capacity() == cap_before; ++i) {
        void* const ptr = alloc.allocate(16, kAlign);
        REQUIRE(ptr != nullptr);
      }

      CHECK_GT(alloc.Capacity(), cap_before);
    }

    SUBCASE("Usable as PMR upstream for std::pmr::vector") {
      FreeListAllocator alloc(4096);

      std::pmr::monotonic_buffer_resource mbr(256, &alloc);
      std::pmr::vector<int> vec(&mbr);
      vec.push_back(10);
      vec.push_back(20);

      CHECK_EQ(vec.size(), 2);
    }
  }

  TEST_CASE("mem::FreeListAllocator::deallocate") {
    SUBCASE("Freed memory is reusable for first subsequent allocation") {
      FreeListAllocator alloc(kMinCap);

      void* const first = alloc.allocate(64, kAlign);
      alloc.deallocate(first, 64, kAlign);
      void* const second = alloc.allocate(64, kAlign);

      CHECK_NE(second, nullptr);
    }

    SUBCASE("Interleaved alloc/dealloc cycle ends with empty allocator") {
      FreeListAllocator alloc(kMinCap * 4);

      void* const first = alloc.allocate(32, kAlign);
      void* const second = alloc.allocate(32, kAlign);
      void* const third = alloc.allocate(32, kAlign);
      alloc.deallocate(second, 32, kAlign);
      alloc.deallocate(first, 32, kAlign);
      alloc.deallocate(third, 32, kAlign);

      CHECK(alloc.Empty());
    }

    SUBCASE("AllocationCount decrements on each call") {
      FreeListAllocator alloc(kMinCap);

      void* const ptr = alloc.allocate(32, kAlign);
      alloc.deallocate(ptr, 32, kAlign);

      CHECK_EQ(alloc.AllocationCount(), 0);
    }
  }

  TEST_CASE("mem::FreeListAllocator thread-safety: concurrent allocate") {
    SUBCASE("All threads receive valid non-null pointers") {
      FreeListAllocator alloc(GrowingOptions(65536));

      constexpr size_t kThreads = 8;
      constexpr size_t kAllocsPerThread = 64;
      constexpr size_t kAllocSize = 16;

      std::atomic<int> null_count{0};
      std::vector<std::thread> threads;
      threads.reserve(kThreads);

      for (size_t j = 0; j < kThreads; ++j) {
        threads.emplace_back([&alloc, &null_count] {
          for (size_t i = 0; i < kAllocsPerThread; ++i) {
            void* const ptr = alloc.allocate(kAllocSize, kAlign);
            if (ptr == nullptr) {
              null_count.fetch_add(1, std::memory_order_relaxed);
            }
          }
        });
      }
      for (auto& th : threads) {
        th.join();
      }

      CHECK_EQ(null_count.load(), 0);
    }

    SUBCASE("total_allocations equals threads * allocs after concurrent run") {
      FreeListAllocator alloc(GrowingOptions(65536));

      constexpr size_t kThreads = 4;
      constexpr size_t kAllocsPerThread = 64;

      std::vector<std::thread> threads;
      threads.reserve(kThreads);
      for (size_t j = 0; j < kThreads; ++j) {
        threads.emplace_back([&alloc] {
          for (size_t i = 0; i < kAllocsPerThread; ++i) {
            [[maybe_unused]] const void* _ = alloc.allocate(8, kAlign);
          }
        });
      }
      for (auto& th : threads) {
        th.join();
      }

      CHECK_EQ(alloc.Stats().total_allocations, kThreads * kAllocsPerThread);
    }
  }

  TEST_CASE("mem::FreeListAllocator thread-safety: concurrent alloc+dealloc") {
    SUBCASE("Allocator is empty after paired alloc/dealloc across threads") {
      FreeListAllocator alloc(GrowingOptions(65536));

      constexpr size_t kThreads = 4;
      constexpr size_t kAllocsPerThread = 64;
      constexpr size_t kAllocSize = 16;

      std::vector<std::vector<void*>> ptrs(
          kThreads, std::vector<void*>(kAllocsPerThread));

      // Phase 1: all threads allocate.
      {
        std::vector<std::thread> threads;
        threads.reserve(kThreads);
        for (size_t j = 0; j < kThreads; ++j) {
          threads.emplace_back([&ptrs, &alloc, j] {
            for (size_t i = 0; i < kAllocsPerThread; ++i) {
              ptrs[j][i] = alloc.allocate(kAllocSize, kAlign);
            }
          });
        }
        for (auto& th : threads) {
          th.join();
        }
      }

      // Phase 2: all threads deallocate.
      {
        std::vector<std::thread> threads;
        threads.reserve(kThreads);
        for (size_t j = 0; j < kThreads; ++j) {
          threads.emplace_back([&alloc, &ptrs, j] {
            for (size_t i = 0; i < kAllocsPerThread; ++i) {
              alloc.deallocate(ptrs[j][i], kAllocSize, kAlign);
            }
          });
        }
        for (auto& th : threads) {
          th.join();
        }
      }

      CHECK(alloc.Empty());
    }
  }
}  // TEST_SUITE
