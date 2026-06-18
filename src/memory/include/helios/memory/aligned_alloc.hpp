#pragma once

#include <helios/assert.hpp>
#include <helios/memory/common.hpp>
#include <helios/memory/details/profile.hpp>

#include <cstddef>
#include <cstdlib>

namespace helios::mem {

/**
 * @brief Allocates memory with the specified alignment.
 * @details Uses `_aligned_malloc` on MSVC and `std::aligned_alloc` on other
 * platforms.
 * On POSIX, `std::aligned_alloc` requires alignment >= `sizeof(void*)`.
 * Smaller alignments are satisfied via `malloc`, which provides at least
 * `alignof(std::max_align_t)` alignment.
 * @warning Triggers assertion in next cases:
 * - alignment is zero.
 * - alignment is not a power of two.
 * - size is zero.
 *
 * @param alignment The alignment in bytes (must be a power of two and non-zero)
 * @param size The size of the memory block to allocate in bytes (must be
 * non-zero)
 * @param enable_profile When false, skips profiler memory events (for internal
 * allocator backing storage that is tracked separately)
 * @return A pointer to the allocated memory block, or `nullptr` on failure
 */
[[nodiscard]] inline void* AlignedAlloc(size_t alignment, size_t size,
                                        bool enable_profile = true) noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::AlignedAlloc");
  HELIOS_MEMORY_PROFILE_ZONE_VALUE(size);
  HELIOS_ASSERT(alignment != 0, "alignment cannot be zero!");

  const auto is_power_of_two = [](size_t value) {
    return value != 0 && (value & (value - 1)) == 0;
  };
  HELIOS_ASSERT(is_power_of_two(alignment),
                "alignment must be a power of two!");
  HELIOS_ASSERT(size != 0, "size cannot be zero!");

#ifdef _MSC_VER
  void* const ptr = _aligned_malloc(size, alignment);
#else
  // POSIX aligned_alloc requires size to be a multiple of alignment and
  // alignment >= sizeof(void*). malloc is sufficient for smaller alignments
  // because it is aligned for any scalar type.
  void* ptr = nullptr;
  if (alignment < sizeof(void*)) {
    ptr = std::malloc(size);
  } else {
    ptr = std::aligned_alloc(alignment, AlignUp(size, alignment));
  }
#endif
  if (ptr != nullptr && enable_profile) {
    HELIOS_MEMORY_PROFILE_ALLOC(ptr, size, "AlignedAlloc");
  }
  return ptr;
}

/**
 * @brief Frees memory allocated with `AlignedAlloc`.
 * @details Uses `_aligned_free` on MSVC and `std::free` on other platforms.
 * @warning Triggers assertion if ptr is `nullptr`.
 * @param ptr The pointer to the memory block to free
 * @param enable_profile When false, skips profiler memory events
 */
inline void AlignedFree(void* ptr, bool enable_profile = true) noexcept {
  if (ptr == nullptr) [[unlikely]] {
    return;
  }

  if (enable_profile) {
    HELIOS_MEMORY_PROFILE_FREE(ptr, "AlignedAlloc");
  }

#ifdef _MSC_VER
  _aligned_free(ptr);
#else
  std::free(ptr);
#endif
}

}  // namespace helios::mem
