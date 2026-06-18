#include <doctest/doctest.h>

#include <helios/memory/aligned_alloc.hpp>
#include <helios/memory/allocator_traits.hpp>

#include <limits>
#include <memory_resource>
#include <type_traits>

namespace {

using namespace helios::mem;

class StatsResource final : public std::pmr::memory_resource {
public:
  [[nodiscard]] AllocatorStats Stats() const noexcept { return stats_; }

protected:
  void* do_allocate(size_t bytes, size_t alignment) override {
    void* const ptr = AlignedAlloc(alignment, bytes);
    if (ptr != nullptr) {
      stats_.total_allocated += bytes;
      stats_.allocation_count += 1;
      stats_.total_allocations += 1;
    }
    return ptr;
  }

  void do_deallocate(void* p, size_t bytes, size_t /*alignment*/) override {
    if (p == nullptr) {
      return;
    }
    AlignedFree(p);
    stats_.total_deallocations += 1;
    if (stats_.total_allocated >= bytes) {
      stats_.total_allocated -= bytes;
    } else {
      stats_.total_allocated = 0;
    }
    if (stats_.allocation_count > 0) {
      stats_.allocation_count -= 1;
    }
  }

  bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override {
    return this == &other;
  }

private:
  AllocatorStats stats_{};
};

class BareResource final : public std::pmr::memory_resource {
protected:
  void* do_allocate(size_t bytes, size_t alignment) override {
    return AlignedAlloc(alignment, bytes);
  }

  void do_deallocate(void* p, size_t /*bytes*/, size_t /*alignment*/) override {
    AlignedFree(p);
  }

  bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override {
    return this == &other;
  }
};

}  // namespace

TEST_SUITE("helios::mem::AllocatorTraitsConcepts") {
  TEST_CASE("mem::AllocatorTraitsConcepts::PmrAllocator") {
    static_assert(PmrAllocator<StatsResource>);
    static_assert(PmrAllocator<BareResource>);
    static_assert(!PmrAllocator<int>);
    CHECK(true);
  }

  TEST_CASE("mem::AllocatorTraitsConcepts::PmrAllocatorWithStats") {
    static_assert(PmrAllocatorWithStats<StatsResource>);
    static_assert(!PmrAllocatorWithStats<BareResource>);
    CHECK(true);
  }

  TEST_CASE("mem::AllocatorTraitsConcepts::ResettablePmrAllocator") {
    static_assert(!ResettablePmrAllocator<StatsResource>);
    static_assert(!ResettablePmrAllocator<BareResource>);
    CHECK(true);
  }
}

TEST_SUITE("helios::mem::TryAllocate") {
  TEST_CASE("mem::TryAllocate::returns typed pointer on success") {
    StatsResource allocator;
    auto result = TryAllocate<int>(allocator);
    REQUIRE(result.has_value());
    REQUIRE(result.value() != nullptr);
    *result.value() = 17;
    CHECK_EQ(*result.value(), 17);

    Deallocate(allocator, result.value());
  }
}

TEST_SUITE("helios::mem::TryAllocateSpan") {
  TEST_CASE("mem::TryAllocateSpan::returns empty span for zero count") {
    StatsResource allocator;
    auto result = TryAllocateSpan<int>(allocator, 0);
    REQUIRE(result.has_value());
    CHECK(result->empty());
  }

  TEST_CASE("mem::TryAllocateSpan::returns span on success") {
    StatsResource allocator;
    auto result = TryAllocateSpan<int>(allocator, 8);
    REQUIRE(result.has_value());
    CHECK_EQ(result->size(), 8);
    (*result)[3] = 42;
    CHECK_EQ((*result)[3], 42);

    Deallocate(allocator, *result);
  }

  TEST_CASE("mem::TryAllocateSpan::returns invalid_size on overflow") {
    StatsResource allocator;
    constexpr size_t kTooMany =
        (std::numeric_limits<size_t>::max() / sizeof(int)) + 1;
    auto result = TryAllocateSpan<int>(allocator, kTooMany);
    REQUIRE_FALSE(result.has_value());
    CHECK_EQ(result.error(), MemoryError::kInvalidSize);
  }
}

TEST_SUITE("helios::mem::DeallocateObject") {
  TEST_CASE("mem::DeallocateObject::accepts null pointer") {
    StatsResource allocator;
    Deallocate<int>(allocator, nullptr);
    CHECK_EQ(allocator.Stats().total_deallocations, 0);
  }
}

TEST_SUITE("helios::mem::DeallocateSpan") {
  TEST_CASE("mem::DeallocateSpan::accepts empty span") {
    StatsResource allocator;
    std::span<int> empty;
    Deallocate<int>(allocator, empty);
    CHECK_EQ(allocator.Stats().total_deallocations, 0);
  }
}

TEST_SUITE("helios::mem::StatsFunction") {
  TEST_CASE("mem::StatsFunction::returns allocator-provided stats") {
    StatsResource allocator;
    auto one = TryAllocate<int>(allocator);
    REQUIRE(one.has_value());

    const AllocatorStats stats = Stats(allocator);
    CHECK_EQ(stats.total_allocations, 1);
    CHECK_EQ(stats.allocation_count, 1);

    Deallocate(allocator, one.value());
  }

  TEST_CASE("mem::StatsFunction::returns zero stats for plain allocators") {
    BareResource allocator;
    const AllocatorStats stats = Stats(allocator);
    CHECK_EQ(stats.total_allocated, 0);
    CHECK_EQ(stats.peak_usage, 0);
    CHECK_EQ(stats.allocation_count, 0);
    CHECK_EQ(stats.total_allocations, 0);
    CHECK_EQ(stats.total_deallocations, 0);
    CHECK_EQ(stats.alignment_waste, 0);
  }
}
