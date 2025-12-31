#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/memory/allocator_traits.hpp>

#include <algorithm>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace helios::memory {

/**
 * @brief Lock-free, thread-safe arena allocator.
 * @details Arena allocator that allocates from a pre-allocated buffer using a bump-pointer strategy.
 * All allocations are performed with lock-free atomic operations on an internal offset.
 *
 * Memory is released only when the arena is reset, which is an O(1) operation.
 * Individual deallocations are not supported.
 *
 * This allocator is suitable for use as a backing allocator for higher level systems that require fast,
 * thread-safe allocation with predictable lifetime.
 *
 * All operations that modify the arena state (`Allocate`, `Reset`) use atomic operations.
 * `Reset` is not safe to call concurrently with `Allocate` and must be externally synchronized when used in that way.
 *
 * @note Thread-safe.
 * @warning `Reset` must not be used concurrently with active allocations.
 * The caller is responsible for enforcing this invariant.
 */
class ArenaAllocator final {
public:
  /**
   * @brief Constructs an arena allocator over an existing buffer.
   * @details The caller provides a raw buffer and its size.
   * The buffer must remain valid for the entire lifetime of the allocator.
   * Alignment guarantees are provided up to `kDefaultAlignment` (or higher alignment if the
   * buffer itself is appropriately aligned).
   *
   * The allocator does not take ownership of the buffer and will not free it.
   *
   * @warning Triggers assertion if:
   * - buffer is nullptr but size is non-zero.
   * - size is 0.
   * - buffer is not aligned to `kMinAlignment`.
   *
   * @param buffer Pointer to the backing memory buffer.
   * @param size Size of the buffer in bytes.
   */
  ArenaAllocator(void* buffer, size_t size) noexcept;
  ArenaAllocator(const ArenaAllocator&) = delete;
  ArenaAllocator(ArenaAllocator&& other) noexcept;
  ~ArenaAllocator() noexcept = default;

  ArenaAllocator& operator=(const ArenaAllocator&) = delete;
  ArenaAllocator& operator=(ArenaAllocator&& other) noexcept;

  /**
   * @brief Allocates a block of memory from the arena.
   * @details Uses a lock-free bump-pointer with `compare_exchange_weak` to reserve space from the backing buffer.
   * The returned memory is not initialized.
   *
   * @warning Triggers assertion in next cases:
   * - Alignment is not a power of 2.
   * - Alignment is less than `kMinAlignment`.
   *
   * @param size Number of bytes to allocate.
   * @param alignment Alignment requirement (must be power of 2).
   * @return AllocationResult with pointer and size, or `{nullptr, 0}` on failure.
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
   * ArenaAllocator alloc(buffer, size);
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
   * ArenaAllocator alloc(buffer, size);
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
   * ArenaAllocator alloc(buffer, size);
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
   * ArenaAllocator alloc(buffer, size);
   * auto* arr = alloc.AllocateAndConstructArray<MyType>(10);
   * @endcode
   */
  template <typename T>
    requires std::default_initializable<T>
  [[nodiscard]] T* AllocateAndConstructArray(size_t count) noexcept(std::is_nothrow_default_constructible_v<T>);

  /**
   * @brief Deallocation is a no-op.
   * @details Arena allocators do not support individual deallocation.
   * Memory is released only via `Reset`.
   * This function exists to satisfy generic allocator interfaces.
   * @param ptr Pointer previously returned by `Allocate` (may be nullptr).
   */
  void Deallocate(const void* /*ptr*/) noexcept {}

  /**
   * @brief Resets the arena, freeing all allocations.
   * @details Sets the internal offset to zero and clears accounting statistics.
   * This does not modify the contents of the underlying buffer.
   * @warning Must not be called concurrently with `Allocate`.
   * The caller must ensure there are no ongoing or future allocations that expect previous pointers to remain valid.
   */
  void Reset() noexcept;

  /**
   * @brief Checks if the arena is empty.
   * @return True if no allocations have been made since the last reset.
   */
  [[nodiscard]] bool Empty() const noexcept { return offset_.load(std::memory_order_relaxed) == 0; }

