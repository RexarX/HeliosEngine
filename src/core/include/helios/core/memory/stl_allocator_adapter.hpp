#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/memory/allocator_traits.hpp>
#include <helios/core/memory/double_frame_allocator.hpp>
#include <helios/core/memory/frame_allocator.hpp>
#include <helios/core/memory/free_list_allocator.hpp>
#include <helios/core/memory/growable_allocator.hpp>
#include <helios/core/memory/pool_allocator.hpp>
#include <helios/core/memory/stack_allocator.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <limits>
#include <type_traits>

namespace helios::memory {

/**
 * @brief STL-compatible allocator adapter for custom allocators.
 * @details Wraps custom allocators to work with STL containers. The underlying
 * allocator must remain alive for the lifetime of any containers using this adapter.
 * @note Thread-safety depends on the underlying allocator.
 * @note The underlying allocator is held by reference - ensure it outlives all containers.
 * @tparam T Type of objects to allocate
 * @tparam UnderlyingAllocator The custom allocator type
 */
template <typename T, typename UnderlyingAllocator>
class STLAllocatorAdapter {
public:
  // Standard allocator type definitions
  using value_type = T;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using propagate_on_container_move_assignment = std::true_type;
  using is_always_equal = std::false_type;

  /**
   * @brief Rebind support for allocating different types.
   * @details Required by STL for internal allocations (e.g., node allocators).
   */
  template <typename U>
  struct rebind {
    using other = STLAllocatorAdapter<U, UnderlyingAllocator>;
  };

  /**
   * @brief Constructs adapter with reference to underlying allocator.
   * @param allocator Reference to the custom allocator to use
   */
  explicit constexpr STLAllocatorAdapter(UnderlyingAllocator& allocator) noexcept : allocator_(&allocator) {}

  /**
   * @brief Rebind copy constructor.
   * @details Allows converting between allocators of different types.
   */
  template <typename U>
  explicit constexpr STLAllocatorAdapter(const STLAllocatorAdapter<U, UnderlyingAllocator>& other) noexcept
      : allocator_(other.allocator_) {}

  constexpr STLAllocatorAdapter(const STLAllocatorAdapter&) noexcept = default;
  constexpr STLAllocatorAdapter(STLAllocatorAdapter&&) noexcept = default;
  constexpr ~STLAllocatorAdapter() noexcept = default;

  template <typename U>
  constexpr STLAllocatorAdapter& operator=(const STLAllocatorAdapter<U, UnderlyingAllocator>& other) noexcept;

  constexpr STLAllocatorAdapter& operator=(const STLAllocatorAdapter&) noexcept = default;
  constexpr STLAllocatorAdapter& operator=(STLAllocatorAdapter&&) noexcept = default;

  /**
   * @brief Allocates memory for n objects of type T.
   * @throws std::bad_alloc if allocation fails
   * @param count Count of objects to allocate space for
   * @return Pointer to allocated memory
   */
  [[nodiscard]] T* allocate(size_type count);

  /**
   * @brief Deallocates memory for n objects.
   * @param ptr Pointer to deallocate
   * @param count Count of objects (may be ignored by some allocators)
   */
  void deallocate(T* ptr, size_type count) noexcept;

  /**
   * @brief Equality comparison.
   * @details Two adapters are equal if they reference the same underlying allocator.
   */
  template <typename U>
  constexpr bool operator==(const STLAllocatorAdapter<U, UnderlyingAllocator>& other) const noexcept {
    return allocator_ == other.allocator_;
  }

  /**
   * @brief Inequality comparison.
   */
  template <typename U>
  constexpr bool operator!=(const STLAllocatorAdapter<U, UnderlyingAllocator>& other) const noexcept {
    return !(*this == other);
  }

  /**
   * @brief Returns maximum number of objects that can be allocated.
   * @return Maximum size
   */
  [[nodiscard]] constexpr size_type max_size() const noexcept {
    return std::numeric_limits<size_type>::max() / sizeof(T);
  }

  /**
   * @brief Gets pointer to underlying allocator.
   * @return Pointer to underlying allocator
   */
  [[nodiscard]] constexpr UnderlyingAllocator* get_allocator() const noexcept { return allocator_; }

private:
  template <typename U, typename A>
  friend class STLAllocatorAdapter;

