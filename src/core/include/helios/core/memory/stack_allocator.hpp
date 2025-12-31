#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/memory/allocator_traits.hpp>
#include <helios/core/memory/common.hpp>

#include <algorithm>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

namespace helios::memory {

/**
 * @brief Stack/linear allocator with LIFO deallocation support.
 * @details Allocates memory sequentially using a bump pointer, but unlike FrameAllocator,
 * supports LIFO (stack-like) deallocations.
 * Each allocation stores a header with the previous offset, allowing proper unwinding.
 *
 * Ideal for hierarchical/scoped allocations where deallocation follows
 * allocation order (e.g., call stacks, recursive algorithms).
 *
 * Each allocation has a small header overhead for tracking.
 *
 * Uses lock-free atomic operations for thread-safe allocations.
 *
 * @note Thread-safe: Multiple threads can safely call Allocate() concurrently.
 * @warning Deallocations must follow LIFO order (stack discipline).
 * @warning Move operations (move constructor and move assignment) are NOT thread-safe
 * and must be externally synchronized. Do not move an allocator while other threads
 * are accessing it.
 */
class StackAllocator final {
public:
  /**
   * @brief Constructs a stack allocator with specified capacity.
   * @warning Triggers assertion if capacity is 0.
   * @param capacity Total size of the memory buffer in bytes
   */
  explicit StackAllocator(size_t capacity);
  StackAllocator(StackAllocator&& other) noexcept;
  StackAllocator(const StackAllocator&) = delete;
  ~StackAllocator() noexcept;

  StackAllocator& operator=(const StackAllocator&) = delete;
  StackAllocator& operator=(StackAllocator&& other) noexcept;

  /**
   * @brief Allocates memory with specified size and alignment.
   * @details Stores allocation header for LIFO deallocation support
   * @warning Triggers assertion in next cases:
   * - Alignment is not a power of 2.
   * - Alignment is less than kMinAlignment.
   * @param size Number of bytes to allocate
   * @param alignment Alignment requirement (must be power of 2)
   * @return AllocationResult with pointer and actual allocated size, or {nullptr, 0} on failure
   */
  [[nodiscard]] AllocationResult Allocate(size_t size, size_t alignment = kDefaultAlignment) noexcept;

  /**
   * @brief Allocates memory for a single object of type T.
   * @details Convenience function that calculates size and alignment from the type.
   * The returned memory is uninitialized - use placement new to construct the object.
   * @tparam T Type to allocate memory for
   * @return Pointer to allocated memory, or nullptr on failure
   *
   * @example
   * @code
   * StackAllocator alloc(1024);
   * int* ptr = alloc.Allocate<int>();
   * if (ptr != nullptr) {
   *   new (ptr) int(42);
   * }
   * @endcode
   */
  template <typename T>
  [[nodiscard]] T* Allocate() noexcept;

  /**
   * @brief Allocates memory for an array of objects of type T.
   * @details Convenience function that calculates size and alignment from the type.
   * The returned memory is uninitialized - use placement new to construct objects.
   * @tparam T Type to allocate memory for
   * @param count Number of objects to allocate space for
   * @return Pointer to allocated memory, or nullptr on failure
   *
   * @example
   * @code
   * StackAllocator alloc(1024);
   * int* arr = alloc.Allocate<int>(10);
   * @endcode
   */
  template <typename T>
  [[nodiscard]] T* Allocate(size_t count) noexcept;

  /**
   * @brief Allocates and constructs a single object of type T.
   * @details Convenience function that allocates memory and constructs the object in-place.
   * @tparam T Type to allocate and construct
   * @tparam Args Constructor argument types
   * @param args Arguments to forward to T's constructor
   * @return Pointer to constructed object, or nullptr on allocation failure
   *
   * @example
   * @code
   * StackAllocator alloc(1024);
   * auto* vec = alloc.AllocateAndConstruct<MyVec3>(1.0f, 2.0f, 3.0f);
   * @endcode
   */
  template <typename T, typename... Args>
    requires std::constructible_from<T, Args...>
  [[nodiscard]] T* AllocateAndConstruct(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>);

  /**
   * @brief Allocates and default-constructs an array of objects of type T.
   * @details Convenience function that allocates memory and default-constructs objects in-place.
   * @tparam T Type to allocate and construct (must be default constructible)
   * @param count Number of objects to allocate and construct
   * @return Pointer to first constructed object, or nullptr on allocation failure
   *
   * @example
   * @code
   * StackAllocator alloc(1024);
   * auto* arr = alloc.AllocateAndConstructArray<MyType>(10);
   * @endcode
   */
  template <typename T>
    requires std::default_initializable<T>
  [[nodiscard]] T* AllocateAndConstructArray(size_t count) noexcept(std::is_nothrow_default_constructible_v<T>);

