#pragma once

#include <helios/memory/common.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory_resource>

namespace helios::mem {

/**
 * @brief Configuration for ArenaAllocator.
 * @details Controls initial capacity and growth behavior.
 */
struct ArenaOptions {
  /// @brief Capacity of the initial backing block in bytes.
  size_t initial_capacity = 0;
  /// @brief Growth policy applied when additional blocks are required.
  GrowthPolicy growth = GrowthPolicy::Geometric();
};

/**
 * @brief PMR arena allocator with lock-free hot allocation path.
 * @details Individual deallocation is a no-op, matching monotonic arena
 * semantics. Memory is reclaimed by `Reset()` or destruction.
 * @note Thread-safe for `allocate` and `deallocate`.
 * @warning `Reset()` must not run concurrently with allocation/deallocation.
 */
class ArenaAllocator final : public std::pmr::memory_resource {
public:
  /**
   * @brief Constructs arena allocator from options.
   * @warning Triggers assertion if `options.initial_capacity == 0` or
   * `options.growth.max_capacity < options.initial_capacity`.
   * @param options Arena configuration
   */
  explicit ArenaAllocator(ArenaOptions options = {}) noexcept;

  /**
   * @brief Constructs arena allocator with geometric growth.
   * @warning Triggers assertion if `initial_capacity == 0`.
   * @param initial_capacity Capacity of initial block in bytes
   */
  explicit ArenaAllocator(size_t initial_capacity) noexcept
      : ArenaAllocator(ArenaOptions{.initial_capacity = initial_capacity,
                                    .growth = GrowthPolicy::Geometric()}) {}

  ArenaAllocator(const ArenaAllocator&) = delete;

  /**
   * @brief Move-constructs arena allocator from another instance.
   * @param other Source allocator; left in empty moved-from state
   */
  ArenaAllocator(ArenaAllocator&& other) noexcept { MoveFrom(other); }
  ~ArenaAllocator() noexcept override {
    FreeChain(head_.load(std::memory_order_acquire));
  }

  ArenaAllocator& operator=(const ArenaAllocator&) = delete;

  /**
   * @brief Move-assigns arena state from another instance.
   * @param other Source allocator; left in empty moved-from state
   * @return Reference to this allocator
   */
  ArenaAllocator& operator=(ArenaAllocator&& other) noexcept;

  /**
   * @brief Soft-resets all block bump pointers to zero.
   * @details All existing allocations become logically invalid. Blocks are
   * kept allocated and offsets reset to the start, allowing immediate reuse.
   * @warning Must not be called concurrently with `allocate`.
   */
  void Reset() noexcept;

  /**
   * @brief Returns true when no allocations are currently active.
   * @details Uses a monotonic allocation counter; stays false after any
   * allocation until `Reset()` even though individual deallocations are no-ops.
   * @return True if arena is empty, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept {
    return allocation_count_.load(std::memory_order_relaxed) == 0;
  }

  /**
   * @brief Returns allocator statistics snapshot.
   * @return AllocatorStats for this arena
   */
  [[nodiscard]] AllocatorStats Stats() const noexcept;

  /**
   * @brief Returns total capacity across all active blocks.
   * @return Capacity in bytes
   */
  [[nodiscard]] size_t TotalCapacity() const noexcept {
    return total_capacity_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Returns initial block capacity.
   * @return Initial capacity in bytes
   */
  [[nodiscard]] size_t InitialCapacity() const noexcept {
    return initial_capacity_;
  }

  /**
   * @brief Returns current number of blocks.
   * @return Block count
   */
  [[nodiscard]] size_t BlockCount() const noexcept {
    return block_count_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Returns configured growth policy.
   * @return Growth policy
   */
  [[nodiscard]] GrowthPolicy Growth() const noexcept { return growth_; }

private:
  struct Block {
    void* buffer = nullptr;
    size_t capacity = 0;
    std::atomic<size_t> offset{0};
    std::atomic<Block*> next{nullptr};
  };

  struct Reservation {
    void* ptr = nullptr;
    size_t size = 0;
    size_t padding = 0;
  };

  enum class GrowState : uint8_t { kIdle, kGrowing };

  void MoveFrom(ArenaAllocator& other) noexcept;

  [[nodiscard]] static Block* CreateBlock(size_t capacity) noexcept;
  void PublishBlock(Block* block) noexcept;
  static void FreeChain(Block* head) noexcept;

  [[nodiscard]] static Reservation TryReserve(Block& block, size_t size,
                                              size_t alignment) noexcept;

  [[nodiscard]] bool EnsureCapacity(size_t min_capacity) noexcept;

  [[nodiscard]] void* do_allocate(size_t bytes, size_t alignment) override;
  void do_deallocate(void* /*pointer*/, size_t /*bytes*/,
                     size_t /*alignment*/) override {
    total_deallocations_.fetch_add(1, std::memory_order_relaxed);
  }

  [[nodiscard]] bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override {
    return this == &other;
  }

  std::atomic<Block*> head_{nullptr};
  std::atomic<GrowState> grow_state_{GrowState::kIdle};
  size_t initial_capacity_ = 0;
  GrowthPolicy growth_;

  std::atomic<size_t> total_capacity_{0};
  std::atomic<size_t> total_allocated_{0};
  std::atomic<size_t> peak_usage_{0};
  std::atomic<size_t> allocation_count_{0};
  std::atomic<size_t> total_allocations_{0};
  std::atomic<size_t> total_deallocations_{0};
  std::atomic<size_t> alignment_waste_{0};
  std::atomic<size_t> block_count_{0};
};

inline ArenaAllocator& ArenaAllocator::operator=(
    ArenaAllocator&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  FreeChain(head_.load(std::memory_order_acquire));
  MoveFrom(other);
  return *this;
}

inline AllocatorStats ArenaAllocator::Stats() const noexcept {
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
