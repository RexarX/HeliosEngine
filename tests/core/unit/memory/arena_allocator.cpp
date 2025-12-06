#include <doctest/doctest.h>

#include <helios/core/memory/arena_allocator.hpp>
#include <helios/core/memory/common.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

using namespace helios::memory;

namespace {

[[nodiscard]] bool IsAlignedTo(const void* ptr, size_t alignment) {
  const auto addr = reinterpret_cast<uintptr_t>(ptr);
  return (addr % alignment) == 0;
}

}  // namespace

TEST_SUITE("memory::ArenaAllocator") {
  TEST_CASE("ArenaAllocator::ctor: construction and basic properties") {
    constexpr size_t kSize = 1024;
    alignas(kDefaultAlignment) uint8_t buffer[kSize] = {};

    ArenaAllocator arena(buffer, kSize);

    CHECK_EQ(arena.Capacity(), kSize);
    CHECK(arena.Empty());
    CHECK_FALSE(arena.Full());
    CHECK_EQ(arena.CurrentOffset(), 0);
    CHECK_EQ(arena.FreeSpace(), kSize);
    CHECK_EQ(arena.Data(), static_cast<void*>(buffer));

    const auto stats = arena.Stats();
    CHECK_EQ(stats.total_allocated, 0);
    CHECK_EQ(stats.total_freed, 0);
    CHECK_EQ(stats.peak_usage, 0);
    CHECK_EQ(stats.allocation_count, 0);
    CHECK_EQ(stats.total_allocations, 0);
    CHECK_EQ(stats.total_deallocations, 0);
    CHECK_EQ(stats.alignment_waste, 0);
  }

  TEST_CASE("ArenaAllocator::Allocate: zero-size returns null") {
    constexpr size_t kSize = 128;
    alignas(kDefaultAlignment) uint8_t buffer[kSize] = {};

    ArenaAllocator arena(buffer, kSize);

    const auto result = arena.Allocate(0);
    CHECK_FALSE(result.Valid());
    CHECK_EQ(result.ptr, nullptr);
    CHECK_EQ(result.allocated_size, 0);
    CHECK(arena.Empty());
  }

  TEST_CASE("ArenaAllocator::Allocate: simple allocations and alignment") {
    constexpr size_t kSize = 1024;
    alignas(kDefaultAlignment) uint8_t buffer[kSize] = {};

    ArenaAllocator arena(buffer, kSize);

    SUBCASE("Default alignment") {
      const size_t alloc_size = 64;
      const auto result = arena.Allocate(alloc_size);
      CHECK(result.Valid());
      CHECK_NE(result.ptr, nullptr);
      CHECK_EQ(result.allocated_size, alloc_size);
      CHECK(IsAlignedTo(result.ptr, kDefaultAlignment));

      CHECK_FALSE(arena.Empty());
      CHECK_GE(arena.CurrentOffset(), alloc_size);
      CHECK_LT(arena.CurrentOffset(), kSize + 1);
    }

    SUBCASE("Custom power-of-two alignments") {
      const size_t sizes[] = {8, 16, 32, 64};
      const size_t alignments[] = {kMinAlignment, 32, 64};

      for (auto size : sizes) {
        for (auto alignment : alignments) {
          const auto result = arena.Allocate(size, alignment);
          CHECK(result.Valid());
          CHECK_NE(result.ptr, nullptr);
          CHECK_EQ(result.allocated_size, size);
          CHECK(IsAlignedTo(result.ptr, alignment));
        }
      }
    }
  }

  TEST_CASE("ArenaAllocator::Allocate: out-of-space fails") {
    constexpr size_t kSize = 128;
    alignas(kDefaultAlignment) uint8_t buffer[kSize] = {};

    ArenaAllocator arena(buffer, kSize);

    const auto first = arena.Allocate(kSize);
    CHECK(first.Valid());
    CHECK_FALSE(arena.Empty());
    if (!arena.Full()) {
      CHECK_EQ(arena.FreeSpace(), 0);
    }

    const auto second = arena.Allocate(1);
    CHECK_FALSE(second.Valid());
    CHECK_EQ(second.ptr, nullptr);
    CHECK_EQ(second.allocated_size, 0);
  }

  TEST_CASE("ArenaAllocator::Reset: clears logical state but not data") {
    constexpr size_t kSize = 256;
    alignas(kDefaultAlignment) uint8_t buffer[kSize] = {};

    ArenaAllocator arena(buffer, kSize);

    const auto first = arena.Allocate(64);
    CHECK(first.Valid());
    auto* bytes = static_cast<uint8_t*>(first.ptr);
    bytes[0] = 0xAA;
    bytes[63] = 0xBB;

    CHECK_FALSE(arena.Empty());
    CHECK_GT(arena.CurrentOffset(), 0U);

    const auto stats_before = arena.Stats();
    CHECK_GT(stats_before.total_allocated, 0U);
    CHECK_EQ(stats_before.total_allocations, 1U);

    arena.Reset();

    CHECK(arena.Empty());
    CHECK_EQ(arena.CurrentOffset(), 0U);
    CHECK_EQ(arena.FreeSpace(), kSize);

    const auto stats_after = arena.Stats();
    CHECK_EQ(stats_after.total_allocated, 0U);
    CHECK_EQ(stats_after.total_freed, 0U);
    CHECK_EQ(stats_after.total_deallocations, 0U);

    CHECK_EQ(bytes[0], 0xAA);
    CHECK_EQ(bytes[63], 0xBB);
  }

  TEST_CASE("ArenaAllocator::Deallocate: is a no-op") {
    constexpr size_t kSize = 256;
    alignas(kDefaultAlignment) uint8_t buffer[kSize] = {};

    ArenaAllocator arena(buffer, kSize);

    const auto alloc1 = arena.Allocate(64);
    CHECK(alloc1.Valid());

    const auto offset_before = arena.CurrentOffset();
    arena.Deallocate(alloc1.ptr);
    const auto offset_after = arena.CurrentOffset();

    CHECK_EQ(offset_before, offset_after);
    CHECK_FALSE(arena.Empty());
  }

  TEST_CASE("ArenaAllocator::ctor: move construction transfers state") {
    constexpr size_t kSize = 512;
    alignas(kDefaultAlignment) uint8_t buffer[kSize] = {};

    ArenaAllocator arena1(buffer, kSize);

    const auto alloc1 = arena1.Allocate(64);
    const auto alloc2 = arena1.Allocate(64);
    CHECK(alloc1.Valid());
    CHECK(alloc2.Valid());

    const auto offset_before = arena1.CurrentOffset();
    const auto stats_before = arena1.Stats();

    ArenaAllocator arena2{std::move(arena1)};

    CHECK_EQ(arena1.Capacity(), 0U);
    CHECK_EQ(arena1.Data(), nullptr);
    CHECK(arena1.Empty());

    CHECK_EQ(arena2.Capacity(), kSize);
    CHECK_EQ(arena2.Data(), static_cast<void*>(buffer));
    CHECK_EQ(arena2.CurrentOffset(), offset_before);

    const auto stats_after = arena2.Stats();
    CHECK_EQ(stats_after.total_allocated, stats_before.total_allocated);
    CHECK_EQ(stats_after.total_allocations, stats_before.total_allocations);
  }

  TEST_CASE("ArenaAllocator::operator=: move assignment transfers state") {
    constexpr size_t kSize = 512;
    alignas(kDefaultAlignment) uint8_t buffer1[kSize]{};
    alignas(kDefaultAlignment) uint8_t buffer2[kSize]{};

    ArenaAllocator arena1{buffer1, kSize};
    ArenaAllocator arena2{buffer2, kSize};

    const auto a1 = arena1.Allocate(128);
    const auto a2 = arena1.Allocate(64);
    CHECK(a1.Valid());
    CHECK(a2.Valid());

    const auto offset_before = arena1.CurrentOffset();
    const auto stats_before = arena1.Stats();

    arena2 = std::move(arena1);

    CHECK_EQ(arena1.Capacity(), 0U);
    CHECK_EQ(arena1.Data(), nullptr);
    CHECK(arena1.Empty());

    CHECK_EQ(arena2.Data(), static_cast<void*>(buffer1));
    CHECK_EQ(arena2.Capacity(), kSize);
    CHECK_EQ(arena2.CurrentOffset(), offset_before);

    const auto stats_after = arena2.Stats();
    CHECK_EQ(stats_after.total_allocated, stats_before.total_allocated);
    CHECK_EQ(stats_after.total_allocations, stats_before.total_allocations);
  }

  TEST_CASE("ArenaAllocator::Allocate: alignment and padding accounting") {
    constexpr size_t kSize = 512;
    alignas(kDefaultAlignment) uint8_t buffer[kSize] = {};

    ArenaAllocator arena(buffer, kSize);

    const auto a1 = arena.Allocate(3, 16);
    CHECK(a1.Valid());
    CHECK(IsAlignedTo(a1.ptr, 16));

    const auto a2 = arena.Allocate(5, 64);
    CHECK(a2.Valid());
    CHECK(IsAlignedTo(a2.ptr, 64));

    const auto stats = arena.Stats();
    CHECK_EQ(stats.total_allocations, 2U);
    CHECK_GE(stats.alignment_waste, 0U);
    CHECK_GE(stats.total_allocated, 8U);
    CHECK_GE(stats.peak_usage, stats.total_allocated);
  }

  TEST_CASE("ArenaAllocator::Allocate: thread-safe concurrent allocations") {
    constexpr size_t kSize = 1u << 20;  // 1 MiB
    // Use heap allocation to avoid stack overflow
    auto* buffer = static_cast<uint8_t*>(AlignedAlloc(kDefaultAlignment, kSize));
    REQUIRE(buffer != nullptr);
    std::memset(buffer, 0, kSize);

    ArenaAllocator arena(buffer, kSize);

    constexpr size_t kThreadCount = 8;
    constexpr size_t kAllocCountPerThread = 1024;
    constexpr size_t kAllocSize = 32;

    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);

    std::atomic<size_t> success_counter{0};
    std::atomic<size_t> failure_counter{0};

    for (size_t t = 0; t < kThreadCount; ++t) {
      threads.emplace_back([&arena, &success_counter, &failure_counter, kAllocSize]() {
        for (size_t i = 0; i < kAllocCountPerThread; ++i) {
          const auto result = arena.Allocate(kAllocSize);
          if (result.Valid()) {
            CHECK_NE(result.ptr, nullptr);
            CHECK_EQ(result.allocated_size, kAllocSize);
            success_counter.fetch_add(1, std::memory_order_relaxed);
          } else {
            failure_counter.fetch_add(1, std::memory_order_relaxed);
          }
        }
      });
    }

    for (auto& thread : threads) {
      thread.join();
    }

    const auto total_requested = kThreadCount * kAllocCountPerThread * kAllocSize;
    CHECK_EQ(success_counter.load() + failure_counter.load(), kThreadCount * kAllocCountPerThread);

    CHECK_LE(success_counter.load() * kAllocSize, kSize);
    CHECK_GE(arena.CurrentOffset(), success_counter.load() * kAllocSize);

    CHECK_FALSE(arena.Empty());

    const auto stats = arena.Stats();
    CHECK_EQ(stats.total_allocations, success_counter.load());
    CHECK_GE(stats.total_allocated, success_counter.load() * kAllocSize);
    CHECK_GE(stats.peak_usage, stats.total_allocated);
    CHECK_LE(stats.total_allocated, total_requested + stats.alignment_waste);

    AlignedFree(buffer);
  }

  TEST_CASE("ArenaAllocator::Reset: after concurrent allocations") {
    constexpr size_t kSize = 1u << 18;  // 256 KiB
    // Use heap allocation to avoid potential stack overflow
    auto* buffer = static_cast<uint8_t*>(AlignedAlloc(kDefaultAlignment, kSize));
    REQUIRE(buffer != nullptr);
    std::memset(buffer, 0, kSize);

    ArenaAllocator arena(buffer, kSize);

    constexpr size_t kThreadCount = 4;
    constexpr size_t kAllocCountPerThread = 512;
    constexpr size_t kAllocSize = 64;

    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);

    for (size_t t = 0; t < kThreadCount; ++t) {
      threads.emplace_back([&arena]() {
        for (size_t i = 0; i < kAllocCountPerThread; ++i) {
          const auto result = arena.Allocate(kAllocSize);
        }
      });
    }

    for (auto& thread : threads) {
      thread.join();
    }

    CHECK_FALSE(arena.Empty());
    CHECK_GT(arena.CurrentOffset(), 0U);

    const auto stats_before = arena.Stats();
    CHECK_GT(stats_before.total_allocations, 0U);

    arena.Reset();

    CHECK(arena.Empty());
    CHECK_EQ(arena.CurrentOffset(), 0U);
    CHECK_EQ(arena.FreeSpace(), kSize);

    const auto stats_after = arena.Stats();
    CHECK_EQ(stats_after.total_allocated, 0U);
    CHECK_EQ(stats_after.total_allocations, 0U);

    AlignedFree(buffer);
  }
}
