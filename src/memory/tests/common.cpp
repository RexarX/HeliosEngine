#include <doctest/doctest.h>

#include <helios/memory/common.hpp>

#include <array>
#include <cstddef>
#include <limits>
#include <ostream>

using namespace helios::mem;

TEST_SUITE("helios::mem::CommonConstants") {
  TEST_CASE("mem::CommonConstants::kDefaultAlignment") {
    CHECK_GT(kDefaultAlignment, 0);
    CHECK(IsPowerOfTwo(kDefaultAlignment));
  }

  TEST_CASE("mem::CommonConstants::kMinAlignment") {
    CHECK_EQ(kMinAlignment, alignof(std::max_align_t));
    CHECK(IsPowerOfTwo(kMinAlignment));
  }
}

TEST_SUITE("helios::mem::GrowthMode") {
  TEST_CASE("mem::GrowthMode::enumerators are distinct") {
    CHECK_NE(static_cast<int>(GrowthMode::kLinear),
             static_cast<int>(GrowthMode::kGeometric));
  }
}

TEST_SUITE("helios::mem::GrowthPolicy") {
  TEST_CASE("mem::GrowthPolicy::Linear") {
    const auto policy = GrowthPolicy::Linear(64, 4096);

    CHECK_EQ(policy.mode, GrowthMode::kLinear);
    CHECK_EQ(policy.linear_step, 64);
    CHECK_EQ(policy.max_capacity, 4096);
    CHECK_EQ(policy.geometric_numerator, 2);
    CHECK_EQ(policy.geometric_denominator, 1);
  }

  TEST_CASE("mem::GrowthPolicy::Geometric") {
    const GrowthPolicy policy = GrowthPolicy::Geometric(3, 2, 8192);

    CHECK_EQ(policy.mode, GrowthMode::kGeometric);
    CHECK_EQ(policy.geometric_numerator, 3);
    CHECK_EQ(policy.geometric_denominator, 2);
    CHECK_EQ(policy.max_capacity, 8192);
    CHECK_EQ(policy.linear_step, 0);
  }

  TEST_CASE("mem::GrowthPolicy::NextCapacity") {
    SUBCASE("Returns current when already sufficient") {
      const auto policy = GrowthPolicy::Geometric();
      CHECK_EQ(policy.NextCapacity(256, 128), 256);
    }

    SUBCASE("Linear grows by configured step") {
      const GrowthPolicy policy = GrowthPolicy::Linear(64, 1024);
      CHECK_EQ(policy.NextCapacity(128, 129), 192);
      CHECK_EQ(policy.NextCapacity(128, 256), 256);
      CHECK_EQ(policy.NextCapacity(128, 257), 320);
    }

    SUBCASE("Linear with zero step cannot grow") {
      GrowthPolicy policy = GrowthPolicy::Linear(1, 1024);
      policy.linear_step = 0;
      CHECK_EQ(policy.NextCapacity(128, 256), 0);
    }

    SUBCASE("Geometric grows by ratio") {
      const GrowthPolicy policy = GrowthPolicy::Geometric(2, 1, 4096);
      CHECK_EQ(policy.NextCapacity(128, 129), 256);
      CHECK_EQ(policy.NextCapacity(128, 300), 512);
    }

    SUBCASE("Geometric with invalid denominator cannot grow") {
      GrowthPolicy policy = GrowthPolicy::Geometric(2, 1, 1024);
      policy.geometric_denominator = 0;
      CHECK_EQ(policy.NextCapacity(128, 256), 0);
    }

    SUBCASE("Geometric with ratio below one cannot grow") {
      GrowthPolicy policy = GrowthPolicy::Geometric(2, 1, 1024);
      policy.geometric_numerator = 1;
      policy.geometric_denominator = 2;
      CHECK_EQ(policy.NextCapacity(128, 256), 0);
    }

    SUBCASE("Result is clamped to max_capacity") {
      const GrowthPolicy policy = GrowthPolicy::Linear(512, 300);
      CHECK_EQ(policy.NextCapacity(128, 129), 300);
    }
  }
}

TEST_SUITE("helios::mem::AllocatorStats") {
  TEST_CASE("mem::AllocatorStats::default initializes to zero") {
    const AllocatorStats stats{};
    CHECK_EQ(stats.total_allocated, 0);
    CHECK_EQ(stats.peak_usage, 0);
    CHECK_EQ(stats.allocation_count, 0);
    CHECK_EQ(stats.total_allocations, 0);
    CHECK_EQ(stats.total_deallocations, 0);
    CHECK_EQ(stats.alignment_waste, 0);
  }
}

