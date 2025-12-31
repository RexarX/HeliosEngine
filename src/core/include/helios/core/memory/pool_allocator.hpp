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
 * @brief Pool allocator for fixed-size allocations.
 * @details Allocates objects of a fixed size from a pre-allocated pool.
 * Extremely efficient for scenarios where many objects of the same size are allocated and deallocated frequently.
 *
 * Uses a lock-free free list to track available slots. Each free slot stores a pointer to the next free slot,
 * forming a linked list through the free blocks.
 *
 * Supports proper deallocation and reuse of freed blocks.
 *
 * Uses lock-free atomic operations for allocation/deallocation,
 * providing excellent performance in multi-threaded scenarios.
 *
 * @note Thread-safe with lock-free operations.
 * All allocations must be the same size (or smaller than block_size).
 */
class PoolAllocator final {
public:
  /**
   * @brief Creates a pool allocator sized for type T.
   * @tparam T Type to allocate
   * @param block_count Number of blocks to allocate
   * @return PoolAllocator configured for type T
   *
   * @example
   * @code
   * auto pool = PoolAllocator::ForType<Entity>(1000);
   * @endcode
   */
  template <typename T>
  [[nodiscard]] static PoolAllocator ForType(size_t block_count) {
    constexpr size_t min_alignment = alignof(void*);
    constexpr size_t type_alignment = alignof(T);
    constexpr size_t alignment = type_alignment > min_alignment ? type_alignment : min_alignment;
    return {sizeof(T), block_count, alignment};
  }

  /**
   * @brief Constructs a pool allocator.
   * @warning Triggers assertion in next cases:
   * - Block count is 0.
   * - Alignment is not a power of 2.
   * - Alignment is less than alignof(void*).
   * @param block_size Size of each block in bytes (minimum 8 bytes for free list pointer)
   * @param block_count Number of blocks to allocate
   * @param alignment Alignment for each block (must be power of 2)
   */
  PoolAllocator(size_t block_size, size_t block_count, size_t alignment = kDefaultAlignment);
  PoolAllocator(const PoolAllocator&) = delete;
  PoolAllocator(PoolAllocator&& other) noexcept;
  ~PoolAllocator() noexcept;

  PoolAllocator& operator=(const PoolAllocator&) = delete;
  PoolAllocator& operator=(PoolAllocator&& other) noexcept;

  /**
   * @brief Allocates a block from the pool.
   * @note Size is ignored but kept for interface compatibility.
   * @warning Triggers assertion if size exceeds block_size.
   * @param size Number of bytes to allocate (must be <= block_size)
   * @return AllocationResult with pointer and actual allocated size, or {nullptr, 0} on failure
   */
  [[nodiscard]] AllocationResult Allocate(size_t size) noexcept;

  /**
   * @brief Allocates memory for a single object of type T.
   * @details Convenience function that allocates a single block for type T.
   * The returned memory is uninitialized - use placement new to construct the object.
   * @warning Triggers assertion if sizeof(T) exceeds block_size.
   * @tparam T Type to allocate memory for
   * @return Pointer to allocated memory, or nullptr on failure
   *
   * @example
   * @code
   * auto pool = PoolAllocator::ForType<Entity>(100);
   * Entity* ptr = pool.Allocate<Entity>();
   * if (ptr != nullptr) {
   *   new (ptr) Entity(42);
   * }
   * @endcode
   */
  template <typename T>
  [[nodiscard]] T* Allocate() noexcept;

  /**
   * @brief Allocates and constructs a single object of type T.
   * @details Convenience function that allocates memory and constructs the object in-place.
   * @warning Triggers assertion if sizeof(T) exceeds block_size.
   * @tparam T Type to allocate and construct
   * @tparam Args Constructor argument types
   * @param args Arguments to forward to T's constructor
   * @return Pointer to constructed object, or nullptr on allocation failure
   *
   * @example
   * @code
   * auto pool = PoolAllocator::ForType<MyVec3>(100);
   * auto* vec = pool.AllocateAndConstruct<MyVec3>(1.0f, 2.0f, 3.0f);
   * @endcode
   */
  template <typename T, typename... Args>
    requires std::constructible_from<T, Args...>
  [[nodiscard]] T* AllocateAndConstruct(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>);

