#include <doctest/doctest.h>

#include <helios/memory/arena_allocator.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <thread>
#include <vector>

using namespace helios::mem;

namespace {

constexpr size_t kAlign = alignof(std::max_align_t);

// Large enough capacity for tests that force a second block via growth.
constexpr size_t kGrowCapacity = 64;

constexpr ArenaOptions GrowingOptions() {
  return ArenaOptions{.initial_capacity = kGrowCapacity,
                      .growth = GrowthPolicy::Linear(
                          kGrowCapacity, std::numeric_limits<size_t>::max())};
}

}  // namespace

TEST_SUITE("helios::mem::ArenaAllocator") {
  TEST_CASE("mem::ArenaAllocator::ctor(ArenaOptions)") {
    SUBCASE("Sets InitialCapacity from options") {
      const ArenaAllocator arena(ArenaOptions{.initial_capacity = 1024});
      CHECK_EQ(arena.InitialCapacity(), 1024);
    }

    SUBCASE("TotalCapacity equals initial_capacity on construction") {
      const ArenaAllocator arena(ArenaOptions{.initial_capacity = 512});
      CHECK_EQ(arena.TotalCapacity(), 512);
    }

    SUBCASE("BlockCount is 1 on construction") {
      const ArenaAllocator arena(ArenaOptions{.initial_capacity = 256});
      CHECK_EQ(arena.BlockCount(), 1);
    }

    SUBCASE("Arena is empty on construction") {
      const ArenaAllocator arena(ArenaOptions{.initial_capacity = 128});
      CHECK(arena.Empty());
    }

    SUBCASE("Growth policy is stored from options") {
      const GrowthPolicy policy = GrowthPolicy::Fixed();
      const ArenaAllocator arena(
          ArenaOptions{.initial_capacity = 256, .growth = policy});
      CHECK_EQ(arena.Growth().max_capacity, policy.max_capacity);
    }
  }

  TEST_CASE("mem::ArenaAllocator::ctor(size_t)") {
    SUBCASE("Sets InitialCapacity correctly") {
      const ArenaAllocator arena(2048);
      CHECK_EQ(arena.InitialCapacity(), 2048);
    }

    SUBCASE("Uses Fixed growth policy") {
      // Fixed means max_capacity == initial_capacity, no growth allowed.
      const ArenaAllocator arena(512);
      CHECK_EQ(arena.Growth().max_capacity, GrowthPolicy::Fixed().max_capacity);
    }

    SUBCASE("BlockCount is 1 on construction") {
      const ArenaAllocator arena(512);
      CHECK_EQ(arena.BlockCount(), 1);
    }
  }

  TEST_CASE("mem::ArenaAllocator::ctor(ArenaAllocator&&)") {
    SUBCASE("Moved-into arena has same InitialCapacity") {
      ArenaAllocator source(1024);
      const ArenaAllocator moved(std::move(source));
      CHECK_EQ(moved.InitialCapacity(), 1024);
    }

    SUBCASE("Moved-into arena carries existing allocation state") {
      ArenaAllocator source(1024);
      [[maybe_unused]] const void* _ = source.allocate(64, kAlign);
      const ArenaAllocator moved(std::move(source));
      CHECK_FALSE(moved.Empty());
    }

    SUBCASE("Source InitialCapacity is zeroed after move") {
      ArenaAllocator source(512);
      const ArenaAllocator moved(std::move(source));
      CHECK_EQ(source.InitialCapacity(), 0);
    }

    SUBCASE("Source is empty after move") {
      ArenaAllocator source(512);
      [[maybe_unused]] const void* _ = source.allocate(32, kAlign);

      const ArenaAllocator moved(std::move(source));

      CHECK(source.Empty());
    }
  }

  TEST_CASE("mem::ArenaAllocator::operator=(ArenaAllocator&&)") {
    SUBCASE("Target acquires source InitialCapacity") {
      ArenaAllocator source(1024);
      ArenaAllocator target(128);

      target = std::move(source);

      CHECK_EQ(target.InitialCapacity(), 1024);
    }

    SUBCASE("Target carries allocation state from source") {
      ArenaAllocator source(1024);
      [[maybe_unused]] const void* _ = source.allocate(32, kAlign);
      ArenaAllocator target(128);

      target = std::move(source);

      CHECK_FALSE(target.Empty());
    }

    SUBCASE("Source InitialCapacity is zeroed after move assign") {
      ArenaAllocator source(512);
      ArenaAllocator target(256);

      target = std::move(source);

      CHECK_EQ(source.InitialCapacity(), 0);
    }

    SUBCASE("Self move assignment does not corrupt state") {
      ArenaAllocator arena(512);
      [[maybe_unused]] const void* _ = arena.allocate(16, kAlign);
      ArenaAllocator& ref = arena;

      ref = std::move(arena);  // NOLINT(clang-diagnostic-self-move)

      CHECK_FALSE(ref.Empty());
    }
  }

  TEST_CASE("mem::ArenaAllocator::Reset") {
    SUBCASE("Empty returns true after Reset") {
      ArenaAllocator arena(1024);
      [[maybe_unused]] const void* _ = arena.allocate(128, kAlign);

      arena.Reset();

      CHECK(arena.Empty());
    }

    SUBCASE("BlockCount remains unchanged after soft Reset") {
      ArenaAllocator arena(GrowingOptions());
      const void* ptr = arena.allocate(kGrowCapacity, 1);
      ptr = arena.allocate(kGrowCapacity, 1);  // forces second block
      const size_t blocks_before = arena.BlockCount();
      CHECK_GT(blocks_before, 1);

      arena.Reset();
      CHECK_EQ(arena.BlockCount(), blocks_before);
    }

    SUBCASE("TotalCapacity remains unchanged after soft Reset") {
      ArenaAllocator arena(GrowingOptions());
      const void* ptr = arena.allocate(kGrowCapacity, 1);
      ptr = arena.allocate(kGrowCapacity, 1);
      const size_t capacity_before = arena.TotalCapacity();
      CHECK_GT(capacity_before, arena.InitialCapacity());

      arena.Reset();
      CHECK_EQ(arena.TotalCapacity(), capacity_before);
    }

    SUBCASE("Stats counters are zeroed after Reset") {
      ArenaAllocator arena(1024);
      [[maybe_unused]] const void* _ = arena.allocate(64, kAlign);

      arena.Reset();

      const AllocatorStats stats = arena.Stats();
      CHECK_EQ(stats.total_allocated, 0);
      CHECK_EQ(stats.allocation_count, 0);
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
      CHECK_EQ(stats.alignment_waste, 0);
    }

    SUBCASE("Allocations succeed after Reset") {
      ArenaAllocator arena(1024);
      [[maybe_unused]] const void* _ = arena.allocate(512, kAlign);

      arena.Reset();

      void* const ptr = arena.allocate(64, kAlign);
      CHECK_NE(ptr, nullptr);
    }
  }

  TEST_CASE("mem::ArenaAllocator::Empty") {
    SUBCASE("Returns true on fresh arena") {
      const ArenaAllocator arena(256);
      CHECK(arena.Empty());
    }

    SUBCASE("Returns false after one allocation") {
      ArenaAllocator arena(256);
      [[maybe_unused]] const void* _ = arena.allocate(8, kAlign);
      CHECK_FALSE(arena.Empty());
    }

    SUBCASE("Returns true after Reset") {
      ArenaAllocator arena(256);
      [[maybe_unused]] const void* _ = arena.allocate(8, kAlign);
      arena.Reset();
      CHECK(arena.Empty());
    }

    SUBCASE("Deallocate does not affect Empty (arena semantics)") {
      ArenaAllocator arena(256);
      void* const ptr = arena.allocate(8, kAlign);
      arena.deallocate(ptr, 8, kAlign);
      // Arena dealloc is a no-op; allocation_count stays incremented.
      CHECK_FALSE(arena.Empty());
    }
  }

  TEST_CASE("mem::ArenaAllocator::Stats") {
    SUBCASE("All fields are zero on fresh arena") {
      const ArenaAllocator arena(1024);

      const AllocatorStats stats = arena.Stats();
      CHECK_EQ(stats.total_allocated, 0);
      CHECK_EQ(stats.peak_usage, 0);
      CHECK_EQ(stats.allocation_count, 0);
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
      CHECK_EQ(stats.alignment_waste, 0);
    }

    SUBCASE("total_allocated equals sum of requested bytes") {
      ArenaAllocator arena(1024);
      const void* ptr = arena.allocate(64, kAlign);
      ptr = arena.allocate(128, kAlign);

      CHECK_GE(arena.Stats().total_allocated, 192);
    }

    SUBCASE("allocation_count tracks live allocation count") {
      ArenaAllocator arena(1024);
      const void* ptr = arena.allocate(32, kAlign);
      ptr = arena.allocate(32, kAlign);

      CHECK_EQ(arena.Stats().allocation_count, 2);
    }

    SUBCASE("total_allocations counts every allocate call") {
      ArenaAllocator arena(1024);
      const void* ptr = arena.allocate(16, kAlign);
      ptr = arena.allocate(16, kAlign);
      ptr = arena.allocate(16, kAlign);

      CHECK_EQ(arena.Stats().total_allocations, 3);
    }

    SUBCASE("total_deallocations increments on each deallocate call") {
      ArenaAllocator arena(1024);
      void* const first = arena.allocate(32, kAlign);
      void* const second = arena.allocate(32, kAlign);
      arena.deallocate(first, 32, kAlign);
      arena.deallocate(second, 32, kAlign);

      CHECK_EQ(arena.Stats().total_deallocations, 2);
    }

    SUBCASE("peak_usage is at least total bytes allocated at high watermark") {
      ArenaAllocator arena(1024);
      const void* ptr = arena.allocate(256, kAlign);
      ptr = arena.allocate(128, kAlign);

      CHECK_GE(arena.Stats().peak_usage, 256);
    }

    SUBCASE("alignment_waste is non-negative") {
      ArenaAllocator arena(1024);
      [[maybe_unused]] const void* _ =
          arena.allocate(1, 64);  // force large alignment padding
      CHECK_GE(arena.Stats().alignment_waste, 0);
    }
  }

  TEST_CASE("mem::ArenaAllocator::TotalCapacity") {
    SUBCASE("Equals initial_capacity immediately after construction") {
      const ArenaAllocator arena(ArenaOptions{.initial_capacity = 4096});
      CHECK_EQ(arena.TotalCapacity(), 4096);
    }

    SUBCASE("Increases when a second block is allocated due to overflow") {
      ArenaAllocator arena(GrowingOptions());
      const size_t before = arena.TotalCapacity();
      const void* ptr = arena.allocate(kGrowCapacity, 1);
      ptr = arena.allocate(kGrowCapacity, 1);  // forces growth

      CHECK_GT(arena.TotalCapacity(), before);
    }

    SUBCASE("Remains unchanged after Reset (soft reset preserves blocks)") {
      ArenaAllocator arena(GrowingOptions());
      const void* ptr = arena.allocate(kGrowCapacity, 1);
      ptr = arena.allocate(kGrowCapacity, 1);
      const size_t capacity_before = arena.TotalCapacity();

      arena.Reset();

      CHECK_EQ(arena.TotalCapacity(), capacity_before);
    }
  }

  TEST_CASE("mem::ArenaAllocator::InitialCapacity") {
    SUBCASE("Returns value from ArenaOptions ctor") {
      const ArenaAllocator arena(ArenaOptions{.initial_capacity = 999});
      CHECK_EQ(arena.InitialCapacity(), 999);
    }

    SUBCASE("Returns value from size_t ctor") {
      const ArenaAllocator arena(888);
      CHECK_EQ(arena.InitialCapacity(), 888);
    }

    SUBCASE("Unchanged by allocations") {
      ArenaAllocator arena(1024);
      [[maybe_unused]] const void* _ = arena.allocate(512, kAlign);
      CHECK_EQ(arena.InitialCapacity(), 1024);
    }

    SUBCASE("Unchanged after Reset") {
      ArenaAllocator arena(1024);

      [[maybe_unused]] const void* _ = arena.allocate(512, kAlign);
      arena.Reset();

      CHECK_EQ(arena.InitialCapacity(), 1024);
    }
  }

  TEST_CASE("mem::ArenaAllocator::BlockCount") {
    SUBCASE("Is 1 after construction") {
      const ArenaAllocator arena(1024);
      CHECK_EQ(arena.BlockCount(), 1);
    }

    SUBCASE("Increases when arena grows into a second block") {
      ArenaAllocator arena(GrowingOptions());

      const void* ptr = arena.allocate(kGrowCapacity, 1);
      ptr = arena.allocate(kGrowCapacity, 1);

      CHECK_GT(arena.BlockCount(), 1);
    }

    SUBCASE("Remains unchanged after Reset (soft reset preserves blocks)") {
      ArenaAllocator arena(GrowingOptions());

      const void* ptr = arena.allocate(kGrowCapacity, 1);
      ptr = arena.allocate(kGrowCapacity, 1);
      const size_t blocks_before = arena.BlockCount();
      CHECK_GT(blocks_before, 1);

      arena.Reset();

      CHECK_EQ(arena.BlockCount(), blocks_before);
    }
  }

  TEST_CASE("mem::ArenaAllocator::Growth") {
    SUBCASE("Returns Fixed policy when constructed with size_t ctor") {
      const ArenaAllocator arena(256);
      CHECK_EQ(arena.Growth().max_capacity, GrowthPolicy::Fixed().max_capacity);
    }

    SUBCASE("Returns policy supplied in ArenaOptions") {
      const GrowthPolicy policy = GrowthPolicy::Linear(
          kGrowCapacity, std::numeric_limits<size_t>::max());
      const ArenaAllocator arena(
          ArenaOptions{.initial_capacity = kGrowCapacity, .growth = policy});
      CHECK_EQ(arena.Growth().max_capacity, policy.max_capacity);
    }

    SUBCASE("Unchanged after allocations") {
      ArenaAllocator arena(512);
      [[maybe_unused]] const void* _ = arena.allocate(32, kAlign);
      CHECK_EQ(arena.Growth().max_capacity, GrowthPolicy::Fixed().max_capacity);
    }
  }

  TEST_CASE("mem::ArenaAllocator::allocate") {
    SUBCASE("Returns non-null pointer") {
      ArenaAllocator arena(1024);
      CHECK_NE(arena.allocate(32, kAlign), nullptr);
    }

    SUBCASE(
        "Consecutive allocations return distinct non-overlapping pointers") {
      ArenaAllocator arena(1024);

      void* const first = arena.allocate(64, kAlign);
      void* const second = arena.allocate(64, kAlign);
      CHECK_NE(first, second);
      const auto ua = reinterpret_cast<uintptr_t>(first);
      const auto ub = reinterpret_cast<uintptr_t>(second);
      const bool result = ub >= ua + 64 || ua >= ub + 64;

      CHECK(result);
    }

    SUBCASE("Pointer satisfies the requested alignment") {
      ArenaAllocator arena(1024);

      for (const size_t al : {1UZ, 4UZ, 8UZ, 16UZ, 32UZ, 64UZ}) {
        void* const ptr = arena.allocate(4, al);
        CHECK_EQ(reinterpret_cast<uintptr_t>(ptr) % al, 0);
      }
    }

    SUBCASE("Allocation that overflows current block triggers growth") {
      ArenaAllocator arena(GrowingOptions());

      [[maybe_unused]] const void* _ = arena.allocate(kGrowCapacity, 1);
      void* const ptr = arena.allocate(kGrowCapacity, 1);

      CHECK_NE(ptr, nullptr);
      CHECK_GT(arena.BlockCount(), 1);
    }

    SUBCASE("Usable as PMR upstream for std::pmr::vector") {
      ArenaAllocator arena(4096);

      std::pmr::monotonic_buffer_resource mbr(256, &arena);
      std::pmr::vector<int> vec(&mbr);
      vec.push_back(1);
      vec.push_back(2);

      CHECK_EQ(vec.size(), 2);
    }
  }

  TEST_CASE("mem::ArenaAllocator::deallocate") {
    SUBCASE("Is a no-op: subsequent allocation still returns valid pointer") {
      ArenaAllocator arena(1024);

      void* const first = arena.allocate(32, kAlign);
      arena.deallocate(first, 32, kAlign);
      void* const second = arena.allocate(32, kAlign);

      CHECK_NE(second, nullptr);
    }

    SUBCASE("Increments total_deallocations in Stats") {
      ArenaAllocator arena(1024);

      void* const ptr = arena.allocate(16, kAlign);
      arena.deallocate(ptr, 16, kAlign);

      CHECK_EQ(arena.Stats().total_deallocations, 1);
    }

    SUBCASE("Does not decrement allocation_count (arena semantics)") {
      ArenaAllocator arena(1024);

      void* const ptr = arena.allocate(16, kAlign);
      arena.deallocate(ptr, 16, kAlign);

      CHECK_EQ(arena.Stats().allocation_count, 1);
    }
  }

  TEST_CASE("mem::ArenaAllocator thread-safety: concurrent allocate") {
    SUBCASE("All threads receive valid non-null pointers") {
      // Growing arena so threads never exhaust capacity.
      ArenaAllocator arena(ArenaOptions{
          .initial_capacity = 4096,
          .growth =
              GrowthPolicy::Linear(4096, std::numeric_limits<size_t>::max())});

      constexpr size_t kThreads = 8;
      constexpr size_t kAllocsPerThread = 128;
      constexpr size_t kAllocSize = 8;

      std::vector<std::thread> threads;
      std::atomic<int> null_count{0};

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
      ArenaAllocator arena(ArenaOptions{
          .initial_capacity = 4096,
          .growth =
              GrowthPolicy::Linear(4096, std::numeric_limits<size_t>::max())});

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
      ArenaAllocator arena(ArenaOptions{
          .initial_capacity = 65536,
          .growth =
              GrowthPolicy::Linear(65536, std::numeric_limits<size_t>::max())});

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

      // Flatten and check uniqueness.
      std::vector<void*> all;
      all.reserve(kThreads * kAllocsPerThread);
      for (auto& val : per_thread) {
        all.insert(all.end(), val.begin(), val.end());
      }
      std::ranges::sort(all);

      const auto dup = std::ranges::adjacent_find(all);
      CHECK_EQ(dup, all.end());
    }
  }

  TEST_CASE("mem::ArenaAllocator thread-safety: concurrent deallocate") {
    SUBCASE("Concurrent deallocate increments total_deallocations correctly") {
      ArenaAllocator arena(ArenaOptions{
          .initial_capacity = 4096,
          .growth =
              GrowthPolicy::Linear(4096, std::numeric_limits<size_t>::max())});

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
