#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.memory;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <atomic>
#include <cstddef>
#include <memory_resource>
#include <shared_mutex>
#endif
#include <helios/memory/common.hpp>
#include <helios/memory/details/profile.hpp>

HELIOS_MODULE_EXPORT
namespace helios::mem {

/**
 * @brief Fixed-capacity best-fit allocator with coalescing.
 * @details Mutex-protected variable-size allocations inside one region.
 * @warning `Reset()` must not run concurrently with `allocate` or `deallocate`.
 */
class FixedFreeListAllocator final : public std::pmr::memory_resource {
public:
  /**
   * @brief Constructs fixed free-list with the given backing capacity.
   * @warning Triggers assertion if capacity is too small. Triggers
   * verification failure if backing allocation fails.
   * @param capacity Backing buffer size in bytes
   */
  explicit FixedFreeListAllocator(size_t capacity) noexcept;
  FixedFreeListAllocator(const FixedFreeListAllocator&) = delete;

  /**
   * @brief Move-constructs fixed free-list from another instance.
   * @param other Source allocator; left in empty moved-from state
   */
  FixedFreeListAllocator(FixedFreeListAllocator&& other) noexcept;
  ~FixedFreeListAllocator() noexcept override { Release(); }

  FixedFreeListAllocator& operator=(const FixedFreeListAllocator&) = delete;

  /**
   * @brief Move-assigns fixed free-list state from another instance.
   * @param other Source allocator; left in empty moved-from state
   * @return Reference to this allocator
   */
  FixedFreeListAllocator& operator=(FixedFreeListAllocator&& other) noexcept;

  /**
   * @brief Resets allocator to a single free region.
   * @warning Must not be called concurrently with `allocate` or `deallocate`.
   */
  void Reset() noexcept;

  /**
   * @brief Returns true when no allocations are active.
   * @return True if empty, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept {
    return allocation_count_.load(std::memory_order_relaxed) == 0;
  }

  /**
   * @brief Checks whether pointer belongs to this allocator's backing buffer.
   * @param ptr Pointer to test
   * @return True when owned, false otherwise
   */
  [[nodiscard]] bool Owns(const void* ptr) const noexcept;

  /**
   * @brief Returns runtime statistics snapshot.
   * @return Allocator stats for this free-list
   */
  [[nodiscard]] AllocatorStats Stats() const noexcept;

  /**
   * @brief Returns configured backing capacity.
   * @return Capacity in bytes
   */
  [[nodiscard]] size_t InitialCapacity() const noexcept { return capacity_; }

  /**
   * @brief Returns configured backing capacity.
   * @return Capacity in bytes
   */
  [[nodiscard]] size_t TotalCapacity() const noexcept { return capacity_; }

  /**
   * @brief Returns free-list block count.
   * @return Number of disjoint free blocks
   */
  [[nodiscard]] size_t FreeBlockCount() const noexcept {
    return free_block_count_.load(std::memory_order_relaxed);
  }

private:
  struct FreeBlock {
    size_t size = 0;
    FreeBlock* next = nullptr;
  };

  struct AllocationHeader {
    size_t size = 0;
    size_t padding = 0;
  };

  void Initialize() noexcept;
  void MoveFrom(FixedFreeListAllocator& other) noexcept;
  void InsertAndCoalesce(FreeBlock* block) noexcept;
  void ReleaseUnlocked() noexcept;
  void Release() noexcept;

  [[nodiscard]] void* do_allocate(size_t bytes, size_t alignment) override;
  void do_deallocate(void* ptr, size_t bytes, size_t alignment) override;

  [[nodiscard]] bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override {
    return this == &other;
  }

  size_t capacity_ = 0;
  std::byte* buffer_ = nullptr;
  FreeBlock* free_list_ = nullptr;
  mutable HELIOS_MEMORY_PROFILE_SHARED_LOCKABLE(std::shared_mutex, mutex_);

  std::atomic<size_t> free_block_count_{0};
  std::atomic<size_t> used_memory_{0};
  std::atomic<size_t> peak_usage_{0};
  std::atomic<size_t> allocation_count_{0};
  std::atomic<size_t> total_allocations_{0};
  std::atomic<size_t> total_deallocations_{0};
  std::atomic<size_t> alignment_waste_{0};
};

inline AllocatorStats FixedFreeListAllocator::Stats() const noexcept {
  return {
      .total_allocated = used_memory_.load(std::memory_order_relaxed),
      .peak_usage = peak_usage_.load(std::memory_order_relaxed),
      .allocation_count = allocation_count_.load(std::memory_order_relaxed),
      .total_allocations = total_allocations_.load(std::memory_order_relaxed),
      .total_deallocations =
          total_deallocations_.load(std::memory_order_relaxed),
      .alignment_waste = alignment_waste_.load(std::memory_order_relaxed),
  };
}

}  // namespace helios::mem
#endif  // HELIOS_MODULE_CONSUMER_SHIM
