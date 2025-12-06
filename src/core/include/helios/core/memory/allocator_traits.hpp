#pragma once

#include <helios/core_pch.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace helios::memory {

/**
 * @brief Statistics for tracking allocator usage.
 * @details Provides metrics about memory usage, allocation counts, and fragmentation.
 */
struct AllocatorStats {
  size_t total_allocated = 0;      ///< Total bytes currently allocated
  size_t total_freed = 0;          ///< Total bytes freed
  size_t peak_usage = 0;           ///< Peak memory usage
  size_t allocation_count = 0;     ///< Number of active allocations
  size_t total_allocations = 0;    ///< Total number of allocations made
  size_t total_deallocations = 0;  ///< Total number of deallocations made
  size_t alignment_waste = 0;      ///< Bytes wasted due to alignment
};

/**
 * @brief Result type for allocation operations.
 * @details Contains pointer and actual allocated size, or error code.
 */
struct AllocationResult {
  void* ptr = nullptr;        ///< Pointer to allocated memory, nullptr on failure
  size_t allocated_size = 0;  ///< Actual size allocated (may be larger than requested)

  /**
   * @brief Checks if the allocation was successful.
   * @return True if ptr is not nullptr, false otherwise
   */
  [[nodiscard]] constexpr bool Valid() const noexcept { return ptr != nullptr && allocated_size > 0; }
};

/**
 * @brief Concept for basic allocator interface.
 * @details Allocators must provide allocate and deallocate methods.
 * Deallocate signature may vary between allocators (e.g., frame allocators don't need parameters).
 * @tparam T Type to check for allocator requirements
 */
template <typename T>
concept Allocator = requires(T allocator, size_t size, size_t alignment) {
  { allocator.Allocate(size, alignment) } -> std::same_as<AllocationResult>;
  // Note: Deallocate signature varies by allocator type, so not checked here
};

/**
 * @brief Concept for allocators that can be reset/cleared.
 * @details Frame allocators and stack allocators typically support this operation.
 * @tparam T Type to check for resettable allocator requirements
 */
template <typename T>
concept ResettableAllocator = Allocator<T> && requires(T allocator) {
  { allocator.Reset() } -> std::same_as<void>;
};

/**
 * @brief Concept for allocators that provide statistics.
 * @details Most allocators should provide statistics for debugging and profiling.
 * @tparam T Type to check for stats support
 */
template <typename T>
concept AllocatorWithStats = Allocator<T> && requires(const T allocator) {
  { allocator.Stats() } -> std::same_as<AllocatorStats>;
};

/**
 * @brief Default alignment for allocations (cache line size for most modern CPUs).
 */
constexpr size_t kDefaultAlignment = 64;

/**
 * @brief Minimum alignment for any allocation.
 */
constexpr size_t kMinAlignment = alignof(std::max_align_t);

/**
 * @brief Helper function to align a size up to the given alignment.
 * @param size Size to align
 * @param alignment Alignment boundary (must be power of 2)
 * @return Aligned size
 */
[[nodiscard]] constexpr size_t AlignUp(size_t size, size_t alignment) noexcept {
  return (size + alignment - 1) & ~(alignment - 1);
}

/**
 * @brief Helper function to align a pointer up to the given alignment.
 * @param ptr Pointer to align
 * @param alignment Alignment boundary (must be power of 2)
 * @return Aligned pointer
 */
[[nodiscard]] inline void* AlignUpPtr(void* ptr, size_t alignment) noexcept {
  const auto addr = reinterpret_cast<uintptr_t>(ptr);
  const auto aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
  return reinterpret_cast<void*>(aligned_addr);
}

/**
 * @brief Helper function to check if a pointer is aligned.
 * @param ptr Pointer to check
 * @param alignment Alignment boundary
 * @return True if pointer is aligned
 */
[[nodiscard]] inline bool IsAligned(const void* ptr, size_t alignment) noexcept {
  return (reinterpret_cast<uintptr_t>(ptr) & (alignment - 1)) == 0;
}

/**
 * @brief Helper function to check if a size is a power of 2.
 * @param size Size to check
 * @return True if size is power of 2
 */
[[nodiscard]] constexpr bool IsPowerOfTwo(size_t size) noexcept {
  return size != 0 && (size & (size - 1)) == 0;
}

/**
 * @brief Calculate padding needed for alignment.
 * @param ptr Current pointer
 * @param alignment Desired alignment
 * @return Padding needed in bytes
 */
[[nodiscard]] inline size_t CalculatePadding(const void* ptr, size_t alignment) noexcept {
  const auto addr = reinterpret_cast<uintptr_t>(ptr);
  const auto aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
  return aligned_addr - addr;
}

/**
 * @brief Calculate padding with header for alignment.
 * @param ptr Current pointer
 * @param alignment Desired alignment
 * @param header_size Size of header to include
 * @return Padding needed in bytes
 */