  /**
   * @brief Checks if the arena is full.
   * @return True if no more allocations can be made without reset.
   */
  [[nodiscard]] bool Full() const noexcept { return offset_.load(std::memory_order_relaxed) >= capacity_; }

  /**
   * @brief Gets current allocator statistics.
   * @details Statistics are updated in a relaxed manner and are not guaranteed to be
   * perfectly precise under heavy contention, but are sufficient for profiling and diagnostics.
   * @return AllocatorStats with current usage information.
   */
  [[nodiscard]] AllocatorStats Stats() const noexcept;

  /**
   * @brief Gets the total capacity of the arena.
   * @return Capacity in bytes.
   */
  [[nodiscard]] size_t Capacity() const noexcept { return capacity_; }

  /**
   * @brief Gets the current offset (amount of memory used).
   * @return Current offset in bytes.
   */
  [[nodiscard]] size_t CurrentOffset() const noexcept { return offset_.load(std::memory_order_relaxed); }

  /**
   * @brief Gets the amount of free space remaining.
   * @return Free space in bytes.
   */
  [[nodiscard]] size_t FreeSpace() const noexcept;

  /**
   * @brief Gets the raw backing buffer pointer.
   * @return Pointer to the beginning of the backing buffer.
   */
  [[nodiscard]] void* Data() noexcept { return buffer_; }

  /**
   * @brief Gets the raw backing buffer pointer (const).
   * @return Const pointer to the beginning of the backing buffer.
   */
  [[nodiscard]] const void* Data() const noexcept { return buffer_; }

private:
  void* buffer_ = nullptr;                   ///< Backing memory buffer (non-owning).
  size_t capacity_ = 0;                      ///< Total capacity in bytes.
  std::atomic<size_t> offset_{0};            ///< Current bump offset.
  std::atomic<size_t> peak_offset_{0};       ///< Peak offset reached.
  std::atomic<size_t> allocation_count_{0};  ///< Number of successful allocations.
  std::atomic<size_t> alignment_waste_{0};   ///< Total bytes wasted due to alignment.
};

inline ArenaAllocator::ArenaAllocator(void* buffer, size_t size) noexcept : buffer_(buffer), capacity_(size) {
  HELIOS_ASSERT(size > 0, "Failed to construct ArenaAllocator: size must be greater than 0!");
  HELIOS_ASSERT(buffer != nullptr, "Failed to construct ArenaAllocator: buffer must not be nullptr!");
  HELIOS_ASSERT(CalculatePadding(buffer_, kMinAlignment) == 0,
                "Failed to construct ArenaAllocator: buffer must be at least {}-byte aligned!", kMinAlignment);

  offset_.store(0, std::memory_order_release);
  peak_offset_.store(0, std::memory_order_release);
  allocation_count_.store(0, std::memory_order_release);
  alignment_waste_.store(0, std::memory_order_release);
}

inline ArenaAllocator::ArenaAllocator(ArenaAllocator&& other) noexcept
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

inline ArenaAllocator& ArenaAllocator::operator=(ArenaAllocator&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  buffer_ = other.buffer_;
  capacity_ = other.capacity_;
  offset_.store(other.offset_.load(std::memory_order_acquire), std::memory_order_release);
  peak_offset_.store(other.peak_offset_.load(std::memory_order_acquire), std::memory_order_release);
  allocation_count_.store(other.allocation_count_.load(std::memory_order_acquire), std::memory_order_release);
  alignment_waste_.store(other.alignment_waste_.load(std::memory_order_acquire), std::memory_order_release);

  other.buffer_ = nullptr;
  other.capacity_ = 0;
  other.offset_.store(0, std::memory_order_release);
  other.peak_offset_.store(0, std::memory_order_release);
  other.allocation_count_.store(0, std::memory_order_release);
  other.alignment_waste_.store(0, std::memory_order_release);

  return *this;
}

