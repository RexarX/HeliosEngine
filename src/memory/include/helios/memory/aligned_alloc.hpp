#pragma once

#include <cstddef>

namespace helios::mem {

/**
 * @brief Allocates memory with the specified alignment.
 * @details Uses mimalloc when enabled, `_aligned_malloc` on MSVC, and
 * `std::aligned_alloc` on other platforms.
 * On POSIX, `std::aligned_alloc` requires alignment >= `sizeof(void*)`.
 * Smaller alignments are satisfied via `malloc`, which provides at least
 * `alignof(std::max_align_t)` alignment.
 * @warning Triggers assertion when `alignment` is zero, `alignment` is not a
 * power of two, or `size` is zero.
 * @param alignment The alignment in bytes (must be a power of two and non-zero)
 * @param size The size of the memory block to allocate in bytes (must be
 * non-zero)
 * @param enable_profile When false, skips profiler memory events (for internal
 * allocator backing storage that is tracked separately)
 * @return A pointer to the allocated memory block, or `nullptr` on failure
 */
[[nodiscard]] void* AlignedAlloc(size_t alignment, size_t size,
                                 bool enable_profile = true) noexcept;

/**
 * @brief Frees memory allocated with `AlignedAlloc`.
 * @details Uses the allocation backend paired with `AlignedAlloc`. Passing
 * `nullptr` is a no-op.
 * @param ptr The pointer to the memory block to free
 * @param enable_profile When false, skips profiler memory events
 */
void AlignedFree(void* ptr, bool enable_profile = true) noexcept;

}  // namespace helios::mem
