#include <doctest/doctest.h>

#include <helios/memory/fixed_pool_allocator.hpp>

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

constexpr size_t kBlockSize = 64;
constexpr size_t kBlockCount = 8;
constexpr size_t kAlign = 64;

struct alignas(16) Vec3 {
  float x = 0.F;
  float y = 0.F;
  float z = 0.F;
};

}  // namespace

TEST_SUITE("helios::mem::FixedPoolAllocator") {
  TEST_CASE("mem::FixedPoolAllocator::ctor") {
    SUBCASE("BlockSize is at least the requested block size") {
      const FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      CHECK_GE(pool.BlockSize(), kBlockSize);
    }

    SUBCASE("BlockCount equals the configured block count") {
      const FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      CHECK_EQ(pool.BlockCount(), kBlockCount);
    }

    SUBCASE("FreeBlockCount equals BlockCount on construction") {
      const FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      CHECK_EQ(pool.FreeBlockCount(), pool.BlockCount());
    }

    SUBCASE("Pool is empty on construction") {
      const FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      CHECK(pool.Empty());
    }

    SUBCASE("Pool is not full on construction when block count exceeds one") {
      const FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      CHECK_FALSE(pool.Full());
    }

    SUBCASE("Alignment matches the configured alignment") {
      const FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      CHECK_EQ(pool.Alignment(), kAlign);
    }

    SUBCASE("InitialBlockCount equals BlockCount") {
      const FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      CHECK_EQ(pool.InitialBlockCount(), kBlockCount);
    }
  }

  TEST_CASE("mem::FixedPoolAllocator::ctor(FixedPoolAllocator&&)") {
    SUBCASE("Moved-into pool carries allocation state") {
      FixedPoolAllocator source(kBlockSize, kBlockCount, kAlign);
      [[maybe_unused]] const void* _ = source.allocate(kBlockSize, kAlign);

      FixedPoolAllocator moved(std::move(source));

      CHECK_FALSE(moved.Empty());
    }

    SUBCASE("Source is in moved-from state after move") {
      FixedPoolAllocator source(kBlockSize, kBlockCount, kAlign);
      void* const ptr = source.allocate(kBlockSize, kAlign);

      FixedPoolAllocator moved(std::move(source));

      CHECK_EQ(source.FreeBlockCount(), 0);
      CHECK_FALSE(source.Owns(ptr));
    }

    SUBCASE("Moved-into pool owns prior allocation") {
      FixedPoolAllocator source(kBlockSize, kBlockCount, kAlign);
      void* const ptr = source.allocate(kBlockSize, kAlign);

      FixedPoolAllocator moved(std::move(source));

      CHECK(moved.Owns(ptr));
    }
  }

  TEST_CASE("mem::FixedPoolAllocator::operator=(FixedPoolAllocator&&)") {
    SUBCASE("Target acquires source allocation state") {
      FixedPoolAllocator source(kBlockSize, kBlockCount, kAlign);
      [[maybe_unused]] const void* _ = source.allocate(kBlockSize, kAlign);
      FixedPoolAllocator target(kBlockSize, kBlockCount, kAlign);

      target = std::move(source);

      CHECK_FALSE(target.Empty());
    }

    SUBCASE("Source is in moved-from state after move assign") {
      FixedPoolAllocator source(kBlockSize, kBlockCount, kAlign);
      void* const ptr = source.allocate(kBlockSize, kAlign);
      FixedPoolAllocator target(kBlockSize, kBlockCount, kAlign);

      target = std::move(source);

      CHECK_EQ(source.FreeBlockCount(), 0);
      CHECK_FALSE(source.Owns(ptr));
    }

    SUBCASE("Self move assignment does not corrupt state") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      FixedPoolAllocator& ref = pool;

      ref = std::move(pool);  // NOLINT(clang-diagnostic-self-move)

      CHECK_FALSE(ref.Empty());
    }
  }

  TEST_CASE("mem::FixedPoolAllocator::Reset") {
    SUBCASE("Empty returns true after Reset") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);

      pool.Reset();

      CHECK(pool.Empty());
    }

    SUBCASE("FreeBlockCount equals BlockCount after Reset") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);

      pool.Reset();

      CHECK_EQ(pool.FreeBlockCount(), pool.BlockCount());
    }

    SUBCASE("Stats counters are zeroed after Reset") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);

      pool.Reset();

      const AllocatorStats stats = pool.Stats();
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
      CHECK_EQ(stats.allocation_count, 0);
    }

    SUBCASE("Allocation succeeds after Reset") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      for (size_t index = 0; index < kBlockCount; ++index) {
        [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      }

      pool.Reset();

      void* const ptr = pool.allocate(kBlockSize, kAlign);
      CHECK_NE(ptr, nullptr);
    }
  }

  TEST_CASE("mem::FixedPoolAllocator::Full") {
    SUBCASE("Returns false on fresh pool") {
      const FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      CHECK_FALSE(pool.Full());
    }

    SUBCASE("Returns true after all blocks are allocated") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      for (size_t index = 0; index < kBlockCount; ++index) {
        [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      }

      CHECK(pool.Full());
    }

    SUBCASE("Returns false after one block is freed") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      void* const second = pool.allocate(kBlockSize, kAlign);
      pool.deallocate(second, kBlockSize, kAlign);

      CHECK_FALSE(pool.Full());
    }

    SUBCASE("Returns false after Reset") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      for (size_t index = 0; index < kBlockCount; ++index) {
        [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      }

      pool.Reset();

      CHECK_FALSE(pool.Full());
    }
  }

  TEST_CASE("mem::FixedPoolAllocator::Empty") {
    SUBCASE("Returns true on fresh pool") {
      const FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      CHECK(pool.Empty());
    }

    SUBCASE("Returns false after any allocation") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      CHECK_FALSE(pool.Empty());
    }

    SUBCASE("Returns true after all allocations are freed") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);

      void* const first = pool.allocate(kBlockSize, kAlign);
      void* const second = pool.allocate(kBlockSize, kAlign);
      pool.deallocate(first, kBlockSize, kAlign);
      pool.deallocate(second, kBlockSize, kAlign);

      CHECK(pool.Empty());
    }

    SUBCASE("Returns true after Reset") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);

      pool.Reset();

      CHECK(pool.Empty());
    }
  }

  TEST_CASE("mem::FixedPoolAllocator::Owns") {
    SUBCASE("Returns true for pointer from this pool") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      void* const ptr = pool.allocate(kBlockSize, kAlign);
      CHECK(pool.Owns(ptr));
    }

    SUBCASE("Returns false for nullptr") {
      const FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      CHECK_FALSE(pool.Owns(nullptr));
    }

    SUBCASE("Returns false for stack variable pointer") {
      const FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      int local = 0;
      CHECK_FALSE(pool.Owns(&local));
    }

    SUBCASE("Returns false for pointer from a different pool") {
      FixedPoolAllocator first(kBlockSize, kBlockCount, kAlign);
      FixedPoolAllocator second(kBlockSize, kBlockCount, kAlign);

      void* const ptr = second.allocate(kBlockSize, kAlign);

      CHECK_FALSE(first.Owns(ptr));
    }

    SUBCASE("Returns true for pointer after deallocation") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);

      void* const ptr = pool.allocate(kBlockSize, kAlign);
      pool.deallocate(ptr, kBlockSize, kAlign);

      CHECK(pool.Owns(ptr));
    }
  }

  TEST_CASE("mem::FixedPoolAllocator::Stats") {
    SUBCASE("All relevant fields are zero on fresh pool") {
      const FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);

      const AllocatorStats stats = pool.Stats();
      CHECK_EQ(stats.total_allocated, 0);
      CHECK_EQ(stats.allocation_count, 0);
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
    }

    SUBCASE("total_allocations counts every allocate call") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);

      const void* ptr = pool.allocate(kBlockSize, kAlign);
      ptr = pool.allocate(kBlockSize, kAlign);

      CHECK_EQ(pool.Stats().total_allocations, 2);
    }

    SUBCASE("allocation_count tracks live allocations") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);

      void* const first = pool.allocate(kBlockSize, kAlign);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);

      CHECK_EQ(pool.Stats().allocation_count, 2);
      pool.deallocate(first, kBlockSize, kAlign);
      CHECK_EQ(pool.Stats().allocation_count, 1);
    }

    SUBCASE("total_deallocations counts every deallocate call") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);

      void* const first = pool.allocate(kBlockSize, kAlign);
      void* const second = pool.allocate(kBlockSize, kAlign);
      pool.deallocate(first, kBlockSize, kAlign);
      pool.deallocate(second, kBlockSize, kAlign);

      CHECK_EQ(pool.Stats().total_deallocations, 2);
    }

    SUBCASE("peak_usage reflects blocks * block_size high watermark") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);

      void* const first = pool.allocate(kBlockSize, kAlign);
      void* const second = pool.allocate(kBlockSize, kAlign);
      pool.deallocate(first, kBlockSize, kAlign);
      pool.deallocate(second, kBlockSize, kAlign);

      CHECK_GE(pool.Stats().peak_usage, kBlockSize * 2);
    }

    SUBCASE("alignment_waste is always zero for fixed pool allocator") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      CHECK_EQ(pool.Stats().alignment_waste, 0);
    }
  }

  TEST_CASE("mem::FixedPoolAllocator::BlockSize") {
    SUBCASE("Is at least the requested configured block size") {
      const FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      CHECK_GE(pool.BlockSize(), kBlockSize);
    }

    SUBCASE("Is at least sizeof(void*) to hold freelist pointer") {
      CHECK_GE(FixedPoolAllocator(1, 4, kDefaultAlignment).BlockSize(),
               sizeof(void*));
    }

    SUBCASE("Unchanged by allocations") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      CHECK_GE(pool.BlockSize(), kBlockSize);
    }
  }

  TEST_CASE("mem::FixedPoolAllocator::Alignment") {
    SUBCASE("Returns the configured alignment value") {
      const FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      CHECK_EQ(pool.Alignment(), kAlign);
    }

    SUBCASE("Custom alignment is reflected in returned pointers") {
      FixedPoolAllocator pool(32, 4, 32);
      void* const ptr = pool.allocate(16, 32);
      CHECK_EQ(reinterpret_cast<uintptr_t>(ptr) % 32, 0);
    }
  }

  TEST_CASE("mem::FixedPoolAllocator::InitialBlockCount") {
    SUBCASE("Returns the configured block count") {
      const FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      CHECK_EQ(pool.InitialBlockCount(), kBlockCount);
    }

    SUBCASE("Unchanged after allocations") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      CHECK_EQ(pool.InitialBlockCount(), kBlockCount);
    }
  }

  TEST_CASE("mem::FixedPoolAllocator::BlockCount") {
    SUBCASE("Equals the configured block count after construction") {
      const FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      CHECK_EQ(pool.BlockCount(), kBlockCount);
    }

    SUBCASE("Unchanged after allocations and deallocations") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      void* const ptr = pool.allocate(kBlockSize, kAlign);
      pool.deallocate(ptr, kBlockSize, kAlign);
      CHECK_EQ(pool.BlockCount(), kBlockCount);
    }

    SUBCASE("Preserved after Reset") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      pool.Reset();
      CHECK_EQ(pool.BlockCount(), kBlockCount);
    }
  }

  TEST_CASE("mem::FixedPoolAllocator::FreeBlockCount") {
    SUBCASE("Equals BlockCount on fresh pool") {
      const FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      CHECK_EQ(pool.FreeBlockCount(), kBlockCount);
    }

    SUBCASE("Decreases by one per allocate") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      CHECK_EQ(pool.FreeBlockCount(), kBlockCount - 1);
    }

    SUBCASE("Increases by one per deallocate") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);

      void* const ptr = pool.allocate(kBlockSize, kAlign);
      const size_t free_after_alloc = pool.FreeBlockCount();
      pool.deallocate(ptr, kBlockSize, kAlign);

      CHECK_EQ(pool.FreeBlockCount(), free_after_alloc + 1);
    }

    SUBCASE("Equals BlockCount after Reset") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);

      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      pool.Reset();

      CHECK_EQ(pool.FreeBlockCount(), pool.BlockCount());
    }
  }

  TEST_CASE("mem::FixedPoolAllocator::UsedBlockCount") {
    SUBCASE("Is zero on fresh pool") {
      const FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      CHECK_EQ(pool.UsedBlockCount(), 0);
    }

    SUBCASE("Increases by one per allocate") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);

      const void* ptr = pool.allocate(kBlockSize, kAlign);
      ptr = pool.allocate(kBlockSize, kAlign);

      CHECK_EQ(pool.UsedBlockCount(), 2);
    }

    SUBCASE("Decreases by one per deallocate") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);

      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      void* const second = pool.allocate(kBlockSize, kAlign);
      pool.deallocate(second, kBlockSize, kAlign);

      CHECK_EQ(pool.UsedBlockCount(), 1);
    }

    SUBCASE("Is zero after Reset") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);

      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      pool.Reset();

      CHECK_EQ(pool.UsedBlockCount(), 0);
    }

    SUBCASE("Equals BlockCount minus FreeBlockCount") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);

      const void* ptr = pool.allocate(kBlockSize, kAlign);
      ptr = pool.allocate(kBlockSize, kAlign);

      CHECK_EQ(pool.UsedBlockCount(),
               pool.BlockCount() - pool.FreeBlockCount());
    }
  }

  TEST_CASE("mem::FixedPoolAllocator::allocate") {
    SUBCASE("Returns non-null pointer") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      CHECK_NE(pool.allocate(kBlockSize, kAlign), nullptr);
    }

    SUBCASE("Multiple allocations return distinct pointers") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);

      void* const first = pool.allocate(kBlockSize, kAlign);
      void* const second = pool.allocate(kBlockSize, kAlign);

      CHECK_NE(first, second);
    }

    SUBCASE("Returned pointer satisfies pool alignment") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      void* const ptr = pool.allocate(kBlockSize, kAlign);
      CHECK_EQ(reinterpret_cast<uintptr_t>(ptr) % kAlign, 0);
    }

    SUBCASE("Usable as PMR polymorphic_allocator for typed objects") {
      FixedPoolAllocator pool = FixedPoolAllocator::ForType<Vec3>(16);

      std::pmr::polymorphic_allocator<Vec3> pa(&pool);
      Vec3* const vec = pa.allocate(1);
      vec->x = 1.0F;
      vec->y = 2.0F;
      vec->z = 3.0F;

      CHECK_EQ(vec->x, 1.0F);
      pa.deallocate(vec, 1);
    }

    SUBCASE("Usable as PMR upstream for std::pmr::vector") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);
      std::pmr::vector<int> values(&pool);
      values.resize(8, 7);
      CHECK_EQ(values.size(), 8);
    }
  }

  TEST_CASE("mem::FixedPoolAllocator::deallocate") {
    SUBCASE("Freed block is returned to free list") {
      FixedPoolAllocator pool(kBlockSize, 2, kAlign);

      [[maybe_unused]] const void* _ = pool.allocate(kBlockSize, kAlign);
      void* const second = pool.allocate(kBlockSize, kAlign);
      CHECK(pool.Full());
      pool.deallocate(second, kBlockSize, kAlign);

      CHECK_FALSE(pool.Full());
    }

    SUBCASE("Pool is empty after all blocks freed") {
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);

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
      FixedPoolAllocator pool(kBlockSize, kBlockCount, kAlign);

      void* const ptr = pool.allocate(kBlockSize, kAlign);
      pool.deallocate(ptr, kBlockSize, kAlign);

      CHECK_EQ(pool.Stats().total_deallocations, 1);
    }
  }

  TEST_CASE("mem::FixedPoolAllocator thread-safety: concurrent allocate") {
    SUBCASE("All threads receive valid non-null pointers") {
      FixedPoolAllocator pool(64, 512, 64);

      constexpr size_t kThreads = 8;
      constexpr size_t kAllocsPerThread = 64;

      std::atomic<int> null_count{0};
      std::vector<std::thread> threads;
      threads.reserve(kThreads);

      for (size_t index = 0; index < kThreads; ++index) {
        threads.emplace_back([&pool, &null_count] {
          for (size_t allocation = 0; allocation < kAllocsPerThread;
               ++allocation) {
            void* const ptr = pool.allocate(32, 64);
            if (ptr == nullptr) {
              null_count.fetch_add(1, std::memory_order_relaxed);
            }
          }
        });
      }

      for (auto& thread : threads) {
        thread.join();
      }

      CHECK_EQ(null_count.load(), 0);
    }

    SUBCASE("Concurrent allocate and deallocate succeed under contention") {
      FixedPoolAllocator pool(64, 64, 64);

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
            void* const ptr = pool.allocate(32, 64);
            pool.deallocate(ptr, 32, 64);
            if (ptr != nullptr) {
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
}
