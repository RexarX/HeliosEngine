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
#include <mutex>

namespace helios::memory {

/**
 * @brief Free list allocator with best-fit strategy.
 * @details General-purpose allocator that maintains a free list of availablememory blocks.
 * Supports arbitrary allocation sizes and orders. Uses a best-fit allocation strategy to minimize fragmentation.
 *
 * Each free block contains a header with size and pointer to the next free block.
 * Allocated blocks also store a header with size information for deallocation.
 *
 * Supports arbitrary allocation and deallocation order.
 * May suffer from fragmentation over time with varied allocation patterns.
 * Adjacent free blocks are coalesced to reduce fragmentation.
 *
 * @note Thread-safe.
 */
class FreeListAllocator final {
public:
  /**
   * @brief Constructs a free list allocator with specified capacity.
   * @warning Triggers assertion if capacity is less than or equal to sizeof(FreeBlockHeader).
   * @param capacity Total size of the memory buffer in bytes
   */
  explicit FreeListAllocator(size_t capacity);
  FreeListAllocator(const FreeListAllocator&) = delete;
  FreeListAllocator(FreeListAllocator&& other) noexcept;
  ~FreeListAllocator() noexcept;

  FreeListAllocator& operator=(const FreeListAllocator&) = delete;
  FreeListAllocator& operator=(FreeListAllocator&& other) noexcept;

  /**
   * @brief Allocates memory with specified size and alignment.
   * @note Uses best-fit strategy to find the smallest suitable free block
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
   * FreeListAllocator alloc(1024);
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
   * FreeListAllocator alloc(1024);
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
   * FreeListAllocator alloc(1024);
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
   * FreeListAllocator alloc(1024);
   * auto* arr = alloc.AllocateAndConstructArray<MyType>(10);
   * @endcode
   */
  template <typename T>
    requires std::default_initializable<T>
  [[nodiscard]] T* AllocateAndConstructArray(size_t count) noexcept(std::is_nothrow_default_constructible_v<T>);

  /**
   * @brief Deallocates memory.
   * @note Returns the block to the free list and coalesces adjacent free blocks
   * @warning Triggers assertion if pointer does not belong to this allocator.
   * @param ptr Pointer to deallocate
   */
  void Deallocate(void* ptr) noexcept;

  /**
   * @brief Resets the allocator, freeing all allocations.
   * @details Recreates a single large free block encompassing the entire buffer.
   */
  void Reset() noexcept;

  /**
   * @brief Checks if the allocator is empty (all memory free).
   * @return True if no allocations exist
   */
  [[nodiscard]] bool Empty() const noexcept { return allocation_count_.load(std::memory_order_acquire) == 0; }

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
   * @brief Gets the total capacity of the allocator.
   * @return Capacity in bytes
   */
  [[nodiscard]] size_t Capacity() const noexcept { return capacity_; }

  /**
   * @brief Gets the amount of allocated memory.
   * @return Allocated bytes
   */
  [[nodiscard]] size_t UsedMemory() const noexcept { return used_memory_.load(std::memory_order_acquire); }

  /**
   * @brief Gets the amount of free memory.
   * @return Free bytes
   */
  [[nodiscard]] size_t FreeMemory() const noexcept { return capacity_ - used_memory_.load(std::memory_order_acquire); }

  /**
   * @brief Gets the number of free blocks in the free list.
   * @return Number of free blocks
   */
  [[nodiscard]] size_t FreeBlockCount() const noexcept { return free_block_count_.load(std::memory_order_acquire); }

  /**
   * @brief Gets the number of active allocations.
   * @return Number of allocations
   */
  [[nodiscard]] size_t AllocationCount() const noexcept { return allocation_count_.load(std::memory_order_acquire); }

private:
  /**
   * @brief Header for free blocks in the free list.
   * @details Stores block size and links to next free block.
   */
  struct FreeBlockHeader {
    size_t size = 0;                  ///< Size of the free block (including header)
    FreeBlockHeader* next = nullptr;  ///< Pointer to next free block
  };

