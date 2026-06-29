#include <helios/memory/details/profile.hpp>

#include <cstddef>
#include <cstdlib>
#include <new>

#ifdef HELIOS_MEMORY_USE_MIMALLOC
#include <mimalloc.h>
#endif

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

// mimalloc global hooks bypass MSVC ASan-instrumented allocation. Route through
// the CRT/debug heap when AddressSanitizer is enabled (MSVC /fsanitize=address
// or Clang -fsanitize=address).
#if defined(HELIOS_MEMORY_USE_MIMALLOC) && !defined(__SANITIZE_ADDRESS__)
#define HELIOS_GLOBAL_ALLOC_USE_MIMALLOC_BACKEND
#endif

namespace {

#if defined(HELIOS_MEMORY_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
thread_local bool g_in_global_alloc_hook = false;
#endif

[[nodiscard]] constexpr size_t AlignUp(size_t value,
                                       size_t alignment) noexcept {
  return (value + alignment - 1) & ~(alignment - 1);
}

[[nodiscard]] void* RawAlloc(size_t size) noexcept {
#ifdef HELIOS_GLOBAL_ALLOC_USE_MIMALLOC_BACKEND
  return mi_malloc(size);
#elif defined(_MSC_VER) && defined(_DEBUG)
  // MSVC debug builds route new/delete through the debug heap. Using malloc
  // here mixes allocators and can deadlock during static initialization.
  return _malloc_dbg(size, _NORMAL_BLOCK, nullptr, 0);
#else
  return std::malloc(size);
#endif
}

[[nodiscard]] void* RawAlignedAlloc(size_t size,
                                    std::align_val_t alignment) noexcept {
#ifdef HELIOS_GLOBAL_ALLOC_USE_MIMALLOC_BACKEND
  return mi_malloc_aligned(size, static_cast<size_t>(alignment));
#elif defined(_MSC_VER) && defined(_DEBUG)
  return _aligned_malloc_dbg(size, static_cast<size_t>(alignment), nullptr, 0);
#elif defined(_MSC_VER)
  return _aligned_malloc(size, static_cast<size_t>(alignment));
#else
  const auto align = static_cast<size_t>(alignment);
  return std::aligned_alloc(align, AlignUp(size, align));
#endif
}

void RawFree(void* ptr) noexcept {
#ifdef HELIOS_GLOBAL_ALLOC_USE_MIMALLOC_BACKEND
  mi_free(ptr);
#elif defined(_MSC_VER) && defined(_DEBUG)
  _free_dbg(ptr, _NORMAL_BLOCK);
#else
  std::free(ptr);
#endif
}

void RawAlignedFree(void* ptr) noexcept {
#ifdef HELIOS_GLOBAL_ALLOC_USE_MIMALLOC_BACKEND
  mi_free(ptr);
#elif defined(_MSC_VER)
  _aligned_free(ptr);
#else
  std::free(ptr);
#endif
}

void ProfileAlloc(const void* ptr, size_t size) noexcept {
#if defined(HELIOS_MEMORY_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
  if (ptr == nullptr || g_in_global_alloc_hook) {
    return;
  }

  g_in_global_alloc_hook = true;
  HELIOS_MEMORY_GLOBAL_ALLOC_PROFILE_ALLOC(ptr, size);
  g_in_global_alloc_hook = false;
#else
  (void)ptr;
  (void)size;
#endif
}

void ProfileFree(const void* ptr) noexcept {
#if defined(HELIOS_MEMORY_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
  if (ptr == nullptr || g_in_global_alloc_hook) {
    return;
  }

  g_in_global_alloc_hook = true;
  HELIOS_MEMORY_GLOBAL_ALLOC_PROFILE_FREE(ptr);
  g_in_global_alloc_hook = false;
#else
  (void)ptr;
#endif
}

}  // namespace

void* operator new(size_t size) {
  void* const ptr = RawAlloc(size);
  if (ptr == nullptr) [[unlikely]] {
    throw std::bad_alloc{};
  }

  ProfileAlloc(ptr, size);
  return ptr;
}

void* operator new[](size_t size) {
  return ::operator new(size);
}

void* operator new(size_t size, const std::nothrow_t&) noexcept {
  void* const ptr = RawAlloc(size);
  if (ptr != nullptr) {
    ProfileAlloc(ptr, size);
  }
  return ptr;
}

void* operator new[](size_t size, const std::nothrow_t&) noexcept {
  return ::operator new(size, std::nothrow);
}

void operator delete(void* ptr) noexcept {
  if (ptr == nullptr) [[unlikely]] {
    return;
  }

  ProfileFree(ptr);
  RawFree(ptr);
}

void operator delete[](void* ptr) noexcept {
  ::operator delete(ptr);
}

void operator delete(void* ptr, size_t) noexcept {
  ::operator delete(ptr);
}

void operator delete[](void* ptr, size_t) noexcept {
  ::operator delete(ptr);
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept {
  ::operator delete(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept {
  ::operator delete(ptr);
}

void* operator new(size_t size, std::align_val_t alignment) {
  void* const ptr = RawAlignedAlloc(size, alignment);
  if (ptr == nullptr) [[unlikely]] {
    throw std::bad_alloc{};
  }

  ProfileAlloc(ptr, size);
  return ptr;
}

void* operator new[](size_t size, std::align_val_t alignment) {
  return ::operator new(size, alignment);
}

void* operator new(size_t size, std::align_val_t alignment,
                   const std::nothrow_t&) noexcept {
  void* const ptr = RawAlignedAlloc(size, alignment);
  if (ptr != nullptr) {
    ProfileAlloc(ptr, size);
  }
  return ptr;
}

void* operator new[](size_t size, std::align_val_t alignment,
                     const std::nothrow_t&) noexcept {
  return ::operator new(size, alignment, std::nothrow);
}

void operator delete(void* ptr, std::align_val_t /*alignment*/) noexcept {
  if (ptr == nullptr) [[unlikely]] {
    return;
  }

  ProfileFree(ptr);
  RawAlignedFree(ptr);
}

void operator delete[](void* ptr, std::align_val_t alignment) noexcept {
  ::operator delete(ptr, alignment);
}

void operator delete(void* ptr, size_t /*size*/,
                     std::align_val_t alignment) noexcept {
  ::operator delete(ptr, alignment);
}

void operator delete[](void* ptr, size_t size,
                       std::align_val_t alignment) noexcept {
  ::operator delete(ptr, size, alignment);
}