  UnderlyingAllocator* allocator_ = nullptr;
};

template <typename T, typename UnderlyingAllocator>
template <typename U>
constexpr auto STLAllocatorAdapter<T, UnderlyingAllocator>::operator=(
    const STLAllocatorAdapter<U, UnderlyingAllocator>& other) noexcept -> STLAllocatorAdapter& {
  allocator_ = other.allocator_;
  return *this;
}

template <typename T, typename UnderlyingAllocator>
inline T* STLAllocatorAdapter<T, UnderlyingAllocator>::allocate(size_type count) {
  if (count > max_size()) {
    throw std::bad_alloc();
  }

  const size_t size = count * sizeof(T);
  constexpr size_t alignment = std::max(alignof(T), kMinAlignment);

  auto result = allocator_->Allocate(size, alignment);

  if (result.ptr == nullptr) {
    throw std::bad_alloc();
  }

  return static_cast<T*>(result.ptr);
}

template <typename T, typename UnderlyingAllocator>
inline void STLAllocatorAdapter<T, UnderlyingAllocator>::deallocate(T* ptr, size_type count) noexcept {
  if (ptr == nullptr) [[unlikely]] {
    return;
  }

  // FrameAllocator, DoubleFrameAllocator, and NFrameAllocator don't support individual deallocation
  if constexpr (std::same_as<UnderlyingAllocator, FrameAllocator> ||
                std::same_as<UnderlyingAllocator, DoubleFrameAllocator>) {
    // No-op: Frame allocators don't support individual deallocation
  }
  // PoolAllocator and FreeListAllocator only need ptr
  else if constexpr (std::same_as<UnderlyingAllocator, PoolAllocator> ||
                     std::same_as<UnderlyingAllocator, FreeListAllocator>) {
    allocator_->Deallocate(ptr);
  }
  // StackAllocator needs ptr and size
  else {
    allocator_->Deallocate(ptr, count * sizeof(T));
  }
}

/**
 * @brief STL adapter for FrameAllocator.
 * @note Deallocation is a no-op. Use Reset() on the allocator to free all memory.
 * @warning Do not use with containers that frequently deallocate individual elements.
 * Best for temporary containers that live for one frame.
 * @tparam T Type of objects to allocate
 *
 * @example
 * @code
 * FrameAllocator frame_alloc(1024 * 1024);
 * std::vector<int, STLFrameAllocator<int>> vec{STLFrameAllocator<int>(frame_alloc)};
 * @endcode
 */
template <typename T>
using STLFrameAllocator = STLAllocatorAdapter<T, FrameAllocator>;

/**
 * @brief STL adapter for PoolAllocator.
 * @note Only suitable if sizeof(T) <= block_size of the pool.
 * @warning May throw if T is larger than the pool's block size.
 * @tparam T Type of objects to allocate
 *
 * @example
 * @code
 * PoolAllocator pool_alloc(sizeof(int), 100);
 * std::vector<int, STLPoolAllocator<int>> vec{STLPoolAllocator<int>(pool_alloc)};
 * @endcode
 */
template <typename T>
using STLPoolAllocator = STLAllocatorAdapter<T, PoolAllocator>;

/**
 * @brief STL adapter for StackAllocator.
 * @note Deallocations should follow LIFO order for optimal behavior.
 * @warning Violating LIFO order will trigger assertions in debug builds.
 * @tparam T Type of objects to allocate
 *
 * @example
 * @code
 * StackAllocator stack_alloc(1024 * 1024);
 * std::vector<int, STLStackAllocator<int>> vec{STLStackAllocator<int>(stack_alloc)};
 * @endcode
 */
template <typename T>
using STLStackAllocator = STLAllocatorAdapter<T, StackAllocator>;

/**
 * @brief STL adapter for FreeListAllocator.
 * @details General-purpose adapter.
 * @note Deallocations can occur in any order.
 * @tparam T Type of objects to allocate
 *
 * @example
 * @code
 * FreeListAllocator freelist_alloc(1024 * 1024);
 * std::vector<int, STLFreeListAllocator<int>> vec{STLFreeListAllocator<int>(freelist_alloc)};
 * @endcode
 */
template <typename T>
using STLFreeListAllocator = STLAllocatorAdapter<T, FreeListAllocator>;

/**
 * @brief STL adapter for GrowableAllocator.
 * @details Provides automatic growth for STL containers. The wrapped allocator
 * must be compatible with GrowableAllocator (FrameAllocator, StackAllocator, FreeListAllocator).
 * @note Deallocations are supported depending on the underlying allocator.
 * @tparam T Type of objects to allocate
 * @tparam UnderlyingAlloc The base allocator type to wrap with GrowableAllocator
 *
 * @example
 * @code
 * GrowableAllocator<FrameAllocator> growable_alloc(1024);
 * std::vector<int, STLGrowableAllocator<int, FrameAllocator>> vec{
 *     STLGrowableAllocator<int, FrameAllocator>(growable_alloc)};
 * @endcode
 */
template <typename T, typename UnderlyingAlloc>
using STLGrowableAllocator = STLAllocatorAdapter<T, GrowableAllocator<UnderlyingAlloc>>;

}  // namespace helios::memory