[[nodiscard]] inline size_t CalculatePaddingWithHeader(const void* ptr, size_t alignment, size_t header_size) noexcept {
  const auto addr = reinterpret_cast<uintptr_t>(ptr);
  const auto aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
  size_t padding = aligned_addr - addr;

  // Check if we need more padding to fit the header
  if (padding < header_size) {
    header_size -= padding;
    // Align header_size to alignment
    if (header_size % alignment > 0) {
      padding += alignment * (1 + (header_size / alignment));
    } else {
      padding += alignment * (header_size / alignment);
    }
  }

  return padding;
}

/**
 * @brief Allocates memory for a single object of type T.
 * @details Convenience function that calculates size and alignment from the type.
 * The returned memory is uninitialized - use placement new to construct the object.
 * @tparam T Type to allocate memory for
 * @tparam Alloc Allocator type (deduced)
 * @param allocator Allocator to use for allocation
 * @return Pointer to allocated memory, or nullptr on failure
 *
 * @example
 * @code
 * FrameAllocator alloc(1024);
 * int* ptr = Allocate<int>(alloc);
 * if (ptr != nullptr) {
 *   new (ptr) int(42);  // Placement new
 * }
 * @endcode
 */
template <typename T, Allocator Alloc>
[[nodiscard]] constexpr T* Allocate(Alloc& allocator) noexcept {
  constexpr size_t size = sizeof(T);
  constexpr size_t alignment = std::max(alignof(T), kMinAlignment);
  auto result = allocator.Allocate(size, alignment);
  return static_cast<T*>(result.ptr);
}

/**
 * @brief Allocates memory for an array of objects of type T.
 * @details Convenience function that calculates size and alignment from the type.
 * The returned memory is uninitialized - use placement new to construct objects.
 * @tparam T Type to allocate memory for
 * @tparam Alloc Allocator type (deduced)
 * @param allocator Allocator to use for allocation
 * @param count Number of objects to allocate space for
 * @return Pointer to allocated memory, or nullptr on failure
 *
 * @example
 * @code
 * FrameAllocator alloc(1024);
 * int* arr = Allocate<int>(alloc, 10);
 * if (arr != nullptr) {
 *   for (size_t i = 0; i < 10; ++i) {
 *     new (&arr[i]) int(static_cast<int>(i));  // Placement new
 *   }
 * }
 * @endcode
 */
template <typename T, Allocator Alloc>
[[nodiscard]] constexpr T* Allocate(Alloc& allocator, size_t count) noexcept {
  if (count == 0) [[unlikely]] {
    return nullptr;
  }
  constexpr size_t alignment = std::max(alignof(T), kMinAlignment);
  const size_t size = sizeof(T) * count;
  auto result = allocator.Allocate(size, alignment);
  return static_cast<T*>(result.ptr);
}

/**
 * @brief Allocates and constructs a single object of type T.
 * @details Convenience function that allocates memory and constructs the object in-place.
 * Uses placement new internally.
 * @tparam T Type to allocate and construct
 * @tparam Alloc Allocator type (deduced)
 * @tparam Args Constructor argument types
 * @param allocator Allocator to use for allocation
 * @param args Arguments to forward to T's constructor
 * @return Pointer to constructed object, or nullptr on allocation failure
 *
 * @example
 * @code
 * FrameAllocator alloc(1024);
 * auto* vec = AllocateAndConstruct<MyVec3>(alloc, 1.0f, 2.0f, 3.0f);
 * @endcode
 */
template <typename T, Allocator Alloc, typename... Args>
  requires std::constructible_from<T, Args...>
[[nodiscard]] constexpr T* AllocateAndConstruct(Alloc& allocator,
                                                Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
  T* ptr = Allocate<T>(allocator);
  if (ptr != nullptr) {
    ::new (static_cast<void*>(ptr)) T(std::forward<Args>(args)...);
  }
  return ptr;
}

/**
 * @brief Allocates and default-constructs an array of objects of type T.
 * @details Convenience function that allocates memory and default-constructs objects in-place.
 * Uses placement new internally.
 * @tparam T Type to allocate and construct (must be default constructible)
 * @tparam Alloc Allocator type (deduced)
 * @param allocator Allocator to use for allocation
 * @param count Number of objects to allocate and construct
 * @return Pointer to first constructed object, or nullptr on allocation failure
 *
 * @example
 * @code
 * FrameAllocator alloc(1024);
 * auto* arr = AllocateAndConstructArray<MyType>(alloc, 10);
 * @endcode
 */
template <typename T, Allocator Alloc>
  requires std::default_initializable<T>
[[nodiscard]] constexpr T* AllocateAndConstructArray(Alloc& allocator, size_t count) noexcept(
    std::is_nothrow_default_constructible_v<T>) {
  T* ptr = Allocate<T>(allocator, count);
  if (ptr != nullptr) {
    for (size_t i = 0; i < count; ++i) {
      ::new (static_cast<void*>(ptr + i)) T{};
    }
  }
  return ptr;
}

}  // namespace helios::memory
