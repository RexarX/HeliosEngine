#pragma once

#include <helios/assert.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

namespace helios::mem {

/// @brief Default alignment used by memory resources.
inline constexpr size_t kDefaultAlignment = 64;

/// @brief Minimum alignment used by memory resources.
inline constexpr size_t kMinAlignment = alignof(std::max_align_t);

[[nodiscard]] constexpr size_t SaturatingAdd(size_t lhs, size_t rhs) noexcept;
[[nodiscard]] constexpr size_t SaturatingMul(size_t lhs, size_t rhs) noexcept;

/// @brief Growth strategy used by growable allocators.
enum class GrowthMode : uint8_t {
  kFixed,
  kLinear,
  kGeometric,
};

/**
 * @brief Capacity growth configuration.
 * @details This policy is shared by all PMR allocators in the memory module.
 */
struct GrowthPolicy {
  GrowthMode mode = GrowthMode::kFixed;
  size_t linear_step = 0;
  size_t geometric_numerator = 2;
  size_t geometric_denominator = 1;
  size_t max_capacity = std::numeric_limits<size_t>::max();

  [[nodiscard]] static constexpr GrowthPolicy Fixed(
      size_t max = std::numeric_limits<size_t>::max()) noexcept {
    return {
        .mode = GrowthMode::kFixed,
        .linear_step = 0,
        .geometric_numerator = 2,
        .geometric_denominator = 1,
        .max_capacity = max,
    };
  }

  [[nodiscard]] static constexpr GrowthPolicy Linear(
      size_t step, size_t max = std::numeric_limits<size_t>::max()) noexcept {
    return {
        .mode = GrowthMode::kLinear,
        .linear_step = step,
        .geometric_numerator = 2,
        .geometric_denominator = 1,
        .max_capacity = max,
    };
  }

  [[nodiscard]] static constexpr GrowthPolicy Geometric(
      size_t numerator = 2, size_t denominator = 1,
      size_t max = std::numeric_limits<size_t>::max()) noexcept {
    return {
        .mode = GrowthMode::kGeometric,
        .linear_step = 0,
        .geometric_numerator = numerator,
        .geometric_denominator = denominator,
        .max_capacity = max,
    };
  }

  [[nodiscard]] constexpr bool Growable() const noexcept {
    return mode != GrowthMode::kFixed;
  }

