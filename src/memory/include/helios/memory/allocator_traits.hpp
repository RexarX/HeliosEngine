#pragma once

#include <helios/memory/common.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory_resource>
#include <span>
#include <type_traits>

namespace helios::mem {

/**
 * @brief Concept for PMR resources used by the memory module.
 * @tparam T Candidate type
 */
template <typename T>
concept PmrAllocator = std::derived_from<T, std::pmr::memory_resource>;

/**
 * @brief Concept for PMR resources that expose allocator statistics.
 * @tparam T Candidate type
 */
template <typename T>
concept PmrAllocatorWithStats =
    PmrAllocator<T> && requires(const T& allocator) {
      { allocator.Stats() } -> std::same_as<AllocatorStats>;
    };

/**
 * @brief Concept for PMR resources that support reset.
 * @tparam T Candidate type
 */
template <typename T>
concept ResettablePmrAllocator = PmrAllocator<T> && requires(T& allocator) {
  { allocator.Reset() } -> std::same_as<void>;
};

/**
 * @brief Tries to allocate one object from a PMR allocator.
 * @tparam T Object type
 * @tparam Alloc Allocator type
 * @param allocator Allocator reference
 * @return Pointer to allocated object memory, or `MemoryError`
 */
template <typename T, PmrAllocator Alloc>
[[nodiscard]] inline auto TryAllocate(Alloc& allocator) noexcept
    -> std::expected<T*, MemoryError> {
  if constexpr (sizeof(T) == 0) {
    return std::unexpected(MemoryError::kInvalidSize);
  }

  constexpr size_t kAlignment = std::max(alignof(T), kMinAlignment);
  if (!IsPowerOfTwo(kAlignment)) [[unlikely]] {
    return std::unexpected(MemoryError::kInvalidAlignment);
  }

  void* const ptr = allocator.allocate(sizeof(T), kAlignment);
  if (ptr == nullptr) [[unlikely]] {
    return std::unexpected(MemoryError::kOutOfMemory);
  }

  return static_cast<T*>(ptr);
}

/**
 * @brief Tries to allocate an array of objects from a PMR allocator.
 * @tparam T Element type
 * @tparam Alloc Allocator type
 * @param allocator Allocator reference
 * @param count Number of elements
 * @return Span over allocated memory, or `MemoryError`
 */
template <typename T, PmrAllocator Alloc>
[[nodiscard]] inline auto TryAllocateSpan(Alloc& allocator,
                                          size_t count) noexcept
    -> std::expected<std::span<T>, MemoryError> {
  if (count == 0) [[unlikely]] {
    return std::span<T>{};
  }

  if (count > (std::numeric_limits<size_t>::max() / sizeof(T))) [[unlikely]] {
    return std::unexpected(MemoryError::kInvalidSize);
  }

  constexpr size_t kAlignment = std::max(alignof(T), kMinAlignment);
  if (!IsPowerOfTwo(kAlignment)) [[unlikely]] {
    return std::unexpected(MemoryError::kInvalidAlignment);
  }

  void* const ptr = allocator.allocate(sizeof(T) * count, kAlignment);
  if (ptr == nullptr) [[unlikely]] {
    return std::unexpected(MemoryError::kOutOfMemory);
  }
  return std::span<T>(static_cast<T*>(ptr), count);
}

/**
 * @brief Deallocates an object allocated by `TryAllocate`.
 * @tparam T Object type
 * @tparam Alloc Allocator type
 * @param allocator Allocator reference
 * @param ptr Pointer returned by `TryAllocate`
 */
template <typename T, PmrAllocator Alloc>
inline void Deallocate(Alloc& allocator, T* ptr) noexcept {
  if (ptr == nullptr) [[unlikely]] {
    return;
  }

  constexpr size_t kAlignment = std::max(alignof(T), kMinAlignment);
  allocator.deallocate(ptr, sizeof(T), kAlignment);
}

/**
 * @brief Deallocates an array allocated by `TryAllocateSpan`.
 * @tparam T Element type
 * @tparam Alloc Allocator type
 * @param allocator Allocator reference
 * @param span Span returned by `TryAllocateSpan`
 */
template <typename T, PmrAllocator Alloc>
inline void Deallocate(Alloc& allocator, std::span<T> span) noexcept {
  if (span.empty()) [[unlikely]] {
    return;
  }

  constexpr size_t kAlignment = std::max(alignof(T), kMinAlignment);
  allocator.deallocate(span.data(), sizeof(T) * span.size(), kAlignment);
}

/**
 * @brief Adapts stats-aware PMR resources.
 * @tparam Alloc Allocator type
 * @param allocator Allocator reference
 * @return Stats if available, otherwise zero-initialized stats
 */
template <PmrAllocator Alloc>
[[nodiscard]] constexpr AllocatorStats Stats(const Alloc& allocator) noexcept {
  if constexpr (PmrAllocatorWithStats<Alloc>) {
    return allocator.Stats();
  }
  return {};
}

}  // namespace helios::mem
