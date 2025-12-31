#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/memory/allocator_traits.hpp>
#include <helios/core/memory/frame_allocator.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <memory>
#include <utility>

namespace helios::memory {

/**
 * @brief N-buffered frame allocator.
 * @details Maintains N frame buffers, allowing memory from the previous N-1 frames to remain valid.
 * Useful for pipelined operations (e.g., CPU-GPU synchronization with multiple frames in flight).
 *
 * The allocator cycles through N buffers, ensuring that data from the previous
 * N-1 frames remains accessible while allocating for the current frame.
 *
 * @note Thread-safe.
 * Previous N-1 frames' data remains valid until the buffer cycles back.
 *
 * @tparam N Number of frame buffers (must be > 0)
 */
template <size_t N>
  requires(N > 0)
class NFrameAllocator final {
public:
  static constexpr size_t kBufferCount = N;

  /**
   * @brief Constructs an N-frame allocator with specified capacity per buffer.
   * @warning Triggers assertion if capacity_per_buffer is 0.
   * @param capacity_per_buffer Size of each buffer in bytes
   */
  explicit NFrameAllocator(size_t capacity_per_buffer) : allocators_(CreateAllocators(capacity_per_buffer)) {}
  NFrameAllocator(const NFrameAllocator&) = delete;
  NFrameAllocator(NFrameAllocator&& other) noexcept;
  ~NFrameAllocator() noexcept = default;

  NFrameAllocator& operator=(const NFrameAllocator&) = delete;
  NFrameAllocator& operator=(NFrameAllocator&& other) noexcept;

  /**
   * @brief Allocates memory from the current frame buffer.
   * @warning Triggers assertion in next cases:
   * - Alignment is not a power of 2.
   * - Alignment is less than kMinAlignment.
   * @param size Number of bytes to allocate
   * @param alignment Alignment requirement (must be power of 2)
   * @return AllocationResult with pointer and actual allocated size, or {nullptr, 0} on failure
   */
  [[nodiscard]] AllocationResult Allocate(size_t size, size_t alignment = kDefaultAlignment) noexcept {
    return allocators_[current_buffer_.load(std::memory_order_acquire)].Allocate(size, alignment);
  }

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
   * @brief Advances to the next frame, cycling through buffers.
   * @details Resets the new current buffer and makes previous buffers accessible.
   * @warning Not thread-safe with Allocate(). Must be called from a single thread while
   * no other threads are allocating. Typically called once per frame by the main thread.
   */
  void NextFrame() noexcept;

  /**
   * @brief Resets all buffers.
   * @details Clears all allocations from all buffers.
   */
  void Reset() noexcept;

  /**
   * @brief Gets combined statistics for all buffers.
   * @return AllocatorStats with combined usage information
   */
  [[nodiscard]] AllocatorStats Stats() const noexcept;

  /**
   * @brief Gets statistics for the current frame buffer.
   * @return AllocatorStats for current buffer
   */
  [[nodiscard]] AllocatorStats CurrentFrameStats() const noexcept {
    return allocators_[current_buffer_.load(std::memory_order_acquire)].Stats();
  }

  /**
   * @brief Gets statistics for a specific buffer.
   * @warning Triggers assertion if buffer_index is out of range [0, N).
   * @param buffer_index Buffer index (0 to N-1)
   * @return AllocatorStats for specified buffer
   */
  [[nodiscard]] AllocatorStats BufferStats(size_t buffer_index) const noexcept;

  /**
   * @brief Gets the total capacity across all buffers.
   * @return Total capacity in bytes
   */
  [[nodiscard]] size_t Capacity() const noexcept;

  /**
   * @brief Gets the current frame buffer index.
   * @return Current buffer index (0 to N-1)
   */
  [[nodiscard]] size_t CurrentBufferIndex() const noexcept { return current_buffer_.load(std::memory_order_relaxed); }

  /**
   * @brief Gets free space in current buffer.
   * @return Free space in bytes
   */
  [[nodiscard]] size_t FreeSpace() const noexcept {
    return allocators_[current_buffer_.load(std::memory_order_acquire)].FreeSpace();
  }

  /**
   * @brief Gets the number of buffers.
   * @return Number of buffers (N)
   */
  [[nodiscard]] static constexpr size_t BufferCount() noexcept { return N; }

private:
  /**
   * @brief Helper to create array of allocators.
   */
  [[nodiscard]] static auto CreateAllocators(size_t capacity_per_buffer) -> std::array<FrameAllocator, N>;