  /**
   * @brief Header for allocated blocks.
   * @details Stores block size for deallocation.
   */
  struct AllocationHeader {
    size_t size = 0;     ///< Size of the allocated block (including header)
    size_t padding = 0;  ///< Padding used for alignment
  };

  /**
   * @brief Finds the best-fit free block for the requested size.
   * @param size Requested size (including header and padding)
   * @param prev_block Output parameter for previous block in list
   * @return Pointer to best-fit block or nullptr if not found
   */
  [[nodiscard]] FreeBlockHeader* FindBestFit(size_t size, FreeBlockHeader** prev_block) noexcept;

  /**
   * @brief Coalesces adjacent free blocks.
   * @param block Block to coalesce with neighbors
   */
  void CoalesceAdjacent(FreeBlockHeader* block) noexcept;

  /**
   * @brief Initializes the allocator with a single large free block.
   */
  void InitializeFreeList() noexcept;

  void* buffer_ = nullptr;                     ///< Underlying memory buffer
  FreeBlockHeader* free_list_head_ = nullptr;  ///< Head of free list
  size_t capacity_ = 0;                        ///< Total capacity in bytes

  std::atomic<size_t> used_memory_{0};          ///< Currently allocated bytes
  std::atomic<size_t> peak_usage_{0};           ///< Peak memory usage
  std::atomic<size_t> free_block_count_{0};     ///< Number of free blocks
  std::atomic<size_t> allocation_count_{0};     ///< Number of active allocations
  std::atomic<size_t> total_allocations_{0};    ///< Total allocations made
  std::atomic<size_t> total_deallocations_{0};  ///< Total deallocations made
  std::atomic<size_t> alignment_waste_{0};      ///< Total bytes wasted due to alignment