TEST_SUITE("helios::mem::MemoryErrorToString") {
  TEST_CASE("mem::MemoryErrorToString::maps each error to a message") {
    CHECK(MemoryErrorToString(MemoryError::kOutOfMemory) == "Out of memory");
    CHECK(MemoryErrorToString(MemoryError::kInvalidAlignment) ==
          "Invalid alignment");
    CHECK(MemoryErrorToString(MemoryError::kInvalidSize) == "Invalid size");
    CHECK(MemoryErrorToString(MemoryError::kGrowthDisabled) ==
          "Growth is disabled");
    CHECK(MemoryErrorToString(MemoryError::kOwnershipMismatch) ==
          "Pointer is not owned by allocator");
  }
}

TEST_SUITE("helios::mem::IsPowerOfTwo") {
  TEST_CASE("mem::IsPowerOfTwo::detects powers of two correctly") {
    CHECK_FALSE(IsPowerOfTwo(0));
    CHECK(IsPowerOfTwo(1));
    CHECK(IsPowerOfTwo(2));
    CHECK_FALSE(IsPowerOfTwo(3));
    CHECK(IsPowerOfTwo(64));
    CHECK_FALSE(IsPowerOfTwo(65));
  }
}

TEST_SUITE("helios::mem::SaturatingAdd") {
  TEST_CASE("mem::SaturatingAdd::adds without overflow") {
    CHECK_EQ(SaturatingAdd(10, 20), 30);
    CHECK_EQ(SaturatingAdd(0, 42), 42);
  }

  TEST_CASE("mem::SaturatingAdd::clamps on overflow") {
    constexpr size_t kMax = std::numeric_limits<size_t>::max();
    CHECK_EQ(SaturatingAdd(kMax, 1), kMax);
    CHECK_EQ(SaturatingAdd(kMax - 5, 10), kMax);
  }
}

TEST_SUITE("helios::mem::SaturatingMul") {
  TEST_CASE("mem::SaturatingMul::multiplies without overflow") {
    CHECK_EQ(SaturatingMul(3, 7), 21);
    CHECK_EQ(SaturatingMul(0, 99), 0);
  }

  TEST_CASE("mem::SaturatingMul::clamps on overflow") {
    constexpr size_t kMax = std::numeric_limits<size_t>::max();
    CHECK_EQ(SaturatingMul(kMax, 2), kMax);
    CHECK_EQ(SaturatingMul((kMax / 2) + 1, 3), kMax);
  }
}

TEST_SUITE("helios::mem::AlignUp") {
  TEST_CASE("mem::AlignUp::returns aligned value") {
    CHECK_EQ(AlignUp(0, 8), 0);
    CHECK_EQ(AlignUp(1, 8), 8);
    CHECK_EQ(AlignUp(8, 8), 8);
    CHECK_EQ(AlignUp(9, 8), 16);
  }

  TEST_CASE("mem::AlignUp::saturates on overflow") {
    constexpr size_t kMax = std::numeric_limits<size_t>::max();
    CHECK_EQ(AlignUp(kMax - 1, 8), kMax);
  }
}

TEST_SUITE("helios::mem::AlignUpPtr") {
  TEST_CASE("mem::AlignUpPtr::returns aligned pointer") {
    std::array<std::byte, 128> buffer{};

    void* const raw = buffer.data() + 1;
    void* const aligned = AlignUpPtr(raw, 16);

    CHECK(IsAligned(aligned, 16));
    CHECK_GE(reinterpret_cast<uintptr_t>(aligned),
             reinterpret_cast<uintptr_t>(raw));
  }
}

TEST_SUITE("helios::mem::IsAligned") {
  TEST_CASE("mem::IsAligned::detects pointer alignment") {
    std::array<std::byte, 128> buffer{};
    void* const aligned = AlignUpPtr(buffer.data(), 32);
    CHECK(IsAligned(aligned, 32));

    void* const unaligned = static_cast<void*>(buffer.data() + 1);
    CHECK_FALSE(IsAligned(unaligned, 32));
  }
}

TEST_SUITE("helios::mem::CalculatePadding") {
  TEST_CASE("mem::CalculatePadding::returns required bytes") {
    std::array<std::byte, 128> buffer{};
    const void* aligned = AlignUpPtr(buffer.data(), 16);
    CHECK_EQ(CalculatePadding(aligned, 16), 0);

    const void* misaligned = static_cast<const void*>(buffer.data() + 3);
    CHECK_EQ(CalculatePadding(misaligned, 8), 5);
  }
}

TEST_SUITE("helios::mem::CalculatePaddingWithHeader") {
  TEST_CASE(
      "mem::CalculatePaddingWithHeader::accounts for alignment and "
      "header") {
    std::array<std::byte, 128> buffer{};

    const void* ptr1 = static_cast<const void*>(buffer.data() + 1);
    CHECK_EQ(CalculatePaddingWithHeader(ptr1, 8, 4), 7);

    const void* ptr2 = static_cast<const void*>(buffer.data());
    CHECK_EQ(CalculatePaddingWithHeader(ptr2, 8, 9), 16);
  }
}
