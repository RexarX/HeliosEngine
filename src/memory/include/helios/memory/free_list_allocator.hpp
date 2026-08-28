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
#include <cstdint>
#include <memory_resource>
#include <mutex>
#endif
#include <helios/memory/common.hpp>
#include <helios/memory/details/profile.hpp>
#include <helios/memory/treiber_stack.hpp>

HELIOS_MODULE_EXPORT
namespace helios::mem {

/// @brief Configuration for `FreeListAllocator`.
struct FreeListAllocatorOptions {
  /// @brief Capacity of the initial backing region in bytes.
  size_t initial_capacity = 0;
  /// @brief Growth policy applied when additional regions are required.
  GrowthPolicy growth = GrowthPolicy::Geometric();
};

/**
 * @brief General-purpose PMR free-list allocator.
 * @details Best-fit free-list with coalescing under a mutex. Growable backing
 * regions retire through epoch-based reclamation so `Owns()` can scan
 * regions without holding the free-list mutex.
 */
class FreeListAllocator final : public std::pmr::memory_resource {
public:
  /**
   * @brief Constructs free-list allocator from options.
   * @warning Triggers assertion if `options.initial_capacity` is too small or
   * `options.growth.max_capacity < options.initial_capacity`.
   * @param options Allocator options
   */
  explicit FreeListAllocator(FreeListAllocatorOptions options) noexcept;

  /**
   * @brief Constructs free-list allocator with geometric growth.
   * @warning Triggers assertion when initial capacity is too small.
   * @param initial_capacity Initial backing capacity
   */
  explicit FreeListAllocator(size_t initial_capacity) noexcept
      : FreeListAllocator(FreeListAllocatorOptions{
            .initial_capacity = initial_capacity,
            .growth = GrowthPolicy::Geometric(),
        }) {}

  FreeListAllocator(const FreeListAllocator&) = delete;

  /**
   * @brief Move-constructs free-list allocator from another instance.
   * @param other Source allocator; left in empty moved-from state
   */
  FreeListAllocator(FreeListAllocator&& other) noexcept;
  ~FreeListAllocator() noexcept override { ReleaseRegions(); }

  FreeListAllocator& operator=(const FreeListAllocator&) = delete;

  /**
   * @brief Move-assigns free-list state from another instance.
   * @param other Source allocator; left in empty moved-from state
   * @return Reference to this allocator
   */
  FreeListAllocator& operator=(FreeListAllocator&& other) noexcept;

  /**
   * @brief Resets allocator to initial state.
   * @warning Must not be called concurrently with allocate/deallocate.
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
   * @brief Checks whether pointer belongs to allocator storage.
   * @param ptr Pointer to test
   * @return True when owned, false otherwise
   */
  [[nodiscard]] bool Owns(const void* ptr) const noexcept;

  /**
   * @brief Returns runtime stats.
   * @return Allocator stats snapshot
   */
  [[nodiscard]] AllocatorStats Stats() const noexcept;

  /**
   * @brief Returns total capacity across regions.
   * @return Capacity in bytes
   */
  [[nodiscard]] size_t Capacity() const noexcept {
    return capacity_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Returns used memory bytes.
   * @return Used bytes
   */
  [[nodiscard]] size_t UsedMemory() const noexcept {
    return used_memory_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Returns free memory bytes.
   * @return Free bytes
   */
  [[nodiscard]] size_t FreeMemory() const noexcept;

  /**
   * @brief Returns free-list block count.
   * @return Number of free blocks
   */
  [[nodiscard]] size_t FreeBlockCount() const noexcept {
    return free_block_count_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Returns current active allocation count.
   * @return Number of active allocations
   */
  [[nodiscard]] size_t AllocationCount() const noexcept {
    return allocation_count_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Returns growth policy.
   * @return Growth policy
   */
  [[nodiscard]] GrowthPolicy Growth() const noexcept { return growth_; }

private:
  struct FreeBlockHeader {
    size_t size = 0;
    FreeBlockHeader* next = nullptr;
  };

  struct RegionHeader {
    RegionHeader* next = nullptr;
    void* buffer = nullptr;
    size_t capacity = 0;
  };

  enum class GrowState : uint8_t { kIdle, kGrowing };

  static RegionHeader* CreateRegion(size_t capacity) noexcept;
  [[nodiscard]] RegionHeader* HeadRegion() const noexcept {
    return static_cast<RegionHeader*>(regions_.Top());
  }

  [[nodiscard]] static RegionHeader* NextRegion(
      const RegionHeader* region) noexcept {
    return static_cast<RegionHeader*>(TreiberStack::Next(region));
  }
  void FreeRegions(RegionHeader* region) noexcept;
  void ReleaseRegions() noexcept;

  [[nodiscard]] void* AllocateLocked(size_t bytes, size_t alignment) noexcept;
  void DeallocateLocked(void* ptr) noexcept;
  [[nodiscard]] bool EnsureCapacity(size_t min_capacity) noexcept;
  void InsertAndCoalesceLocked(FreeBlockHeader* block) noexcept;
  void InitializeRegionLocked(RegionHeader& region) noexcept;
  void MoveFrom(FreeListAllocator& other) noexcept;

  [[nodiscard]] void* do_allocate(size_t bytes, size_t alignment) override;
  void do_deallocate(void* ptr, size_t bytes, size_t alignment) override;
  [[nodiscard]] bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override {
    return this == &other;
  }

  FreeBlockHeader* free_list_ = nullptr;
  TreiberStack regions_;
  std::atomic<GrowState> grow_state_{GrowState::kIdle};
  size_t initial_capacity_ = 0;
  GrowthPolicy growth_;

  std::atomic<size_t> capacity_{0};
  std::atomic<size_t> used_memory_{0};
  std::atomic<size_t> peak_usage_{0};
  std::atomic<size_t> free_block_count_{0};
  std::atomic<size_t> allocation_count_{0};
  std::atomic<size_t> total_allocations_{0};
  std::atomic<size_t> total_deallocations_{0};
  std::atomic<size_t> alignment_waste_{0};

  mutable HELIOS_MEMORY_PROFILE_LOCKABLE(std::mutex, mutex_);
};

inline AllocatorStats FreeListAllocator::Stats() const noexcept {
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