  std::array<FrameAllocator, N> allocators_;  ///< N frame allocators
  std::atomic<size_t> current_buffer_{0};     ///< Current buffer index
};

template <size_t N>
  requires(N > 0)
inline NFrameAllocator<N>::NFrameAllocator(NFrameAllocator&& other) noexcept
    : allocators_(std::move(other.allocators_)),
      current_buffer_(other.current_buffer_.load(std::memory_order_acquire)) {
  other.current_buffer_.store(0, std::memory_order_release);
}

template <size_t N>
  requires(N > 0)
inline auto NFrameAllocator<N>::operator=(NFrameAllocator&& other) noexcept -> NFrameAllocator<N>& {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  allocators_ = std::move(other.allocators_);
  current_buffer_.store(other.current_buffer_.load(std::memory_order_acquire), std::memory_order_release);
  other.current_buffer_.store(0, std::memory_order_release);

  return *this;
}

template <size_t N>
  requires(N > 0)
template <typename T>
inline T* NFrameAllocator<N>::Allocate() noexcept {
  constexpr size_t size = sizeof(T);
  constexpr size_t alignment = std::max(alignof(T), kMinAlignment);
  auto result = Allocate(size, alignment);
  return static_cast<T*>(result.ptr);
}

template <size_t N>
  requires(N > 0)
template <typename T>
inline T* NFrameAllocator<N>::Allocate(size_t count) noexcept {
  if (count == 0) [[unlikely]] {
    return nullptr;
  }
  constexpr size_t alignment = std::max(alignof(T), kMinAlignment);
  const size_t size = sizeof(T) * count;
  auto result = Allocate(size, alignment);
  return static_cast<T*>(result.ptr);
}

template <size_t N>
  requires(N > 0)
template <typename T, typename... Args>
  requires std::constructible_from<T, Args...>
inline T* NFrameAllocator<N>::AllocateAndConstruct(Args&&... args) noexcept(
    std::is_nothrow_constructible_v<T, Args...>) {
  T* ptr = Allocate<T>();
  if (ptr != nullptr) [[likely]] {
    std::construct_at(ptr, std::forward<Args>(args)...);
  }
  return ptr;
}

template <size_t N>
  requires(N > 0)
template <typename T>
  requires std::default_initializable<T>
inline T* NFrameAllocator<N>::AllocateAndConstructArray(size_t count) noexcept(
    std::is_nothrow_default_constructible_v<T>) {
  T* ptr = Allocate<T>(count);
  if (ptr != nullptr) [[likely]] {
    for (size_t i = 0; i < count; ++i) {
      std::construct_at(ptr + i);
    }
  }
  return ptr;
}

template <size_t N>
  requires(N > 0)
inline void NFrameAllocator<N>::NextFrame() noexcept {
  // Advance to next buffer (wrapping around)
  const size_t buffer = (current_buffer_.load(std::memory_order_relaxed) + 1) % N;

  // Reset the new current buffer before switching
  allocators_[buffer].Reset();

  // Switch to new buffer
  current_buffer_.store(buffer, std::memory_order_release);
}

template <size_t N>
  requires(N > 0)
inline void NFrameAllocator<N>::Reset() noexcept {
  for (auto& allocator : allocators_) {
    allocator.Reset();
  }
}

template <size_t N>
  requires(N > 0)
inline AllocatorStats NFrameAllocator<N>::Stats() const noexcept {
  AllocatorStats combined{};

  for (const auto& allocator : allocators_) {
    const auto stats = allocator.Stats();
    combined.total_allocated += stats.total_allocated;
    combined.total_freed += stats.total_freed;
    combined.peak_usage = std::max(combined.peak_usage, stats.peak_usage);
    combined.allocation_count += stats.allocation_count;
    combined.total_allocations += stats.total_allocations;
    combined.total_deallocations += stats.total_deallocations;
    combined.alignment_waste += stats.alignment_waste;
  }

  return combined;
}

template <size_t N>
  requires(N > 0)
inline AllocatorStats NFrameAllocator<N>::BufferStats(size_t buffer_index) const noexcept {
  HELIOS_ASSERT(buffer_index < N, "Failed to get buffer stats: buffer_index '{}' is out of range [0, {}]!",
                buffer_index, N);
  return allocators_[buffer_index].Stats();
}

template <size_t N>
  requires(N > 0)
inline size_t NFrameAllocator<N>::Capacity() const noexcept {
  size_t total = 0;
  for (const auto& allocator : allocators_) {
    total += allocator.Capacity();
  }
  return total;
}

template <size_t N>
  requires(N > 0)
inline auto NFrameAllocator<N>::CreateAllocators(size_t capacity_per_buffer) -> std::array<FrameAllocator, N> {
  // Helper to construct array with compile-time recursion
  auto construct = [capacity_per_buffer]<size_t... Is>(std::index_sequence<Is...>) -> std::array<FrameAllocator, N> {
    return {(static_cast<void>(Is), FrameAllocator(capacity_per_buffer))...};
  };
  return construct(std::make_index_sequence<N>{});
}

using TripleFrameAllocator = NFrameAllocator<3>;  ///< Triple-buffered frame allocator
using QuadFrameAllocator = NFrameAllocator<4>;    ///< Quad-buffered frame allocator

}  // namespace helios::memory