  /**
   * @brief Deallocates a block back to the pool.
   * @note Returns the block to the free list for reuse.
   * @warning Triggers assertion if pointer does not belong to this pool.
   * @param ptr Pointer to deallocate (must have been allocated from this pool)
   */
  void Deallocate(void* ptr) noexcept;

  /**
   * @brief Resets the pool, making all blocks available.
   * @details Rebuilds the free list, invalidating all current allocations.
   */
  void Reset() noexcept;

  /**
   * @brief Checks if the allocator is full.
   * @return True if all blocks are allocated
   */
  [[nodiscard]] bool Full() const noexcept { return free_block_count_.load(std::memory_order_relaxed) == 0; }
  [[nodiscard]] bool Empty() const noexcept {
    return free_block_count_.load(std::memory_order_relaxed) == block_count_;
  }

  /**
   * @brief Checks if a pointer belongs to this pool.
   * @param ptr Pointer to check
   * @return True if pointer is within pool's memory range
   */
  [[nodiscard]] bool Owns(const void* ptr) const noexcept;

  /**
   * @brief Gets current allocator statistics.
   * @return AllocatorStats with current usage information
   */
  [[nodiscard]] AllocatorStats Stats() const noexcept;

  /**
   * @brief Gets the block size.
   * @return Block size in bytes
   */
  [[nodiscard]] size_t BlockSize() const noexcept { return block_size_; }

  /**
   * @brief Gets the block count.
   * @return Total number of blocks
   */
  [[nodiscard]] size_t BlockCount() const noexcept { return block_count_; }

  /**
   * @brief Gets the total capacity of the pool.
   * @return Capacity in bytes
   */
  [[nodiscard]] size_t Capacity() const noexcept { return capacity_; }

  /**
   * @brief Gets the number of free blocks.
   * @return Number of blocks available for allocation
   */
  [[nodiscard]] size_t FreeBlockCount() const noexcept { return free_block_count_.load(std::memory_order_relaxed); }

  /**
   * @brief Gets the number of used blocks.
   * @return Number of blocks currently allocated
   */
  [[nodiscard]] size_t UsedBlockCount() const noexcept {
    return block_count_ - free_block_count_.load(std::memory_order_relaxed);
  }

private:
  /**
   * @brief Initializes the free list.
   */
  void InitializeFreeList() noexcept;

  void* buffer_ = nullptr;                             ///< Underlying memory buffer
  void* free_list_head_ = nullptr;                     ///< Head of free list
  size_t block_size_ = 0;                              ///< Size of each block
  size_t block_count_ = 0;                             ///< Total number of blocks
  size_t capacity_ = 0;                                ///< Total capacity in bytes
  size_t alignment_ = 0;                               ///< Alignment of blocks
  std::atomic<void*> free_list_head_atomic_{nullptr};  ///< Atomic head of free list for lock-free operations
  std::atomic<size_t> free_block_count_{0};            ///< Number of free blocks
  std::atomic<size_t> peak_used_blocks_{0};            ///< Peak number of used blocks
  std::atomic<size_t> total_allocations_{0};           ///< Total allocations made
  std::atomic<size_t> total_deallocations_{0};         ///< Total deallocations made
};

inline PoolAllocator::PoolAllocator(size_t block_size, size_t block_count, size_t alignment)
    : block_size_(std::max(block_size, sizeof(void*))),  // Minimum size for free list pointer
      block_count_(block_count),
      alignment_(alignment),
      free_block_count_{block_count} {
  HELIOS_ASSERT(block_count_ > 0, "Failed to construct PoolAllocator: block_count must be greater than 0!, got '{}'",
                block_count_);
  HELIOS_ASSERT(IsPowerOfTwo(alignment), "Failed to construct PoolAllocator: alignment must be power of 2, got '{}'!",
                alignment);
  HELIOS_ASSERT(alignment >= alignof(void*),
                "Failed to construct PoolAllocator: alignment must be at least '{}', got '{}'!", alignof(void*),
                alignment);

  // Align block size to alignment
  block_size_ = AlignUp(block_size_, alignment_);
  capacity_ = block_size_ * block_count_;

  // Allocate aligned buffer
  buffer_ = AlignedAlloc(alignment_, capacity_);
  HELIOS_VERIFY(buffer_ != nullptr, "Failed to construct PoolAllocator: Allocation of buffer failed!");

  InitializeFreeList();
  free_list_head_atomic_.store(free_list_head_, std::memory_order_release);
}

