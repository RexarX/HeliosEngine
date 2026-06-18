#include <helios/cstring_view.hpp>
#include <helios/profile/details/memory_dispatch.hpp>
#include <helios/profile/memory.hpp>

#include <cstddef>
#include <cstdlib>
#include <new>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

namespace helios::profile::details {

namespace {

thread_local bool g_in_global_alloc_hook = false;

constexpr CStringView kGlobalAllocName{"global"};

[[nodiscard]] void* RawAlloc(size_t size) noexcept {
#if defined(_MSC_VER) && defined(_DEBUG)
  // MSVC debug builds route new/delete through the debug heap. Using malloc
  // here mixes allocators and can deadlock during static initialization.
  return _malloc_dbg(size, _NORMAL_BLOCK, nullptr, 0);
#else
  return std::malloc(size);
#endif
}

[[nodiscard]] void* RawAlignedAlloc(size_t size,
                                    std::align_val_t alignment) noexcept {
#if defined(_MSC_VER) && defined(_DEBUG)
  return _aligned_malloc_dbg(size, static_cast<size_t>(alignment), nullptr, 0);
#elif defined(_MSC_VER)
  return _aligned_malloc(size, static_cast<size_t>(alignment));
#else
  const auto align = static_cast<size_t>(alignment);
  return std::aligned_alloc(align, size);
#endif
}

void RawFree(void* ptr) noexcept {
#if defined(_MSC_VER) && defined(_DEBUG)
  _free_dbg(ptr, _NORMAL_BLOCK);
#else
  std::free(ptr);
#endif
}

void RawAlignedFree(void* ptr) noexcept {
#ifdef _MSC_VER
  _aligned_free(ptr);
#else
  std::free(ptr);
#endif
}

void ProfileAlloc(const void* ptr, size_t size) noexcept {
  if (ptr == nullptr || g_in_global_alloc_hook || !IsProfilerFinalized() ||
      !IsProfilerMemoryDispatchEnabled() || IsMemoryDispatchSuspended()) {
    return;
  }

  g_in_global_alloc_hook = true;
  Alloc(ptr, size, kGlobalAllocName, 0);
  g_in_global_alloc_hook = false;
}

void ProfileFree(const void* ptr) noexcept {
  if (ptr == nullptr || g_in_global_alloc_hook || !IsProfilerFinalized() ||
      !IsProfilerMemoryDispatchEnabled() || IsMemoryDispatchSuspended()) {
    return;
  }

  g_in_global_alloc_hook = true;
  Free(ptr, kGlobalAllocName, 0);
  g_in_global_alloc_hook = false;
}

}  // namespace

}  // namespace helios::profile::details

void* operator new(size_t size) {
  void* const ptr = helios::profile::details::RawAlloc(size);
  if (ptr == nullptr) [[unlikely]] {
    throw std::bad_alloc{};
  }

  helios::profile::details::ProfileAlloc(ptr, size);
  return ptr;
}

void* operator new[](size_t size) {
  return ::operator new(size);
}

void* operator new(size_t size, const std::nothrow_t&) noexcept {
  void* const ptr = helios::profile::details::RawAlloc(size);
  if (ptr != nullptr) {
    helios::profile::details::ProfileAlloc(ptr, size);
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

  helios::profile::details::ProfileFree(ptr);
  helios::profile::details::RawFree(ptr);
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
  void* const ptr = helios::profile::details::RawAlignedAlloc(size, alignment);
  if (ptr == nullptr) [[unlikely]] {
    throw std::bad_alloc{};
  }

  helios::profile::details::ProfileAlloc(ptr, size);
  return ptr;
}

void* operator new[](size_t size, std::align_val_t alignment) {
  return ::operator new(size, alignment);
}

void* operator new(size_t size, std::align_val_t alignment,
                   const std::nothrow_t&) noexcept {
  void* const ptr = helios::profile::details::RawAlignedAlloc(size, alignment);
  if (ptr != nullptr) {
    helios::profile::details::ProfileAlloc(ptr, size);
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

  helios::profile::details::ProfileFree(ptr);
  helios::profile::details::RawAlignedFree(ptr);
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