inline AllocationResult ArenaAllocator::Allocate(size_t size, size_t alignment) noexcept {
  HELIOS_ASSERT(IsPowerOfTwo(alignment), "ArenaAllocator::Allocate failed: alignment must be power of 2, got '{}'!",
                alignment);
  HELIOS_ASSERT(alignment >= kMinAlignment,
                "ArenaAllocator::Allocate failed: alignment must be at least '{}', got '{}'!", kMinAlignment,
                alignment);

  if (size == 0) [[unlikely]] {
    return {.ptr = nullptr, .allocated_size = 0};
  }

  // Lock-free bump-pointer allocation.
  size_t current_offset = offset_.load(std::memory_order_acquire);
  size_t aligned_offset = 0;
  size_t new_offset = 0;
  size_t padding = 0;

  do {
    auto* current_ptr = static_cast<uint8_t*>(buffer_) + current_offset;
    padding = CalculatePadding(current_ptr, alignment);
    aligned_offset = current_offset + padding;

    if (aligned_offset + size > capacity_) {
      return {.ptr = nullptr, .allocated_size = 0};
    }

    new_offset = aligned_offset + size;
  } while (
      !offset_.compare_exchange_weak(current_offset, new_offset, std::memory_order_release, std::memory_order_acquire));

  allocation_count_.fetch_add(1, std::memory_order_relaxed);
  alignment_waste_.fetch_add(padding, std::memory_order_relaxed);

  size_t current_peak = peak_offset_.load(std::memory_order_acquire);
  while (new_offset > current_peak) {
    if (peak_offset_.compare_exchange_weak(current_peak, new_offset, std::memory_order_release,
                                           std::memory_order_acquire)) {
      break;
    }
  }

  auto* result = static_cast<uint8_t*>(buffer_) + aligned_offset;
  return {.ptr = result, .allocated_size = size};
}

inline void ArenaAllocator::Reset() noexcept {
  offset_.store(0, std::memory_order_release);
  alignment_waste_.store(0, std::memory_order_release);
  allocation_count_.store(0, std::memory_order_release);
  // We intentionally keep peak_offset_ to track high-water mark over lifetime.
}

inline AllocatorStats ArenaAllocator::Stats() const noexcept {
  const auto current_offset = offset_.load(std::memory_order_relaxed);
  const auto peak = peak_offset_.load(std::memory_order_relaxed);
  const auto alloc_count = allocation_count_.load(std::memory_order_relaxed);
  const auto waste = alignment_waste_.load(std::memory_order_relaxed);

  // In an arena, we conceptually "free" everything on reset, but we do not track per-block frees.
  // total_freed is modeled as 0; deallocations count remains 0 because `Deallocate` is a no-op.
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

inline size_t ArenaAllocator::FreeSpace() const noexcept {
  const auto current = offset_.load(std::memory_order_relaxed);
  return current < capacity_ ? capacity_ - current : 0;
}

template <typename T>
inline T* ArenaAllocator::Allocate() noexcept {
  constexpr size_t size = sizeof(T);
  constexpr size_t alignment = std::max(alignof(T), kMinAlignment);
  auto result = Allocate(size, alignment);
  return static_cast<T*>(result.ptr);
}

template <typename T>
inline T* ArenaAllocator::Allocate(size_t count) noexcept {
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
inline T* ArenaAllocator::AllocateAndConstruct(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
  T* ptr = Allocate<T>();
  if (ptr != nullptr) [[likely]] {
    std::construct_at(ptr, std::forward<Args>(args)...);
  }
  return ptr;
}

template <typename T>
  requires std::default_initializable<T>
inline T* ArenaAllocator::AllocateAndConstructArray(size_t count) noexcept(std::is_nothrow_default_constructible_v<T>) {
  T* ptr = Allocate<T>(count);
  if (ptr != nullptr) [[likely]] {
    for (size_t i = 0; i < count; ++i) {
      std::construct_at(ptr + i);
    }
  }
  return ptr;
}

}  // namespace helios::memory
