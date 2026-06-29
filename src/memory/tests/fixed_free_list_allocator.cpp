#include <doctest/doctest.h>

#include <helios/memory/fixed_free_list_allocator.hpp>

#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <vector>

using namespace helios::mem;

namespace {

constexpr size_t kMinFreeListCapacity = sizeof(void*) * 4;
constexpr size_t kCapacity = 4096;
constexpr size_t kSmallCapacity = 512;
constexpr size_t kAlign = alignof(std::max_align_t);

}  // namespace

TEST_SUITE("helios::mem::FixedFreeListAllocator") {
  TEST_CASE("mem::FixedFreeListAllocator::ctor()") {
    SUBCASE("InitialCapacity equals configured capacity") {
      const FixedFreeListAllocator alloc(kCapacity);
      CHECK_EQ(alloc.InitialCapacity(), kCapacity);
    }

    SUBCASE("TotalCapacity equals configured capacity") {
      const FixedFreeListAllocator alloc(kCapacity);
      CHECK_EQ(alloc.TotalCapacity(), kCapacity);
    }

    SUBCASE("FreeBlockCount is 1 on construction") {
      const FixedFreeListAllocator alloc(kCapacity);
      CHECK_EQ(alloc.FreeBlockCount(), 1);
    }

    SUBCASE("Allocator is empty on construction") {
      const FixedFreeListAllocator alloc(kCapacity);
      CHECK(alloc.Empty());
    }

    SUBCASE("Stats counters are zero on construction") {
      const FixedFreeListAllocator alloc(kCapacity);
      const AllocatorStats stats = alloc.Stats();
      CHECK_EQ(stats.total_allocated, 0);
      CHECK_EQ(stats.peak_usage, 0);
      CHECK_EQ(stats.allocation_count, 0);
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
      CHECK_EQ(stats.alignment_waste, 0);
    }
  }

  TEST_CASE("mem::FixedFreeListAllocator::ctor(FixedFreeListAllocator&&)") {
    SUBCASE("Moved-into allocator carries allocation state") {
      FixedFreeListAllocator source(kCapacity);
      [[maybe_unused]] const void* _ = source.allocate(64, kAlign);
      FixedFreeListAllocator moved(std::move(source));
      CHECK_FALSE(moved.Empty());
      CHECK_EQ(moved.Stats().allocation_count, 1);
    }

    SUBCASE("Source is empty after move") {
      FixedFreeListAllocator source(kCapacity);
      [[maybe_unused]] const void* _ = source.allocate(32, kAlign);
      FixedFreeListAllocator moved(std::move(source));
      CHECK(source.Empty());
    }

    SUBCASE("Moved-into allocator owns prior allocation") {
      FixedFreeListAllocator source(kCapacity);
      void* const ptr = source.allocate(64, kAlign);
      FixedFreeListAllocator moved(std::move(source));
      CHECK(moved.Owns(ptr));
    }

    SUBCASE("Moved-into allocator TotalCapacity is unchanged") {
      FixedFreeListAllocator source(kCapacity);
      FixedFreeListAllocator moved(std::move(source));
      CHECK_EQ(moved.TotalCapacity(), kCapacity);
    }

    SUBCASE("Source FreeBlockCount is zero after move") {
      FixedFreeListAllocator source(kCapacity);
      FixedFreeListAllocator moved(std::move(source));
      CHECK_EQ(source.FreeBlockCount(), 0);
    }
  }

  TEST_CASE(
      "mem::FixedFreeListAllocator::operator=(FixedFreeListAllocator&&)") {
    SUBCASE("Target acquires source allocation state") {
      FixedFreeListAllocator source(kCapacity);
      [[maybe_unused]] const void* _ = source.allocate(32, kAlign);
      FixedFreeListAllocator target(kCapacity);
      target = std::move(source);
      CHECK_FALSE(target.Empty());
      CHECK_EQ(target.Stats().allocation_count, 1);
    }

    SUBCASE("Source is empty after move assign") {
      FixedFreeListAllocator source(kCapacity);
      [[maybe_unused]] const void* _ = source.allocate(32, kAlign);
      FixedFreeListAllocator target(kCapacity);
      target = std::move(source);
      CHECK(source.Empty());
    }

    SUBCASE("Self move assignment does not corrupt state") {
      FixedFreeListAllocator alloc(kCapacity);
      [[maybe_unused]] const void* _ = alloc.allocate(16, kAlign);
      FixedFreeListAllocator& ref = alloc;
      ref = std::move(alloc);  // NOLINT(clang-diagnostic-self-move)
      CHECK_FALSE(ref.Empty());
    }

    SUBCASE("Target releases prior backing storage on move assign") {
      FixedFreeListAllocator source(kCapacity);
      void* const source_ptr = source.allocate(64, kAlign);
      FixedFreeListAllocator target(kCapacity);
      void* const target_ptr = target.allocate(64, kAlign);
      target = std::move(source);
      CHECK(target.Owns(source_ptr));
      CHECK_FALSE(target.Owns(target_ptr));
    }
  }

  TEST_CASE("mem::FixedFreeListAllocator::Reset") {
    SUBCASE("Empty returns true after Reset") {
      FixedFreeListAllocator alloc(kCapacity);
      [[maybe_unused]] const void* _ = alloc.allocate(32, kAlign);
      alloc.Reset();
      CHECK(alloc.Empty());
    }

    SUBCASE("FreeBlockCount is 1 after Reset") {
      FixedFreeListAllocator alloc(kCapacity);
      void* const first = alloc.allocate(96, kAlign);
      void* const second = alloc.allocate(128, kAlign);
      alloc.deallocate(first, 96, kAlign);
      alloc.deallocate(second, 128, kAlign);
      alloc.Reset();
      CHECK_EQ(alloc.FreeBlockCount(), 1);
    }

    SUBCASE("Stats counters are zeroed after Reset") {
      FixedFreeListAllocator alloc(kCapacity);
      [[maybe_unused]] const void* _ = alloc.allocate(32, kAlign);
      alloc.Reset();
      const AllocatorStats stats = alloc.Stats();
      CHECK_EQ(stats.total_allocated, 0);
      CHECK_EQ(stats.peak_usage, 0);
      CHECK_EQ(stats.allocation_count, 0);
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
      CHECK_EQ(stats.alignment_waste, 0);
    }

    SUBCASE("TotalCapacity remains unchanged after Reset") {
      FixedFreeListAllocator alloc(kCapacity);
      [[maybe_unused]] const void* _ = alloc.allocate(32, kAlign);
      alloc.Reset();
      CHECK_EQ(alloc.TotalCapacity(), kCapacity);
    }

    SUBCASE("Allocation succeeds after Reset") {
      FixedFreeListAllocator alloc(kCapacity);
      [[maybe_unused]] const void* _ = alloc.allocate(64, kAlign);
      alloc.Reset();
      void* const ptr = alloc.allocate(32, kAlign);
      CHECK_NE(ptr, nullptr);
    }
  }

  TEST_CASE("mem::FixedFreeListAllocator::Empty") {
    SUBCASE("Returns true on fresh allocator") {
      const FixedFreeListAllocator alloc(kCapacity);
      CHECK(alloc.Empty());
    }

    SUBCASE("Returns false after allocation") {
      FixedFreeListAllocator alloc(kCapacity);
      [[maybe_unused]] const void* _ = alloc.allocate(16, kAlign);
      CHECK_FALSE(alloc.Empty());
    }

    SUBCASE("Returns true after all allocations are freed") {
      FixedFreeListAllocator alloc(kCapacity);
      void* const first = alloc.allocate(32, kAlign);
      void* const second = alloc.allocate(32, kAlign);
      alloc.deallocate(first, 32, kAlign);
      alloc.deallocate(second, 32, kAlign);
      CHECK(alloc.Empty());
    }

    SUBCASE("Returns true after Reset") {
      FixedFreeListAllocator alloc(kCapacity);
      [[maybe_unused]] const void* _ = alloc.allocate(16, kAlign);
      alloc.Reset();
      CHECK(alloc.Empty());
    }
  }

  TEST_CASE("mem::FixedFreeListAllocator::Owns") {
    SUBCASE("Returns true for pointer from this allocator") {
      FixedFreeListAllocator alloc(kCapacity);
      void* const ptr = alloc.allocate(64, kAlign);
      CHECK(alloc.Owns(ptr));
    }

    SUBCASE("Returns false for nullptr") {
      const FixedFreeListAllocator alloc(kCapacity);
      CHECK_FALSE(alloc.Owns(nullptr));
    }

    SUBCASE("Returns false for stack variable pointer") {
      const FixedFreeListAllocator alloc(kCapacity);
      int local = 0;
      CHECK_FALSE(alloc.Owns(&local));
    }

    SUBCASE("Returns false for pointer from a different allocator") {
      FixedFreeListAllocator first(kCapacity);
      FixedFreeListAllocator second(kCapacity);
      void* const ptr = second.allocate(64, kAlign);
      CHECK_FALSE(first.Owns(ptr));
    }

    SUBCASE("Returns true for pointer after deallocation") {
      FixedFreeListAllocator alloc(kCapacity);
      void* const ptr = alloc.allocate(64, kAlign);
      alloc.deallocate(ptr, 64, kAlign);
      CHECK(alloc.Owns(ptr));
    }
  }

  TEST_CASE("mem::FixedFreeListAllocator::Stats") {
    SUBCASE("All fields are zero on fresh allocator") {
      const FixedFreeListAllocator alloc(kCapacity);
      const AllocatorStats stats = alloc.Stats();
      CHECK_EQ(stats.total_allocated, 0);
      CHECK_EQ(stats.peak_usage, 0);
      CHECK_EQ(stats.allocation_count, 0);
      CHECK_EQ(stats.total_allocations, 0);
      CHECK_EQ(stats.total_deallocations, 0);
      CHECK_EQ(stats.alignment_waste, 0);
    }

    SUBCASE("allocation_count tracks live allocations") {
      FixedFreeListAllocator alloc(kCapacity);
      void* const first = alloc.allocate(32, kAlign);
      [[maybe_unused]] const void* _ = alloc.allocate(32, kAlign);
      CHECK_EQ(alloc.Stats().allocation_count, 2);
      alloc.deallocate(first, 32, kAlign);
      CHECK_EQ(alloc.Stats().allocation_count, 1);
    }

    SUBCASE("total_allocations counts every allocate call") {
      FixedFreeListAllocator alloc(kCapacity);
      const void* ptr = alloc.allocate(16, kAlign);
      ptr = alloc.allocate(16, kAlign);
      ptr = alloc.allocate(16, kAlign);
      CHECK_EQ(alloc.Stats().total_allocations, 3);
    }

    SUBCASE("total_deallocations counts every deallocate call") {
      FixedFreeListAllocator alloc(kCapacity);
      void* const first = alloc.allocate(32, kAlign);
      void* const second = alloc.allocate(32, kAlign);
      alloc.deallocate(first, 32, kAlign);
      alloc.deallocate(second, 32, kAlign);
      CHECK_EQ(alloc.Stats().total_deallocations, 2);
    }

    SUBCASE("total_allocated reflects used memory while blocks are live") {
      FixedFreeListAllocator alloc(kCapacity);
      [[maybe_unused]] const void* _ = alloc.allocate(96, 32);
      const size_t used_after_first = alloc.Stats().total_allocated;
      CHECK_GT(used_after_first, 0);
      [[maybe_unused]] const void* _2 = alloc.allocate(128, 64);
      CHECK_GT(alloc.Stats().total_allocated, used_after_first);
    }

    SUBCASE("total_allocated returns to zero after all blocks freed") {
      FixedFreeListAllocator alloc(kCapacity);
      void* const first = alloc.allocate(96, 32);
      void* const second = alloc.allocate(128, 64);
      alloc.deallocate(second, 128, 64);
      alloc.deallocate(first, 96, 32);
      CHECK_EQ(alloc.Stats().total_allocated, 0);
    }

    SUBCASE("peak_usage is at least high watermark of used memory") {
      FixedFreeListAllocator alloc(kCapacity);
      [[maybe_unused]] const void* _ = alloc.allocate(256, kAlign);
      [[maybe_unused]] const void* _2 = alloc.allocate(128, kAlign);
      CHECK_GE(alloc.Stats().peak_usage, alloc.Stats().total_allocated);
    }

    SUBCASE("alignment_waste is non-negative") {
      FixedFreeListAllocator alloc(kCapacity);
      [[maybe_unused]] const void* _ = alloc.allocate(1, 64);
      CHECK_GE(alloc.Stats().alignment_waste, 0);
    }
  }

  TEST_CASE("mem::FixedFreeListAllocator::InitialCapacity") {
    SUBCASE("Returns configured capacity template argument") {
      const FixedFreeListAllocator alloc(2048);
      CHECK_EQ(alloc.InitialCapacity(), 2048);
    }

    SUBCASE("Unchanged by allocations") {
      FixedFreeListAllocator alloc(kCapacity);
      [[maybe_unused]] const void* _ = alloc.allocate(512, kAlign);
      CHECK_EQ(alloc.InitialCapacity(), kCapacity);
    }

    SUBCASE("Unchanged after Reset") {
      FixedFreeListAllocator alloc(kCapacity);
      [[maybe_unused]] const void* _ = alloc.allocate(512, kAlign);
      alloc.Reset();
      CHECK_EQ(alloc.InitialCapacity(), kCapacity);
    }
  }

  TEST_CASE("mem::FixedFreeListAllocator::TotalCapacity") {
    SUBCASE("Equals configured capacity immediately after construction") {
      const FixedFreeListAllocator alloc(kCapacity);
      CHECK_EQ(alloc.TotalCapacity(), kCapacity);
    }

    SUBCASE("Unchanged by allocations") {
      FixedFreeListAllocator alloc(kCapacity);
      [[maybe_unused]] const void* _ = alloc.allocate(512, kAlign);
      CHECK_EQ(alloc.TotalCapacity(), kCapacity);
    }

    SUBCASE("Unchanged after Reset") {
      FixedFreeListAllocator alloc(kCapacity);
      [[maybe_unused]] const void* _ = alloc.allocate(512, kAlign);
      alloc.Reset();
      CHECK_EQ(alloc.TotalCapacity(), kCapacity);
    }
  }

  TEST_CASE("mem::FixedFreeListAllocator::FreeBlockCount") {
    SUBCASE("Equals 1 on fresh allocator") {
      const FixedFreeListAllocator alloc(kCapacity);
      CHECK_EQ(alloc.FreeBlockCount(), 1);
    }

    SUBCASE("Increases when allocation splits the sole free block") {
      FixedFreeListAllocator alloc(kCapacity);
      [[maybe_unused]] const void* _ = alloc.allocate(64, kAlign);
      CHECK_GE(alloc.FreeBlockCount(), 1);
    }

    SUBCASE("Coalesces to 1 after all allocations are freed") {
      FixedFreeListAllocator alloc(kCapacity);
      void* const first = alloc.allocate(96, 32);
      void* const second = alloc.allocate(128, 64);
      alloc.deallocate(second, 128, 64);
      alloc.deallocate(first, 96, 32);
      CHECK_EQ(alloc.FreeBlockCount(), 1);
    }

    SUBCASE("Coalesces adjacent frees in reverse order") {
      FixedFreeListAllocator alloc(kCapacity);
      void* const first = alloc.allocate(96, 32);
      void* const second = alloc.allocate(128, 64);
      alloc.deallocate(first, 96, 32);
      CHECK_GE(alloc.FreeBlockCount(), 1);
      alloc.deallocate(second, 128, 64);
      CHECK_EQ(alloc.FreeBlockCount(), 1);
    }

    SUBCASE("Equals 1 after Reset") {
      FixedFreeListAllocator alloc(kCapacity);
      void* const first = alloc.allocate(96, 32);
      void* const second = alloc.allocate(128, 64);
      alloc.deallocate(first, 96, 32);
      alloc.deallocate(second, 128, 64);
      alloc.Reset();
      CHECK_EQ(alloc.FreeBlockCount(), 1);
    }
  }

  TEST_CASE("mem::FixedFreeListAllocator::allocate") {
    SUBCASE("Returns non-null pointer") {
      FixedFreeListAllocator alloc(kCapacity);
      CHECK_NE(alloc.allocate(32, kAlign), nullptr);
    }

    SUBCASE("Multiple allocations return distinct pointers") {
      FixedFreeListAllocator alloc(kCapacity);
      void* const first = alloc.allocate(64, kAlign);
      void* const second = alloc.allocate(64, kAlign);
      CHECK_NE(first, second);
    }

    SUBCASE("Returned pointer satisfies requested alignment") {
      FixedFreeListAllocator alloc(kCapacity);
      for (const size_t al : {1UZ, 4UZ, 8UZ, 16UZ, 32UZ, 64UZ}) {
        void* const ptr = alloc.allocate(4, al);
        CHECK_EQ(reinterpret_cast<uintptr_t>(ptr) % al, 0);
      }
    }

    SUBCASE("Allocates until capacity is nearly exhausted") {
      constexpr size_t kChunk = 64;
      constexpr size_t kChunkAlign = 32;
      FixedFreeListAllocator alloc(kSmallCapacity);
      std::vector<void*> live;
      live.reserve(kSmallCapacity / kChunk);
      while (alloc.Stats().total_allocated + kChunk + kChunkAlign <=
             kSmallCapacity) {
        void* const ptr = alloc.allocate(kChunk, kChunkAlign);
        CHECK_NE(ptr, nullptr);
        live.push_back(ptr);
      }
      CHECK_GT(live.size(), 1);
      CHECK_LT(alloc.Stats().total_allocated, kSmallCapacity);
      for (void* const ptr : live) {
        alloc.deallocate(ptr, kChunk, kChunkAlign);
      }
      CHECK(alloc.Empty());
    }

    SUBCASE("Usable as PMR upstream for std::pmr::vector") {
      FixedFreeListAllocator alloc(kCapacity);
      std::pmr::vector<int> vec(&alloc);
      vec.push_back(1);
      vec.push_back(2);
      vec.push_back(3);
      CHECK_EQ(vec.size(), 3);
      CHECK_EQ(vec[0], 1);
      CHECK_EQ(vec[2], 3);
    }
  }

  TEST_CASE("mem::FixedFreeListAllocator::deallocate") {
    SUBCASE("Freed memory becomes available for reallocation") {
      FixedFreeListAllocator alloc(kCapacity);
      void* const first = alloc.allocate(64, kAlign);
      alloc.deallocate(first, 64, kAlign);
      void* const second = alloc.allocate(64, kAlign);
      CHECK_NE(second, nullptr);
      CHECK(alloc.Owns(second));
    }

    SUBCASE("Decrements allocation_count") {
      FixedFreeListAllocator alloc(kCapacity);
      void* const ptr = alloc.allocate(64, kAlign);
      alloc.deallocate(ptr, 64, kAlign);
      CHECK_EQ(alloc.Stats().allocation_count, 0);
    }

    SUBCASE("Reduces total_allocated") {
      FixedFreeListAllocator alloc(kCapacity);
      void* const ptr = alloc.allocate(64, kAlign);
      const size_t used_before = alloc.Stats().total_allocated;
      alloc.deallocate(ptr, 64, kAlign);
      CHECK_LT(alloc.Stats().total_allocated, used_before);
    }

    SUBCASE("Increments total_deallocations") {
      FixedFreeListAllocator alloc(kCapacity);
      void* const ptr = alloc.allocate(64, kAlign);
      alloc.deallocate(ptr, 64, kAlign);
      CHECK_EQ(alloc.Stats().total_deallocations, 1);
    }

    SUBCASE("Coalescing restores a single free block") {
      FixedFreeListAllocator alloc(kCapacity);
      void* const first = alloc.allocate(96, 32);
      void* const second = alloc.allocate(128, 64);
      alloc.deallocate(second, 128, 64);
      alloc.deallocate(first, 96, 32);
      CHECK(alloc.Empty());
      CHECK_EQ(alloc.FreeBlockCount(), 1);
      CHECK_EQ(alloc.Stats().total_allocated, 0);
    }
  }
}  // TEST_SUITE
