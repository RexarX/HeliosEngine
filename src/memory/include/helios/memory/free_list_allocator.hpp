#pragma once

#include <helios/assert.hpp>
#include <helios/memory/common.hpp>
#include <helios/memory/details/profile.hpp>

#include <atomic>
#include <cstddef>
#include <memory_resource>
#include <mutex>
#include <string_view>

namespace helios::mem {

/// @brief Configuration for `FreeListAllocator`.
struct FreeListAllocatorOptions {
  size_t initial_capacity = 0;
  GrowthPolicy growth = GrowthPolicy::Fixed();
};

/**
 * @brief General-purpose PMR free-list allocator.
 * @details Uses a best-fit free-list with coalescing, protected by mutex.
 *
 * Hot path is optimized for minimal lock hold time, but allocator is not
 * lock-free due to variable-size block management and coalescing.
 */
class FreeListAllocator final : public std::pmr::memory_resource {
public:
  /**
   * @brief Constructs free-list allocator from options.
   * @warning Triggers assertion if initial capacity is too small.
   * @param options Allocator options
   */
  explicit FreeListAllocator(FreeListAllocatorOptions options) noexcept;

  /**
   * @brief Constructs free-list allocator with fixed growth.
   * @param initial_capacity Initial backing capacity
   */
  explicit FreeListAllocator(size_t initial_capacity) noexcept;
  FreeListAllocator(const FreeListAllocator&) = delete;
  FreeListAllocator(FreeListAllocator&& other) noexcept;
  ~FreeListAllocator() noexcept override {
    FreeRegions(regions_.load(std::memory_order_acquire));
  }

  FreeListAllocator& operator=(const FreeListAllocator&) = delete;
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
    void* buffer = nullptr;
    size_t capacity = 0;
    std::atomic<RegionHeader*> next{nullptr};
  };

  static RegionHeader* CreateRegion(size_t capacity) noexcept;
  void FreeRegions(RegionHeader* region) noexcept;

  [[nodiscard]] void* AllocateLocked(size_t bytes, size_t alignment) noexcept;
  void DeallocateLocked(void* ptr) noexcept;
  [[nodiscard]] bool GrowLocked(size_t min_capacity) noexcept;
  void InsertAndCoalesceLocked(FreeBlockHeader* block) noexcept;
  void InitializeRegionLocked(RegionHeader& region) noexcept;

  [[nodiscard]] void* do_allocate(size_t bytes, size_t alignment) override;
  void do_deallocate(void* ptr, size_t bytes, size_t alignment) override;
  [[nodiscard]] bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override {
    return this == &other;
  }

