#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/memory/allocator_traits.hpp>

#include <cstddef>
#include <cstdlib>

namespace helios::memory {

/**
 * @brief Allocate memory with the specified alignment.
 * @details Allocates a block of memory of the specified size aligned to the specified alignment boundary.
 * The alignment must be a power of two and non-zero. The size must also be non-zero.
 *
 * Uses `_aligned_malloc` on MSVC and `std::aligned_alloc` on other platforms.
 *
 * @warning Triggers assertion in next cases:
 * - alignment is zero
 * - alignment is not a power of two
 * - size is zero
 * @param alignment The alignment in bytes (must be a power of two and non-zero).
 * @param size The size of the memory block to allocate in bytes (must be non-zero).
 * @return A pointer to the allocated memory block.
 * @throws std::bad_alloc if the allocation fails.
 */
inline void* AlignedAlloc(size_t alignment, size_t size) {
  HELIOS_ASSERT(alignment != 0, "Failed to allocate memory: alignment cannot be zero!");
  HELIOS_ASSERT(IsPowerOfTwo(alignment), "Failed to allocate memory: alignment must be a power of two!");
  HELIOS_ASSERT(size != 0, "Failed to allocate memory: size cannot be zero!");
#ifdef _MSC_VER
  return _aligned_malloc(size, alignment);
#else
  // POSIX aligned_alloc requires size to be a multiple of alignment
  // Round up size to the next multiple of alignment
  return std::aligned_alloc(alignment, AlignUp(size, alignment));
#endif
}

/**
 * @brief Free memory allocated with AlignedAlloc.
 * @details Frees a block of memory allocated with AlignedAlloc.
 * The ptr must be a pointer returned by AlignedAlloc.
 *
 * Uses `_aligned_free` on MSVC and `std::free` on other platforms.
 *
 * @warning Triggers assertion if ptr is nullptr.
 * @param ptr The pointer to the memory block to free.
 */
inline void AlignedFree(void* ptr) {
  HELIOS_ASSERT(ptr != nullptr, "Failed to free memory: pointer cannot be nullptr!");
#ifdef _MSC_VER
  _aligned_free(ptr);
#else
  std::free(ptr);
#endif
}

}  // namespace helios::memory
