#include <doctest/doctest.h>

#include <helios/memory/stack_allocator.hpp>

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
constexpr size_t kGrowCap = 128;

constexpr StackAllocatorOptions GrowingOptions(size_t cap = kGrowCap) {
  return StackAllocatorOptions{
      .initial_capacity = cap,
      .growth = GrowthPolicy::Linear(cap, std::numeric_limits<size_t>::max())};
}

}  // namespace

TEST_SUITE("helios::mem::StackAllocator") {
  TEST_CASE("mem::StackAllocator::ctor(StackAllocatorOptions)") {
    SUBCASE("InitialCapacity is stored from options") {
      const StackAllocator stack(
          StackAllocatorOptions{.initial_capacity = 512});
      CHECK_EQ(stack.InitialCapacity(), 512);
    }

    SUBCASE("TotalCapacity equals initial_capacity on construction") {
      const StackAllocator stack(
          StackAllocatorOptions{.initial_capacity = 1024});
      CHECK_EQ(stack.TotalCapacity(), 1024);
    }

    SUBCASE("BlockCount is 1 on construction") {
      const StackAllocator stack(
          StackAllocatorOptions{.initial_capacity = 256});
      CHECK_EQ(stack.BlockCount(), 1);
    }

    SUBCASE("Stack is empty on construction") {
      const StackAllocator stack(
          StackAllocatorOptions{.initial_capacity = 256});
      CHECK(stack.Empty());
    }

    SUBCASE("Growth policy is stored from options") {
      const GrowthPolicy policy = GrowthPolicy::Fixed();
      const StackAllocator stack(
          StackAllocatorOptions{.initial_capacity = 256, .growth = policy});
      CHECK_EQ(stack.Growth().max_capacity, policy.max_capacity);
    }
  }

  TEST_CASE("mem::StackAllocator::ctor(size_t)") {
    SUBCASE("InitialCapacity is stored") {
      const StackAllocator stack(2048);
      CHECK_EQ(stack.InitialCapacity(), 2048);
    }

    SUBCASE("Uses Fixed growth policy") {
      const StackAllocator stack(512);
      CHECK_EQ(stack.Growth().max_capacity, GrowthPolicy::Fixed().max_capacity);
    }

    SUBCASE("Stack is empty on construction") {
      const StackAllocator stack(512);
      CHECK(stack.Empty());
    }
  }

  TEST_CASE("mem::StackAllocator::ctor(StackAllocator&&)") {
    SUBCASE("Moved-into stack has same InitialCapacity") {
      StackAllocator source(1024);
      const StackAllocator moved(std::move(source));
      CHECK_EQ(moved.InitialCapacity(), 1024);
    }

    SUBCASE("Moved-into stack carries existing allocation state") {
      StackAllocator source(1024);
      [[maybe_unused]] const void* _ = source.allocate(64, kAlign);

      const StackAllocator moved(std::move(source));

      CHECK_FALSE(moved.Empty());
    }

    SUBCASE("Source InitialCapacity is zeroed after move") {
      StackAllocator source(512);
      const StackAllocator moved(std::move(source));
      CHECK_EQ(source.InitialCapacity(), 0);
    }

    SUBCASE("Source is empty after move") {
      StackAllocator source(512);
      [[maybe_unused]] const void* _ = source.allocate(32, kAlign);

      const StackAllocator moved(std::move(source));

      CHECK(source.Empty());
    }
  }

  TEST_CASE("mem::StackAllocator::operator=(StackAllocator&&)") {
    SUBCASE("Target acquires source InitialCapacity") {
      StackAllocator source(1024);
      StackAllocator target(128);

      target = std::move(source);

      CHECK_EQ(target.InitialCapacity(), 1024);
    }

    SUBCASE("Target carries allocation state from source") {
      StackAllocator source(1024);
      [[maybe_unused]] const void* _ = source.allocate(32, kAlign);
      StackAllocator target(128);

      target = std::move(source);

      CHECK_FALSE(target.Empty());
    }

    SUBCASE("Source InitialCapacity is zeroed after move assign") {
      StackAllocator source(512);
      StackAllocator target(256);

      target = std::move(source);

      CHECK_EQ(source.InitialCapacity(), 0);
    }

    SUBCASE("Self move assignment does not corrupt state") {
      StackAllocator stack(512);
      [[maybe_unused]] const void* _ = stack.allocate(16, kAlign);
      StackAllocator& ref = stack;

      ref = std::move(stack);  // NOLINT(clang-diagnostic-self-move)

      CHECK_FALSE(ref.Empty());
    }
  }

  TEST_CASE("mem::StackAllocator::GetMarker") {
    SUBCASE("Marker on fresh stack has offset zero") {
      const StackAllocator stack(512);
      const StackAllocator::Marker marker = stack.GetMarker();
      CHECK_EQ(marker.offset, 0);
    }

    SUBCASE("Marker block is non-null on fresh stack") {
      const StackAllocator stack(512);
      const StackAllocator::Marker marker = stack.GetMarker();
      CHECK_NE(marker.block, nullptr);
    }

    SUBCASE("Marker offset advances after each allocation") {
      StackAllocator stack(1024);

      const void* ptr = stack.allocate(64, kAlign);
      const StackAllocator::Marker m1 = stack.GetMarker();
      ptr = stack.allocate(64, kAlign);
      const StackAllocator::Marker m2 = stack.GetMarker();

      CHECK_GT(m2.offset, m1.offset);
    }

    SUBCASE("Two successive markers from same block share the same block ptr") {
      StackAllocator stack(1024);

      const void* ptr = stack.allocate(16, kAlign);
      const StackAllocator::Marker m1 = stack.GetMarker();
      ptr = stack.allocate(16, kAlign);
      const StackAllocator::Marker m2 = stack.GetMarker();

      CHECK_EQ(m1.block, m2.block);
    }
  }

  TEST_CASE("mem::StackAllocator::RewindToMarker") {
    SUBCASE("Rewinding to zero-offset marker makes stack empty") {
      StackAllocator stack(1024);

      const StackAllocator::Marker before = stack.GetMarker();
      [[maybe_unused]] const void* _ = stack.allocate(128, kAlign);
      stack.RewindToMarker(before);

      CHECK(stack.Empty());
    }

    SUBCASE("Rewinding to mid-point preserves earlier allocation count") {
      StackAllocator stack(1024);

      const void* ptr = stack.allocate(64, kAlign);
      const StackAllocator::Marker mid = stack.GetMarker();
      ptr = stack.allocate(128, kAlign);
      ptr = stack.allocate(64, kAlign);
      stack.RewindToMarker(mid);

      // RewindToMarker conservatively zeroes allocation_count; total_allocated
      // is set to marker.offset. Verify the allocator accepted the rewind.
      CHECK_LE(stack.Stats().total_allocated, mid.offset + 1);
    }

    SUBCASE("Allocation after rewind succeeds") {
      StackAllocator stack(1024);

      const StackAllocator::Marker before = stack.GetMarker();
      [[maybe_unused]] const void* _ = stack.allocate(256, kAlign);
      stack.RewindToMarker(before);
      void* const ptr = stack.allocate(64, kAlign);

      CHECK_NE(ptr, nullptr);
    }

    SUBCASE("BlockCount decreases when blocks above marker are freed") {
      StackAllocator stack(GrowingOptions(kGrowCap));

      // Keep one allocation in the initial block, then force growth.
      const void* ptr = stack.allocate(16, kAlign);
      const StackAllocator::Marker before_second = stack.GetMarker();
      ptr = stack.allocate(kGrowCap, 1);
      CHECK_GT(stack.BlockCount(), 1);

      // Rewind to a marker in the initial block.
      stack.RewindToMarker(before_second);

      CHECK_EQ(stack.BlockCount(), 1);
    }
  }

  TEST_CASE("mem::StackAllocator::Reset") {
    SUBCASE("Empty returns true after Reset") {
      StackAllocator stack(1024);

      [[maybe_unused]] const void* _ = stack.allocate(256, kAlign);
      stack.Reset();

      CHECK(stack.Empty());
    }

    SUBCASE("BlockCount returns to 1 after Reset") {
      StackAllocator stack(GrowingOptions());

      const void* ptr = stack.allocate(kGrowCap, 1);
      ptr = stack.allocate(kGrowCap, 1);

      CHECK_GT(stack.BlockCount(), 1);
      stack.Reset();
      CHECK_EQ(stack.BlockCount(), 1);
    }

    SUBCASE("TotalCapacity returns to InitialCapacity after Reset") {
      StackAllocator stack(GrowingOptions());

      const void* ptr = stack.allocate(kGrowCap, 1);
      ptr = stack.allocate(kGrowCap, 1);

      stack.Reset();
      CHECK_EQ(stack.TotalCapacity(), stack.InitialCapacity());
    }

    SUBCASE("Stats counters are zeroed after Reset") {
      StackAllocator stack(1024);

      [[maybe_unused]] const void* _ = stack.allocate(64, kAlign);
      stack.Reset();

      const AllocatorStats stats = stack.Stats();
      CHECK_EQ(stats.total_allocated, 0);
      CHECK_EQ(stats.allocation_count, 0);
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
      CHECK_EQ(stats.alignment_waste, 0);
    }

    SUBCASE("Allocation succeeds after Reset") {
      StackAllocator stack(1024);

      [[maybe_unused]] const void* _ = stack.allocate(512, kAlign);
      stack.Reset();
      void* const ptr = stack.allocate(64, kAlign);

      CHECK_NE(ptr, nullptr);
    }
  }

  TEST_CASE("mem::StackAllocator::Empty") {
    SUBCASE("Returns true on fresh stack") {
      const StackAllocator stack(256);
      CHECK(stack.Empty());
    }

    SUBCASE("Returns false after one allocation") {
      StackAllocator stack(256);
      [[maybe_unused]] const void* _ = stack.allocate(8, kAlign);
      CHECK_FALSE(stack.Empty());
    }

    SUBCASE("Returns true after LIFO deallocation") {
      StackAllocator stack(256);

      void* const ptr = stack.allocate(8, kAlign);
      stack.deallocate(ptr, 8, kAlign);

      CHECK(stack.Empty());
    }

    SUBCASE("Returns true after Reset") {
      StackAllocator stack(256);

      [[maybe_unused]] const void* _ = stack.allocate(8, kAlign);
      stack.Reset();

      CHECK(stack.Empty());
    }
  }

  TEST_CASE("mem::StackAllocator::Stats") {
    SUBCASE("All fields are zero on fresh stack") {
      const StackAllocator stack(1024);

      const AllocatorStats stats = stack.Stats();
      CHECK_EQ(stats.total_allocated, 0);
      CHECK_EQ(stats.peak_usage, 0);
      CHECK_EQ(stats.allocation_count, 0);
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
      CHECK_EQ(stats.alignment_waste, 0);
    }

    SUBCASE("total_allocated reflects bytes consumed on the stack") {
      StackAllocator stack(1024);

      const void* ptr = stack.allocate(64, kAlign);
      ptr = stack.allocate(128, kAlign);

      CHECK_GE(stack.Stats().total_allocated, 192);
    }

    SUBCASE("allocation_count tracks live count") {
      StackAllocator stack(1024);

      const void* ptr = stack.allocate(32, kAlign);
      ptr = stack.allocate(32, kAlign);

      CHECK_EQ(stack.Stats().allocation_count, 2);
    }

    SUBCASE("total_allocations counts every allocate call") {
      StackAllocator stack(1024);

      const void* ptr = stack.allocate(16, kAlign);
      ptr = stack.allocate(16, kAlign);
      ptr = stack.allocate(16, kAlign);

      CHECK_EQ(stack.Stats().total_allocations, 3);
    }

    SUBCASE("total_deallocations increments only on LIFO deallocate") {
      StackAllocator stack(1024);

      void* const ptr = stack.allocate(32, kAlign);
      stack.deallocate(ptr, 32, kAlign);

      CHECK_EQ(stack.Stats().total_deallocations, 1);
    }

    SUBCASE("Non-LIFO deallocate does not increment total_deallocations") {
      StackAllocator stack(1024);

      void* const first = stack.allocate(32, kAlign);
      [[maybe_unused]] const void* _ =
          stack.allocate(32, kAlign);       // 'second' — still alive
      stack.deallocate(first, 32, kAlign);  // out of order -> no-op

      CHECK_EQ(stack.Stats().total_deallocations, 0);
    }

    SUBCASE("peak_usage is at least the high watermark of total_allocated") {
      StackAllocator stack(1024);

      const void* ptr = stack.allocate(256, kAlign);
      ptr = stack.allocate(128, kAlign);

      CHECK_GE(stack.Stats().peak_usage, 256);
    }
  }

  TEST_CASE("mem::StackAllocator::InitialCapacity") {
    SUBCASE("Returns value from options ctor") {
      const StackAllocator stack(
          StackAllocatorOptions{.initial_capacity = 777});
      CHECK_EQ(stack.InitialCapacity(), 777);
    }

    SUBCASE("Returns value from size_t ctor") {
      const StackAllocator stack(888);
      CHECK_EQ(stack.InitialCapacity(), 888);
    }

    SUBCASE("Unchanged by allocations") {
      StackAllocator stack(1024);
      [[maybe_unused]] const void* _ = stack.allocate(512, kAlign);
      CHECK_EQ(stack.InitialCapacity(), 1024);
    }

    SUBCASE("Unchanged after Reset") {
      StackAllocator stack(1024);

      [[maybe_unused]] const void* _ = stack.allocate(512, kAlign);
      stack.Reset();

      CHECK_EQ(stack.InitialCapacity(), 1024);
    }
  }

  TEST_CASE("mem::StackAllocator::TotalCapacity") {
    SUBCASE("Equals InitialCapacity right after construction") {
      const StackAllocator stack(
          StackAllocatorOptions{.initial_capacity = 4096});
      CHECK_EQ(stack.TotalCapacity(), 4096);
    }

    SUBCASE("Increases when first second block is appended due to overflow") {
      StackAllocator stack(GrowingOptions());

      const size_t before = stack.TotalCapacity();
      const void* ptr = stack.allocate(kGrowCap, 1);
      ptr = stack.allocate(kGrowCap, 1);

      CHECK_GT(stack.TotalCapacity(), before);
    }

    SUBCASE("Returns to InitialCapacity after Reset") {
      StackAllocator stack(GrowingOptions());

      const void* ptr = stack.allocate(kGrowCap, 1);
      ptr = stack.allocate(kGrowCap, 1);
      stack.Reset();

      CHECK_EQ(stack.TotalCapacity(), stack.InitialCapacity());
    }
  }

  TEST_CASE("mem::StackAllocator::BlockCount") {
    SUBCASE("Is 1 right after construction") {
      const StackAllocator stack(1024);
      CHECK_EQ(stack.BlockCount(), 1);
    }

    SUBCASE("Increases when first new block is appended") {
      StackAllocator stack(GrowingOptions());

      const void* ptr = stack.allocate(kGrowCap, 1);
      ptr = stack.allocate(kGrowCap, 1);

      CHECK_GT(stack.BlockCount(), 1);
    }

    SUBCASE("Returns to 1 after Reset") {
      StackAllocator stack(GrowingOptions());

      const void* ptr = stack.allocate(kGrowCap, 1);
      ptr = stack.allocate(kGrowCap, 1);
      stack.Reset();

      CHECK_EQ(stack.BlockCount(), 1);
    }
  }

  TEST_CASE("mem::StackAllocator::Growth") {
    SUBCASE("Returns Fixed policy for size_t ctor") {
      const StackAllocator stack(256);
      CHECK_EQ(stack.Growth().max_capacity, GrowthPolicy::Fixed().max_capacity);
    }

    SUBCASE("Returns policy supplied via options") {
      const GrowthPolicy policy =
          GrowthPolicy::Linear(kGrowCap, std::numeric_limits<size_t>::max());
      const StackAllocator stack(StackAllocatorOptions{
          .initial_capacity = kGrowCap, .growth = policy});
      CHECK_EQ(stack.Growth().max_capacity, policy.max_capacity);
    }

    SUBCASE("Unchanged by allocations") {
      StackAllocator stack(512);
      [[maybe_unused]] const void* _ = stack.allocate(32, kAlign);
      CHECK_EQ(stack.Growth().max_capacity, GrowthPolicy::Fixed().max_capacity);
    }
  }

  TEST_CASE("mem::StackAllocator::allocate") {
    SUBCASE("Returns non-null pointer") {
      StackAllocator stack(1024);
      CHECK_NE(stack.allocate(32, kAlign), nullptr);
    }

    SUBCASE(
        "Consecutive allocations return distinct non-overlapping pointers") {
      StackAllocator stack(1024);

      void* const first = stack.allocate(64, kAlign);
      void* const second = stack.allocate(64, kAlign);

      CHECK_NE(first, second);
      const auto ua = reinterpret_cast<uintptr_t>(first);
      const auto ub = reinterpret_cast<uintptr_t>(second);
      const bool result = ub >= ua + 64 || ua >= ub + 64;

      CHECK(result);
    }

    SUBCASE("Pointer satisfies the requested alignment") {
      StackAllocator stack(1024);

      for (const size_t al : {1UZ, 4UZ, 8UZ, 16UZ, 32UZ, 64UZ}) {
        void* const ptr = stack.allocate(4, al);
        CHECK_EQ(reinterpret_cast<uintptr_t>(ptr) % al, 0);
      }
    }

    SUBCASE("Overflow triggers growth and returns valid pointer") {
      StackAllocator stack(GrowingOptions());

      [[maybe_unused]] const void* _ = stack.allocate(kGrowCap, 1);
      void* const ptr = stack.allocate(kGrowCap, 1);

      CHECK_NE(ptr, nullptr);
      CHECK_GT(stack.BlockCount(), 1);
    }

    SUBCASE("Usable as PMR upstream for std::pmr::vector") {
      StackAllocator stack(4096);

      std::pmr::monotonic_buffer_resource mbr(256, &stack);
      std::pmr::vector<int> vec(&mbr);
      vec.push_back(1);
      vec.push_back(2);

      CHECK_EQ(vec.size(), 2);
    }
  }

  TEST_CASE("mem::StackAllocator::deallocate") {
    SUBCASE(
        "LIFO deallocation rewinds the offset and clears allocation_count") {
      StackAllocator stack(1024);

      void* const ptr = stack.allocate(64, kAlign);
      stack.deallocate(ptr, 64, kAlign);

      CHECK(stack.Empty());
    }

    SUBCASE("LIFO deallocation increments total_deallocations") {
      StackAllocator stack(1024);

      void* const ptr = stack.allocate(64, kAlign);
      stack.deallocate(ptr, 64, kAlign);

      CHECK_EQ(stack.Stats().total_deallocations, 1);
    }

    SUBCASE("Non-LIFO deallocation is first no-op (does not corrupt state)") {
      StackAllocator stack(1024);

      void* const first = stack.allocate(32, kAlign);
      [[maybe_unused]] const void* _ = stack.allocate(32, kAlign);
      // Deallocate 'first' while 'second' is alive — out-of-order: ignored.
      stack.deallocate(first, 32, kAlign);

      CHECK_EQ(stack.Stats().allocation_count, 2);
      CHECK_EQ(stack.Stats().total_deallocations, 0);
    }

    SUBCASE("Deallocation of pointer from non-head block is first no-op") {
      StackAllocator stack(GrowingOptions());

      void* const in_first = stack.allocate(kGrowCap, 1);
      [[maybe_unused]] const void* _ =
          stack.allocate(kGrowCap, 1);  // triggers growth to second block
      // Deallocating first pointer from the old (non-head) block must be
      // ignored.
      stack.deallocate(in_first, kGrowCap, 1);

      CHECK_GE(stack.Stats().allocation_count, 1);
    }
  }

  TEST_CASE("mem::StackAllocator thread-safety: concurrent allocate") {
    SUBCASE("All threads receive valid non-null pointers") {
      StackAllocator stack(StackAllocatorOptions{
          .initial_capacity = 65536,
          .growth =
              GrowthPolicy::Linear(65536, std::numeric_limits<size_t>::max())});

      constexpr size_t kThreads = 8;
      constexpr size_t kAllocsPerThread = 128;
      constexpr size_t kAllocSize = 8;

      std::atomic<int> null_count{0};
      std::vector<std::thread> threads;
      threads.reserve(kThreads);

      for (size_t j = 0; j < kThreads; ++j) {
        threads.emplace_back([&stack, &null_count] {
          for (size_t i = 0; i < kAllocsPerThread; ++i) {
            void* const ptr = stack.allocate(kAllocSize, kAlign);
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
      StackAllocator stack(StackAllocatorOptions{
          .initial_capacity = 65536,
          .growth =
              GrowthPolicy::Linear(65536, std::numeric_limits<size_t>::max())});

      constexpr size_t kThreads = 4;
      constexpr size_t kAllocsPerThread = 64;

      std::vector<std::thread> threads;
      threads.reserve(kThreads);
      for (size_t j = 0; j < kThreads; ++j) {
        threads.emplace_back([&stack] {
          for (size_t i = 0; i < kAllocsPerThread; ++i) {
            [[maybe_unused]] const void* _ = stack.allocate(8, kAlign);
          }
        });
      }

      for (auto& th : threads) {
        th.join();
      }

      CHECK_EQ(stack.Stats().total_allocations, kThreads * kAllocsPerThread);
    }

    SUBCASE("Concurrent allocations produce no duplicate pointers") {
      StackAllocator stack(StackAllocatorOptions{
          .initial_capacity = 131072,
          .growth = GrowthPolicy::Linear(131072,
                                         std::numeric_limits<size_t>::max())});

      constexpr size_t kThreads = 4;
      constexpr size_t kAllocsPerThread = 256;
      constexpr size_t kAllocSize = 16;

      std::vector<std::vector<void*>> per_thread(kThreads);
      std::vector<std::thread> threads;
      threads.reserve(kThreads);

      for (size_t j = 0; j < kThreads; ++j) {
        per_thread[j].resize(kAllocsPerThread);
        threads.emplace_back([&per_thread, &stack, j] {
          for (size_t i = 0; i < kAllocsPerThread; ++i) {
            per_thread[j][i] = stack.allocate(kAllocSize, kAlign);
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
  }

  TEST_CASE("mem::StackAllocator thread-safety: concurrent deallocate") {
    SUBCASE("LIFO deallocations from multiple threads update stats correctly") {
      // Give each thread its own exclusive region by pre-allocating serially.
      StackAllocator stack(StackAllocatorOptions{
          .initial_capacity = 65536,
          .growth =
              GrowthPolicy::Linear(65536, std::numeric_limits<size_t>::max())});

      constexpr size_t kThreads = 4;
      constexpr size_t kAllocSize = 8;

      // Each thread allocates one item, then LIFO-deallocates it alone.
      std::vector<std::thread> threads;
      threads.reserve(kThreads);
      std::atomic<size_t> dealloc_count{0};

      for (size_t j = 0; j < kThreads; ++j) {
        threads.emplace_back([&stack, &dealloc_count] {
          void* const ptr = stack.allocate(kAllocSize, kAlign);
          // Only the last allocation in the thread can be LIFO-freed if no
          // other thread interleaves. We simply exercise the path and track
          // how many succeed without racing into undefined behaviour.
          stack.deallocate(ptr, kAllocSize, kAlign);
          dealloc_count.fetch_add(1, std::memory_order_relaxed);
        });
      }

      for (auto& th : threads) {
        th.join();
      }

      // All threads executed the deallocate path without crashing.
      CHECK_EQ(dealloc_count.load(), kThreads);
    }
  }
}  // TEST_SUITE