  mutable std::mutex mutex_;  ///< Mutex for thread-safety
};

inline FreeListAllocator::FreeListAllocator(size_t capacity) : capacity_(capacity) {
  HELIOS_ASSERT(capacity > sizeof(FreeBlockHeader),
                "Failed to construct FreeListAllocator: capacity must be greater than '{}' bytes!",
                sizeof(FreeBlockHeader));

  // Allocate aligned buffer
  buffer_ = AlignedAlloc(kDefaultAlignment, capacity_);
  HELIOS_VERIFY(buffer_ != nullptr, "Failed to construct FreeListAllocator: Allocation of buffer failed!");

  InitializeFreeList();
}

inline FreeListAllocator::FreeListAllocator(FreeListAllocator&& other) noexcept
    : buffer_(other.buffer_),
      free_list_head_(other.free_list_head_),
      capacity_(other.capacity_),
      used_memory_{other.used_memory_.load(std::memory_order_acquire)},
      peak_usage_{other.peak_usage_.load(std::memory_order_acquire)},
      free_block_count_{other.free_block_count_.load(std::memory_order_acquire)},
      allocation_count_{other.allocation_count_.load(std::memory_order_acquire)},
      total_allocations_{other.total_allocations_.load(std::memory_order_acquire)},
      total_deallocations_{other.total_deallocations_.load(std::memory_order_acquire)},
      alignment_waste_{other.alignment_waste_.load(std::memory_order_acquire)} {
  other.buffer_ = nullptr;
  other.free_list_head_ = nullptr;
  other.capacity_ = 0;
  other.used_memory_.store(0, std::memory_order_release);
  other.peak_usage_.store(0, std::memory_order_release);
  other.free_block_count_.store(0, std::memory_order_release);
  other.allocation_count_.store(0, std::memory_order_release);
  other.total_allocations_.store(0, std::memory_order_release);
  other.total_deallocations_.store(0, std::memory_order_release);
  other.alignment_waste_.store(0, std::memory_order_release);
}

inline FreeListAllocator::~FreeListAllocator() noexcept {
  if (buffer_ != nullptr) {
    AlignedFree(buffer_);
  }
}

inline FreeListAllocator& FreeListAllocator::operator=(FreeListAllocator&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  {
    const std::scoped_lock lock(mutex_);
    const std::scoped_lock other_lock(other.mutex_);

    // Free current buffer
    if (buffer_ != nullptr) {
      AlignedFree(buffer_);
    }

    // Move from other
    buffer_ = other.buffer_;
    free_list_head_ = other.free_list_head_;
    capacity_ = other.capacity_;

    // Reset other
    other.buffer_ = nullptr;
    other.free_list_head_ = nullptr;
    other.capacity_ = 0;
  }

  used_memory_.store(other.used_memory_.load(std::memory_order_acquire), std::memory_order_release);
  peak_usage_.store(other.peak_usage_.load(std::memory_order_acquire), std::memory_order_release);
  free_block_count_.store(other.free_block_count_.load(std::memory_order_acquire), std::memory_order_release);
  allocation_count_.store(other.allocation_count_.load(std::memory_order_acquire), std::memory_order_release);
  total_allocations_.store(other.total_allocations_.load(std::memory_order_acquire), std::memory_order_release);
  total_deallocations_.store(other.total_deallocations_.load(std::memory_order_acquire), std::memory_order_release);
  alignment_waste_.store(other.alignment_waste_.load(std::memory_order_acquire), std::memory_order_release);

  other.used_memory_.store(0, std::memory_order_release);
  other.peak_usage_.store(0, std::memory_order_release);
  other.free_block_count_.store(0, std::memory_order_release);
  other.allocation_count_.store(0, std::memory_order_release);
  other.total_allocations_.store(0, std::memory_order_release);
  other.total_deallocations_.store(0, std::memory_order_release);
  other.alignment_waste_.store(0, std::memory_order_release);

  return *this;
}

inline AllocationResult FreeListAllocator::Allocate(size_t size, size_t alignment) noexcept {
  HELIOS_ASSERT(IsPowerOfTwo(alignment), "Failed to allocate memory: alignment must be power of 2, got '{}'!",
                alignment);
  HELIOS_ASSERT(alignment >= kMinAlignment, "Failed to allocate memory: alignment must be at least '{}', got '{}'!",
                kMinAlignment, alignment);

  if (size == 0) {
    return {.ptr = nullptr, .allocated_size = 0};
  }

  // Calculate total size needed (with header and alignment padding)
  const size_t header_size = sizeof(AllocationHeader);
  const size_t min_block_size = header_size + size;

  size_t total_size = 0;
  size_t padding = 0;

  AllocationResult result{.ptr = nullptr, .allocated_size = 0};

  {
    const std::scoped_lock lock(mutex_);

    // Find best-fit block
    FreeBlockHeader* prev_block = nullptr;
    FreeBlockHeader* best_block = FindBestFit(min_block_size + alignment, &prev_block);

    if (best_block == nullptr) [[unlikely]] {
      return result;
    }

    // Calculate aligned position for user data
    auto* block_start = reinterpret_cast<uint8_t*>(best_block);
    uint8_t* header_ptr = block_start;
    padding = CalculatePaddingWithHeader(header_ptr, alignment, header_size);
    uint8_t* aligned_data = block_start + padding;

    total_size = padding + size;
    const size_t remaining_size = best_block->size - total_size;

    // Remove block from free list
    if (prev_block != nullptr) {
      prev_block->next = best_block->next;
    } else {
      free_list_head_ = best_block->next;
    }
    free_block_count_.fetch_sub(1, std::memory_order_relaxed);

    // If there's enough space left, create a new free block
    constexpr size_t min_free_block_size = sizeof(FreeBlockHeader) + 16;
    if (remaining_size >= min_free_block_size) {
      auto* new_free_block = reinterpret_cast<FreeBlockHeader*>(block_start + total_size);
      new_free_block->size = remaining_size;
      new_free_block->next = free_list_head_;
      free_list_head_ = new_free_block;
      free_block_count_.fetch_add(1, std::memory_order_relaxed);
    }

    // Write allocation header
    auto* alloc_header = reinterpret_cast<AllocationHeader*>(aligned_data - header_size);
    alloc_header->size = total_size;
    alloc_header->padding = padding;

    result = {.ptr = aligned_data, .allocated_size = size};
  }

  // Update stats
  const size_t new_used = used_memory_.fetch_add(total_size, std::memory_order_relaxed) + total_size;
  size_t current_peak = peak_usage_.load(std::memory_order_acquire);
  while (new_used > current_peak) {
    if (peak_usage_.compare_exchange_weak(current_peak, new_used, std::memory_order_release,
                                          std::memory_order_acquire)) {
      break;
    }
  }

  allocation_count_.fetch_add(1, std::memory_order_relaxed);
  total_allocations_.fetch_add(1, std::memory_order_relaxed);
  alignment_waste_.fetch_add(padding - header_size, std::memory_order_relaxed);

  return result;
}

inline void FreeListAllocator::Deallocate(void* ptr) noexcept {
  if (ptr == nullptr) [[unlikely]] {
    return;
  }

  HELIOS_ASSERT(Owns(ptr), "Failed to deallocate memory: ptr does not belong to this allocator!");

  // Get allocation header
  auto* alloc_header = reinterpret_cast<AllocationHeader*>(static_cast<uint8_t*>(ptr) - sizeof(AllocationHeader));
  const size_t block_size = alloc_header->size;

  // Calculate block start (account for alignment padding)
  auto* block_start = reinterpret_cast<uint8_t*>(ptr) - alloc_header->padding;

  // Create new free block
  auto* free_block = reinterpret_cast<FreeBlockHeader*>(block_start);
  free_block->size = block_size;
  free_block_count_.fetch_add(1, std::memory_order_relaxed);

  // Update stats
  used_memory_.fetch_sub(block_size, std::memory_order_relaxed);
  allocation_count_.fetch_sub(1, std::memory_order_relaxed);
  total_deallocations_.fetch_add(1, std::memory_order_relaxed);

  {
    const std::scoped_lock lock(mutex_);
    free_block->next = free_list_head_;
    free_list_head_ = free_block;

    // Coalesce adjacent free blocks
    CoalesceAdjacent(free_block);
  }
}

inline void FreeListAllocator::Reset() noexcept {
  {
    const std::scoped_lock lock(mutex_);
    InitializeFreeList();
  }

  used_memory_.store(0, std::memory_order_release);
  allocation_count_.store(0, std::memory_order_release);
  alignment_waste_.store(0, std::memory_order_release);
  // Don't reset peak stats
}

inline AllocatorStats FreeListAllocator::Stats() const noexcept {
  const size_t used = used_memory_.load(std::memory_order_acquire);
  const size_t peak = peak_usage_.load(std::memory_order_acquire);
  const size_t alloc_count = allocation_count_.load(std::memory_order_acquire);
  const size_t total_allocs = total_allocations_.load(std::memory_order_acquire);
  const size_t total_deallocs = total_deallocations_.load(std::memory_order_acquire);
  const size_t waste = alignment_waste_.load(std::memory_order_acquire);

  return {
      .total_allocated = used,
      .total_freed = capacity_ - used,
      .peak_usage = peak,
      .allocation_count = alloc_count,
      .total_allocations = total_allocs,
      .total_deallocations = total_deallocs,
      .alignment_waste = waste,
  };
}

inline bool FreeListAllocator::Owns(const void* ptr) const noexcept {
  if (ptr == nullptr || buffer_ == nullptr) {
    return false;
  }

  const auto addr = reinterpret_cast<uintptr_t>(ptr);
  const auto start = reinterpret_cast<uintptr_t>(buffer_);
  const auto end = start + capacity_;

  return addr >= start && addr < end;
}

inline auto FreeListAllocator::FindBestFit(size_t size, FreeBlockHeader** prev_block) noexcept -> FreeBlockHeader* {
  FreeBlockHeader* best_fit = nullptr;
  FreeBlockHeader* best_fit_prev = nullptr;
  size_t smallest_diff = SIZE_MAX;

  FreeBlockHeader* current = free_list_head_;
  FreeBlockHeader* prev = nullptr;

  while (current != nullptr) {
    if (current->size >= size) {
      const size_t diff = current->size - size;
      if (diff < smallest_diff) {
        smallest_diff = diff;
        best_fit = current;
        best_fit_prev = prev;

        // Perfect fit found
        if (diff == 0) {
          break;
        }
      }
    }
    prev = current;
    current = current->next;
  }

  if (prev_block != nullptr) {
    *prev_block = best_fit_prev;
  }

  return best_fit;
}

inline void FreeListAllocator::CoalesceAdjacent(FreeBlockHeader* block) noexcept {
  if (block == nullptr) [[unlikely]] {
    return;
  }

  // Coalesce adjacent blocks in a single pass
  // We'll track if block gets merged into another block
  bool block_merged = false;
  auto* block_start = reinterpret_cast<uint8_t*>(block);
  uint8_t* block_end = block_start + block->size;

  // First pass: try to merge blocks that come after 'block' in memory
  FreeBlockHeader* current = free_list_head_;
  FreeBlockHeader* prev = nullptr;

  while (current != nullptr) {
    if (current == block) {
      prev = current;
      current = current->next;
      continue;
    }

    auto* current_start = reinterpret_cast<uint8_t*>(current);

    // Check if current block is immediately after our block
    if (current_start == block_end) {
      // Merge current into block
      block->size += current->size;
      block_end = block_start + block->size;

      // Remove current from free list
      if (prev != nullptr) {
        prev->next = current->next;
      } else {
        free_list_head_ = current->next;
      }
      free_block_count_.fetch_sub(1, std::memory_order_relaxed);

      // Continue from the same prev position
      current = (prev != nullptr) ? prev->next : free_list_head_;
      continue;
    }

    prev = current;
    current = current->next;
  }

  // Second pass: check if our block should be merged into a block that comes before it
  current = free_list_head_;
  prev = nullptr;

  while (current != nullptr) {
    if (current == block) {
      prev = current;
      current = current->next;
      continue;
    }

    auto* current_start = reinterpret_cast<uint8_t*>(current);
    uint8_t* current_end = current_start + current->size;

    // Check if our block is immediately after current block
    if (current_end == block_start) {
      // Merge block into current
      current->size += block->size;

      // Remove block from free list
      FreeBlockHeader* scan = free_list_head_;
      FreeBlockHeader* scan_prev = nullptr;
      while (scan != nullptr) {
        if (scan == block) {
          if (scan_prev != nullptr) {
            scan_prev->next = scan->next;
          } else {
            free_list_head_ = scan->next;
          }
          free_block_count_.fetch_sub(1, std::memory_order_relaxed);
          block_merged = true;
          break;
        }
        scan_prev = scan;
        scan = scan->next;
      }

      // If we merged block into current, we're done
      if (block_merged) {
        return;
      }
    }

    prev = current;
    current = current->next;
  }
}

inline void FreeListAllocator::InitializeFreeList() noexcept {
  // Create a single large free block encompassing the entire buffer
  free_list_head_ = static_cast<FreeBlockHeader*>(buffer_);
  free_list_head_->size = capacity_;
  free_list_head_->next = nullptr;
  free_block_count_.store(1, std::memory_order_release);
}

template <typename T>
inline T* FreeListAllocator::Allocate() noexcept {
  constexpr size_t size = sizeof(T);
  constexpr size_t alignment = std::max(alignof(T), kMinAlignment);
  auto result = Allocate(size, alignment);
  return static_cast<T*>(result.ptr);
}

template <typename T>
inline T* FreeListAllocator::Allocate(size_t count) noexcept {
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
inline T* FreeListAllocator::AllocateAndConstruct(Args&&... args) noexcept(
    std::is_nothrow_constructible_v<T, Args...>) {
  T* ptr = Allocate<T>();
  if (ptr != nullptr) [[likely]] {
    std::construct_at(ptr, std::forward<Args>(args)...);
  }
  return ptr;
}

template <typename T>
  requires std::default_initializable<T>
inline T* FreeListAllocator::AllocateAndConstructArray(size_t count) noexcept(
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