  [[nodiscard]] constexpr size_t NextCapacity(
      size_t current_capacity, size_t min_capacity) const noexcept {
    if (current_capacity >= min_capacity) {
      return current_capacity;
    }

    if (mode == GrowthMode::kFixed) {
      return 0;
    }

    size_t next = current_capacity;
    if (next == 0) {
      next = kMinAlignment;
    }

    if (mode == GrowthMode::kLinear) {
      if (linear_step == 0) {
        return 0;
      }
      while (next < min_capacity) {
        const size_t previous = next;
        next = SaturatingAdd(next, linear_step);
        if (next <= previous) {
          next = std::numeric_limits<size_t>::max();
        }
        if (next == std::numeric_limits<size_t>::max()) {
          break;
        }
      }
    } else {
      if (geometric_denominator == 0 ||
          geometric_numerator < geometric_denominator) {
        return 0;
      }
      while (next < min_capacity) {
        const size_t previous = next;
        const size_t scaled = SaturatingMul(next, geometric_numerator);
        next = scaled / geometric_denominator;
        if (next <= previous) {
          next = std::numeric_limits<size_t>::max();
        }
        if (next == std::numeric_limits<size_t>::max()) {
          break;
        }
      }
    }

    return std::min(next, max_capacity);
  }
};

/// @brief Runtime statistics for PMR allocators.
struct AllocatorStats {
  size_t total_allocated = 0;
  size_t peak_usage = 0;
  size_t allocation_count = 0;
  size_t total_allocations = 0;
  size_t total_deallocations = 0;
  size_t alignment_waste = 0;
};

/// @brief Error codes used for explicit non-fatal memory operations.
enum class MemoryError : uint8_t {
  kOutOfMemory,
  kInvalidAlignment,
  kInvalidSize,
  kGrowthDisabled,
  kOwnershipMismatch,
};

/**
 * @brief Converts MemoryError to a readable string.
 * @param error Error value
 * @return String view with the error description
 */
[[nodiscard]] constexpr std::string_view MemoryErrorToString(
    MemoryError error) noexcept {
  switch (error) {
    case MemoryError::kOutOfMemory:
      return "out of memory";
    case MemoryError::kInvalidAlignment:
      return "invalid alignment";
    case MemoryError::kInvalidSize:
      return "invalid size";
    case MemoryError::kGrowthDisabled:
      return "growth is disabled";
    case MemoryError::kOwnershipMismatch:
      return "pointer is not owned by allocator";
    default:
      return "unknown memory error";
  }
}

/**
 * @brief Checks if a value is a power of two.
 * @param value Value to check
 * @return True if value is a power of two
 */
[[nodiscard]] constexpr bool IsPowerOfTwo(size_t value) noexcept {
  return value != 0 && (value & (value - 1)) == 0;
}

/**
 * @brief Saturating add for size_t.
 * @param lhs Left operand
 * @param rhs Right operand
 * @return lhs + rhs, clamped to `size_t` max on overflow
 */
[[nodiscard]] constexpr size_t SaturatingAdd(size_t lhs, size_t rhs) noexcept {
  constexpr size_t kMax = std::numeric_limits<size_t>::max();
  if (rhs > kMax - lhs) {
    return kMax;
  }
  return lhs + rhs;
}

/**
 * @brief Saturating multiply for size_t.
 * @param lhs Left operand
 * @param rhs Right operand
 * @return lhs * rhs, clamped to `size_t` max on overflow
 */
[[nodiscard]] constexpr size_t SaturatingMul(size_t lhs, size_t rhs) noexcept {
  constexpr size_t kMax = std::numeric_limits<size_t>::max();
  if (lhs == 0 || rhs == 0) {
    return 0;
  }
  if (lhs > kMax / rhs) {
    return kMax;
  }
  return lhs * rhs;
}

/**
 * @brief Aligns value up to alignment.
 * @param value Value to align
 * @param alignment Alignment, must be a power of two
 * @return Aligned value
 */
[[nodiscard]] constexpr size_t AlignUp(size_t value,
                                       size_t alignment) noexcept {
  HELIOS_ASSERT(IsPowerOfTwo(alignment), "alignment '{}' must be power of two!",
                alignment);
  const size_t mask = alignment - 1;
  if (value > std::numeric_limits<size_t>::max() - mask) {
    return std::numeric_limits<size_t>::max();
  }
  return (value + mask) & ~mask;
}

/**
 * @brief Aligns pointer up to alignment.
 * @param ptr Pointer to align
 * @param alignment Alignment, must be a power of two
 * @return Aligned pointer
 */
[[nodiscard]] inline void* AlignUpPtr(void* ptr, size_t alignment) noexcept {
  HELIOS_ASSERT(IsPowerOfTwo(alignment), "alignment '{}' must be power of two!",
                alignment);
  auto* const raw = static_cast<std::byte*>(ptr);
  const auto addr = reinterpret_cast<uintptr_t>(raw);
  const auto aligned = AlignUp(addr, alignment);
  return reinterpret_cast<void*>(aligned);
}

/**
 * @brief Checks if pointer is aligned.
 * @param ptr Pointer value
 * @param alignment Alignment to check
 * @return True if pointer is aligned
 */
[[nodiscard]] inline bool IsAligned(const void* ptr,
                                    size_t alignment) noexcept {
  HELIOS_ASSERT(IsPowerOfTwo(alignment), "alignment '{}' must be power of two",
                alignment);
  const auto addr = reinterpret_cast<uintptr_t>(ptr);
  return (addr & (alignment - 1)) == 0;
}

/**
 * @brief Computes padding required for pointer alignment.
 * @param ptr Current pointer
 * @param alignment Requested alignment
 * @return Padding in bytes
 */
[[nodiscard]] inline size_t CalculatePadding(const void* ptr,
                                             size_t alignment) noexcept {
  const auto addr = reinterpret_cast<uintptr_t>(ptr);
  const auto aligned = AlignUp(addr, alignment);
  return aligned - addr;
}

/**
 * @brief Computes padding for aligned storage with a prepended header.
 * @param ptr Current pointer
 * @param alignment Requested alignment
 * @param header_size Header size stored before aligned user memory
 * @return Padding in bytes, including header space
 */
[[nodiscard]] inline size_t CalculatePaddingWithHeader(
    const void* ptr, size_t alignment, size_t header_size) noexcept {
  size_t padding = CalculatePadding(ptr, alignment);
  if (padding < header_size) {
    padding = SaturatingAdd(padding, AlignUp(header_size - padding, alignment));
  }
  return padding;
}

}  // namespace helios::mem
