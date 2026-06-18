#pragma once

#include <helios/memory/common.hpp>

#include <helios/assert.hpp>
#include <helios/memory/details/profile.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory_resource>

namespace helios::mem {

/// @brief Configuration for StackAllocator.
struct StackAllocatorOptions {
  size_t initial_capacity = 0;
  GrowthPolicy growth = GrowthPolicy::Fixed();
};

/**
 * @brief PMR stack allocator with LIFO-aware deallocation.
 * @details Uses block-chained bump allocation with per-allocation headers.
 *
 * Allocation path is lock-free with atomic CAS on head block offset. Deallocate
 * tries LIFO rewind when possible. Non-LIFO deallocation is treated as no-op,
 * which keeps PMR compatibility for containers that may free out of order.
 *
 * @note Thread-safe for `allocate`. `Reset()` and marker operations require
 * external synchronization.
 */
class StackAllocator final : public std::pmr::memory_resource {
public:
  /// @brief Marker for rewind operations.
  struct Marker {
    void* block = nullptr;
    size_t offset = 0;
  };

  /**
   * @brief Constructs stack allocator from options.
   * @warning Triggers assertion if `options.initial_capacity == 0`.
   * @param options Stack allocator options
   */
  explicit StackAllocator(StackAllocatorOptions options) noexcept;

  /**
   * @brief Constructs stack allocator with fixed growth.
   * @warning Triggers assertion if `initial_capacity == 0`.
   * @param initial_capacity Initial block capacity
   */
  explicit StackAllocator(size_t initial_capacity) noexcept;
  StackAllocator(const StackAllocator&) = delete;
  StackAllocator(StackAllocator&& other) noexcept;
  ~StackAllocator() noexcept override {
    FreeChain(head_.load(std::memory_order_acquire));
  }

  StackAllocator& operator=(const StackAllocator&) = delete;
  StackAllocator& operator=(StackAllocator&& other) noexcept;

  /**
   * @brief Returns current marker.
   * @return Marker that can later be rewound to
   */
  [[nodiscard]] Marker GetMarker() const noexcept;

  /**
   * @brief Rewinds allocator to marker.
   * @warning Must not be called concurrently with allocation/deallocation.
   * Triggers assertion if marker does not belong to this allocator.
   * @param marker Marker previously returned by `GetMarker`
   */
  void RewindToMarker(Marker marker) noexcept;

  /**
   * @brief Resets allocator to a fresh single initial block.
   * @warning Must not be called concurrently with allocation/deallocation.
   */
  void Reset() noexcept;

  /**
   * @brief Returns true when no active allocations remain.
   * @return True if empty, false otherwise.
   */
  [[nodiscard]] bool Empty() const noexcept {
    return allocation_count_.load(std::memory_order_relaxed) == 0;
  }

  /**
   * @brief Returns runtime statistics.
   * @return Allocator stats snapshot
   */
  [[nodiscard]] AllocatorStats Stats() const noexcept;

  /**
   * @brief Returns initial block capacity.
   * @return Capacity in bytes
   */
  [[nodiscard]] size_t InitialCapacity() const noexcept {
    return initial_capacity_;
  }

