#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/memory/allocator_traits.hpp>
#include <helios/core/memory/frame_allocator.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <utility>

namespace helios::memory {

/**
 * @brief Double-buffered frame allocator.
 * @details Maintains two frame buffers, allowing memory from the previous frame
 * to remain valid while allocating for the current frame.
 * Useful when data needs to be accessible for one additional frame (e.g., GPU upload buffers, interpolation).
 *
 * The allocator automatically switches between buffers on each frame.
 *
 * Uses atomic for buffer index and mutex only for frame transitions.
 * Previous frame's data remains valid until the next frame begins.
 * Allocations are lock-free since FrameAllocator uses atomic operations internally.
 *
 * @note Thread-safe.
 */
class DoubleFrameAllocator final {
public:
  static constexpr size_t kBufferCount = 2;

  /**
   * @brief Constructs a double frame allocator with specified capacity per buffer.
   * @warning Triggers assertion if capacity_per_buffer is 0.
   * @param capacity_per_buffer Size of each buffer in bytes
   */
  explicit DoubleFrameAllocator(size_t capacity_per_buffer)
      : allocators_{FrameAllocator(capacity_per_buffer), FrameAllocator(capacity_per_buffer)} {}

  DoubleFrameAllocator(const DoubleFrameAllocator&) = delete;
  DoubleFrameAllocator(DoubleFrameAllocator&& other) noexcept;
  ~DoubleFrameAllocator() noexcept = default;

  DoubleFrameAllocator& operator=(const DoubleFrameAllocator&) = delete;
  DoubleFrameAllocator& operator=(DoubleFrameAllocator&& other) noexcept;

  /**
   * @brief Allocates memory from the current frame buffer.
   * @details This operation is lock-free as it only reads the atomic buffer index
   * and delegates to the thread-safe FrameAllocator.
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
   */
  template <typename T>
    requires std::default_initializable<T>
  [[nodiscard]] T* AllocateAndConstructArray(size_t count) noexcept(std::is_nothrow_default_constructible_v<T>);

  /**
   * @brief Advances to the next frame, switching buffers.
   * @details Resets the new current buffer and makes the old current buffer the previous buffer.
   */
  void NextFrame() noexcept;

  /**
   * @brief Resets both buffers.
   * @details Clears all allocations from both buffers.
   */
  void Reset() noexcept;

  /**
   * @brief Gets combined statistics for both buffers.
   * @return AllocatorStats with combined usage information
   */
  [[nodiscard]] AllocatorStats Stats() const noexcept;

  /**
   * @brief Gets statistics for the current frame buffer.
   * @return AllocatorStats for current buffer
   */
  [[nodiscard]] AllocatorStats CurrentFrameStats() const noexcept;

  /**
   * @brief Gets statistics for the previous frame buffer.
   * @return AllocatorStats for previous buffer
   */
  [[nodiscard]] AllocatorStats PreviousFrameStats() const noexcept;

  /**
   * @brief Gets the total capacity across both buffers.
   * @return Total capacity in bytes
   */
  [[nodiscard]] size_t Capacity() const noexcept;

  /**
   * @brief Gets the current frame buffer index.
   * @return Current buffer index (0 or 1)
   */
  [[nodiscard]] size_t CurrentBufferIndex() const noexcept;

  /**
   * @brief Gets the previous frame buffer index.
   * @return Previous buffer index (0 or 1)
   */
  [[nodiscard]] size_t PreviousBufferIndex() const noexcept;

