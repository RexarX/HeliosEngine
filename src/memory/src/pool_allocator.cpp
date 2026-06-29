#include <pch.hpp>

#include <helios/memory/pool_allocator.hpp>

#include <details/accumulate_peak.hpp>
#include <helios/assert.hpp>
#include <helios/memory/aligned_alloc.hpp>
#include <helios/memory/common.hpp>
#include <helios/memory/details/profile.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

namespace {

void PushBlock(std::atomic<void*>& head, void* block) noexcept {
  void* observed = head.load(std::memory_order_acquire);
  for (;;) {
    *static_cast<void**>(block) = observed;
    if (head.compare_exchange_weak(observed, block, std::memory_order_release,
                                   std::memory_order_acquire)) {
      return;
    }
  }
}

[[nodiscard]] void* PopBlock(std::atomic<void*>& head) noexcept {
  void* observed = head.load(std::memory_order_acquire);
  for (;;) {
    if (observed == nullptr) {
      return nullptr;
    }
    void* const next = *static_cast<void**>(observed);
    if (head.compare_exchange_weak(observed, next, std::memory_order_release,
                                   std::memory_order_acquire)) {
      return observed;
    }
  }
}

}  // namespace

namespace helios::mem {

PoolAllocator::PoolAllocator(PoolAllocatorOptions options) noexcept
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

bool PoolAllocator::Owns(const void* ptr) const noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::PoolAllocator::Owns");

  if (ptr == nullptr) [[unlikely]] {
    return false;
  }

  const auto addr = reinterpret_cast<uintptr_t>(ptr);
  ChunkHeader* chunk = chunks_.load(std::memory_order_acquire);
  while (chunk != nullptr) {
    const auto begin = reinterpret_cast<uintptr_t>(chunk->buffer);
    const auto end = begin + chunk->capacity;
    if (addr >= begin && addr < end) {
      return ((addr - begin) % block_size_) == 0;
    }
    chunk = chunk->next.load(std::memory_order_acquire);
  }

  return false;
}

void PoolAllocator::MoveFrom(PoolAllocator& other) noexcept {
  block_size_ = std::exchange(other.block_size_, 0);
  initial_block_count_ = std::exchange(other.initial_block_count_, 0);
  alignment_ = std::exchange(other.alignment_, 0);
  growth_ = std::exchange(other.growth_, {});
  free_head_.store(
      other.free_head_.exchange(nullptr, std::memory_order_acq_rel),
      std::memory_order_release);
  chunks_.store(other.chunks_.exchange(nullptr, std::memory_order_acq_rel),
                std::memory_order_release);
  grow_state_.store(
      other.grow_state_.exchange(GrowState::kIdle, std::memory_order_acq_rel),
      std::memory_order_release);
  total_blocks_.store(
      other.total_blocks_.exchange(0, std::memory_order_acq_rel),
      std::memory_order_release);
  free_blocks_.store(other.free_blocks_.exchange(0, std::memory_order_acq_rel),
                     std::memory_order_release);
  peak_used_blocks_.store(
      other.peak_used_blocks_.exchange(0, std::memory_order_acq_rel),
      std::memory_order_release);
  total_allocations_.store(
      other.total_allocations_.exchange(0, std::memory_order_acq_rel),
      std::memory_order_release);
  total_deallocations_.store(
      other.total_deallocations_.exchange(0, std::memory_order_acq_rel),
      std::memory_order_release);
}

auto PoolAllocator::CreateChunk(size_t block_size, size_t block_count,
                                size_t alignment) noexcept -> ChunkHeader* {
  constexpr size_t kHeaderAlignment = alignof(ChunkHeader);
  const size_t chunk_alignment =
      std::max({alignment, kHeaderAlignment, kMinAlignment});
  const size_t header_size = AlignUp(sizeof(ChunkHeader), chunk_alignment);
  const size_t payload = SaturatingMul(block_size, block_count);
  const size_t total_size = SaturatingAdd(header_size, payload);

  void* const raw = AlignedAlloc(chunk_alignment, total_size, false);
  if (raw == nullptr) [[unlikely]] {
    return nullptr;
  }

  HELIOS_MEMORY_PROFILE_ALLOC(raw, total_size, "PoolAllocator");

  auto* const chunk = std::construct_at(static_cast<ChunkHeader*>(raw));
  chunk->buffer = static_cast<std::byte*>(raw) + header_size;
  chunk->capacity = payload;
  chunk->block_count = block_count;
  chunk->next.store(nullptr, std::memory_order_relaxed);
  return chunk;
}

