#include <doctest/doctest.h>

#include <helios/memory/frame_allocator.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <thread>
#include <tuple>
#include <vector>

using namespace helios::mem;

namespace {

constexpr size_t kAlign = alignof(std::max_align_t);
constexpr size_t kCap = 1024;

constexpr FrameAllocatorOptions BasicOptions() {
  return FrameAllocatorOptions{.initial_capacity = kCap,
                               .growth = GrowthPolicy::Geometric()};
}

constexpr FrameAllocatorOptions GrowingOptions(size_t cap = kCap) {
  return FrameAllocatorOptions{
      .initial_capacity = cap,
      .growth = GrowthPolicy::Linear(cap, std::numeric_limits<size_t>::max())};
}

}  // namespace

TEST_SUITE("helios::mem::FrameAllocator") {
  TEST_CASE("mem::FrameAllocator::ctor(FrameAllocatorOptions)") {
    SUBCASE("InitialCapacity is stored from options") {
      const FrameAllocator<2> alloc(BasicOptions());
      CHECK_EQ(alloc.InitialCapacity(), kCap);
    }

    SUBCASE("FrameIndex starts at zero") {
      const FrameAllocator<3> alloc(BasicOptions());
      CHECK_EQ(alloc.FrameIndex(), 0);
    }

    SUBCASE("Current frame is empty on construction") {
      const FrameAllocator<2> alloc(BasicOptions());
      CHECK(alloc.Empty());
    }

    SUBCASE("BlockCount of current frame is 1 on construction") {
      const FrameAllocator<2> alloc(BasicOptions());
      CHECK_EQ(alloc.BlockCount(), 1);
    }

    SUBCASE("TotalCapacity of current frame equals InitialCapacity") {
      const FrameAllocator<2> alloc(BasicOptions());
      CHECK_EQ(alloc.TotalCapacity(), kCap);
    }

    SUBCASE("Growth policy is stored from options") {
      const auto policy = GrowthPolicy::Geometric();
      const FrameAllocator<2> alloc(
          FrameAllocatorOptions{.initial_capacity = kCap, .growth = policy});
      CHECK_EQ(alloc.Growth().max_capacity, policy.max_capacity);
    }
  }

  TEST_CASE("mem::FrameAllocator::ctor(size_t)") {
    SUBCASE("InitialCapacity matches argument") {
      const FrameAllocator<1> alloc(512);
      CHECK_EQ(alloc.InitialCapacity(), 512);
    }

    SUBCASE("Uses geometric growth policy") {
      const FrameAllocator<2> alloc(256);
      CHECK_EQ(alloc.Growth().max_capacity,
               GrowthPolicy::Geometric().max_capacity);
    }

    SUBCASE("FrameIndex starts at zero") {
      const FrameAllocator<4> alloc(256);
      CHECK_EQ(alloc.FrameIndex(), 0);
    }
  }

  TEST_CASE("mem::FrameAllocator::ctor(FrameAllocator&&)") {
    SUBCASE("Moved-into allocator has same InitialCapacity") {
      FrameAllocator<2> source(BasicOptions());
      const FrameAllocator<2> moved(std::move(source));
      CHECK_EQ(moved.InitialCapacity(), kCap);
    }

    SUBCASE("Moved-into allocator carries existing allocation state") {
      FrameAllocator<2> source(BasicOptions());
      std::ignore = source.allocate(64, kAlign);

      const FrameAllocator<2> moved(std::move(source));

      CHECK_FALSE(moved.Empty());
    }

    SUBCASE("Source InitialCapacity is zeroed after move") {
      FrameAllocator<2> source(BasicOptions());
      const FrameAllocator<2> moved(std::move(source));
      CHECK_EQ(source.InitialCapacity(), 0);
    }
  }

  TEST_CASE("mem::FrameAllocator::operator=(FrameAllocator&&)") {
    SUBCASE("Target acquires source InitialCapacity") {
      FrameAllocator<2> source(BasicOptions());
      FrameAllocator<2> target(FrameAllocatorOptions{.initial_capacity = 128});

      target = std::move(source);

      CHECK_EQ(target.InitialCapacity(), kCap);
    }

    SUBCASE("Target carries allocation state from source") {
      FrameAllocator<2> source(BasicOptions());
      std::ignore = source.allocate(32, kAlign);
      FrameAllocator<2> target(FrameAllocatorOptions{.initial_capacity = 128});

      target = std::move(source);

      CHECK_FALSE(target.Empty());
    }

    SUBCASE("Source InitialCapacity is zeroed after move assign") {
      FrameAllocator<2> source(BasicOptions());
      FrameAllocator<2> target(BasicOptions());

      target = std::move(source);

      CHECK_EQ(source.InitialCapacity(), 0);
    }

    SUBCASE("Self move assignment does not corrupt state") {
      FrameAllocator<2> alloc(BasicOptions());
      std::ignore = alloc.allocate(16, kAlign);
      FrameAllocator<2>& ref = alloc;

      ref = std::move(alloc);  // NOLINT(clang-diagnostic-self-move)

      CHECK_FALSE(ref.Empty());
    }
  }

  TEST_CASE("mem::FrameAllocator::Advance") {
    SUBCASE("FrameIndex increments by one per call") {
      FrameAllocator<3> alloc(BasicOptions());

      alloc.Advance();
      CHECK_EQ(alloc.FrameIndex(), 1);
      alloc.Advance();
      CHECK_EQ(alloc.FrameIndex(), 2);
    }

    SUBCASE("FrameIndex wraps to zero at N") {
      FrameAllocator<2> alloc(BasicOptions());

      alloc.Advance();
      CHECK_EQ(alloc.FrameIndex(), 1);
      alloc.Advance();
      CHECK_EQ(alloc.FrameIndex(), 0);
    }

    SUBCASE("New current arena is empty after Advance") {
      FrameAllocator<2> alloc(BasicOptions());

      alloc.Advance();
      std::ignore = alloc.allocate(64, kAlign);
      alloc.Advance();  // wraps back to frame 0, which gets Reset

      CHECK(alloc.Empty());
    }

    SUBCASE("Arena that was current before Advance retains its allocations") {
      FrameAllocator<2> alloc(BasicOptions());

      [[maybe_unused]] const void* _ =
          alloc.allocate(64, kAlign);  // frame 0 has data
      alloc.Advance();                 // now on frame 1

      CHECK_FALSE(alloc.Arena(0).Empty());
      CHECK(alloc.Empty());  // frame 1 is fresh
    }

    SUBCASE("Stats of new current arena are zeroed after Advance") {
      FrameAllocator<2> alloc(BasicOptions());

      alloc.Advance();
      std::ignore = alloc.allocate(64, kAlign);
      alloc.Advance();  // resets frame 0

      CHECK_EQ(alloc.Stats().allocation_count, 0);
      CHECK_EQ(alloc.Stats().total_allocations, 0);
    }
  }

  TEST_CASE("mem::FrameAllocator::Reset") {
    SUBCASE("FrameIndex returns to zero after Reset") {
      FrameAllocator<3> alloc(BasicOptions());
      alloc.Advance();
      alloc.Advance();

      alloc.Reset();

      CHECK_EQ(alloc.FrameIndex(), 0);
    }

    SUBCASE("All arenas are empty after Reset") {
      FrameAllocator<3> alloc(BasicOptions());
      std::ignore = alloc.allocate(32, kAlign);
      alloc.Advance();
      std::ignore = alloc.allocate(32, kAlign);
      alloc.Advance();
      std::ignore = alloc.allocate(32, kAlign);

      alloc.Reset();
      for (size_t i = 0; i < 3; ++i) {
        CHECK(alloc.Arena(i).Empty());
      }
    }

    SUBCASE("Allocation succeeds after Reset") {
      FrameAllocator<2> alloc(BasicOptions());
      std::ignore = alloc.allocate(512, kAlign);

      alloc.Reset();

      void* const ptr = alloc.allocate(64, kAlign);
      CHECK_NE(ptr, nullptr);
    }
  }

  TEST_CASE("mem::FrameAllocator::Empty") {
    SUBCASE("Returns true on fresh allocator") {
      const FrameAllocator<2> alloc(BasicOptions());
      CHECK(alloc.Empty());
    }

    SUBCASE("Returns false after allocation in current frame") {
      FrameAllocator<2> alloc(BasicOptions());
      std::ignore = alloc.allocate(16, kAlign);
      CHECK_FALSE(alloc.Empty());
    }

    SUBCASE("Returns true for the new frame after Advance") {
      FrameAllocator<2> alloc(BasicOptions());
      std::ignore = alloc.allocate(64, kAlign);
      alloc.Advance();
      CHECK(alloc.Empty());
    }

    SUBCASE("Returns true after Reset") {
      FrameAllocator<2> alloc(BasicOptions());
      std::ignore = alloc.allocate(64, kAlign);
      alloc.Reset();
      CHECK(alloc.Empty());
    }
  }

  TEST_CASE("mem::FrameAllocator::Stats") {
    SUBCASE("All fields zero on fresh current frame") {
      const FrameAllocator<2> alloc(BasicOptions());

      const AllocatorStats stats = alloc.Stats();
      CHECK_EQ(stats.allocation_count, 0);
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_allocated, 0);
    }

    SUBCASE("total_allocations counts allocations in current frame") {
      FrameAllocator<2> alloc(BasicOptions());
      std::ignore = alloc.allocate(64, kAlign);
      std::ignore = alloc.allocate(64, kAlign);

      CHECK_EQ(alloc.Stats().total_allocations, 2);
    }

    SUBCASE("Stats show only current frame after Advance") {
      FrameAllocator<2> alloc(BasicOptions());
      std::ignore = alloc.allocate(64, kAlign);  // frame 0
      alloc.Advance();                           // now frame 1

      CHECK_EQ(alloc.Stats().allocation_count, 0);
    }
  }

  TEST_CASE("mem::FrameAllocator::AggregateStats") {
    SUBCASE("Returns all-zero on fresh allocator") {
      const FrameAllocator<3> alloc(BasicOptions());

      const AllocatorStats agg = alloc.AggregateStats();

      CHECK_EQ(agg.total_allocated, 0);
      CHECK_EQ(agg.allocation_count, 0);
      CHECK_EQ(agg.total_allocations, 0);
      CHECK_EQ(agg.total_deallocations, 0);
      CHECK_EQ(agg.alignment_waste, 0);
    }

    SUBCASE("Sums total_allocations across all frame arenas") {
      FrameAllocator<2> alloc(BasicOptions());

      std::ignore = alloc.allocate(32, kAlign);  // frame 0: 1 alloc
      std::ignore = alloc.allocate(32, kAlign);  // frame 0: 2 allocs
      alloc.Advance();
      std::ignore = alloc.allocate(64, kAlign);  // frame 1: 1 alloc

      const AllocatorStats agg = alloc.AggregateStats();
      CHECK_EQ(agg.total_allocations, 3);
    }

    SUBCASE("Sums peak_usage across all frames (saturating)") {
      FrameAllocator<2> alloc(BasicOptions());

      std::ignore = alloc.allocate(128, kAlign);
      alloc.Advance();
      std::ignore = alloc.allocate(64, kAlign);

      const AllocatorStats agg = alloc.AggregateStats();
      CHECK_GE(agg.peak_usage, 64);
    }

    SUBCASE("Includes all N arenas even when only some have allocations") {
      FrameAllocator<4> alloc(BasicOptions());
      [[maybe_unused]] const void* _ =
          alloc.allocate(16, kAlign);  // frame 0 only
      // frames 1-3 untouched

      const AllocatorStats agg = alloc.AggregateStats();
      CHECK_EQ(agg.total_allocations, 1);
    }
  }

  TEST_CASE("mem::FrameAllocator::FrameIndex") {
    SUBCASE("Is zero on construction") {
      const FrameAllocator<4> alloc(BasicOptions());
      CHECK_EQ(alloc.FrameIndex(), 0);
    }

    SUBCASE("Increments on each Advance") {
      FrameAllocator<4> alloc(BasicOptions());

      alloc.Advance();
      CHECK_EQ(alloc.FrameIndex(), 1);
      alloc.Advance();
      CHECK_EQ(alloc.FrameIndex(), 2);
    }

    SUBCASE("Wraps to zero after N Advances") {
      FrameAllocator<3> alloc(BasicOptions());

      alloc.Advance();
      alloc.Advance();
      alloc.Advance();

      CHECK_EQ(alloc.FrameIndex(), 0);
    }

    SUBCASE("Returns to zero after Reset") {
      FrameAllocator<3> alloc(BasicOptions());

      alloc.Advance();
      alloc.Advance();
      alloc.Reset();

      CHECK_EQ(alloc.FrameIndex(), 0);
    }
  }

  TEST_CASE("mem::FrameAllocator::FrameCount") {
    SUBCASE("Returns 1 for FrameAllocator<1>") {
      CHECK_EQ(FrameAllocator<1>::FrameCount(), 1);
    }

    SUBCASE("Returns 2 for FrameAllocator<2>") {
      CHECK_EQ(FrameAllocator<2>::FrameCount(), 2);
    }

    SUBCASE("Returns N for arbitrary N") {
      CHECK_EQ(FrameAllocator<5>::FrameCount(), 5);
    }

    SUBCASE("Is a compile-time constant (consteval)") {
      // Verify it can be used in a static_assert / as a compile-time
      // expression.
      static_assert(FrameAllocator<3>::FrameCount() == 3);
      CHECK_EQ(FrameAllocator<3>::FrameCount(), 3);
    }
  }

  TEST_CASE("mem::FrameAllocator::InitialCapacity") {
    SUBCASE("Returns value from FrameAllocatorOptions ctor") {
      const FrameAllocator<2> alloc(
          FrameAllocatorOptions{.initial_capacity = 2048});
      CHECK_EQ(alloc.InitialCapacity(), 2048);
    }

    SUBCASE("Returns value from size_t ctor") {
      const FrameAllocator<2> alloc(512);
      CHECK_EQ(alloc.InitialCapacity(), 512);
    }

    SUBCASE("Unchanged by allocations") {
      FrameAllocator<2> alloc(BasicOptions());
      std::ignore = alloc.allocate(128, kAlign);
      CHECK_EQ(alloc.InitialCapacity(), kCap);
    }

    SUBCASE("Unchanged after Advance") {
      FrameAllocator<2> alloc(BasicOptions());
      alloc.Advance();
      CHECK_EQ(alloc.InitialCapacity(), kCap);
    }

    SUBCASE("Unchanged after Reset") {
      FrameAllocator<2> alloc(BasicOptions());

      std::ignore = alloc.allocate(64, kAlign);
      alloc.Reset();

      CHECK_EQ(alloc.InitialCapacity(), kCap);
    }
  }

  TEST_CASE("mem::FrameAllocator::Growth") {
    SUBCASE("Returns geometric policy for size_t ctor") {
      const FrameAllocator<2> alloc(512);
      CHECK_EQ(alloc.Growth().max_capacity,
               GrowthPolicy::Geometric().max_capacity);
    }

    SUBCASE("Returns policy supplied via FrameAllocatorOptions") {
      const auto policy =
          GrowthPolicy::Linear(kCap, std::numeric_limits<size_t>::max());
      const FrameAllocator<2> alloc(
          FrameAllocatorOptions{.initial_capacity = kCap, .growth = policy});
      CHECK_EQ(alloc.Growth().max_capacity, policy.max_capacity);
    }

    SUBCASE("Unchanged after Advance") {
      FrameAllocator<2> alloc(BasicOptions());
      alloc.Advance();
      CHECK_EQ(alloc.Growth().max_capacity,
               GrowthPolicy::Geometric().max_capacity);
    }
  }

  TEST_CASE("mem::FrameAllocator::TotalCapacity") {
    SUBCASE("Equals InitialCapacity on construction") {
      const FrameAllocator<2> alloc(BasicOptions());
      CHECK_EQ(alloc.TotalCapacity(), kCap);
    }

    SUBCASE("Reports current frame arena capacity, not aggregate") {
      FrameAllocator<2> alloc(BasicOptions());
      alloc.Advance();
      // After advancing, the new current arena starts fresh.
      CHECK_EQ(alloc.TotalCapacity(), kCap);
    }

    SUBCASE("Grows when current arena grows") {
      FrameAllocator<2> alloc(GrowingOptions(64));

      const size_t before = alloc.TotalCapacity();
      std::ignore = alloc.allocate(64, 1);
      std::ignore = alloc.allocate(64, 1);  // forces growth in current arena

      CHECK_GT(alloc.TotalCapacity(), before);
    }
  }

  TEST_CASE("mem::FrameAllocator::BlockCount") {
    SUBCASE("Is 1 for fresh current frame") {
      const FrameAllocator<2> alloc(BasicOptions());
      CHECK_EQ(alloc.BlockCount(), 1);
    }

    SUBCASE("Increases when current arena grows") {
      FrameAllocator<2> alloc(GrowingOptions(64));

      std::ignore = alloc.allocate(64, 1);
      std::ignore = alloc.allocate(64, 1);

      CHECK_GT(alloc.BlockCount(), 1);
    }

    SUBCASE("Returns to 1 after Advance (new current arena is fresh)") {
      FrameAllocator<2> alloc(GrowingOptions(64));

      std::ignore = alloc.allocate(64, 1);
      std::ignore = alloc.allocate(64, 1);
      alloc.Advance();  // switch to the other arena which is fresh

      CHECK_EQ(alloc.BlockCount(), 1);
    }
  }

  TEST_CASE("mem::FrameAllocator::Arena (mutable)") {
    SUBCASE("Returns mutable reference to the correct arena by index") {
      FrameAllocator<3> alloc(BasicOptions());

      [[maybe_unused]] const void* _ = alloc.Arena(1).allocate(32, kAlign);

      CHECK_FALSE(alloc.Arena(1).Empty());
      CHECK(alloc.Arena(0).Empty());
      CHECK(alloc.Arena(2).Empty());
    }

    SUBCASE(
        "Arena(FrameIndex()) is the same as the current allocation target") {
      FrameAllocator<2> alloc(BasicOptions());

      alloc.Advance();  // now on frame 1
      std::ignore = alloc.allocate(64, kAlign);

      CHECK_EQ(alloc.Arena(alloc.FrameIndex()).Stats().allocation_count,
               alloc.Stats().allocation_count);
    }
  }

  TEST_CASE("mem::FrameAllocator::Arena (const)") {
    SUBCASE("Const Arena returns correct Stats for each frame") {
      FrameAllocator<2> alloc(BasicOptions());

      std::ignore = alloc.allocate(64, kAlign);  // frame 0: 1 alloc
      alloc.Advance();
      std::ignore = alloc.allocate(128, kAlign);  // frame 1: 1 alloc

      const FrameAllocator<2>& const_alloc = alloc;
      CHECK_EQ(const_alloc.Arena(0).Stats().allocation_count, 1);
      CHECK_EQ(const_alloc.Arena(1).Stats().allocation_count, 1);
    }

    SUBCASE("Const Arena(0) is empty on fresh allocator") {
      const FrameAllocator<2> alloc(BasicOptions());
      CHECK(alloc.Arena(0).Empty());
    }
  }

  TEST_CASE("mem::FrameAllocator::allocate") {
    SUBCASE("Returns non-null pointer") {
      FrameAllocator<2> alloc(BasicOptions());
      CHECK_NE(alloc.allocate(32, kAlign), nullptr);
    }

    SUBCASE(
        "Consecutive allocations return distinct non-overlapping pointers") {
      FrameAllocator<1> alloc(BasicOptions());

      void* const first = alloc.allocate(64, kAlign);
      void* const second = alloc.allocate(64, kAlign);

      CHECK_NE(first, second);
    }

    SUBCASE("Pointer satisfies the requested alignment") {
      FrameAllocator<1> alloc(BasicOptions());

      for (const size_t al : {1UZ, 4UZ, 8UZ, 16UZ, 32UZ, 64UZ}) {
        void* const ptr = alloc.allocate(4, al);
        CHECK_EQ(reinterpret_cast<uintptr_t>(ptr) % al, 0);
      }
    }

    SUBCASE("Routes allocations to current frame arena after Advance") {
      FrameAllocator<2> alloc(BasicOptions());
      alloc.Advance();  // switch to frame 1
      std::ignore = alloc.allocate(32, kAlign);
      CHECK_EQ(alloc.Arena(1).Stats().allocation_count, 1);
      CHECK_EQ(alloc.Arena(0).Stats().allocation_count, 0);
    }

    SUBCASE("Usable as PMR upstream for std::pmr::vector") {
      FrameAllocator<2> alloc(FrameAllocatorOptions{.initial_capacity = 4096});

      std::pmr::monotonic_buffer_resource mbr(256, &alloc);
      std::pmr::vector<int> vec(&mbr);
      vec.push_back(1);
      vec.push_back(2);

      CHECK_EQ(vec.size(), 2);
    }
  }

  TEST_CASE("mem::FrameAllocator::deallocate") {
    SUBCASE("Is a no-op on current arena; subsequent allocation still works") {
      FrameAllocator<2> alloc(BasicOptions());

      void* const first = alloc.allocate(32, kAlign);
      alloc.deallocate(first, 32, kAlign);
      void* const second = alloc.allocate(32, kAlign);

      CHECK_NE(second, nullptr);
    }

    SUBCASE("Increments total_deallocations in current frame Stats") {
      FrameAllocator<1> alloc(BasicOptions());

      void* const ptr = alloc.allocate(16, kAlign);
      alloc.deallocate(ptr, 16, kAlign);

      CHECK_EQ(alloc.Stats().total_deallocations, 1);
    }
  }

  TEST_CASE("mem::FrameAllocator alias") {
    SUBCASE("FrameCount is 1") {
      CHECK_EQ(FrameAllocator<1>::FrameCount(), 1);
    }

    SUBCASE("Behaves as single-buffered FrameAllocator") {
      FrameAllocator<1> alloc(BasicOptions());

      std::ignore = alloc.allocate(16, kAlign);
      CHECK_FALSE(alloc.Empty());
      alloc.Advance();  // wraps back to frame 0, resets it
      CHECK(alloc.Empty());
    }
  }

  TEST_CASE("mem::FrameAllocator<> alias") {
    SUBCASE("FrameCount is 2") {
      CHECK_EQ(FrameAllocator<>::FrameCount(), 2);
    }

    SUBCASE("Behaves as double-buffered FrameAllocator") {
      FrameAllocator<> alloc(BasicOptions());

      [[maybe_unused]] const void* _ =
          alloc.allocate(64, kAlign);  // frame 0 has data
      alloc.Advance();                 // switch to frame 1
      CHECK(alloc.Empty());            // frame 1 is fresh
      CHECK_FALSE(alloc.Arena(0).Empty());
    }
  }

  TEST_CASE("mem::FrameAllocator thread-safety: concurrent allocate") {
    SUBCASE("All threads receive valid non-null pointers from current frame") {
      FrameAllocator<2> alloc(FrameAllocatorOptions{
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

    SUBCASE("total_allocations in current frame equals threads * allocs") {
      FrameAllocator<2> alloc(FrameAllocatorOptions{
          .initial_capacity = 65536,
          .growth =
              GrowthPolicy::Linear(65536, std::numeric_limits<size_t>::max())});

      constexpr size_t kThreads = 4;
      constexpr size_t kAllocsPerThread = 64;

      std::vector<std::thread> threads;
      threads.reserve(kThreads);
      for (size_t j = 0; j < kThreads; ++j) {
        threads.emplace_back([&alloc] {
          for (size_t i = 0; i < kAllocsPerThread; ++i) {
            std::ignore = alloc.allocate(8, kAlign);
          }
        });
      }
      for (auto& th : threads) {
        th.join();
      }

      CHECK_EQ(alloc.Stats().total_allocations, kThreads * kAllocsPerThread);
    }

    SUBCASE(
        "Concurrent allocations in current frame produce distinct pointers") {
      FrameAllocator<2> alloc(FrameAllocatorOptions{
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
        threads.emplace_back([&per_thread, &alloc, j] {
          for (size_t i = 0; i < kAllocsPerThread; ++i) {
            per_thread[j][i] = alloc.allocate(kAllocSize, kAlign);
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

  TEST_CASE("mem::FrameAllocator thread-safety: concurrent deallocate") {
    SUBCASE("Concurrent deallocates increment total_deallocations correctly") {
      FrameAllocator<2> alloc(FrameAllocatorOptions{
          .initial_capacity = 65536,
          .growth =
              GrowthPolicy::Linear(65536, std::numeric_limits<size_t>::max())});

      constexpr size_t kThreads = 4;
      constexpr size_t kAllocsPerThread = 64;
      constexpr size_t kAllocSize = 8;

      std::vector<std::vector<void*>> ptrs(
          kThreads, std::vector<void*>(kAllocsPerThread));

      for (size_t j = 0; j < kThreads; ++j) {
        for (size_t i = 0; i < kAllocsPerThread; ++i) {
          ptrs[j][i] = alloc.allocate(kAllocSize, kAlign);
        }
      }

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

      CHECK_EQ(alloc.Stats().total_deallocations, kThreads * kAllocsPerThread);
    }
  }
}  // TEST_SUITE
