#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/memory/allocator_traits.hpp>
#include <helios/core/memory/common.hpp>

#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace helios::memory {

/**
 * @brief Linear allocator that clears every frame.
 * @details Fast bump-pointer allocator for per-frame temporary allocations.
 * Extremely efficient for short-lived allocations that don't need individual deallocation.
 * All memory is freed at once when Reset() is called (typically at frame end).
 *
 * Uses atomic operations for allocation offset tracking.
 *
 * Ideal for temporary data that lives for a single frame.
 *
 * @note Thread-safe.
 * Deallocation is a no-op - memory is only freed on Reset().
 * @warning Data allocated with this allocator is only valid until Reset() is called.
 * All pointers and references to allocated memory become invalid after Reset().
 * Do not store frame-allocated data in persistent storage (components, resources, etc.).
 */
class FrameAllocator final {
public:
  /**
   * @brief Constructs a frame allocator with specified capacity.
   * @warning Triggers assertion if capacity is 0.
   * @param capacity Total size of the memory buffer in bytes
   */
  explicit FrameAllocator(size_t capacity);
  FrameAllocator(const FrameAllocator&) = delete;
  FrameAllocator(FrameAllocator&& other) noexcept;
  ~FrameAllocator() noexcept;

  FrameAllocator& operator=(const FrameAllocator&) = delete;
  FrameAllocator& operator=(FrameAllocator&& other) noexcept;

  /**
   * @brief Allocates memory with specified size and alignment.
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
   * FrameAllocator alloc(1024);
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
   * FrameAllocator alloc(1024);
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
   * FrameAllocator alloc(1024);
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
   * FrameAllocator alloc(1024);
   * auto* arr = alloc.AllocateAndConstructArray<MyType>(10);
   * @endcode
   */
  template <typename T>
    requires std::default_initializable<T>
  [[nodiscard]] T* AllocateAndConstructArray(size_t count) noexcept(std::is_nothrow_default_constructible_v<T>);

  /**
   * @brief Resets the allocator, freeing all allocations.
   * @details Resets the internal offset to 0, effectively freeing all memory.
   * Does not actually free or zero the underlying buffer.
   * @warning All pointers obtained from this allocator become invalid after this call.
   * Do not store references or pointers to frame-allocated data beyond the current frame.
   */
  void Reset() noexcept;

  /**
   * @brief Checks if the allocator is empty.
   * @return True if no allocations have been made since last reset
   */
  [[nodiscard]] bool Empty() const noexcept { return offset_.load(std::memory_order_acquire) == 0; }

  /**
   * @brief Checks if the allocator is full.
   * @return True if no more allocations can be made without reset
   */
  [[nodiscard]] bool Full() const noexcept { return offset_.load(std::memory_order_acquire) >= capacity_; }

  /**
   * @brief Gets current allocator statistics.
   * @return AllocatorStats with current usage information
   */
  [[nodiscard]] AllocatorStats Stats() const noexcept;

  /**
   * @brief Gets the total capacity of the allocator.
   * @return Capacity in bytes
   */
  [[nodiscard]] size_t Capacity() const noexcept { return capacity_; }

  /**
   * @brief Gets the current offset (amount of memory used).
   * @return Current offset in bytes
   */
  [[nodiscard]] size_t CurrentOffset() const noexcept { return offset_.load(std::memory_order_acquire); }

  /**
   * @brief Gets the amount of free space remaining.
   * @return Free space in bytes
   */
  [[nodiscard]] size_t FreeSpace() const noexcept;

private:
  void* buffer_ = nullptr;                   ///< Underlying memory buffer
  size_t capacity_ = 0;                      ///< Total capacity in bytes
  std::atomic<size_t> offset_{0};            ///< Current allocation offset
  std::atomic<size_t> peak_offset_{0};       ///< Peak offset reached
  std::atomic<size_t> allocation_count_{0};  ///< Total number of allocations made
  std::atomic<size_t> alignment_waste_{0};   ///< Total bytes wasted due to alignment
};

inline FrameAllocator::FrameAllocator(size_t capacity) : capacity_(capacity) {
  HELIOS_ASSERT(capacity > 0, "Failed to construct FrameAllocator: capacity must be greater than 0!");

  // Allocate aligned buffer
  buffer_ = AlignedAlloc(kDefaultAlignment, capacity_);
  HELIOS_VERIFY(buffer_ != nullptr, "Failed to construct FrameAllocator: Allocation of buffer failed!");
}

inline FrameAllocator::FrameAllocator(FrameAllocator&& other) noexcept
    : buffer_(other.buffer_),
      capacity_(other.capacity_),
      offset_(other.offset_.load(std::memory_order_acquire)),
      peak_offset_(other.peak_offset_.load(std::memory_order_acquire)),
      allocation_count_(other.allocation_count_.load(std::memory_order_acquire)),
      alignment_waste_(other.alignment_waste_.load(std::memory_order_acquire)) {
  other.buffer_ = nullptr;
  other.capacity_ = 0;
  other.offset_.store(0, std::memory_order_release);
  other.peak_offset_.store(0, std::memory_order_release);
  other.allocation_count_.store(0, std::memory_order_release);
  other.alignment_waste_.store(0, std::memory_order_release);
}