inline PoolAllocator::PoolAllocator(PoolAllocator&& other) noexcept
    : buffer_(other.buffer_),
      free_list_head_(other.free_list_head_),
      block_size_(other.block_size_),
      block_count_(other.block_count_),
      capacity_(other.capacity_),
      alignment_(other.alignment_),
      free_list_head_atomic_{other.free_list_head_atomic_.load(std::memory_order_acquire)},
      free_block_count_{other.free_block_count_.load(std::memory_order_acquire)},
      peak_used_blocks_{other.peak_used_blocks_.load(std::memory_order_acquire)},
      total_allocations_{other.total_allocations_.load(std::memory_order_acquire)},
      total_deallocations_{other.total_deallocations_.load(std::memory_order_acquire)} {
  other.buffer_ = nullptr;
  other.free_list_head_ = nullptr;
  other.block_size_ = 0;
  other.block_count_ = 0;
  other.capacity_ = 0;
  other.alignment_ = 0;
  other.free_list_head_atomic_.store(nullptr, std::memory_order_release);
  other.free_block_count_.store(0, std::memory_order_release);
  other.peak_used_blocks_.store(0, std::memory_order_release);
  other.total_allocations_.store(0, std::memory_order_release);
  other.total_deallocations_.store(0, std::memory_order_release);
}

inline PoolAllocator::~PoolAllocator() noexcept {
  if (buffer_ != nullptr) {
    AlignedFree(buffer_);
  }
}

inline PoolAllocator& PoolAllocator::operator=(PoolAllocator&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  // Free current buffer
  if (buffer_ != nullptr) {
    AlignedFree(buffer_);
  }

  // Move from other
  buffer_ = other.buffer_;
  free_list_head_ = other.free_list_head_;
  block_size_ = other.block_size_;
  block_count_ = other.block_count_;
  capacity_ = other.capacity_;
  alignment_ = other.alignment_;
  free_list_head_atomic_.store(other.free_list_head_atomic_.load(std::memory_order_acquire), std::memory_order_release);
  free_block_count_.store(other.free_block_count_.load(std::memory_order_acquire), std::memory_order_release);
  peak_used_blocks_.store(other.peak_used_blocks_.load(std::memory_order_acquire), std::memory_order_release);
  total_allocations_.store(other.total_allocations_.load(std::memory_order_acquire), std::memory_order_release);
  total_deallocations_.store(other.total_deallocations_.load(std::memory_order_acquire), std::memory_order_release);

  // Reset other
  other.buffer_ = nullptr;
  other.free_list_head_ = nullptr;
  other.block_size_ = 0;
  other.block_count_ = 0;
  other.capacity_ = 0;
  other.alignment_ = 0;
  other.free_list_head_atomic_.store(nullptr, std::memory_order_release);
  other.free_block_count_.store(0, std::memory_order_release);
  other.peak_used_blocks_.store(0, std::memory_order_release);
  other.total_allocations_.store(0, std::memory_order_release);
  other.total_deallocations_.store(0, std::memory_order_release);

  return *this;
}

inline AllocationResult PoolAllocator::Allocate(size_t size) noexcept {
  if (size == 0) [[unlikely]] {
    return {.ptr = nullptr, .allocated_size = 0};
  }

  HELIOS_ASSERT(size <= block_size_, "Failed to allocate memory: size '{}' exceeds block size '{}'!", size,
                block_size_);

  // Lock-free allocation using compare-and-swap
  void* old_head = free_list_head_atomic_.load(std::memory_order_acquire);
  void* new_head = nullptr;

  do {
    // Check if we have free blocks
    if (old_head == nullptr) {
      return {.ptr = nullptr, .allocated_size = 0};
    }

    // Read the next pointer from the old head
    new_head = *reinterpret_cast<void**>(old_head);

    // Try to atomically update the head pointer
  } while (!free_list_head_atomic_.compare_exchange_weak(old_head, new_head, std::memory_order_release,
                                                         std::memory_order_acquire));

  // Successfully allocated - update counters
  free_block_count_.fetch_sub(1, std::memory_order_relaxed);
  total_allocations_.fetch_add(1, std::memory_order_relaxed);

  // Update peak usage
  const size_t used_blocks = block_count_ - free_block_count_.load(std::memory_order_relaxed);
  size_t current_peak = peak_used_blocks_.load(std::memory_order_acquire);
  while (used_blocks > current_peak) {
    if (peak_used_blocks_.compare_exchange_weak(current_peak, used_blocks, std::memory_order_release,
                                                std::memory_order_acquire)) {
      break;
    }
  }

  return {old_head, block_size_};
}

