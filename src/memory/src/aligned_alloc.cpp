#include <pch.hpp>

#include <helios/memory/aligned_alloc.hpp>

#include <helios/assert.hpp>
#include <helios/memory/common.hpp>
#include <helios/memory/details/profile.hpp>

#include <cstddef>
#include <cstdlib>

#ifdef HELIOS_MEMORY_USE_MIMALLOC
#include <mimalloc.h>
#endif

namespace helios::mem {

void* AlignedAlloc(size_t alignment, size_t size,
                   bool enable_profile) noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::AlignedAlloc");
  HELIOS_MEMORY_PROFILE_ZONE_VALUE(size);
  HELIOS_ASSERT(alignment != 0, "alignment cannot be zero!");
  HELIOS_ASSERT(IsPowerOfTwo(alignment), "alignment must be a power of two!");
  HELIOS_ASSERT(size != 0, "size cannot be zero!");

#ifdef HELIOS_MEMORY_USE_MIMALLOC
  void* const ptr = mi_malloc_aligned(size, alignment);
#elif defined(_MSC_VER)
  void* const ptr = _aligned_malloc(size, alignment);
#else
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

void AlignedFree(void* ptr, bool enable_profile) noexcept {
  if (ptr == nullptr) [[unlikely]] {
    return;
  }
  if (enable_profile) {
    HELIOS_MEMORY_PROFILE_FREE(ptr, "AlignedAlloc");
  }

#ifdef HELIOS_MEMORY_USE_MIMALLOC
  mi_free(ptr);
#elif defined(_MSC_VER)
  _aligned_free(ptr);
#else
  std::free(ptr);
#endif
}

}  // namespace helios::mem
