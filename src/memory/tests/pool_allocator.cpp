#include <doctest/doctest.h>

#include <helios/memory/pool_allocator.hpp>

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

constexpr size_t kBlockSize = 32;
constexpr size_t kBlockCount = 8;
constexpr size_t kAlign = kDefaultAlignment;

constexpr PoolAllocatorOptions GrowingOptions(size_t bs = kBlockSize,
                                              size_t bc = kBlockCount) {
  return PoolAllocatorOptions{.block_size = bs,
                              .block_count = bc,
                              .alignment = kAlign,
                              .growth = GrowthPolicy::Linear(bc, SIZE_MAX)};
}

struct alignas(16) Vec3 {
  float x = 0.F, y = 0.F, z = 0.F;
};

}  // namespace

TEST_SUITE("helios::mem::PoolAllocator") {
  TEST_CASE("mem::PoolAllocator::ctor(PoolAllocatorOptions)") {
    SUBCASE("BlockSize is at least the requested block_size") {
      const PoolAllocator pool(PoolAllocatorOptions{
          .block_size = kBlockSize, .block_count = kBlockCount});
      CHECK_GE(pool.BlockSize(), kBlockSize);
    }

    SUBCASE("InitialBlockCount matches options.block_count") {
      const PoolAllocator pool(PoolAllocatorOptions{
          .block_size = kBlockSize, .block_count = kBlockCount});
      CHECK_EQ(pool.InitialBlockCount(), kBlockCount);
    }

    SUBCASE("BlockCount equals block_count on construction") {
      const PoolAllocator pool(PoolAllocatorOptions{
          .block_size = kBlockSize, .block_count = kBlockCount});
      CHECK_EQ(pool.BlockCount(), kBlockCount);
    }

    SUBCASE("FreeBlockCount equals BlockCount on construction") {
      const PoolAllocator pool(PoolAllocatorOptions{
          .block_size = kBlockSize, .block_count = kBlockCount});
      CHECK_EQ(pool.FreeBlockCount(), pool.BlockCount());
    }

    SUBCASE("Pool is empty on construction") {
      const PoolAllocator pool(PoolAllocatorOptions{
          .block_size = kBlockSize, .block_count = kBlockCount});
      CHECK(pool.Empty());
    }

    SUBCASE("Pool is not full on construction (blocks > 1)") {
      const PoolAllocator pool(PoolAllocatorOptions{
          .block_size = kBlockSize, .block_count = kBlockCount});
      CHECK_FALSE(pool.Full());
    }

    SUBCASE("Alignment is stored correctly") {
      const PoolAllocator pool(PoolAllocatorOptions{.block_size = kBlockSize,
                                                    .block_count = kBlockCount,
                                                    .alignment = 16});
      CHECK_EQ(pool.Alignment(), 16);
    }

    SUBCASE("Growth policy is stored correctly") {
      const auto policy = GrowthPolicy::Geometric();
      const PoolAllocator pool(PoolAllocatorOptions{.block_size = kBlockSize,
                                                    .block_count = kBlockCount,
                                                    .growth = policy});
      CHECK_EQ(pool.Growth().max_capacity, policy.max_capacity);
    }
  }

  TEST_CASE("mem::PoolAllocator::ctor(size_t, size_t, size_t)") {
    SUBCASE("BlockSize is at least the requested value") {
      const PoolAllocator pool(kBlockSize, kBlockCount);
      CHECK_GE(pool.BlockSize(), kBlockSize);
    }

    SUBCASE("InitialBlockCount matches argument") {
      const PoolAllocator pool(kBlockSize, kBlockCount);
      CHECK_EQ(pool.InitialBlockCount(), kBlockCount);
    }

    SUBCASE("Alignment defaults to kDefaultAlignment") {
      const PoolAllocator pool(kBlockSize, kBlockCount);
      CHECK_EQ(pool.Alignment(), kDefaultAlignment);
    }

    SUBCASE("Custom alignment is stored") {
      const PoolAllocator pool(kBlockSize, kBlockCount, 16);
      CHECK_EQ(pool.Alignment(), 16);
    }

    SUBCASE("Uses geometric growth policy") {
      const PoolAllocator pool(kBlockSize, kBlockCount);
      CHECK_EQ(pool.Growth().max_capacity,
               GrowthPolicy::Geometric().max_capacity);
    }
  }

  TEST_CASE("mem::PoolAllocator::ctor(PoolAllocator&&)") {
    SUBCASE("Moved-into pool carries allocation state") {
      PoolAllocator source(kBlockSize, kBlockCount);
      [[maybe_unused]] const void* _ = source.allocate(kBlockSize, kAlign);

      PoolAllocator moved(std::move(source));

      CHECK_FALSE(moved.Empty());
    }

    SUBCASE("Source block size is zeroed after move") {
      PoolAllocator source(kBlockSize, kBlockCount);
      PoolAllocator moved(std::move(source));
      CHECK_EQ(source.BlockSize(), 0);
    }

    SUBCASE("Source is empty after move") {
      PoolAllocator source(kBlockSize, kBlockCount);
      [[maybe_unused]] const void* _ = source.allocate(kBlockSize, kAlign);

      PoolAllocator moved(std::move(source));

      CHECK(source.Empty());
    }
  }

  TEST_CASE("mem::PoolAllocator::operator=(PoolAllocator&&)") {
    SUBCASE("Target acquires source allocation state") {
      PoolAllocator source(kBlockSize, kBlockCount);
      [[maybe_unused]] const void* _ = source.allocate(kBlockSize, kAlign);
      PoolAllocator target(kBlockSize * 2, 4);

      target = std::move(source);

      CHECK_FALSE(target.Empty());
    }

    SUBCASE("Source block size is zeroed after move assign") {
      PoolAllocator source(kBlockSize, kBlockCount);
      PoolAllocator target(kBlockSize, kBlockCount);

      target = std::move(source);

      CHECK_EQ(source.BlockSize(), 0);
    }

    SUBCASE("Self move assignment does not corrupt state") {
      PoolAllocator pool(kBlockSize, kBlockCount);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      PoolAllocator& ref = pool;

      ref = std::move(pool);  // NOLINT(clang-diagnostic-self-move)

      CHECK_FALSE(ref.Empty());
    }
  }

  TEST_CASE("mem::PoolAllocator::ForType") {
    SUBCASE("BlockSize is at least sizeof(T)") {
      const PoolAllocator pool = PoolAllocator::ForType<Vec3>(8);
      CHECK_GE(pool.BlockSize(), sizeof(Vec3));
    }

    SUBCASE("Alignment is at least alignof(T)") {
      const PoolAllocator pool = PoolAllocator::ForType<Vec3>(8);
      CHECK_GE(pool.Alignment(), alignof(Vec3));
    }

    SUBCASE("InitialBlockCount equals the argument") {
      const PoolAllocator pool = PoolAllocator::ForType<Vec3>(16);
      CHECK_EQ(pool.InitialBlockCount(), 16);
    }
  }

  TEST_CASE("mem::PoolAllocator::Reset") {
    SUBCASE("Empty returns true after Reset") {
      PoolAllocator pool(kBlockSize, kBlockCount);
      const void* ptr = pool.allocate(kBlockSize, kAlign);
      ptr = pool.allocate(kBlockSize, kAlign);

      pool.Reset();

      CHECK(pool.Empty());
    }

    SUBCASE("FreeBlockCount equals BlockCount after Reset") {
      PoolAllocator pool(kBlockSize, kBlockCount);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);

      pool.Reset();

      CHECK_EQ(pool.FreeBlockCount(), pool.BlockCount());
    }

    SUBCASE("Stats counters are zeroed after Reset") {
      PoolAllocator pool(kBlockSize, kBlockCount);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);

      pool.Reset();

      const AllocatorStats stats = pool.Stats();
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
      CHECK_EQ(stats.allocation_count, 0);
    }

    SUBCASE("BlockCount across grown chunks is preserved after Reset") {
      PoolAllocator pool(GrowingOptions());
      for (size_t i = 0; i < kBlockCount + 1; ++i) {
        [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      }
      const size_t count_before = pool.BlockCount();

      pool.Reset();

      CHECK_EQ(pool.BlockCount(), count_before);
    }

    SUBCASE("Allocation succeeds after Reset") {
      PoolAllocator pool(kBlockSize, kBlockCount);
      for (size_t i = 0; i < kBlockCount; ++i) {
        [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      }

      pool.Reset();

      void* const ptr = pool.allocate(kBlockSize, kAlign);
      CHECK_NE(ptr, nullptr);
    }
  }

  TEST_CASE("mem::PoolAllocator::Full") {
    SUBCASE("Returns false on fresh pool") {
      const PoolAllocator pool(kBlockSize, kBlockCount);
      CHECK_FALSE(pool.Full());
    }

    SUBCASE("Returns true after all blocks are allocated (geometric growth)") {
      PoolAllocator pool(
          PoolAllocatorOptions{.block_size = kBlockSize,
                               .block_count = 2,
                               .growth = GrowthPolicy::Geometric()});
      const void* ptr = pool.allocate(kBlockSize, kAlign);
      ptr = pool.allocate(kBlockSize, kAlign);

      CHECK(pool.Full());
    }

    SUBCASE("Returns false after one block is freed") {
      PoolAllocator pool(
          PoolAllocatorOptions{.block_size = kBlockSize,
                               .block_count = 2,
                               .growth = GrowthPolicy::Geometric()});
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      void* const second = pool.allocate(kBlockSize, kAlign);
      pool.deallocate(second, kBlockSize, kAlign);

      CHECK_FALSE(pool.Full());
    }

    SUBCASE("Returns false after Reset") {
      PoolAllocator pool(
          PoolAllocatorOptions{.block_size = kBlockSize,
                               .block_count = 2,
                               .growth = GrowthPolicy::Geometric()});
      const void* ptr = pool.allocate(kBlockSize, kAlign);
      ptr = pool.allocate(kBlockSize, kAlign);

      pool.Reset();

      CHECK_FALSE(pool.Full());
    }
  }

  TEST_CASE("mem::PoolAllocator::Empty") {
    SUBCASE("Returns true on fresh pool") {
      const PoolAllocator pool(kBlockSize, kBlockCount);
      CHECK(pool.Empty());
    }

    SUBCASE("Returns false after any allocation") {
      PoolAllocator pool(kBlockSize, kBlockCount);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      CHECK_FALSE(pool.Empty());
    }

    SUBCASE("Returns true after all allocations are freed") {
      PoolAllocator pool(kBlockSize, kBlockCount);

      void* const first = pool.allocate(kBlockSize, kAlign);
      void* const second = pool.allocate(kBlockSize, kAlign);
      pool.deallocate(first, kBlockSize, kAlign);
      pool.deallocate(second, kBlockSize, kAlign);

      CHECK(pool.Empty());
    }

    SUBCASE("Returns true after Reset") {
      PoolAllocator pool(kBlockSize, kBlockCount);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);

      pool.Reset();

      CHECK(pool.Empty());
    }
  }

  TEST_CASE("mem::PoolAllocator::Owns") {
    SUBCASE("Returns true for pointer from this pool") {
      PoolAllocator pool(kBlockSize, kBlockCount);
      void* const ptr = pool.allocate(kBlockSize, kAlign);
      CHECK(pool.Owns(ptr));
    }

    SUBCASE("Returns false for nullptr") {
      const PoolAllocator pool(kBlockSize, kBlockCount);
      CHECK_FALSE(pool.Owns(nullptr));
    }

    SUBCASE("Returns false for stack variable pointer") {
      const PoolAllocator pool(kBlockSize, kBlockCount);
      int local = 0;
      CHECK_FALSE(pool.Owns(&local));
    }

    SUBCASE("Returns false for pointer from first different pool") {
      PoolAllocator first(kBlockSize, kBlockCount);
      PoolAllocator second(kBlockSize, kBlockCount);

      void* const ptr = second.allocate(kBlockSize, kAlign);

      CHECK_FALSE(first.Owns(ptr));
    }

    SUBCASE("Returns true for pointer after deallocation (chunk not freed)") {
      PoolAllocator pool(kBlockSize, kBlockCount);

      void* const ptr = pool.allocate(kBlockSize, kAlign);
      pool.deallocate(ptr, kBlockSize, kAlign);

      CHECK(pool.Owns(ptr));
    }
  }

  TEST_CASE("mem::PoolAllocator::Stats") {
    SUBCASE("All relevant fields are zero on fresh pool") {
      const PoolAllocator pool(kBlockSize, kBlockCount);

      const AllocatorStats stats = pool.Stats();
      CHECK_EQ(stats.total_allocated, 0);
      CHECK_EQ(stats.allocation_count, 0);
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
    }

    SUBCASE("total_allocations counts every allocate call") {
      PoolAllocator pool(kBlockSize, kBlockCount);

      const void* ptr = pool.allocate(kBlockSize, kAlign);
      ptr = pool.allocate(kBlockSize, kAlign);

      CHECK_EQ(pool.Stats().total_allocations, 2);
    }

    SUBCASE("allocation_count tracks live allocations") {
      PoolAllocator pool(kBlockSize, kBlockCount);

      void* const first = pool.allocate(kBlockSize, kAlign);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);

      CHECK_EQ(pool.Stats().allocation_count, 2);
      pool.deallocate(first, kBlockSize, kAlign);
      CHECK_EQ(pool.Stats().allocation_count, 1);
    }

    SUBCASE("total_deallocations counts every deallocate call") {
      PoolAllocator pool(kBlockSize, kBlockCount);

      void* const first = pool.allocate(kBlockSize, kAlign);
      void* const second = pool.allocate(kBlockSize, kAlign);
      pool.deallocate(first, kBlockSize, kAlign);
      pool.deallocate(second, kBlockSize, kAlign);

      CHECK_EQ(pool.Stats().total_deallocations, 2);
    }

    SUBCASE("peak_usage reflects blocks * block_size high watermark") {
      PoolAllocator pool(kBlockSize, kBlockCount);

      void* const first = pool.allocate(kBlockSize, kAlign);
      void* const second = pool.allocate(kBlockSize, kAlign);
      pool.deallocate(first, kBlockSize, kAlign);
      pool.deallocate(second, kBlockSize, kAlign);

      CHECK_GE(pool.Stats().peak_usage, kBlockSize * 2);
    }

    SUBCASE("alignment_waste is always zero for pool allocator") {
      PoolAllocator pool(kBlockSize, kBlockCount);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      CHECK_EQ(pool.Stats().alignment_waste, 0);
    }
  }

  TEST_CASE("mem::PoolAllocator::BlockSize") {
    SUBCASE("Is at least the requested value after construction") {
      const PoolAllocator pool(48, kBlockCount);
      CHECK_GE(pool.BlockSize(), 48);
    }

    SUBCASE("Is at least sizeof(void*) to hold freelist pointer") {
      const PoolAllocator pool(1, kBlockCount);
      CHECK_GE(pool.BlockSize(), sizeof(void*));
    }

    SUBCASE("Unchanged by allocations") {
      PoolAllocator pool(kBlockSize, kBlockCount);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      CHECK_GE(pool.BlockSize(), kBlockSize);
    }
  }

  TEST_CASE("mem::PoolAllocator::Alignment") {
    SUBCASE("Returns kDefaultAlignment when not specified") {
      const PoolAllocator pool(kBlockSize, kBlockCount);
      CHECK_EQ(pool.Alignment(), kDefaultAlignment);
    }

    SUBCASE("Returns custom alignment when specified") {
      const PoolAllocator pool(kBlockSize, kBlockCount, 32);
      CHECK_EQ(pool.Alignment(), 32);
    }
  }

  TEST_CASE("mem::PoolAllocator::InitialBlockCount") {
    SUBCASE("Returns block_count from options ctor") {
      const PoolAllocator pool(
          PoolAllocatorOptions{.block_size = kBlockSize, .block_count = 12});
      CHECK_EQ(pool.InitialBlockCount(), 12);
    }

    SUBCASE("Returns block_count from size_t ctor") {
      const PoolAllocator pool(kBlockSize, 7);
      CHECK_EQ(pool.InitialBlockCount(), 7);
    }

    SUBCASE("Unchanged after growth") {
      PoolAllocator pool(GrowingOptions(kBlockSize, 4));

      for (size_t i = 0; i < 5; ++i) {
        [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      }

      CHECK_EQ(pool.InitialBlockCount(), 4);
    }
  }

  TEST_CASE("mem::PoolAllocator::BlockCount") {
    SUBCASE("Equals initial block_count right after construction") {
      const PoolAllocator pool(kBlockSize, kBlockCount);
      CHECK_EQ(pool.BlockCount(), kBlockCount);
    }

    SUBCASE("Increases when pool grows due to exhaustion") {
      PoolAllocator pool(GrowingOptions(kBlockSize, 4));

      for (size_t i = 0; i < 5; ++i) {
        [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      }

      CHECK_GT(pool.BlockCount(), 4);
    }

    SUBCASE("Preserved (not reduced) after Reset") {
      PoolAllocator pool(GrowingOptions(kBlockSize, 4));

      for (size_t i = 0; i < 5; ++i) {
        [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      }
      const size_t count_before = pool.BlockCount();

      pool.Reset();

      CHECK_EQ(pool.BlockCount(), count_before);
    }
  }

  TEST_CASE("mem::PoolAllocator::FreeBlockCount") {
    SUBCASE("Equals BlockCount on fresh pool") {
      const PoolAllocator pool(kBlockSize, kBlockCount);
      CHECK_EQ(pool.FreeBlockCount(), kBlockCount);
    }

    SUBCASE("Decreases by one per allocate") {
      PoolAllocator pool(kBlockSize, kBlockCount);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      CHECK_EQ(pool.FreeBlockCount(), kBlockCount - 1);
    }

    SUBCASE("Increases by one per deallocate") {
      PoolAllocator pool(kBlockSize, kBlockCount);

      void* const ptr = pool.allocate(kBlockSize, kAlign);
      const size_t free_after_alloc = pool.FreeBlockCount();
      pool.deallocate(ptr, kBlockSize, kAlign);

      CHECK_EQ(pool.FreeBlockCount(), free_after_alloc + 1);
    }

    SUBCASE("Equals BlockCount after Reset") {
      PoolAllocator pool(kBlockSize, kBlockCount);

      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      pool.Reset();

      CHECK_EQ(pool.FreeBlockCount(), pool.BlockCount());
    }
  }

  TEST_CASE("mem::PoolAllocator::UsedBlockCount") {
    SUBCASE("Is zero on fresh pool") {
      const PoolAllocator pool(kBlockSize, kBlockCount);
      CHECK_EQ(pool.UsedBlockCount(), 0);
    }

    SUBCASE("Increases by one per allocate") {
      PoolAllocator pool(kBlockSize, kBlockCount);

      const void* ptr = pool.allocate(kBlockSize, kAlign);
      ptr = pool.allocate(kBlockSize, kAlign);

      CHECK_EQ(pool.UsedBlockCount(), 2);
    }

    SUBCASE("Decreases by one per deallocate") {
      PoolAllocator pool(kBlockSize, kBlockCount);

      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      void* const second = pool.allocate(kBlockSize, kAlign);
      pool.deallocate(second, kBlockSize, kAlign);

      CHECK_EQ(pool.UsedBlockCount(), 1);
    }

    SUBCASE("Is zero after Reset") {
      PoolAllocator pool(kBlockSize, kBlockCount);

      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      pool.Reset();

      CHECK_EQ(pool.UsedBlockCount(), 0);
    }

    SUBCASE("Equals BlockCount minus FreeBlockCount") {
      PoolAllocator pool(kBlockSize, kBlockCount);

      const void* ptr = pool.allocate(kBlockSize, kAlign);
      ptr = pool.allocate(kBlockSize, kAlign);

      CHECK_EQ(pool.UsedBlockCount(),
               pool.BlockCount() - pool.FreeBlockCount());
    }
  }

  TEST_CASE("mem::PoolAllocator::Growth") {
    SUBCASE("Returns geometric policy for size_t ctor") {
      const PoolAllocator pool(kBlockSize, kBlockCount);
      CHECK_EQ(pool.Growth().max_capacity,
               GrowthPolicy::Geometric().max_capacity);
    }

    SUBCASE("Returns Linear policy when supplied via options") {
      const GrowthPolicy policy = GrowthPolicy::Linear(kBlockCount, SIZE_MAX);
      const PoolAllocator pool(PoolAllocatorOptions{.block_size = kBlockSize,
                                                    .block_count = kBlockCount,
                                                    .growth = policy});
      CHECK_EQ(pool.Growth().max_capacity, policy.max_capacity);
    }

    SUBCASE("Unchanged by allocations") {
      PoolAllocator pool(kBlockSize, kBlockCount);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      CHECK_EQ(pool.Growth().max_capacity,
               GrowthPolicy::Geometric().max_capacity);
    }
  }

  TEST_CASE("mem::PoolAllocator::allocate") {
    SUBCASE("Returns non-null pointer") {
      PoolAllocator pool(kBlockSize, kBlockCount);
      CHECK_NE(pool.allocate(kBlockSize, kAlign), nullptr);
    }

    SUBCASE("Multiple allocations return distinct pointers") {
      PoolAllocator pool(kBlockSize, kBlockCount);

      void* const first = pool.allocate(kBlockSize, kAlign);
      void* const second = pool.allocate(kBlockSize, kAlign);

      CHECK_NE(first, second);
    }

    SUBCASE("Returned pointer satisfies pool alignment") {
      PoolAllocator pool(kBlockSize, kBlockCount, 16);
      void* const ptr = pool.allocate(kBlockSize, 16);
      CHECK_EQ(reinterpret_cast<uintptr_t>(ptr) % 16, 0);
    }

    SUBCASE("Allocation triggers growth when pool is exhausted") {
      PoolAllocator pool(GrowingOptions(kBlockSize, 2));

      const void* not_used = pool.allocate(kBlockSize, kAlign);
      not_used = pool.allocate(kBlockSize, kAlign);
      void* const ptr = pool.allocate(kBlockSize, kAlign);  // must grow

      CHECK_NE(ptr, nullptr);
      CHECK_GT(pool.BlockCount(), 2);
    }

    SUBCASE("Usable as PMR polymorphic_allocator for typed objects") {
      PoolAllocator pool = PoolAllocator::ForType<Vec3>(16);

      std::pmr::polymorphic_allocator<Vec3> pa(&pool);
      Vec3* const vec = pa.allocate(1);
      vec->x = 1.0F;
      vec->y = 2.0F;
      vec->z = 3.0F;

      CHECK_EQ(vec->x, 1.0F);
      pa.deallocate(vec, 1);
    }
  }

  TEST_CASE("mem::PoolAllocator::deallocate") {
    SUBCASE("Freed block is returned to free list") {
      PoolAllocator pool(
          PoolAllocatorOptions{.block_size = kBlockSize,
                               .block_count = 2,
                               .growth = GrowthPolicy::Geometric()});

      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      void* const second = pool.allocate(kBlockSize, kAlign);
      CHECK(pool.Full());
      pool.deallocate(second, kBlockSize, kAlign);

      CHECK_FALSE(pool.Full());
    }

    SUBCASE("Pool is empty after all blocks freed") {
      PoolAllocator pool(kBlockSize, kBlockCount);

      std::vector<void*> ptrs(kBlockCount);
      for (auto& ptr : ptrs) {
        ptr = pool.allocate(kBlockSize, kAlign);
      }

      for (auto* ptr : ptrs) {
        pool.deallocate(ptr, kBlockSize, kAlign);
      }

      CHECK(pool.Empty());
    }

    SUBCASE("total_deallocations increments per call") {
      PoolAllocator pool(kBlockSize, kBlockCount);

      void* const ptr = pool.allocate(kBlockSize, kAlign);
      pool.deallocate(ptr, kBlockSize, kAlign);

      CHECK_EQ(pool.Stats().total_deallocations, 1);
    }
  }

  TEST_CASE("mem::PoolAllocator thread-safety: concurrent allocate") {
    SUBCASE("All threads receive valid non-null pointers") {
      PoolAllocator pool(GrowingOptions(kBlockSize, 256));

      constexpr size_t kThreads = 8;
      constexpr size_t kAllocsPerThread = 64;

      std::atomic<int> null_count{0};
      std::vector<std::thread> threads;
      threads.reserve(kThreads);

      for (size_t j = 0; j < kThreads; ++j) {
        threads.emplace_back([&pool, &null_count] {
          for (size_t i = 0; i < kAllocsPerThread; ++i) {
            void* const ptr = pool.allocate(kBlockSize, kAlign);
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
      PoolAllocator pool(GrowingOptions(kBlockSize, 256));

      constexpr size_t kThreads = 4;
      constexpr size_t kAllocsPerThread = 64;

      std::vector<std::thread> threads;
      threads.reserve(kThreads);
      for (size_t j = 0; j < kThreads; ++j) {
        threads.emplace_back([&pool] {
          for (size_t i = 0; i < kAllocsPerThread; ++i) {
            [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
          }
        });
      }

      for (auto& th : threads) {
        th.join();
      }

      CHECK_EQ(pool.Stats().total_allocations, kThreads * kAllocsPerThread);
    }

    SUBCASE("Concurrent allocations produce distinct pointers") {
      PoolAllocator pool(GrowingOptions(kBlockSize, 512));

      constexpr size_t kThreads = 4;
      constexpr size_t kAllocsPerThread = 128;

      std::vector<std::vector<void*>> per_thread(kThreads);
      std::vector<std::thread> threads;
      threads.reserve(kThreads);

      for (size_t j = 0; j < kThreads; ++j) {
        per_thread[j].resize(kAllocsPerThread);
        threads.emplace_back([&per_thread, &pool, j] {
          for (size_t i = 0; i < kAllocsPerThread; ++i) {
            per_thread[j][i] = pool.allocate(kBlockSize, kAlign);
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

    SUBCASE("Retries when another thread consumes a grown chunk") {
      PoolAllocator pool(PoolAllocatorOptions{
          .block_size = 32,
          .block_count = 1,
          .alignment = kAlign,
          .growth =
              GrowthPolicy::Linear(1, std::numeric_limits<size_t>::max())});

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
            if (pool.allocate(32, kAlign) != nullptr) {
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

    SUBCASE("Pool is empty after all paired alloc/dealloc across threads") {
      PoolAllocator pool(GrowingOptions(kBlockSize, 256));

      constexpr size_t kThreads = 4;
      constexpr size_t kAllocsPerThread = 64;

      std::vector<std::vector<void*>> ptrs(
          kThreads, std::vector<void*>(kAllocsPerThread));

      // Phase 1: allocate.
      {
        std::vector<std::thread> threads;
        threads.reserve(kThreads);
        for (size_t j = 0; j < kThreads; ++j) {
          threads.emplace_back([&ptrs, &pool, j] {
            for (size_t i = 0; i < kAllocsPerThread; ++i) {
              ptrs[j][i] = pool.allocate(kBlockSize, kAlign);
            }
          });
        }
        for (auto& th : threads) {
          th.join();
        }
      }

      // Phase 2: deallocate.
      {
        std::vector<std::thread> threads;
        threads.reserve(kThreads);
        for (size_t j = 0; j < kThreads; ++j) {
          threads.emplace_back([&ptrs, &pool, j] {
            for (size_t i = 0; i < kAllocsPerThread; ++i) {
              pool.deallocate(ptrs[j][i], kBlockSize, kAlign);
            }
          });
        }
        for (auto& th : threads) {
          th.join();
        }
      }

      CHECK(pool.Empty());
    }
  }
}  // TEST_SUITE