  /**
   * @brief Deallocates memory in LIFO order.
   * @warning Triggers assertion in next cases:
   * - Pointer does not belong to this allocator.
   * - Deallocation violates LIFO order.
   * @param ptr Pointer to deallocate (must be the most recent allocation)
   * @param size Size of allocation (used for validation)
   */
  void Deallocate(void* ptr, size_t size) noexcept;

  /**
   * @brief Resets the allocator, freeing all allocations.
   * @details Resets the internal offset to 0, effectively freeing all memory.
   */
  void Reset() noexcept;

  /**
   * @brief Rewinds the stack to a previous marker position.
   * @details Invalidates all allocations made after the marker
   * @warning Triggers assertion in next cases:
   * - Marker is ahead of current offset.
   * - Marker exceeds capacity.
   * @param marker Previously obtained marker from Marker()
   */
  void RewindToMarker(size_t marker) noexcept;

  /**
   * @brief Checks if the allocator is empty.
   * @return True if no allocations have been made since last reset
   */
  [[nodiscard]] bool Empty() const noexcept { return offset_.load(std::memory_order_relaxed) == 0; }

  /**
   * @brief Checks if the allocator is full.
   * @return True if no more allocations can be made without reset
   */
  [[nodiscard]] bool Full() const noexcept { return offset_.load(std::memory_order_relaxed) >= capacity_; }

  /**
   * @brief Checks if a pointer belongs to this allocator.
   * @param ptr Pointer to check
   * @return True if pointer is within allocator's memory range
   */
  [[nodiscard]] bool Owns(const void* ptr) const noexcept;

  /**
   * @brief Gets current allocator statistics.
   * @return AllocatorStats with current usage information
   */
  [[nodiscard]] AllocatorStats Stats() const noexcept;

  /**
   * @brief Gets a marker for the current stack position.
   * @note Can be used with RewindToMarker for bulk deallocations.
   * @return Current offset as a marker
   */
  [[nodiscard]] size_t Marker() const noexcept { return offset_.load(std::memory_order_relaxed); }

  /**
   * @brief Gets the total capacity of the allocator.
   * @return Capacity in bytes
   */
  [[nodiscard]] size_t Capacity() const noexcept { return capacity_.load(std::memory_order_relaxed); }

  /**
   * @brief Gets the current offset (amount of memory used).
   * @return Current offset in bytes
   */
  [[nodiscard]] size_t CurrentOffset() const noexcept { return offset_.load(std::memory_order_relaxed); }

  /**
   * @brief Gets the amount of free space remaining.
   * @return Free space in bytes
   */
  [[nodiscard]] size_t FreeSpace() const noexcept;

private:
  /**
   * @brief Allocation header stored before each allocation.
   * @details Tracks previous offset for stack unwinding.
   */
  struct AllocationHeader {
    size_t previous_offset = 0;  ///< Offset before this allocation
    size_t padding = 0;          ///< Padding used for alignment
  };

