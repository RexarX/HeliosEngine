#pragma once

#include <helios/assert.hpp>
#include <helios/memory/aligned_alloc.hpp>
#include <helios/memory/common.hpp>
#include <helios/memory/details/profile.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory_resource>

namespace helios::mem {

/**
 * @brief PMR arena with fixed runtime capacity.
 * @details Lock-free bump allocation with no growth. Individual deallocation is
 * a no-op; memory is reclaimed by `Reset()` or destruction.
 * @warning `Reset()` must not run concurrently with `allocate`.
 */
class FixedArenaAllocator final : public std::pmr::memory_resource {
public:
  /**
   * @brief Constructs fixed arena with the given backing capacity.
   * @warning Triggers assertion if `capacity == 0`. Triggers verification
   * failure if backing allocation fails.
   * @param capacity Backing buffer size in bytes
   */
  explicit FixedArenaAllocator(size_t capacity) noexcept;
  FixedArenaAllocator(const FixedArenaAllocator&) = delete;

  /**
   * @brief Move-constructs fixed arena from another instance.
   * @param other Source allocator; left in empty moved-from state
   */
  FixedArenaAllocator(FixedArenaAllocator&& other) noexcept { MoveFrom(other); }
  ~FixedArenaAllocator() noexcept override { Release(); }

  FixedArenaAllocator& operator=(const FixedArenaAllocator&) = delete;

  /**
   * @brief Move-assigns fixed arena state from another instance.
   * @param other Source allocator; left in empty moved-from state
   * @return Reference to this allocator
   */
  FixedArenaAllocator& operator=(FixedArenaAllocator&& other) noexcept;

  /**
   * @brief Soft-resets bump offset and statistics.
   * @warning Must not be called concurrently with `allocate`.
   */
  void Reset() noexcept;

  /**
   * @brief Returns true when no bytes have been bumped.
   * @return True if empty, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept {
    return offset_.load(std::memory_order_relaxed) == 0;
  }

  /**
   * @brief Checks whether pointer belongs to this arena's backing buffer.
   * @param ptr Pointer to test
   * @return True when owned, false otherwise
   */
  [[nodiscard]] bool Owns(const void* ptr) const noexcept;

  /**
   * @brief Returns runtime statistics snapshot.
   * @return Allocator stats for this arena
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

private:
  void MoveFrom(FixedArenaAllocator& other) noexcept;
  void Release() noexcept;

  [[nodiscard]] void* do_allocate(size_t bytes, size_t alignment) override;
  void do_deallocate(void* ptr, size_t bytes, size_t alignment) override;
  [[nodiscard]] bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override {
    return this == &other;
  }

  size_t capacity_ = 0;
  std::byte* buffer_ = nullptr;
  std::atomic<size_t> offset_{0};
  std::atomic<size_t> peak_usage_{0};
  std::atomic<size_t> allocation_count_{0};
  std::atomic<size_t> total_allocations_{0};
  std::atomic<size_t> total_deallocations_{0};
  std::atomic<size_t> alignment_waste_{0};
};

inline FixedArenaAllocator::FixedArenaAllocator(size_t capacity) noexcept
    : capacity_(capacity) {
  HELIOS_ASSERT(capacity_ > 0, "capacity must be greater than zero!");
  buffer_ = static_cast<std::byte*>(
      AlignedAlloc(kDefaultAlignment, capacity_, false));
  HELIOS_VERIFY(buffer_ != nullptr, "Failed to allocate fixed arena!");
  HELIOS_MEMORY_PROFILE_ALLOC(buffer_, capacity_, "FixedArenaAllocator");
}

inline FixedArenaAllocator& FixedArenaAllocator::operator=(
    FixedArenaAllocator&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  Release();
  MoveFrom(other);
  return *this;
}

inline void FixedArenaAllocator::Reset() noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::FixedArenaAllocator::Reset");

  offset_.store(0, std::memory_order_relaxed);
  peak_usage_.store(0, std::memory_order_relaxed);
  allocation_count_.store(0, std::memory_order_relaxed);
  total_allocations_.store(0, std::memory_order_relaxed);
  total_deallocations_.store(0, std::memory_order_relaxed);
  alignment_waste_.store(0, std::memory_order_relaxed);
}

inline bool FixedArenaAllocator::Owns(const void* ptr) const noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::FixedArenaAllocator::Owns");

  if (buffer_ == nullptr || ptr == nullptr) {
    return false;
  }

  const auto address = reinterpret_cast<uintptr_t>(ptr);
  const auto begin = reinterpret_cast<uintptr_t>(buffer_);
  return address >= begin && address < begin + capacity_;
}

inline AllocatorStats FixedArenaAllocator::Stats() const noexcept {
  return {
      .total_allocated = offset_.load(std::memory_order_relaxed),
      .peak_usage = peak_usage_.load(std::memory_order_relaxed),
      .allocation_count = allocation_count_.load(std::memory_order_relaxed),
      .total_allocations = total_allocations_.load(std::memory_order_relaxed),
      .total_deallocations =
          total_deallocations_.load(std::memory_order_relaxed),
      .alignment_waste = alignment_waste_.load(std::memory_order_relaxed),
  };
}

}  // namespace helios::mem
