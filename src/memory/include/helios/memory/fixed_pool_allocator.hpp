#pragma once

#include <helios/assert.hpp>
#include <helios/memory/aligned_alloc.hpp>
#include <helios/memory/common.hpp>
#include <helios/memory/details/profile.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <memory_resource>

namespace helios::mem {

/// @brief Configuration for FixedPoolAllocator.
struct FixedPoolAllocatorOptions {
  /// @brief Minimum payload size of each pool block in bytes.
  size_t block_size = 0;
  /// @brief Number of blocks in the pool.
  size_t block_count = 0;
  /// @brief Alignment of each block in bytes; must be a power of two.
  size_t alignment = kDefaultAlignment;
};

/**
 * @brief Fixed-capacity intrusive block pool.
 * @details Lock-free Treiber free-list over a single preallocated chunk.
 * @warning `Reset()` must not run concurrently with `allocate` or `deallocate`.
 */
class FixedPoolAllocator final : public std::pmr::memory_resource {
public:
  /**
   * @brief Constructs fixed pool from options.
   * @warning Triggers assertion when block_size or block_count is zero, or when
   * alignment is invalid. Triggers verification failure if backing allocation
   * fails.
   * @param options Fixed pool configuration
   */
  explicit FixedPoolAllocator(FixedPoolAllocatorOptions options) noexcept;

  /**
   * @brief Constructs fixed pool with explicit block geometry.
   * @warning Triggers assertion when `block_size` or `block_count` is zero, or
   * when alignment is invalid.
   * @param block_size Minimum payload size of each block in bytes
   * @param block_count Number of blocks in the pool
   * @param alignment Block alignment in bytes
   */
  FixedPoolAllocator(size_t block_size, size_t block_count,
                     size_t alignment = kDefaultAlignment) noexcept;
  FixedPoolAllocator(const FixedPoolAllocator&) = delete;

  /**
   * @brief Move-constructs fixed pool from another instance.
   * @param other Source allocator; left in empty moved-from state
   */
  FixedPoolAllocator(FixedPoolAllocator&& other) noexcept { MoveFrom(other); }
  ~FixedPoolAllocator() noexcept override { Release(); }

  FixedPoolAllocator& operator=(const FixedPoolAllocator&) = delete;

  /**
   * @brief Move-assigns fixed pool state from another instance.
   * @param other Source allocator; left in empty moved-from state
   * @return Reference to this allocator
   */
  FixedPoolAllocator& operator=(FixedPoolAllocator&& other) noexcept;

  /**
   * @brief Helper for constructing fixed pool allocator for a specific type.
   * @tparam T Type to pool
   * @param block_count Number of blocks in the pool
   * @return Configured fixed pool allocator
   */
  template <typename T>
  [[nodiscard]] static FixedPoolAllocator ForType(size_t block_count) noexcept;

  /**
   * @brief Resets pool to fully free state.
   * @warning Must not be called concurrently with `allocate` or `deallocate`.
   */
  void Reset() noexcept;

  /**
   * @brief Returns true when no free blocks remain.
   * @return True if full, false otherwise
   */
  [[nodiscard]] bool Full() const noexcept {
    return free_blocks_.load(std::memory_order_relaxed) == 0;
  }

  /**
   * @brief Returns true when all blocks are free.
   * @return True if empty, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept {
    return free_blocks_.load(std::memory_order_relaxed) == block_count_;
  }

  /**
   * @brief Checks pointer ownership.
   * @param ptr Pointer to test
   * @return True when pointer belongs to this pool chunk, false otherwise
   */
  [[nodiscard]] bool Owns(const void* ptr) const noexcept;

  /**
   * @brief Returns runtime statistics snapshot.
   * @return Allocator stats for this pool
   */
  [[nodiscard]] AllocatorStats Stats() const noexcept;

  /**
   * @brief Returns effective block size.
   * @return Block size in bytes
   */
  [[nodiscard]] size_t BlockSize() const noexcept { return block_size_; }

  /**
   * @brief Returns configured block alignment.
   * @return Alignment in bytes
   */
  [[nodiscard]] size_t Alignment() const noexcept { return alignment_; }

  /**
   * @brief Returns configured block count.
   * @return Number of blocks in the pool
   */
  [[nodiscard]] size_t InitialBlockCount() const noexcept {
    return block_count_;
  }

  /**
   * @brief Returns configured block count.
   * @return Number of blocks in the pool
   */
  [[nodiscard]] size_t BlockCount() const noexcept { return block_count_; }