  void* buffer_ = nullptr;                      ///< Underlying memory buffer
  std::atomic<size_t> capacity_{0};             ///< Total capacity in bytes
  std::atomic<size_t> offset_{0};               ///< Current allocation offset
  std::atomic<size_t> peak_offset_{0};          ///< Peak offset reached
  std::atomic<size_t> allocation_count_{0};     ///< Number of active allocations
  std::atomic<size_t> total_allocations_{0};    ///< Total allocations made
  std::atomic<size_t> total_deallocations_{0};  ///< Total deallocations made
  std::atomic<size_t> alignment_waste_{0};      ///< Total bytes wasted due to alignment
};

inline StackAllocator::StackAllocator(size_t capacity) : capacity_{capacity} {
  HELIOS_ASSERT(capacity > 0, "Failed to construct StackAllocator: capacity must be greater than 0!");

  // Allocate aligned buffer
  buffer_ = AlignedAlloc(kDefaultAlignment, capacity);
  HELIOS_VERIFY(buffer_ != nullptr, "Failed to construct StackAllocator: Allocation of StackAllocator buffer failed!");
}

inline StackAllocator::~StackAllocator() noexcept {
  if (buffer_ != nullptr) {
    AlignedFree(buffer_);
  }
}

inline StackAllocator::StackAllocator(StackAllocator&& other) noexcept
    : buffer_(other.buffer_),
      capacity_{other.capacity_.load(std::memory_order_relaxed)},
      offset_(other.offset_.load(std::memory_order_acquire)),
      peak_offset_(other.peak_offset_.load(std::memory_order_acquire)),
      allocation_count_(other.allocation_count_.load(std::memory_order_acquire)),
      total_allocations_(other.total_allocations_.load(std::memory_order_acquire)),
      total_deallocations_(other.total_deallocations_.load(std::memory_order_acquire)),
      alignment_waste_(other.alignment_waste_.load(std::memory_order_acquire)) {
  other.buffer_ = nullptr;
  other.capacity_.store(0, std::memory_order_relaxed);
  other.offset_.store(0, std::memory_order_release);
  other.peak_offset_.store(0, std::memory_order_release);
  other.allocation_count_.store(0, std::memory_order_release);
  other.total_allocations_.store(0, std::memory_order_release);
  other.total_deallocations_.store(0, std::memory_order_release);
  other.alignment_waste_.store(0, std::memory_order_release);
}

inline StackAllocator& StackAllocator::operator=(StackAllocator&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  // Free current buffer
  if (buffer_ != nullptr) {
    AlignedFree(buffer_);
  }

  // Move from other
  buffer_ = other.buffer_;
  capacity_.store(other.capacity_.load(std::memory_order_relaxed), std::memory_order_relaxed);

  offset_.store(other.offset_.load(std::memory_order_acquire), std::memory_order_release);
  peak_offset_.store(other.peak_offset_.load(std::memory_order_acquire), std::memory_order_release);
  allocation_count_.store(other.allocation_count_.load(std::memory_order_acquire), std::memory_order_release);
  total_allocations_.store(other.total_allocations_.load(std::memory_order_acquire), std::memory_order_release);
  total_deallocations_.store(other.total_deallocations_.load(std::memory_order_acquire), std::memory_order_release);
  alignment_waste_.store(other.alignment_waste_.load(std::memory_order_acquire), std::memory_order_release);

  // Reset other
  other.buffer_ = nullptr;
  other.capacity_.store(0, std::memory_order_relaxed);
  other.offset_.store(0, std::memory_order_release);
  other.peak_offset_.store(0, std::memory_order_release);
  other.allocation_count_.store(0, std::memory_order_release);
  other.total_allocations_.store(0, std::memory_order_release);
  other.total_deallocations_.store(0, std::memory_order_release);
  other.alignment_waste_.store(0, std::memory_order_release);

  return *this;
}

inline AllocationResult StackAllocator::Allocate(size_t size, size_t alignment) noexcept {
  HELIOS_ASSERT(IsPowerOfTwo(alignment), "Failed to allocate memory: alignment must be power of 2, got '{}'!",
                alignment);
  HELIOS_ASSERT(alignment >= kMinAlignment, "Failed to allocate memory: alignment must be at least '{}', got '{}'!",
                kMinAlignment, alignment);

  if (size == 0) [[unlikely]] {
    return {.ptr = nullptr, .allocated_size = 0};
  }

  // Lock-free allocation using compare-and-swap
  size_t current_offset = offset_.load(std::memory_order_acquire);
  size_t new_offset = 0;
  size_t header_padding = 0;
  uint8_t* current_ptr = nullptr;

  do {
    // Calculate space needed for header + alignment
    current_ptr = static_cast<uint8_t*>(buffer_) + current_offset;
    header_padding = CalculatePaddingWithHeader(current_ptr, alignment, sizeof(AllocationHeader));
    const size_t required_space = header_padding + size;

    // Check if we have enough space
    if (current_offset + required_space > capacity_.load(std::memory_order_relaxed)) {
      return {.ptr = nullptr, .allocated_size = 0};
    }

    new_offset = current_offset + required_space;

    // Try to atomically update offset
  } while (
      !offset_.compare_exchange_weak(current_offset, new_offset, std::memory_order_release, std::memory_order_acquire));

  // Successfully reserved space - now write the header
  // This is safe because we own the range [current_offset, new_offset)
  auto* header_ptr = current_ptr + header_padding - sizeof(AllocationHeader);
  auto* header = reinterpret_cast<AllocationHeader*>(header_ptr);
  header->previous_offset = current_offset;
  header->padding = header_padding;

  // Update stats
  allocation_count_.fetch_add(1, std::memory_order_relaxed);
  total_allocations_.fetch_add(1, std::memory_order_relaxed);
  alignment_waste_.fetch_add(header_padding - sizeof(AllocationHeader), std::memory_order_relaxed);

  // Update peak offset
  size_t current_peak = peak_offset_.load(std::memory_order_acquire);
  while (new_offset > current_peak) {
    if (peak_offset_.compare_exchange_weak(current_peak, new_offset, std::memory_order_release,
                                           std::memory_order_acquire)) {
      break;
    }
  }

  // Calculate aligned data pointer
  uint8_t* data_ptr = current_ptr + header_padding;
  return {data_ptr, size};
}

inline void StackAllocator::Deallocate(void* ptr, [[maybe_unused]] size_t size) noexcept {
  if (ptr == nullptr) [[unlikely]] {
    return;
  }

  HELIOS_ASSERT(Owns(ptr), "Failed to deallocate memory: ptr does not belong to this allocator!");

  // Get the header stored before the allocation
  auto* header = reinterpret_cast<AllocationHeader*>(static_cast<uint8_t*>(ptr) - sizeof(AllocationHeader));

#ifdef HELIOS_DEBUG_MODE
  {
    // Verify this is the most recent allocation (LIFO check)
    const size_t current_offset = offset_.load(std::memory_order_acquire);
    HELIOS_ASSERT(static_cast<uint8_t*>(ptr) + size <= static_cast<uint8_t*>(buffer_) + current_offset,
                  "Failed to deallocate memory:  Deallocation violates LIFO order!");
  }
#endif

  // Rewind to previous offset
  // Note: LIFO deallocations are expected to be single-threaded per stack or externally synchronized
  offset_.store(header->previous_offset, std::memory_order_release);

  // Update stats
  allocation_count_.fetch_sub(1, std::memory_order_relaxed);
  total_deallocations_.fetch_add(1, std::memory_order_relaxed);
}

inline void StackAllocator::Reset() noexcept {
  offset_.store(0, std::memory_order_release);
  allocation_count_.store(0, std::memory_order_release);
  alignment_waste_.store(0, std::memory_order_release);
}

inline void StackAllocator::RewindToMarker(size_t marker) noexcept {
  [[maybe_unused]] const size_t current_offset = offset_.load(std::memory_order_acquire);
  HELIOS_ASSERT(marker <= current_offset, "Failed to rewind to marker: marker '{}' is ahead of current offset '{}'!",
                marker, current_offset);

  const size_t capacity = capacity_.load(std::memory_order_relaxed);
  HELIOS_ASSERT(marker <= capacity, "Failed to rewind to marker: marker '{}' exceeds capacity '{}'!", marker, capacity);

  offset_.store(marker, std::memory_order_release);

  // Note: allocation_count_ becomes inaccurate after rewind, but that's acceptable
  // since we're doing bulk deallocation
}

inline bool StackAllocator::Owns(const void* ptr) const noexcept {
  if (ptr == nullptr || buffer_ == nullptr) {
    return false;
  }

  const uintptr_t start = reinterpret_cast<uintptr_t>(buffer_);
  const auto addr = reinterpret_cast<uintptr_t>(ptr);
  const uintptr_t end = start + capacity_.load(std::memory_order_relaxed);

  return addr >= start && addr < end;
}

inline AllocatorStats StackAllocator::Stats() const noexcept {
  const size_t current_offset = offset_.load(std::memory_order_relaxed);
  const size_t peak = peak_offset_.load(std::memory_order_relaxed);
  const size_t alloc_count = allocation_count_.load(std::memory_order_relaxed);
  const size_t total_allocs = total_allocations_.load(std::memory_order_relaxed);
  const size_t total_deallocs = total_deallocations_.load(std::memory_order_relaxed);
  const size_t waste = alignment_waste_.load(std::memory_order_relaxed);

  return {
      .total_allocated = current_offset,
      .total_freed = 0,
      .peak_usage = peak,
      .allocation_count = alloc_count,
      .total_allocations = total_allocs,
      .total_deallocations = total_deallocs,
      .alignment_waste = waste,
  };
}

inline size_t StackAllocator::FreeSpace() const noexcept {
  const size_t current = offset_.load(std::memory_order_relaxed);
  const size_t capacity = capacity_.load(std::memory_order_relaxed);
  return current < capacity ? capacity - current : 0;
}

template <typename T>
inline T* StackAllocator::Allocate() noexcept {
  constexpr size_t size = sizeof(T);
  constexpr size_t alignment = std::max(alignof(T), kMinAlignment);
  auto result = Allocate(size, alignment);
  return static_cast<T*>(result.ptr);
}

template <typename T>
inline T* StackAllocator::Allocate(size_t count) noexcept {
  if (count == 0) [[unlikely]] {
    return nullptr;
  }
  constexpr size_t alignment = std::max(alignof(T), kMinAlignment);
  const size_t size = sizeof(T) * count;
  auto result = Allocate(size, alignment);
  return static_cast<T*>(result.ptr);
}

template <typename T, typename... Args>
  requires std::constructible_from<T, Args...>
inline T* StackAllocator::AllocateAndConstruct(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
  T* ptr = Allocate<T>();
  if (ptr != nullptr) [[likely]] {
    std::construct_at(ptr, std::forward<Args>(args)...);
  }
  return ptr;
}

template <typename T>
  requires std::default_initializable<T>
inline T* StackAllocator::AllocateAndConstructArray(size_t count) noexcept(std::is_nothrow_default_constructible_v<T>) {
  T* ptr = Allocate<T>(count);
  if (ptr != nullptr) [[likely]] {
    for (size_t i = 0; i < count; ++i) {
      std::construct_at(ptr + i);
    }
  }
  return ptr;
}

}  // namespace helios::memory
