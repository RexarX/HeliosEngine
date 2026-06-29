#pragma once

#include <helios/assert.hpp>
#include <helios/memory/aligned_alloc.hpp>
#include <helios/memory/common.hpp>
#include <helios/memory/details/profile.hpp>

#include <atomic>
#include <cstddef>
#include <memory_resource>

namespace helios::mem {

/**
 * @brief PMR stack allocator with fixed runtime capacity.
 * @details LIFO-aware deallocation on the current top allocation uses CAS on
 * the bump offset. Non-LIFO frees are no-ops for PMR compatibility.
 * @note `allocate` and LIFO `deallocate` on the current head are lock-free.
 * @warning `Reset()` and `RewindToMarker()` must not run concurrently with
 * `allocate` or `deallocate`.
 */
class FixedStackAllocator final : public std::pmr::memory_resource {
public:
  /// @brief Marker capturing the current bump offset for rewind operations.
  struct Marker {
    /// @brief Bump offset at marker capture time.
    size_t offset = 0;
  };

  /**
   * @brief Constructs fixed stack with the given backing capacity.
   * @warning Triggers assertion if capacity is too small. Triggers
   * verification failure if backing allocation fails.
   * @param capacity Backing buffer size in bytes
   */
  explicit FixedStackAllocator(size_t capacity) noexcept;
  FixedStackAllocator(const FixedStackAllocator&) = delete;

  /**
   * @brief Move-constructs fixed stack from another instance.
   * @param other Source allocator; left in empty moved-from state
   */
  FixedStackAllocator(FixedStackAllocator&& other) noexcept { MoveFrom(other); }
  ~FixedStackAllocator() noexcept override { Release(); }

  FixedStackAllocator& operator=(const FixedStackAllocator&) = delete;

  /**
   * @brief Move-assigns fixed stack state from another instance.
   * @param other Source allocator; left in empty moved-from state
   * @return Reference to this allocator
   */
  FixedStackAllocator& operator=(FixedStackAllocator&& other) noexcept;

  /**
   * @brief Rewinds bump offset to marker.
   * @warning Must not be called concurrently with `allocate` or `deallocate`.
   * Triggers assertion when `marker.offset` exceeds the current offset.
   * @param marker Marker previously returned by `GetMarker`
   */
  void RewindToMarker(Marker marker) noexcept;

  /**
   * @brief Soft-resets bump offset and statistics.
   * @warning Must not be called concurrently with `allocate` or `deallocate`.
   */
  void Reset() noexcept;

  /**
   * @brief Returns true when no active allocations remain.
   * @return True if empty, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept {
    return offset_.load(std::memory_order_relaxed) == 0;
  }

  /**
   * @brief Checks whether pointer belongs to this stack's backing buffer.
   * @param ptr Pointer to test
   * @return True when owned, false otherwise
   */
  [[nodiscard]] bool Owns(const void* ptr) const noexcept;

  /**
   * @brief Returns current marker.
   * @return Marker that can later be rewound to
   */
  [[nodiscard]] Marker GetMarker() const noexcept {
    return {offset_.load(std::memory_order_acquire)};
  }

  /**
   * @brief Returns runtime statistics snapshot.
   * @return Allocator stats for this stack
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
  struct AllocationHeader {
    size_t previous_offset = 0;
    size_t end_offset = 0;
  };

  void MoveFrom(FixedStackAllocator& other) noexcept;

  void ClearStats() noexcept;
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

inline FixedStackAllocator::FixedStackAllocator(size_t capacity) noexcept
    : capacity_(capacity) {
  HELIOS_ASSERT(capacity_ > sizeof(size_t) * 2,
                "capacity '{}' is too small for fixed stack!", capacity_);
  buffer_ = static_cast<std::byte*>(
      AlignedAlloc(kDefaultAlignment, capacity_, false));
  HELIOS_VERIFY(buffer_ != nullptr, "Failed to allocate fixed stack!");
  HELIOS_MEMORY_PROFILE_ALLOC(buffer_, capacity_, "FixedStackAllocator");
}

inline FixedStackAllocator& FixedStackAllocator::operator=(
    FixedStackAllocator&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  Release();
  MoveFrom(other);
  return *this;
}

inline void FixedStackAllocator::RewindToMarker(Marker marker) noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N(
      "helios::mem::FixedStackAllocator::RewindToMarker");

  HELIOS_ASSERT(marker.offset <= offset_.load(std::memory_order_acquire),
                "marker does not belong to fixed stack!");

  offset_.store(marker.offset, std::memory_order_release);
  allocation_count_.store(0, std::memory_order_relaxed);
}

inline void FixedStackAllocator::Reset() noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::FixedStackAllocator::Reset");
  ClearStats();
}

inline bool FixedStackAllocator::Owns(const void* ptr) const noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::FixedStackAllocator::Owns");

  if (buffer_ == nullptr || ptr == nullptr) [[unlikely]] {
    return false;
  }

  const auto address = reinterpret_cast<uintptr_t>(ptr);
  const auto begin = reinterpret_cast<uintptr_t>(buffer_);
  return address >= begin && address < begin + capacity_;
}

inline AllocatorStats FixedStackAllocator::Stats() const noexcept {
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
