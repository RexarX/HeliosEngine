#pragma once

#include <helios/assert.hpp>
#include <helios/memory/common.hpp>
#include <helios/memory/details/profile.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory_resource>

namespace helios::mem {

/// @brief Configuration for PoolAllocator.
struct PoolAllocatorOptions {
  size_t block_size = 0;
  size_t block_count = 0;
  size_t alignment = kDefaultAlignment;
  GrowthPolicy growth = GrowthPolicy::Fixed();
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
   * @warning Triggers assertion when block_size or block_count is zero.
   * @param options Pool allocator options
   */
  explicit PoolAllocator(PoolAllocatorOptions options) noexcept;

  /**
   * @brief Constructs pool allocator with fixed growth.
   * @param block_size Size of each block in bytes
   * @param block_count Number of blocks in initial chunk
   * @param alignment Block alignment
   */
  PoolAllocator(size_t block_size, size_t block_count,
                size_t alignment = kDefaultAlignment) noexcept;

  PoolAllocator(const PoolAllocator&) = delete;
  PoolAllocator(PoolAllocator&& other) noexcept;
  ~PoolAllocator() noexcept override {
    FreeChunkChain(chunks_.load(std::memory_order_acquire));
  }

  PoolAllocator& operator=(const PoolAllocator&) = delete;
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

inline PoolAllocator::PoolAllocator(PoolAllocatorOptions options) noexcept
    : block_size_(std::max(options.block_size, sizeof(void*))),
      initial_block_count_(options.block_count),
      alignment_(options.alignment),
      growth_(options.growth) {
  HELIOS_ASSERT(block_size_ > 0, "block_size must be greater than zero!");
  HELIOS_ASSERT(initial_block_count_ > 0,
                "block_count must be greater than zero!");
  HELIOS_ASSERT(IsPowerOfTwo(alignment_),
                "alignment '{}' must be power of two!", alignment_);
  HELIOS_ASSERT(alignment_ >= alignof(void*), "alignment '{}' must be >= '{}'!",
                alignment_, alignof(void*));

  block_size_ = AlignUp(block_size_, alignment_);

  ChunkHeader* const initial_chunk =
      CreateChunk(block_size_, initial_block_count_, alignment_);
  HELIOS_VERIFY(initial_chunk != nullptr, "Failed to allocate pool chunk!");

  chunks_.store(initial_chunk, std::memory_order_release);
  total_blocks_.store(initial_chunk->block_count, std::memory_order_relaxed);
  free_blocks_.store(initial_chunk->block_count, std::memory_order_relaxed);
  PushChunkBlocks(*initial_chunk);
}

inline PoolAllocator::PoolAllocator(size_t block_size, size_t block_count,
                                    size_t alignment) noexcept
    : PoolAllocator(PoolAllocatorOptions{.block_size = block_size,
                                         .block_count = block_count,
                                         .alignment = alignment,
                                         .growth = GrowthPolicy::Fixed()}) {}

template <typename T>
inline PoolAllocator PoolAllocator::ForType(size_t block_count) noexcept {
  constexpr size_t kAlign =
      alignof(T) > alignof(void*) ? alignof(T) : alignof(void*);
  return {sizeof(T), block_count, kAlign};
}

inline PoolAllocator::PoolAllocator(PoolAllocator&& other) noexcept
    : block_size_(other.block_size_),
      initial_block_count_(other.initial_block_count_),
      alignment_(other.alignment_),
      growth_(other.growth_),
      free_head_(other.free_head_.load(std::memory_order_acquire)),
      chunks_(other.chunks_.load(std::memory_order_acquire)),
      grow_state_(other.grow_state_.load(std::memory_order_acquire)),
      total_blocks_(other.total_blocks_.load(std::memory_order_acquire)),
      free_blocks_(other.free_blocks_.load(std::memory_order_acquire)),
      peak_used_blocks_(
          other.peak_used_blocks_.load(std::memory_order_acquire)),
      total_allocations_(
          other.total_allocations_.load(std::memory_order_acquire)),
      total_deallocations_(
          other.total_deallocations_.load(std::memory_order_acquire)) {
  other.block_size_ = 0;
  other.initial_block_count_ = 0;
  other.alignment_ = 0;
  other.free_head_.store(nullptr, std::memory_order_release);
  other.chunks_.store(nullptr, std::memory_order_release);
  other.grow_state_.store(GrowState::kIdle, std::memory_order_release);
  other.total_blocks_.store(0, std::memory_order_release);
  other.free_blocks_.store(0, std::memory_order_release);
  other.peak_used_blocks_.store(0, std::memory_order_release);
  other.total_allocations_.store(0, std::memory_order_release);
  other.total_deallocations_.store(0, std::memory_order_release);
}

inline PoolAllocator& PoolAllocator::operator=(PoolAllocator&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  FreeChunkChain(chunks_.load(std::memory_order_acquire));

  block_size_ = other.block_size_;
  initial_block_count_ = other.initial_block_count_;
  alignment_ = other.alignment_;
  growth_ = other.growth_;
  free_head_.store(other.free_head_.load(std::memory_order_acquire),
                   std::memory_order_release);
  chunks_.store(other.chunks_.load(std::memory_order_acquire),
                std::memory_order_release);
  grow_state_.store(other.grow_state_.load(std::memory_order_acquire),
                    std::memory_order_release);
  total_blocks_.store(other.total_blocks_.load(std::memory_order_acquire),
                      std::memory_order_release);
  free_blocks_.store(other.free_blocks_.load(std::memory_order_acquire),
                     std::memory_order_release);
  peak_used_blocks_.store(
      other.peak_used_blocks_.load(std::memory_order_acquire),
      std::memory_order_release);
  total_allocations_.store(
      other.total_allocations_.load(std::memory_order_acquire),
      std::memory_order_release);
  total_deallocations_.store(
      other.total_deallocations_.load(std::memory_order_acquire),
      std::memory_order_release);

  other.block_size_ = 0;
  other.initial_block_count_ = 0;
  other.alignment_ = 0;
  other.free_head_.store(nullptr, std::memory_order_release);
  other.chunks_.store(nullptr, std::memory_order_release);
  other.grow_state_.store(GrowState::kIdle, std::memory_order_release);
  other.total_blocks_.store(0, std::memory_order_release);
  other.free_blocks_.store(0, std::memory_order_release);
  other.peak_used_blocks_.store(0, std::memory_order_release);
  other.total_allocations_.store(0, std::memory_order_release);
  other.total_deallocations_.store(0, std::memory_order_release);

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