  /**
   * @brief Returns free block count.
   * @return Number of blocks currently on the free list
   */
  [[nodiscard]] size_t FreeBlockCount() const noexcept {
    return free_blocks_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Returns used block count.
   * @return Number of blocks currently allocated
   */
  [[nodiscard]] size_t UsedBlockCount() const noexcept {
    return block_count_ - FreeBlockCount();
  }

private:
  void MoveFrom(FixedPoolAllocator& other) noexcept;
  void Rebuild() noexcept;
  void Release() noexcept;

  [[nodiscard]] void* do_allocate(size_t bytes, size_t alignment) override;
  void do_deallocate(void* ptr, size_t bytes, size_t alignment) override;
  [[nodiscard]] bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override {
    return this == &other;
  }

  size_t block_size_ = 0;
  size_t block_count_ = 0;
  size_t alignment_ = 0;
  size_t chunk_capacity_ = 0;
  std::byte* buffer_ = nullptr;
  std::atomic<void*> free_head_{nullptr};
  std::atomic<size_t> free_blocks_{0};
  std::atomic<size_t> peak_used_blocks_{0};
  std::atomic<size_t> total_allocations_{0};
  std::atomic<size_t> total_deallocations_{0};
};

inline FixedPoolAllocator::FixedPoolAllocator(
    FixedPoolAllocatorOptions options) noexcept
    : block_size_(std::max(options.block_size, sizeof(void*))),
      block_count_(options.block_count),
      alignment_(options.alignment) {
  HELIOS_ASSERT(block_size_ > 0, "block_size must be greater than zero!");
  HELIOS_ASSERT(block_count_ > 0, "block_count must be greater than zero!");
  HELIOS_ASSERT(IsPowerOfTwo(alignment_),
                "alignment '{}' must be power of two!", alignment_);
  HELIOS_ASSERT(alignment_ >= alignof(void*), "alignment '{}' must be >= '{}'!",
                alignment_, alignof(void*));

  block_size_ = AlignUp(block_size_, alignment_);
  chunk_capacity_ = block_size_ * block_count_;

  buffer_ =
      static_cast<std::byte*>(AlignedAlloc(alignment_, chunk_capacity_, false));
  HELIOS_VERIFY(buffer_ != nullptr, "Failed to allocate fixed pool!");
  HELIOS_MEMORY_PROFILE_ALLOC(buffer_, chunk_capacity_, "FixedPoolAllocator");
  Rebuild();
}

inline FixedPoolAllocator::FixedPoolAllocator(size_t block_size,
                                              size_t block_count,
                                              size_t alignment) noexcept
    : FixedPoolAllocator(FixedPoolAllocatorOptions{
          .block_size = block_size,
          .block_count = block_count,
          .alignment = alignment,
      }) {}

inline FixedPoolAllocator& FixedPoolAllocator::operator=(
    FixedPoolAllocator&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  Release();
  MoveFrom(other);
  return *this;
}

template <typename T>
inline FixedPoolAllocator FixedPoolAllocator::ForType(
    size_t block_count) noexcept {
  constexpr size_t type_align =
      alignof(T) > alignof(void*) ? alignof(T) : alignof(void*);
  return {sizeof(T), block_count, type_align};
}

inline void FixedPoolAllocator::Reset() noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::FixedPoolAllocator::Reset");

  Rebuild();
  peak_used_blocks_.store(0, std::memory_order_relaxed);
  total_allocations_.store(0, std::memory_order_relaxed);
  total_deallocations_.store(0, std::memory_order_relaxed);
}

inline bool FixedPoolAllocator::Owns(const void* ptr) const noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::FixedPoolAllocator::Owns");

  if (buffer_ == nullptr || ptr == nullptr) [[unlikely]] {
    return false;
  }

  const auto address = reinterpret_cast<uintptr_t>(ptr);
  const auto begin = reinterpret_cast<uintptr_t>(buffer_);
  return address >= begin && address < begin + chunk_capacity_ &&
         (address - begin) % block_size_ == 0;
}

inline AllocatorStats FixedPoolAllocator::Stats() const noexcept {
  const size_t used = UsedBlockCount();
  return {
      .total_allocated = used * block_size_,
      .peak_usage =
          peak_used_blocks_.load(std::memory_order_relaxed) * block_size_,
      .allocation_count = used,
      .total_allocations = total_allocations_.load(std::memory_order_relaxed),
      .total_deallocations =
          total_deallocations_.load(std::memory_order_relaxed),
      .alignment_waste = 0,
  };
}

}  // namespace helios::mem