  /**
   * @brief Returns total capacity across block chain.
   * @return Capacity in bytes
   */
  [[nodiscard]] size_t TotalCapacity() const noexcept {
    return total_capacity_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Returns current number of blocks.
   * @return Block count
   */
  [[nodiscard]] size_t BlockCount() const noexcept {
    return block_count_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Returns growth policy.
   * @return Growth policy
   */
  [[nodiscard]] GrowthPolicy Growth() const noexcept { return growth_; }

private:
  struct AllocationHeader {
    size_t previous_offset = 0;
    size_t total_size = 0;
  };

  struct Block {
    void* buffer = nullptr;
    size_t capacity = 0;
    std::atomic<size_t> offset{0};
    std::atomic<Block*> next{nullptr};
  };

  struct Reservation {
    void* ptr = nullptr;
    size_t allocation_size = 0;
    size_t consumed = 0;
    size_t previous_offset = 0;
  };

  enum class GrowState : uint8_t {
    kIdle,
    kGrowing,
  };

  [[nodiscard]] static Block* CreateBlock(size_t capacity) noexcept;
  static void FreeChain(Block* head) noexcept;
  [[nodiscard]] static Reservation TryReserve(Block& block, size_t size,
                                              size_t alignment) noexcept;
  [[nodiscard]] bool EnsureCapacity(size_t min_capacity) noexcept;
  void PublishBlock(Block* block) noexcept;

  [[nodiscard]] void* do_allocate(size_t bytes, size_t alignment) override;
  void do_deallocate(void* ptr, size_t bytes, size_t alignment) override;
  [[nodiscard]] bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override {
    return this == &other;
  }

  std::atomic<Block*> head_{nullptr};
  std::atomic<GrowState> grow_state_{GrowState::kIdle};
  size_t initial_capacity_ = 0;
  GrowthPolicy growth_{};

  std::atomic<size_t> total_capacity_{0};
  std::atomic<size_t> total_allocated_{0};
  std::atomic<size_t> peak_usage_{0};
  std::atomic<size_t> allocation_count_{0};
  std::atomic<size_t> total_allocations_{0};
  std::atomic<size_t> total_deallocations_{0};
  std::atomic<size_t> alignment_waste_{0};
  std::atomic<size_t> block_count_{0};
};

inline StackAllocator::StackAllocator(StackAllocatorOptions options) noexcept
    : initial_capacity_(options.initial_capacity), growth_(options.growth) {
  HELIOS_ASSERT(initial_capacity_ > 0,
                "initial_capacity must be greater than zero!");
  HELIOS_ASSERT(growth_.max_capacity >= initial_capacity_,
                "max_capacity '{}' must be >= initial_capacity '{}'!",
                growth_.max_capacity, initial_capacity_);

  Block* const initial_block = CreateBlock(initial_capacity_);
  HELIOS_VERIFY(initial_block != nullptr, "Failed to allocate initial block!");
  head_.store(initial_block, std::memory_order_release);
  total_capacity_.store(initial_capacity_, std::memory_order_relaxed);
  block_count_.store(1, std::memory_order_relaxed);
}

inline StackAllocator::StackAllocator(size_t initial_capacity) noexcept
    : StackAllocator(StackAllocatorOptions{.initial_capacity = initial_capacity,
                                           .growth = GrowthPolicy::Fixed()}) {}

inline StackAllocator::StackAllocator(StackAllocator&& other) noexcept
    : head_(other.head_.load(std::memory_order_acquire)),
      grow_state_(other.grow_state_.load(std::memory_order_acquire)),
      initial_capacity_(other.initial_capacity_),
      growth_(other.growth_),
      total_capacity_(other.total_capacity_.load(std::memory_order_acquire)),
      total_allocated_(other.total_allocated_.load(std::memory_order_acquire)),
      peak_usage_(other.peak_usage_.load(std::memory_order_acquire)),
      allocation_count_(
          other.allocation_count_.load(std::memory_order_acquire)),
      total_allocations_(
          other.total_allocations_.load(std::memory_order_acquire)),
      total_deallocations_(
          other.total_deallocations_.load(std::memory_order_acquire)),
      alignment_waste_(other.alignment_waste_.load(std::memory_order_acquire)),
      block_count_(other.block_count_.load(std::memory_order_acquire)) {
  other.head_.store(nullptr, std::memory_order_release);
  other.grow_state_.store(GrowState::kIdle, std::memory_order_release);
  other.initial_capacity_ = 0;
  other.total_capacity_.store(0, std::memory_order_release);
  other.total_allocated_.store(0, std::memory_order_release);
  other.peak_usage_.store(0, std::memory_order_release);
  other.allocation_count_.store(0, std::memory_order_release);
  other.total_allocations_.store(0, std::memory_order_release);
  other.total_deallocations_.store(0, std::memory_order_release);
  other.alignment_waste_.store(0, std::memory_order_release);
  other.block_count_.store(0, std::memory_order_release);
}

inline StackAllocator& StackAllocator::operator=(
    StackAllocator&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  FreeChain(head_.load(std::memory_order_acquire));

  head_.store(other.head_.load(std::memory_order_acquire),
              std::memory_order_release);
  grow_state_.store(other.grow_state_.load(std::memory_order_acquire),
                    std::memory_order_release);
  initial_capacity_ = other.initial_capacity_;
  growth_ = other.growth_;
  total_capacity_.store(other.total_capacity_.load(std::memory_order_acquire),
                        std::memory_order_release);
  total_allocated_.store(other.total_allocated_.load(std::memory_order_acquire),
                         std::memory_order_release);
  peak_usage_.store(other.peak_usage_.load(std::memory_order_acquire),
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
  block_count_.store(other.block_count_.load(std::memory_order_acquire),
                     std::memory_order_release);

  other.head_.store(nullptr, std::memory_order_release);
  other.grow_state_.store(GrowState::kIdle, std::memory_order_release);
  other.initial_capacity_ = 0;
  other.total_capacity_.store(0, std::memory_order_release);
  other.total_allocated_.store(0, std::memory_order_release);
  other.peak_usage_.store(0, std::memory_order_release);
  other.allocation_count_.store(0, std::memory_order_release);
  other.total_allocations_.store(0, std::memory_order_release);
  other.total_deallocations_.store(0, std::memory_order_release);
  other.alignment_waste_.store(0, std::memory_order_release);
  other.block_count_.store(0, std::memory_order_release);

  return *this;
}

inline auto StackAllocator::GetMarker() const noexcept -> Marker {
  Block* const head = head_.load(std::memory_order_acquire);
  if (head == nullptr) {
    return {};
  }
  return {
      .block = head,
      .offset = head->offset.load(std::memory_order_acquire),
  };
}

inline void StackAllocator::Reset() noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::StackAllocator::Reset");

  Block* const head = head_.load(std::memory_order_acquire);
  if (head == nullptr) {
    return;
  }

  FreeChain(head);
  Block* const fresh = CreateBlock(initial_capacity_);
  HELIOS_VERIFY(fresh != nullptr, "Failed to allocate block during reset!");

  head_.store(fresh, std::memory_order_release);
  total_capacity_.store(initial_capacity_, std::memory_order_release);
  total_allocated_.store(0, std::memory_order_release);
  allocation_count_.store(0, std::memory_order_release);
  total_allocations_.store(0, std::memory_order_release);
  total_deallocations_.store(0, std::memory_order_release);
  alignment_waste_.store(0, std::memory_order_release);
  block_count_.store(1, std::memory_order_release);
}

inline AllocatorStats StackAllocator::Stats() const noexcept {
  return {
      .total_allocated = total_allocated_.load(std::memory_order_relaxed),
      .peak_usage = peak_usage_.load(std::memory_order_relaxed),
      .allocation_count = allocation_count_.load(std::memory_order_relaxed),
      .total_allocations = total_allocations_.load(std::memory_order_relaxed),
      .total_deallocations =
          total_deallocations_.load(std::memory_order_relaxed),
      .alignment_waste = alignment_waste_.load(std::memory_order_relaxed),
  };
}

}  // namespace helios::mem
