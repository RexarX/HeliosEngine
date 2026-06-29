#pragma once

#include <helios/assert.hpp>
#include <helios/memory/common.hpp>
#include <helios/memory/details/profile.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <utility>

namespace helios::mem {

/// @brief Configuration for PoolAllocator.
struct PoolAllocatorOptions {
  /// @brief Minimum payload size of each pool block in bytes.
  size_t block_size = 0;
  /// @brief Number of blocks in the initial chunk.
  size_t block_count = 0;
  /// @brief Alignment of each block in bytes; must be a power of two.
  size_t alignment = kDefaultAlignment;
  /// @brief Growth policy applied when additional chunks are required.
  GrowthPolicy growth = GrowthPolicy::Geometric();
};

/**
 * @brief Lock-free PMR pool allocator for fixed-size blocks.
 * @details Allocates from fixed-size blocks stored in growable chunks.
 *
 * Fast path uses lock-free freelist push/pop based on embedded next pointers.
 * Growth allocates and links a new chunk, then pushes all its blocks into the
 * freelist.
 */
class PoolAllocator final : public std::pmr::memory_resource {
public:
  /**
   * @brief Constructs pool allocator from options.
   * @warning Triggers assertion when block_size or block_count is zero, when
   * alignment is not a power of two, or when alignment is less than
   * `alignof(void*)`.
   * @param options Pool allocator options
   */
  explicit PoolAllocator(PoolAllocatorOptions options) noexcept;

  /**
   * @brief Constructs pool allocator with geometric growth.
   * @warning Triggers assertion when `block_size` or `block_count` is zero,
   * when `alignment` is not a power of two, or when `alignment` is less than
   * `alignof(void*)`.
   * @param block_size Size of each block in bytes
   * @param block_count Number of blocks in initial chunk
   * @param alignment Block alignment
   */
  PoolAllocator(size_t block_size, size_t block_count,
                size_t alignment = kDefaultAlignment) noexcept
      : PoolAllocator(
            PoolAllocatorOptions{.block_size = block_size,
                                 .block_count = block_count,
                                 .alignment = alignment,
                                 .growth = GrowthPolicy::Geometric()}) {}

  PoolAllocator(const PoolAllocator&) = delete;

  /**
   * @brief Move-constructs pool allocator from another instance.
   * @param other Source allocator; left in empty moved-from state
   */
  PoolAllocator(PoolAllocator&& other) noexcept { MoveFrom(other); }
  ~PoolAllocator() noexcept override {
    FreeChunkChain(chunks_.load(std::memory_order_acquire));
  }

  PoolAllocator& operator=(const PoolAllocator&) = delete;

  /**
   * @brief Move-assigns pool state from another instance.
   * @param other Source allocator; left in empty moved-from state
   * @return Reference to this allocator
   */
  PoolAllocator& operator=(PoolAllocator&& other) noexcept;

  /**
   * @brief Helper for constructing pool allocator for specific type.
   * @tparam T Type to pool
   * @param block_count Number of blocks in initial chunk
   * @return Configured pool allocator
   */
  template <typename T>
  [[nodiscard]] static PoolAllocator ForType(size_t block_count) noexcept;

  /**
   * @brief Resets pool to fully free state.
   * @warning Must not be called concurrently with allocate/deallocate.
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
    return free_blocks_.load(std::memory_order_relaxed) ==
           total_blocks_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Checks pointer ownership.
   * @param ptr Pointer to test
   * @return True when pointer belongs to any pool chunk, false otherwise
   */
  [[nodiscard]] bool Owns(const void* ptr) const noexcept;

  /**
   * @brief Returns runtime stats.
   * @return Allocator stats
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
   * @brief Returns number of blocks in first chunk.
   * @return Initial block count
   */
  [[nodiscard]] size_t InitialBlockCount() const noexcept {
    return initial_block_count_;
  }