  /**
   * @brief Gets free space in current buffer.
   * @return Free space in bytes
   */
  [[nodiscard]] size_t FreeSpace() const noexcept;

private:
  std::array<FrameAllocator, kBufferCount> allocators_;  ///< Two frame allocators
  std::atomic<size_t> current_buffer_{0};                ///< Current buffer index (atomic for lock-free reads)
  mutable std::shared_mutex mutex_;                      ///< Mutex for frame transitions only
};

inline DoubleFrameAllocator::DoubleFrameAllocator(DoubleFrameAllocator&& other) noexcept
    : allocators_(std::move(other.allocators_)),
      current_buffer_(other.current_buffer_.load(std::memory_order_acquire)) {
  other.current_buffer_.store(0, std::memory_order_release);
}

inline DoubleFrameAllocator& DoubleFrameAllocator::operator=(DoubleFrameAllocator&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  const std::scoped_lock lock(mutex_);
  const std::scoped_lock other_lock(other.mutex_);

  allocators_ = std::move(other.allocators_);
  current_buffer_.store(other.current_buffer_.load(std::memory_order_acquire), std::memory_order_release);
  other.current_buffer_.store(0, std::memory_order_release);

  return *this;
}

inline AllocationResult DoubleFrameAllocator::Allocate(size_t size, size_t alignment) noexcept {
  // Lock-free: current_buffer_ is atomic, FrameAllocator::Allocate is thread-safe via atomics
  const size_t buffer = current_buffer_.load(std::memory_order_acquire);
  return allocators_[buffer].Allocate(size, alignment);
}

inline void DoubleFrameAllocator::NextFrame() noexcept {
  const std::scoped_lock lock(mutex_);
  // Switch to the other buffer
  const size_t new_buffer = 1 - current_buffer_.load(std::memory_order_relaxed);
  // Reset the new current buffer before switching
  allocators_[new_buffer].Reset();
  // Atomically switch to new buffer
  current_buffer_.store(new_buffer, std::memory_order_release);
}

inline void DoubleFrameAllocator::Reset() noexcept {
  const std::scoped_lock lock(mutex_);
  allocators_.front().Reset();
  allocators_.back().Reset();
}

inline AllocatorStats DoubleFrameAllocator::Stats() const noexcept {
  const std::shared_lock lock(mutex_);
  const auto stats0 = allocators_.front().Stats();
  const auto stats1 = allocators_.back().Stats();

  return {
      .total_allocated = stats0.total_allocated + stats1.total_allocated,
      .total_freed = stats0.total_freed + stats1.total_freed,
      .peak_usage = std::max(stats0.peak_usage, stats1.peak_usage),
      .allocation_count = stats0.allocation_count + stats1.allocation_count,
      .total_allocations = stats0.total_allocations + stats1.total_allocations,
      .total_deallocations = stats0.total_deallocations + stats1.total_deallocations,
      .alignment_waste = stats0.alignment_waste + stats1.alignment_waste,
  };
}

inline AllocatorStats DoubleFrameAllocator::CurrentFrameStats() const noexcept {
  const size_t buffer = current_buffer_.load(std::memory_order_acquire);
  return allocators_[buffer].Stats();
}

inline AllocatorStats DoubleFrameAllocator::PreviousFrameStats() const noexcept {
  const size_t buffer = current_buffer_.load(std::memory_order_acquire);
  return allocators_[1 - buffer].Stats();
}

inline size_t DoubleFrameAllocator::Capacity() const noexcept {
  // Capacity is immutable after construction, no lock needed
  return allocators_.front().Capacity() + allocators_.back().Capacity();
}

inline size_t DoubleFrameAllocator::CurrentBufferIndex() const noexcept {
  return current_buffer_.load(std::memory_order_acquire);
}

inline size_t DoubleFrameAllocator::PreviousBufferIndex() const noexcept {
  return 1 - current_buffer_.load(std::memory_order_acquire);
}

inline size_t DoubleFrameAllocator::FreeSpace() const noexcept {
  const size_t buffer = current_buffer_.load(std::memory_order_acquire);
  return allocators_[buffer].FreeSpace();
}

template <typename T>
inline T* DoubleFrameAllocator::Allocate() noexcept {
  constexpr size_t size = sizeof(T);
  constexpr size_t alignment = std::max(alignof(T), kMinAlignment);
  auto result = Allocate(size, alignment);
  return static_cast<T*>(result.ptr);
}

template <typename T>
inline T* DoubleFrameAllocator::Allocate(size_t count) noexcept {
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
inline T* DoubleFrameAllocator::AllocateAndConstruct(Args&&... args) noexcept(
    std::is_nothrow_constructible_v<T, Args...>) {
  T* ptr = Allocate<T>();
  if (ptr != nullptr) [[likely]] {
    std::construct_at(ptr, std::forward<Args>(args)...);
  }
  return ptr;
}

template <typename T>
  requires std::default_initializable<T>
inline T* DoubleFrameAllocator::AllocateAndConstructArray(size_t count) noexcept(
    std::is_nothrow_default_constructible_v<T>) {
  T* ptr = Allocate<T>(count);
  if (ptr != nullptr) [[likely]] {
    for (size_t i = 0; i < count; ++i) {
      std::construct_at(ptr + i);
    }
  }
  return ptr;
}

}  // namespace helios::memory