  FreeBlockHeader* free_list_ = nullptr;
  std::atomic<RegionHeader*> regions_{nullptr};
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

inline FreeListAllocator::FreeListAllocator(
    FreeListAllocatorOptions options) noexcept
    : initial_capacity_(options.initial_capacity), growth_(options.growth) {
  HELIOS_ASSERT(initial_capacity_ > sizeof(FreeBlockHeader),
                "initial_capacity '{}' is too small!", initial_capacity_);
  HELIOS_ASSERT(growth_.max_capacity >= initial_capacity_,
                "max_capacity '{}' must be >= initial_capacity '{}'!",
                growth_.max_capacity, initial_capacity_);

  HELIOS_MEMORY_PROFILE_LOCK_NAME(mutex_,
                                  std::string_view{"FreeListAllocator"});

  RegionHeader* const initial_region = CreateRegion(initial_capacity_);
  HELIOS_VERIFY(initial_region != nullptr,
                "Failed to allocate free-list region!");

  regions_.store(initial_region, std::memory_order_release);
  capacity_.store(initial_capacity_, std::memory_order_relaxed);

  {
    const std::scoped_lock lock(mutex_);
    InitializeRegionLocked(*initial_region);
  }
}

inline FreeListAllocator::FreeListAllocator(size_t initial_capacity) noexcept
    : FreeListAllocator(FreeListAllocatorOptions{
          .initial_capacity = initial_capacity,
          .growth = GrowthPolicy::Fixed(),
      }) {}

inline FreeListAllocator::FreeListAllocator(FreeListAllocator&& other) noexcept
    : free_list_(other.free_list_),
      regions_(other.regions_.load(std::memory_order_acquire)),
      initial_capacity_(other.initial_capacity_),
      growth_(other.growth_),
      capacity_(other.capacity_.load(std::memory_order_acquire)),
      used_memory_(other.used_memory_.load(std::memory_order_acquire)),
      peak_usage_(other.peak_usage_.load(std::memory_order_acquire)),
      free_block_count_(
          other.free_block_count_.load(std::memory_order_acquire)),
      allocation_count_(
          other.allocation_count_.load(std::memory_order_acquire)),
      total_allocations_(
          other.total_allocations_.load(std::memory_order_acquire)),
      total_deallocations_(
          other.total_deallocations_.load(std::memory_order_acquire)),
      alignment_waste_(other.alignment_waste_.load(std::memory_order_acquire)) {
  other.free_list_ = nullptr;
  other.regions_.store(nullptr, std::memory_order_release);
  other.initial_capacity_ = 0;
  other.capacity_.store(0, std::memory_order_release);
  other.used_memory_.store(0, std::memory_order_release);
  other.peak_usage_.store(0, std::memory_order_release);
  other.free_block_count_.store(0, std::memory_order_release);
  other.allocation_count_.store(0, std::memory_order_release);
  other.total_allocations_.store(0, std::memory_order_release);
  other.total_deallocations_.store(0, std::memory_order_release);
  other.alignment_waste_.store(0, std::memory_order_release);
}

inline FreeListAllocator& FreeListAllocator::operator=(
    FreeListAllocator&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  auto transfer = [this, &other]() {
    FreeRegions(regions_.load(std::memory_order_acquire));
    free_list_ = other.free_list_;
    regions_.store(other.regions_.load(std::memory_order_acquire),
                   std::memory_order_release);
    other.free_list_ = nullptr;
    other.regions_.store(nullptr, std::memory_order_release);
  };

  if (this < &other) {
    const std::scoped_lock self_lock(mutex_);
    const std::scoped_lock other_lock(other.mutex_);
    transfer();
  } else {
    const std::scoped_lock other_lock(other.mutex_);
    const std::scoped_lock self_lock(mutex_);
    transfer();
  }

  initial_capacity_ = other.initial_capacity_;
  growth_ = other.growth_;
  capacity_.store(other.capacity_.load(std::memory_order_acquire),
                  std::memory_order_release);
  used_memory_.store(other.used_memory_.load(std::memory_order_acquire),
                     std::memory_order_release);
  peak_usage_.store(other.peak_usage_.load(std::memory_order_acquire),
                    std::memory_order_release);
  free_block_count_.store(
      other.free_block_count_.load(std::memory_order_acquire),
      std::memory_order_release);
  allocation_count_.store(
      other.allocation_count_.load(std::memory_order_acquire),
      std::memory_order_release);
  total_allocations_.store(
      other.total_allocations_.load(std::memory_order_acquire),
      std::memory_order_release);
  total_deallocations_.store(
      other.total_deallocations_.load(std::memory_order_acquire),
      std::memory_order_release);
  alignment_waste_.store(other.alignment_waste_.load(std::memory_order_acquire),
                         std::memory_order_release);

  other.initial_capacity_ = 0;
  other.capacity_.store(0, std::memory_order_release);
  other.used_memory_.store(0, std::memory_order_release);
  other.peak_usage_.store(0, std::memory_order_release);
  other.free_block_count_.store(0, std::memory_order_release);
  other.allocation_count_.store(0, std::memory_order_release);
  other.total_allocations_.store(0, std::memory_order_release);
  other.total_deallocations_.store(0, std::memory_order_release);
  other.alignment_waste_.store(0, std::memory_order_release);

  return *this;
}

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

inline size_t FreeListAllocator::FreeMemory() const noexcept {
  const size_t cap = capacity_.load(std::memory_order_relaxed);
  const size_t used = used_memory_.load(std::memory_order_relaxed);
  return cap >= used ? cap - used : 0;
}

}  // namespace helios::mem