void PoolAllocator::FreeChunkChain(ChunkHeader* chunk) noexcept {
  if (chunk == nullptr) [[unlikely]] {
    return;
  }

  ChunkHeader* current = chunk;
  while (current != nullptr) {
    ChunkHeader* const next = current->next.load(std::memory_order_relaxed);
    HELIOS_MEMORY_PROFILE_FREE(current, "PoolAllocator");
    std::destroy_at(current);
    AlignedFree(current, false);
    current = next;
  }
}

bool PoolAllocator::GrowIfNeeded() noexcept {
  const size_t current_blocks = total_blocks_.load(std::memory_order_acquire);
  const size_t desired_blocks =
      growth_.NextCapacity(current_blocks, current_blocks + 1);
  if (desired_blocks <= current_blocks) {
    return false;
  }

  auto expected = GrowState::kIdle;
  if (!grow_state_.compare_exchange_strong(expected, GrowState::kGrowing,
                                           std::memory_order_acq_rel,
                                           std::memory_order_acquire)) {
    grow_state_.wait(GrowState::kGrowing, std::memory_order_acquire);
    // Another allocator completed the coordinated attempt. Its blocks may
    // already have been consumed, so the caller must retry the pop/grow loop.
    return true;
  }

  const size_t chunk_blocks = desired_blocks - current_blocks;
  ChunkHeader* const chunk = CreateChunk(block_size_, chunk_blocks, alignment_);
  if (chunk == nullptr) {
    grow_state_.store(GrowState::kIdle, std::memory_order_release);
    grow_state_.notify_all();
    return false;
  }

  ChunkHeader* observed = chunks_.load(std::memory_order_acquire);
  do {
    chunk->next.store(observed, std::memory_order_relaxed);
  } while (!chunks_.compare_exchange_weak(
      observed, chunk, std::memory_order_release, std::memory_order_acquire));

  total_blocks_.fetch_add(chunk->block_count, std::memory_order_relaxed);
  free_blocks_.fetch_add(chunk->block_count, std::memory_order_relaxed);
  PushChunkBlocks(*chunk);

  grow_state_.store(GrowState::kIdle, std::memory_order_release);
  grow_state_.notify_all();
  return true;
}

void PoolAllocator::PushChunkBlocks(ChunkHeader& chunk) noexcept {
  auto* current = static_cast<std::byte*>(chunk.buffer);
  for (size_t i = 0; i < chunk.block_count; ++i) {
    PushBlock(free_head_, current);
    current += block_size_;
  }
}

void PoolAllocator::RebuildFreeList() noexcept {
  free_head_.store(nullptr, std::memory_order_release);
  ChunkHeader* chunk = chunks_.load(std::memory_order_acquire);
  size_t total = 0;

  while (chunk != nullptr) {
    PushChunkBlocks(*chunk);
    total += chunk->block_count;
    chunk = chunk->next.load(std::memory_order_acquire);
  }

  total_blocks_.store(total, std::memory_order_release);
  free_blocks_.store(total, std::memory_order_release);
}

void* PoolAllocator::do_allocate(size_t bytes, size_t alignment) {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::PoolAllocator::do_allocate");
  HELIOS_MEMORY_PROFILE_ZONE_VALUE(bytes);

  if (bytes == 0) [[unlikely]] {
    return nullptr;
  }

  HELIOS_VERIFY(IsPowerOfTwo(alignment),
                "alignment must be a power of two, got {}!", alignment);

  HELIOS_VERIFY(alignment <= alignment_,
                "Requested alignment ({}) exceeds pool alignment ({})!",
                alignment, alignment_);

  HELIOS_VERIFY(bytes <= block_size_,
                "Requested size ({}) exceeds pool block size ({})!", bytes,
                block_size_);

  void* block = nullptr;
  while ((block = PopBlock(free_head_)) == nullptr) {
    HELIOS_VERIFY(GrowIfNeeded(), "Pool exhausted and growth failed!");
  }

  const size_t free_now =
      free_blocks_.fetch_sub(1, std::memory_order_relaxed) - 1;
  total_allocations_.fetch_add(1, std::memory_order_relaxed);

  const size_t total = total_blocks_.load(std::memory_order_relaxed);
  const size_t used = total - free_now;
  details::AccumulatePeak(peak_used_blocks_, used);

  return block;
}

void PoolAllocator::do_deallocate(void* ptr, [[maybe_unused]] size_t bytes,
                                  size_t /*alignment*/) {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::PoolAllocator::do_deallocate");
  HELIOS_MEMORY_PROFILE_ZONE_VALUE(bytes);

  if (ptr == nullptr) [[unlikely]] {
    return;
  }

  HELIOS_ASSERT(Owns(ptr), "ptr does not belong to pool allocator!");

  PushBlock(free_head_, ptr);
  free_blocks_.fetch_add(1, std::memory_order_relaxed);
  total_deallocations_.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace helios::mem