inline FrameAllocator::~FrameAllocator() noexcept {
  if (buffer_ != nullptr) {
    AlignedFree(buffer_);
  }
}

inline FrameAllocator& FrameAllocator::operator=(FrameAllocator&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  // Free current buffer
  if (buffer_ != nullptr) {
    AlignedFree(buffer_);
  }

  // Move from other
  buffer_ = other.buffer_;
  capacity_ = other.capacity_;
  offset_.store(other.offset_.load(std::memory_order_acquire), std::memory_order_release);
  peak_offset_.store(other.peak_offset_.load(std::memory_order_acquire), std::memory_order_release);
  allocation_count_.store(other.allocation_count_.load(std::memory_order_acquire), std::memory_order_release);
  alignment_waste_.store(other.alignment_waste_.load(std::memory_order_acquire), std::memory_order_release);

  // Reset other
  other.buffer_ = nullptr;
  other.capacity_ = 0;
  other.offset_.store(0, std::memory_order_release);
  other.peak_offset_.store(0, std::memory_order_release);
  other.allocation_count_.store(0, std::memory_order_release);
  other.alignment_waste_.store(0, std::memory_order_release);

  return *this;
}

inline AllocationResult FrameAllocator::Allocate(size_t size, size_t alignment) noexcept {
  HELIOS_ASSERT(IsPowerOfTwo(alignment), "Failed to allocate memory: alignment must be power of 2, got '{}'!",
                alignment);
  HELIOS_ASSERT(alignment >= kMinAlignment, "Failed to allocate memory: alignment must be at least '{}', got '{}'!",
                kMinAlignment, alignment);

  if (size == 0) [[unlikely]] {
    return {.ptr = nullptr, .allocated_size = 0};
  }

  // Atomically allocate using compare-and-swap
  size_t current_offset = offset_.load(std::memory_order_acquire);
  size_t aligned_offset = 0;
  size_t new_offset = 0;
  size_t padding = 0;

  do {
    // Calculate aligned offset
    auto* current_ptr = static_cast<uint8_t*>(buffer_) + current_offset;
    padding = CalculatePadding(current_ptr, alignment);
    aligned_offset = current_offset + padding;

    // Check if we have enough space
    if (aligned_offset + size > capacity_) {
      return {.ptr = nullptr, .allocated_size = 0};
    }

    new_offset = aligned_offset + size;

    // Try to atomically update offset
  } while (
      !offset_.compare_exchange_weak(current_offset, new_offset, std::memory_order_release, std::memory_order_acquire));

  // Update stats (these don't need to be perfectly accurate)
  allocation_count_.fetch_add(1, std::memory_order_relaxed);
  alignment_waste_.fetch_add(padding, std::memory_order_relaxed);

  // Update peak offset
  size_t current_peak = peak_offset_.load(std::memory_order_acquire);
  while (new_offset > current_peak) {
    if (peak_offset_.compare_exchange_weak(current_peak, new_offset, std::memory_order_release,
                                           std::memory_order_acquire)) {
      break;
    }
  }

  void* result = static_cast<uint8_t*>(buffer_) + aligned_offset;
  return {.ptr = result, .allocated_size = size};
}

inline void FrameAllocator::Reset() noexcept {
  offset_.store(0, std::memory_order_release);
  alignment_waste_.store(0, std::memory_order_release);
  allocation_count_.store(0, std::memory_order_release);
}

inline AllocatorStats FrameAllocator::Stats() const noexcept {
  const size_t current_offset = offset_.load(std::memory_order_acquire);
  const size_t peak = peak_offset_.load(std::memory_order_acquire);
  const size_t alloc_count = allocation_count_.load(std::memory_order_acquire);
  const size_t waste = alignment_waste_.load(std::memory_order_acquire);

  return {
      .total_allocated = current_offset,
      .total_freed = 0,
      .peak_usage = peak,
      .allocation_count = alloc_count,
      .total_allocations = alloc_count,
      .total_deallocations = 0,
      .alignment_waste = waste,
  };
}

inline size_t FrameAllocator::FreeSpace() const noexcept {
  const size_t current = offset_.load(std::memory_order_acquire);
  return current < capacity_ ? capacity_ - current : 0;
}

template <typename T>
inline T* FrameAllocator::Allocate() noexcept {
  constexpr size_t size = sizeof(T);
  constexpr size_t alignment = std::max(alignof(T), kMinAlignment);
  auto result = Allocate(size, alignment);
  return static_cast<T*>(result.ptr);
}

template <typename T>
inline T* FrameAllocator::Allocate(size_t count) noexcept {
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
inline T* FrameAllocator::AllocateAndConstruct(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
  T* ptr = Allocate<T>();
  if (ptr != nullptr) {
    ::new (static_cast<void*>(ptr)) T(std::forward<Args>(args)...);
  }
  return ptr;
}

template <typename T>
  requires std::default_initializable<T>
inline T* FrameAllocator::AllocateAndConstructArray(size_t count) noexcept(std::is_nothrow_default_constructible_v<T>) {
  T* ptr = Allocate<T>(count);
  if (ptr != nullptr) {
    for (size_t i = 0; i < count; ++i) {
      ::new (static_cast<void*>(ptr + i)) T{};
    }
  }
  return ptr;
}

}  // namespace helios::memory