  /**
   * @brief Returns total blocks across chunks.
   * @return Total block count
   */
  [[nodiscard]] size_t BlockCount() const noexcept {
    return total_blocks_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Returns free block count.
   * @return Free block count
   */
  [[nodiscard]] size_t FreeBlockCount() const noexcept {
    return free_blocks_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Returns used block count.
   * @return Used block count
   */
  [[nodiscard]] size_t UsedBlockCount() const noexcept;

  /**
   * @brief Returns growth policy.
   * @return Growth policy
   */
  [[nodiscard]] GrowthPolicy Growth() const noexcept { return growth_; }

private:
  struct ChunkHeader {
    void* buffer = nullptr;
    size_t capacity = 0;
    size_t block_count = 0;
    std::atomic<ChunkHeader*> next{nullptr};
  };

  enum class GrowState : uint8_t {
    kIdle,
    kGrowing,
  };

  [[nodiscard]] static ChunkHeader* CreateChunk(size_t block_size,
                                                size_t block_count,
                                                size_t alignment) noexcept;
  void FreeChunkChain(ChunkHeader* chunk) noexcept;

  [[nodiscard]] bool GrowIfNeeded() noexcept;
  void PushChunkBlocks(ChunkHeader& chunk) noexcept;
  void RebuildFreeList() noexcept;
  void MoveFrom(PoolAllocator& other) noexcept;

  [[nodiscard]] void* do_allocate(size_t bytes, size_t alignment) override;
  void do_deallocate(void* ptr, size_t bytes, size_t alignment) override;
  [[nodiscard]] bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override {
    return this == &other;
  }

  size_t block_size_ = 0;
  size_t initial_block_count_ = 0;
  size_t alignment_ = 0;
  GrowthPolicy growth_;

  std::atomic<void*> free_head_{nullptr};
  std::atomic<ChunkHeader*> chunks_{nullptr};
  std::atomic<GrowState> grow_state_{GrowState::kIdle};

  std::atomic<size_t> total_blocks_{0};
  std::atomic<size_t> free_blocks_{0};
  std::atomic<size_t> peak_used_blocks_{0};
  std::atomic<size_t> total_allocations_{0};
  std::atomic<size_t> total_deallocations_{0};
};

template <typename T>
inline PoolAllocator PoolAllocator::ForType(size_t block_count) noexcept {
  constexpr size_t kAlign =
      alignof(T) > alignof(void*) ? alignof(T) : alignof(void*);
  return {sizeof(T), block_count, kAlign};
}

inline PoolAllocator& PoolAllocator::operator=(PoolAllocator&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  FreeChunkChain(chunks_.load(std::memory_order_acquire));
  MoveFrom(other);
  return *this;
}

inline void PoolAllocator::Reset() noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::PoolAllocator::Reset");

  RebuildFreeList();
  total_allocations_.store(0, std::memory_order_release);
  total_deallocations_.store(0, std::memory_order_release);
}

inline auto PoolAllocator::Stats() const noexcept -> AllocatorStats {
  const size_t total = total_blocks_.load(std::memory_order_relaxed);
  const size_t free = free_blocks_.load(std::memory_order_relaxed);
  const size_t used = total >= free ? total - free : 0;
  const size_t peak = peak_used_blocks_.load(std::memory_order_relaxed);

  return {
      .total_allocated = SaturatingMul(used, block_size_),
      .peak_usage = SaturatingMul(peak, block_size_),
      .allocation_count = used,
      .total_allocations = total_allocations_.load(std::memory_order_relaxed),
      .total_deallocations =
          total_deallocations_.load(std::memory_order_relaxed),
      .alignment_waste = 0,
  };
}

inline size_t PoolAllocator::UsedBlockCount() const noexcept {
  const size_t total = total_blocks_.load(std::memory_order_relaxed);
  const size_t free = free_blocks_.load(std::memory_order_relaxed);
  return total >= free ? total - free : 0;
}

}  // namespace helios::mem
