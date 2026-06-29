#include <doctest/doctest.h>

#include <helios/memory/fixed_arena_allocator.hpp>

#include <algorithm>
#include <atomic>
#include <barrier>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <thread>
#include <vector>

using namespace helios::mem;

namespace {

constexpr size_t kCapacity = 4096;
constexpr size_t kSmallCapacity = 256;
constexpr size_t kAlign = alignof(std::max_align_t);

}  // namespace

TEST_SUITE("helios::mem::FixedArenaAllocator") {
  TEST_CASE("mem::FixedArenaAllocator::ctor()") {
    SUBCASE("InitialCapacity equals configured capacity") {
      const FixedArenaAllocator arena(kCapacity);
      CHECK_EQ(arena.InitialCapacity(), kCapacity);
    }

    SUBCASE("TotalCapacity equals configured capacity") {
      const FixedArenaAllocator arena(kCapacity);
      CHECK_EQ(arena.TotalCapacity(), kCapacity);
    }

    SUBCASE("Arena is empty on construction") {
      const FixedArenaAllocator arena(kCapacity);
      CHECK(arena.Empty());
    }

    SUBCASE("Stats counters are zero on construction") {
      const FixedArenaAllocator arena(kCapacity);
      const AllocatorStats stats = arena.Stats();
      CHECK_EQ(stats.total_allocated, 0);
      CHECK_EQ(stats.peak_usage, 0);
      CHECK_EQ(stats.allocation_count, 0);
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
      CHECK_EQ(stats.alignment_waste, 0);
    }
  }

  TEST_CASE("mem::FixedArenaAllocator::ctor(FixedArenaAllocator&&)") {
    SUBCASE("Moved-into arena has same InitialCapacity") {
      FixedArenaAllocator source(kCapacity);
      const FixedArenaAllocator moved(std::move(source));
      CHECK_EQ(moved.InitialCapacity(), kCapacity);
    }

    SUBCASE("Moved-into arena carries existing allocation state") {
      FixedArenaAllocator source(kCapacity);
      [[maybe_unused]] const void* _ = source.allocate(64, kAlign);
      const FixedArenaAllocator moved(std::move(source));
      CHECK_FALSE(moved.Empty());
      CHECK_EQ(moved.Stats().allocation_count, 1);
    }

    SUBCASE("Source is empty after move") {
      FixedArenaAllocator source(kCapacity);
      [[maybe_unused]] const void* _ = source.allocate(32, kAlign);
      const FixedArenaAllocator moved(std::move(source));
      CHECK(source.Empty());
    }

    SUBCASE("Source stats are zeroed after move") {
      FixedArenaAllocator source(kCapacity);
      [[maybe_unused]] const void* _ = source.allocate(32, kAlign);
      const FixedArenaAllocator moved(std::move(source));
      CHECK_EQ(source.Stats().allocation_count, 0);
      CHECK_EQ(source.Stats().total_allocated, 0);
    }

    SUBCASE("Moved-into arena TotalCapacity is unchanged") {
      FixedArenaAllocator source(kCapacity);
      const FixedArenaAllocator moved(std::move(source));
      CHECK_EQ(moved.TotalCapacity(), kCapacity);
    }
  }

  TEST_CASE("mem::FixedArenaAllocator::operator=(FixedArenaAllocator&&)") {
    SUBCASE("Target acquires source allocation state") {
      FixedArenaAllocator source(kCapacity);
      [[maybe_unused]] const void* _ = source.allocate(32, kAlign);
      FixedArenaAllocator target(kCapacity);

      target = std::move(source);

      CHECK_FALSE(target.Empty());
      CHECK_EQ(target.Stats().allocation_count, 1);
    }

    SUBCASE("Source is empty after move assign") {
      FixedArenaAllocator source(kCapacity);
      [[maybe_unused]] const void* _ = source.allocate(32, kAlign);
      FixedArenaAllocator target(kCapacity);

      target = std::move(source);

      CHECK(source.Empty());
      CHECK_EQ(source.Stats().allocation_count, 0);
    }

    SUBCASE("Self move assignment does not corrupt state") {
      FixedArenaAllocator arena(kCapacity);
      [[maybe_unused]] const void* _ = arena.allocate(16, kAlign);
      FixedArenaAllocator& ref = arena;

      ref = std::move(arena);  // NOLINT(clang-diagnostic-self-move)

      CHECK_FALSE(ref.Empty());
    }

    SUBCASE("Target releases prior backing storage on move assign") {
      FixedArenaAllocator source(kCapacity);
      void* const source_ptr = source.allocate(64, kAlign);
      FixedArenaAllocator target(kCapacity);
      void* const target_ptr = target.allocate(64, kAlign);

      target = std::move(source);

      CHECK(target.Owns(source_ptr));
      CHECK_FALSE(target.Owns(target_ptr));
    }
  }

  TEST_CASE("mem::FixedArenaAllocator::Reset") {
    SUBCASE("Empty returns true after Reset") {
      FixedArenaAllocator arena(kCapacity);
      [[maybe_unused]] const void* _ = arena.allocate(128, kAlign);
      arena.Reset();
      CHECK(arena.Empty());
    }

    SUBCASE("TotalCapacity remains unchanged after Reset") {
      FixedArenaAllocator arena(kCapacity);
      [[maybe_unused]] const void* _ = arena.allocate(128, kAlign);
      arena.Reset();
      CHECK_EQ(arena.TotalCapacity(), kCapacity);
    }

    SUBCASE("Stats counters are zeroed after Reset") {
      FixedArenaAllocator arena(kCapacity);
      [[maybe_unused]] const void* _ = arena.allocate(64, kAlign);
      arena.Reset();
      const AllocatorStats stats = arena.Stats();
      CHECK_EQ(stats.total_allocated, 0);
      CHECK_EQ(stats.peak_usage, 0);
      CHECK_EQ(stats.allocation_count, 0);
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
      CHECK_EQ(stats.alignment_waste, 0);
    }

    SUBCASE("Allocations succeed after Reset") {
      FixedArenaAllocator arena(kCapacity);
      [[maybe_unused]] const void* _ = arena.allocate(512, kAlign);
      arena.Reset();
      void* const ptr = arena.allocate(64, kAlign);
      CHECK_NE(ptr, nullptr);
    }
  }

  TEST_CASE("mem::FixedArenaAllocator::Empty") {
    SUBCASE("Returns true on fresh arena") {
      const FixedArenaAllocator arena(kCapacity);
      CHECK(arena.Empty());
    }

    SUBCASE("Returns false after one allocation") {
      FixedArenaAllocator arena(kCapacity);
      [[maybe_unused]] const void* _ = arena.allocate(8, kAlign);
      CHECK_FALSE(arena.Empty());
    }

    SUBCASE("Returns true after Reset") {
      FixedArenaAllocator arena(kCapacity);
      [[maybe_unused]] const void* _ = arena.allocate(8, kAlign);
      arena.Reset();
      CHECK(arena.Empty());
    }

    SUBCASE("Deallocate does not affect Empty (arena semantics)") {
      FixedArenaAllocator arena(kCapacity);
      void* const ptr = arena.allocate(8, kAlign);
      arena.deallocate(ptr, 8, kAlign);
      CHECK_FALSE(arena.Empty());
    }
  }

  TEST_CASE("mem::FixedArenaAllocator::Owns") {
    SUBCASE("Returns true for pointer from this arena") {
      FixedArenaAllocator arena(kCapacity);
      void* const ptr = arena.allocate(64, kAlign);
      CHECK(arena.Owns(ptr));
    }

    SUBCASE("Returns false for nullptr") {
      const FixedArenaAllocator arena(kCapacity);
      CHECK_FALSE(arena.Owns(nullptr));
    }

    SUBCASE("Returns false for stack variable pointer") {
      const FixedArenaAllocator arena(kCapacity);
      int local = 0;
      CHECK_FALSE(arena.Owns(&local));
    }

    SUBCASE("Returns false for pointer from a different arena") {
      FixedArenaAllocator first(kCapacity);
      FixedArenaAllocator second(kCapacity);
      void* const ptr = second.allocate(64, kAlign);
      CHECK_FALSE(first.Owns(ptr));
    }

    SUBCASE("Returns true for pointer after deallocation (backing not freed)") {
      FixedArenaAllocator arena(kCapacity);
      void* const ptr = arena.allocate(64, kAlign);
      arena.deallocate(ptr, 64, kAlign);
      CHECK(arena.Owns(ptr));
    }

    SUBCASE("Returns false for moved-from arena pointer") {
      FixedArenaAllocator source(kCapacity);
      void* const ptr = source.allocate(64, kAlign);
      FixedArenaAllocator moved(std::move(source));
      CHECK(moved.Owns(ptr));
      CHECK_FALSE(source.Owns(ptr));
    }
  }

  TEST_CASE("mem::FixedArenaAllocator::Stats") {
    SUBCASE("All fields are zero on fresh arena") {
      const FixedArenaAllocator arena(kCapacity);
      const AllocatorStats stats = arena.Stats();
      CHECK_EQ(stats.total_allocated, 0);
      CHECK_EQ(stats.peak_usage, 0);
      CHECK_EQ(stats.allocation_count, 0);
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
      CHECK_EQ(stats.alignment_waste, 0);
    }

    SUBCASE("total_allocated reflects bump offset after allocations") {
      FixedArenaAllocator arena(kCapacity);
      const void* ptr = arena.allocate(64, kAlign);
      ptr = arena.allocate(128, kAlign);
      CHECK_GE(arena.Stats().total_allocated, 192);
    }

    SUBCASE("allocation_count tracks live allocation count") {
      FixedArenaAllocator arena(kCapacity);
      const void* ptr = arena.allocate(32, kAlign);
      ptr = arena.allocate(32, kAlign);
      CHECK_EQ(arena.Stats().allocation_count, 2);
    }

    SUBCASE("total_allocations counts every allocate call") {
      FixedArenaAllocator arena(kCapacity);
      const void* ptr = arena.allocate(16, kAlign);
      ptr = arena.allocate(16, kAlign);
      ptr = arena.allocate(16, kAlign);
      CHECK_EQ(arena.Stats().total_allocations, 3);
    }

    SUBCASE("total_deallocations increments on each deallocate call") {
      FixedArenaAllocator arena(kCapacity);
      void* const first = arena.allocate(32, kAlign);
      void* const second = arena.allocate(32, kAlign);
      arena.deallocate(first, 32, kAlign);
      arena.deallocate(second, 32, kAlign);
      CHECK_EQ(arena.Stats().total_deallocations, 2);
    }

    SUBCASE("peak_usage is at least total bytes allocated at high watermark") {
      FixedArenaAllocator arena(kCapacity);
      const void* ptr = arena.allocate(256, kAlign);
      ptr = arena.allocate(128, kAlign);
      CHECK_GE(arena.Stats().peak_usage, 256);
    }

    SUBCASE("alignment_waste is non-negative with large alignment request") {
      FixedArenaAllocator arena(kCapacity);
      [[maybe_unused]] const void* _ = arena.allocate(1, 64);
      CHECK_GE(arena.Stats().alignment_waste, 0);
    }
  }

  TEST_CASE("mem::FixedArenaAllocator::InitialCapacity") {
    SUBCASE("Returns configured capacity template argument") {
      const FixedArenaAllocator arena(1024);
      CHECK_EQ(arena.InitialCapacity(), 1024);
    }

    SUBCASE("Unchanged by allocations") {
      FixedArenaAllocator arena(kCapacity);
      [[maybe_unused]] const void* _ = arena.allocate(512, kAlign);
      CHECK_EQ(arena.InitialCapacity(), kCapacity);
    }

    SUBCASE("Unchanged after Reset") {
      FixedArenaAllocator arena(kCapacity);
      [[maybe_unused]] const void* _ = arena.allocate(512, kAlign);
      arena.Reset();
      CHECK_EQ(arena.InitialCapacity(), kCapacity);
    }
  }

  TEST_CASE("mem::FixedArenaAllocator::TotalCapacity") {
    SUBCASE("Equals configured capacity immediately after construction") {
      const FixedArenaAllocator arena(kCapacity);
      CHECK_EQ(arena.TotalCapacity(), kCapacity);
    }

    SUBCASE("Unchanged by allocations") {
      FixedArenaAllocator arena(kCapacity);
      [[maybe_unused]] const void* _ = arena.allocate(512, kAlign);
      CHECK_EQ(arena.TotalCapacity(), kCapacity);
    }

    SUBCASE("Unchanged after Reset") {
      FixedArenaAllocator arena(kCapacity);
      [[maybe_unused]] const void* _ = arena.allocate(512, kAlign);
      arena.Reset();
      CHECK_EQ(arena.TotalCapacity(), kCapacity);
    }
  }

  TEST_CASE("mem::FixedArenaAllocator::allocate") {
    SUBCASE("Returns non-null pointer") {
      FixedArenaAllocator arena(kCapacity);
      CHECK_NE(arena.allocate(32, kAlign), nullptr);
    }

    SUBCASE(
        "Consecutive allocations return distinct non-overlapping pointers") {
      FixedArenaAllocator arena(kCapacity);
      void* const first = arena.allocate(64, kAlign);
      void* const second = arena.allocate(64, kAlign);
      CHECK_NE(first, second);
      const auto ua = reinterpret_cast<uintptr_t>(first);
      const auto ub = reinterpret_cast<uintptr_t>(second);
      const bool non_overlapping = ub >= ua + 64 || ua >= ub + 64;
      CHECK(non_overlapping);
    }

    SUBCASE("Pointer satisfies the requested alignment") {
      FixedArenaAllocator arena(kCapacity);
      for (const size_t al : {1UZ, 4UZ, 8UZ, 16UZ, 32UZ, 64UZ}) {
        void* const ptr = arena.allocate(4, al);
        CHECK_EQ(reinterpret_cast<uintptr_t>(ptr) % al, 0);
      }
    }

    SUBCASE("Allocates until capacity is exhausted") {
      constexpr size_t kChunk = 32;
      constexpr size_t kChunkAlign = 32;
      FixedArenaAllocator arena(kSmallCapacity);
      while (arena.Stats().total_allocated + kChunk + (kChunkAlign - 1) <=
             kSmallCapacity) {
        CHECK_NE(arena.allocate(kChunk, kChunkAlign), nullptr);
      }
      CHECK_GT(arena.Stats().allocation_count, 0);
      CHECK_LE(arena.Stats().total_allocated, kSmallCapacity);
    }

    SUBCASE("Usable as PMR upstream for std::pmr::vector") {
      FixedArenaAllocator arena(kCapacity);
      std::pmr::vector<int> vec(&arena);
      vec.push_back(1);
      vec.push_back(2);
      CHECK_EQ(vec.size(), 2);
    }
  }

  TEST_CASE("mem::FixedArenaAllocator::deallocate") {
    SUBCASE("Is a no-op: subsequent allocation still returns valid pointer") {
      FixedArenaAllocator arena(kCapacity);
      void* const first = arena.allocate(32, kAlign);
      arena.deallocate(first, 32, kAlign);
      void* const second = arena.allocate(32, kAlign);
      CHECK_NE(second, nullptr);
    }

    SUBCASE("Increments total_deallocations in Stats") {
      FixedArenaAllocator arena(kCapacity);
      void* const ptr = arena.allocate(16, kAlign);
      arena.deallocate(ptr, 16, kAlign);
      CHECK_EQ(arena.Stats().total_deallocations, 1);
    }

    SUBCASE("Does not decrement allocation_count (arena semantics)") {
      FixedArenaAllocator arena(kCapacity);
      void* const ptr = arena.allocate(16, kAlign);
      arena.deallocate(ptr, 16, kAlign);
      CHECK_EQ(arena.Stats().allocation_count, 1);
    }

    SUBCASE("Does not reduce total_allocated (arena semantics)") {
      FixedArenaAllocator arena(kCapacity);
      void* const ptr = arena.allocate(64, kAlign);
      const size_t allocated_before = arena.Stats().total_allocated;
      arena.deallocate(ptr, 64, kAlign);
      CHECK_EQ(arena.Stats().total_allocated, allocated_before);
    }
  }

  TEST_CASE("mem::FixedArenaAllocator thread-safety: concurrent allocate") {
    SUBCASE("All threads receive valid non-null pointers") {
      FixedArenaAllocator arena(65536);
      constexpr size_t kThreads = 8;
      constexpr size_t kAllocsPerThread = 128;
      constexpr size_t kAllocSize = 8;
      std::atomic<int> null_count{0};
      std::vector<std::thread> threads;
      threads.reserve(kThreads);
      for (size_t j = 0; j < kThreads; ++j) {
        threads.emplace_back([&arena, &null_count] {
          for (size_t i = 0; i < kAllocsPerThread; ++i) {
            void* const ptr = arena.allocate(kAllocSize, kAlign);
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

    SUBCASE("total_allocations equals exact thread * allocs product") {
      FixedArenaAllocator arena(65536);
      constexpr size_t kThreads = 4;
      constexpr size_t kAllocsPerThread = 64;
      std::vector<std::thread> threads;
      threads.reserve(kThreads);
      for (size_t j = 0; j < kThreads; ++j) {
        threads.emplace_back([&arena] {
          for (size_t i = 0; i < kAllocsPerThread; ++i) {
            [[maybe_unused]] const void* _ = arena.allocate(8, kAlign);
          }
        });
      }
      for (auto& th : threads) {
        th.join();
      }
      CHECK_EQ(arena.Stats().total_allocations, kThreads * kAllocsPerThread);
    }

    SUBCASE("Concurrent allocations produce no duplicate pointers") {
      FixedArenaAllocator arena(65536);
      constexpr size_t kThreads = 4;
      constexpr size_t kAllocsPerThread = 256;
      constexpr size_t kAllocSize = 16;
      std::vector<std::vector<void*>> per_thread(kThreads);
      std::vector<std::thread> threads;
      threads.reserve(kThreads);
      for (size_t j = 0; j < kThreads; ++j) {
        per_thread[j].resize(kAllocsPerThread);
        threads.emplace_back([&per_thread, &arena, j] {
          for (size_t i = 0; i < kAllocsPerThread; ++i) {
            per_thread[j][i] = arena.allocate(kAllocSize, kAlign);
          }
        });
      }
      for (auto& th : threads) {
        th.join();
      }
      std::vector<void*> all;
      all.reserve(kThreads * kAllocsPerThread);
      for (auto& val : per_thread) {
        all.insert(all.end(), val.begin(), val.end());
      }
      std::ranges::sort(all);
      CHECK_EQ(std::ranges::adjacent_find(all), all.end());
    }

    SUBCASE("Retries when another thread advances the bump offset") {
      FixedArenaAllocator arena(65536);
      constexpr size_t kThreads = 8;
      constexpr size_t kAllocations = 64;
      std::barrier start(static_cast<std::ptrdiff_t>(kThreads));
      std::atomic<size_t> successes{0};
      std::vector<std::thread> threads;
      threads.reserve(kThreads);
      for (size_t index = 0; index < kThreads; ++index) {
        threads.emplace_back([&] {
          start.arrive_and_wait();
          for (size_t allocation = 0; allocation < kAllocations; ++allocation) {
            if (arena.allocate(32, kAlign) != nullptr) {
              successes.fetch_add(1, std::memory_order_relaxed);
            }
          }
        });
      }
      for (auto& thread : threads) {
        thread.join();
      }
      CHECK_EQ(successes.load(), kThreads * kAllocations);
    }
  }

  TEST_CASE("mem::FixedArenaAllocator thread-safety: concurrent deallocate") {
    SUBCASE("Concurrent deallocate increments total_deallocations correctly") {
      FixedArenaAllocator arena(65536);
      constexpr size_t kThreads = 4;
      constexpr size_t kAllocsPerThread = 64;
      std::vector<std::vector<void*>> ptrs(kThreads);
      for (size_t j = 0; j < kThreads; ++j) {
        ptrs[j].resize(kAllocsPerThread);
        for (size_t i = 0; i < kAllocsPerThread; ++i) {
          ptrs[j][i] = arena.allocate(8, kAlign);
        }
      }
      std::vector<std::thread> threads;
      threads.reserve(kThreads);
      for (size_t j = 0; j < kThreads; ++j) {
        threads.emplace_back([&arena, &ptrs, j] {
          for (size_t i = 0; i < kAllocsPerThread; ++i) {
            arena.deallocate(ptrs[j][i], 8, kAlign);
          }
        });
      }
      for (auto& th : threads) {
        th.join();
      }
      CHECK_EQ(arena.Stats().total_deallocations, kThreads * kAllocsPerThread);
    }
  }
}  // TEST_SUITE
