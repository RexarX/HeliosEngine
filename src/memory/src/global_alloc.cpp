#include <helios/memory/details/profile.hpp>

#ifdef HELIOS_MEMORY_USE_MIMALLOC
#include <mimalloc.h>
#endif

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

#include <cstddef>
#include <cstdlib>
#include <new>

// mimalloc global hooks bypass MSVC ASan-instrumented allocation. Route through
// the CRT/debug heap when AddressSanitizer is enabled (MSVC /fsanitize=address
// or Clang -fsanitize=address).
#if defined(HELIOS_MEMORY_USE_MIMALLOC) && !defined(__SANITIZE_ADDRESS__)
#define HELIOS_GLOBAL_ALLOC_USE_MIMALLOC_BACKEND
#endif

// These replaceable operator new/delete overloads form a single atomic set:
// once any of them is provided, every allocation must be paired with a
// deallocation from this same file. LTO and linker dead-stripping can each
// independently decide that an individual overload has no directly-visible
// caller (calls to it may only be emitted implicitly, e.g. by scalar
// deleting destructors) and discard it, while keeping others - silently
// falling back to the default library implementation for the discarded
// overloads. Mixing this file's operator new with the default operator
// delete (or vice versa) corrupts the heap, so every overload below is
// force-kept regardless of apparent reachability.
#if defined(__has_attribute)
#if __has_attribute(retain)
#define HELIOS_GLOBAL_ALLOC_KEEP __attribute__((used, retain))
#else
#define HELIOS_GLOBAL_ALLOC_KEEP __attribute__((used))
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define HELIOS_GLOBAL_ALLOC_KEEP __attribute__((used))
#else
#define HELIOS_GLOBAL_ALLOC_KEEP
#endif

namespace {

#if defined(HELIOS_MEMORY_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
thread_local bool g_in_global_alloc_hook = false;
#endif

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
  const size_t aligned = (size + align - 1) & ~(align - 1);
  return std::aligned_alloc(align, aligned);
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

void ProfileAlloc([[maybe_unused]] const void* ptr,
                  [[maybe_unused]] size_t size) noexcept {
#if defined(HELIOS_MEMORY_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
  if (ptr == nullptr || g_in_global_alloc_hook) {
    return;
  }

  g_in_global_alloc_hook = true;
  HELIOS_MEMORY_GLOBAL_ALLOC_PROFILE_ALLOC(ptr, size);
  g_in_global_alloc_hook = false;
#endif
}

void ProfileFree([[maybe_unused]] const void* ptr) noexcept {
#if defined(HELIOS_MEMORY_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
  if (ptr == nullptr || g_in_global_alloc_hook) {
    return;
  }

  g_in_global_alloc_hook = true;
  HELIOS_MEMORY_GLOBAL_ALLOC_PROFILE_FREE(ptr);
  g_in_global_alloc_hook = false;
#endif
}

}  // namespace

HELIOS_GLOBAL_ALLOC_KEEP void* operator new(size_t size) {
  void* const ptr = RawAlloc(size);
  if (ptr == nullptr) [[unlikely]] {
    throw std::bad_alloc{};
  }

  ProfileAlloc(ptr, size);
  return ptr;
}

HELIOS_GLOBAL_ALLOC_KEEP void* operator new[](size_t size) {
  return ::operator new(size);
}

HELIOS_GLOBAL_ALLOC_KEEP void* operator new(size_t size,
                                            const std::nothrow_t&) noexcept {
  void* const ptr = RawAlloc(size);
  if (ptr != nullptr) {
    ProfileAlloc(ptr, size);
  }
  return ptr;
}

HELIOS_GLOBAL_ALLOC_KEEP void* operator new[](size_t size,
                                              const std::nothrow_t&) noexcept {
  return ::operator new(size, std::nothrow);
}

HELIOS_GLOBAL_ALLOC_KEEP void operator delete(void* ptr) noexcept {
  if (ptr == nullptr) [[unlikely]] {
    return;
  }

  ProfileFree(ptr);
  RawFree(ptr);
}

HELIOS_GLOBAL_ALLOC_KEEP void operator delete[](void* ptr) noexcept {
  ::operator delete(ptr);
}

HELIOS_GLOBAL_ALLOC_KEEP void operator delete(void* ptr, size_t) noexcept {
  ::operator delete(ptr);
}

HELIOS_GLOBAL_ALLOC_KEEP void operator delete[](void* ptr, size_t) noexcept {
  ::operator delete(ptr);
}

HELIOS_GLOBAL_ALLOC_KEEP void operator delete(void* ptr,
                                              const std::nothrow_t&) noexcept {
  ::operator delete(ptr);
}

HELIOS_GLOBAL_ALLOC_KEEP void operator delete[](
    void* ptr, const std::nothrow_t&) noexcept {
  ::operator delete(ptr);
}

HELIOS_GLOBAL_ALLOC_KEEP void* operator new(size_t size,
                                            std::align_val_t alignment) {
  void* const ptr = RawAlignedAlloc(size, alignment);
  if (ptr == nullptr) [[unlikely]] {
    throw std::bad_alloc{};
  }

  ProfileAlloc(ptr, size);
  return ptr;
}

HELIOS_GLOBAL_ALLOC_KEEP void* operator new[](size_t size,
                                              std::align_val_t alignment) {
  return ::operator new(size, alignment);
}

HELIOS_GLOBAL_ALLOC_KEEP void* operator new(size_t size,
                                            std::align_val_t alignment,
                                            const std::nothrow_t&) noexcept {
  void* const ptr = RawAlignedAlloc(size, alignment);
  if (ptr != nullptr) {
    ProfileAlloc(ptr, size);
  }
  return ptr;
}

HELIOS_GLOBAL_ALLOC_KEEP void* operator new[](size_t size,
                                              std::align_val_t alignment,
                                              const std::nothrow_t&) noexcept {
  return ::operator new(size, alignment, std::nothrow);
}

HELIOS_GLOBAL_ALLOC_KEEP void operator delete(
    void* ptr, std::align_val_t /*alignment*/) noexcept {
  if (ptr == nullptr) [[unlikely]] {
    return;
  }

  ProfileFree(ptr);
  RawAlignedFree(ptr);
}

HELIOS_GLOBAL_ALLOC_KEEP void operator delete[](
    void* ptr, std::align_val_t alignment) noexcept {
  ::operator delete(ptr, alignment);
}

HELIOS_GLOBAL_ALLOC_KEEP void operator delete(
    void* ptr, size_t /*size*/, std::align_val_t alignment) noexcept {
  ::operator delete(ptr, alignment);
}

HELIOS_GLOBAL_ALLOC_KEEP void operator delete[](
    void* ptr, size_t size, std::align_val_t alignment) noexcept {
  ::operator delete(ptr, size, alignment);
}