inline void PoolAllocator::Deallocate(void* ptr) noexcept {
  if (ptr == nullptr) [[unlikely]] {
    return;
  }

  HELIOS_ASSERT(Owns(ptr), "Failed to deallocate memory: ptr does not belong to this pool!");

  // Lock-free deallocation using compare-and-swap
  void* old_head = free_list_head_atomic_.load(std::memory_order_acquire);

  do {
    // Set the next pointer of the block to the current head
    *reinterpret_cast<void**>(ptr) = old_head;

    // Try to atomically update the head pointer
  } while (!free_list_head_atomic_.compare_exchange_weak(old_head, ptr, std::memory_order_release,
                                                         std::memory_order_acquire));

  // Successfully deallocated - update counters
  free_block_count_.fetch_add(1, std::memory_order_relaxed);
  total_deallocations_.fetch_add(1, std::memory_order_relaxed);
}

inline void PoolAllocator::Reset() noexcept {
  InitializeFreeList();
  free_list_head_atomic_.store(free_list_head_, std::memory_order_release);
  free_block_count_.store(block_count_, std::memory_order_release);
  // Don't reset peak stats
}

inline bool PoolAllocator::Owns(const void* ptr) const noexcept {
  if (ptr == nullptr || buffer_ == nullptr) {
    return false;
  }

  const auto addr = reinterpret_cast<uintptr_t>(ptr);
  const auto start = reinterpret_cast<uintptr_t>(buffer_);
  const auto end = start + capacity_;

  return addr >= start && addr < end;
}

inline AllocatorStats PoolAllocator::Stats() const noexcept {
  const size_t free_blocks = free_block_count_.load(std::memory_order_relaxed);
  const size_t used_blocks = block_count_ - free_blocks;
  const size_t used_bytes = used_blocks * block_size_;
  const size_t peak_blocks = peak_used_blocks_.load(std::memory_order_relaxed);
  const size_t peak_bytes = peak_blocks * block_size_;
  const size_t total_allocs = total_allocations_.load(std::memory_order_relaxed);
  const size_t total_deallocs = total_deallocations_.load(std::memory_order_relaxed);

  return {
      .total_allocated = used_bytes,
      .total_freed = 0,
      .peak_usage = peak_bytes,
      .allocation_count = used_blocks,
      .total_allocations = total_allocs,
      .total_deallocations = total_deallocs,
      .alignment_waste = 0,
  };
}

inline void PoolAllocator::InitializeFreeList() noexcept {
  // Build free list by linking all blocks
  auto* current = static_cast<uint8_t*>(buffer_);
  free_list_head_ = current;

  for (size_t i = 0; i < block_count_ - 1; ++i) {
    uint8_t* next = current + block_size_;
    *reinterpret_cast<void**>(current) = next;
    current = next;
  }

  // Last block points to nullptr
  *reinterpret_cast<void**>(current) = nullptr;

  // Initialize atomic free list head
  free_list_head_atomic_.store(free_list_head_, std::memory_order_release);
}

template <typename T>
inline T* PoolAllocator::Allocate() noexcept {
  auto result = Allocate(sizeof(T));
  return static_cast<T*>(result.ptr);
}

template <typename T, typename... Args>
  requires std::constructible_from<T, Args...>
inline T* PoolAllocator::AllocateAndConstruct(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
  T* ptr = Allocate<T>();
  if (ptr != nullptr) [[likely]] {
    std::construct_at(ptr, std::forward<Args>(args)...);
  }
  return ptr;
}

}  // namespace helios::memory
