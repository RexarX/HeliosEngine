#include <pch.hpp>

#include <helios/memory/fixed_pool_allocator.hpp>

#include <helios/memory/aligned_alloc.hpp>
#include <helios/memory/common.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <helios/assert.hpp>
#include <helios/memory/details/profile.hpp>
#include <utility>

namespace helios::mem {

FixedPoolAllocator::FixedPoolAllocator(
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

FixedPoolAllocator::FixedPoolAllocator(size_t block_size, size_t block_count,
                                       size_t alignment) noexcept
    : FixedPoolAllocator(FixedPoolAllocatorOptions{
          .block_size = block_size,
          .block_count = block_count,
          .alignment = alignment,
      }) {}

FixedPoolAllocator& FixedPoolAllocator::operator=(
    FixedPoolAllocator&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  Release();
  MoveFrom(other);
  return *this;
}

void FixedPoolAllocator::Reset() noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::FixedPoolAllocator::Reset");

  Rebuild();
  peak_used_blocks_.store(0, std::memory_order_relaxed);
  total_allocations_.store(0, std::memory_order_relaxed);
  total_deallocations_.store(0, std::memory_order_relaxed);
}

void FixedPoolAllocator::MoveFrom(FixedPoolAllocator& other) noexcept {
  block_size_ = std::exchange(other.block_size_, 0);
  block_count_ = std::exchange(other.block_count_, 0);
  alignment_ = std::exchange(other.alignment_, 0);
  chunk_capacity_ = std::exchange(other.chunk_capacity_, 0);
  buffer_ = std::exchange(other.buffer_, nullptr);
  free_list_ = std::move(other.free_list_);
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

void FixedPoolAllocator::Rebuild() noexcept {
  free_list_.Clear();
  for (size_t index = 0; index < block_count_; ++index) {
    free_list_.Push(buffer_ + index * block_size_);
  }
  free_blocks_.store(block_count_, std::memory_order_release);
}

void FixedPoolAllocator::Release() noexcept {
  if (buffer_ != nullptr) {
    HELIOS_MEMORY_PROFILE_FREE(buffer_, "FixedPoolAllocator");
    AlignedFree(buffer_, false);
    buffer_ = nullptr;
  }
}

void* FixedPoolAllocator::do_allocate(size_t bytes, size_t alignment) {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::FixedPoolAllocator::do_allocate");
  HELIOS_MEMORY_PROFILE_ZONE_VALUE(bytes);

  if (bytes == 0) [[unlikely]] {
    return nullptr;
  }

  HELIOS_VERIFY(bytes <= block_size_,
                "Requested size exceeds fixed pool block size!");
  HELIOS_VERIFY(alignment <= alignment_ && IsPowerOfTwo(alignment),
                "Requested alignment exceeds fixed pool alignment!");

  void* const block = free_list_.Pop();
  if (block == nullptr) [[unlikely]] {
    return nullptr;
  }

  const size_t free = free_blocks_.fetch_sub(1) - 1;
  const size_t used = block_count_ - free;
  size_t peak = peak_used_blocks_.load(std::memory_order_relaxed);
  while (used > peak && !peak_used_blocks_.compare_exchange_weak(
                            peak, used, std::memory_order_relaxed)) {
  }

  total_allocations_.fetch_add(1, std::memory_order_relaxed);
  return block;
}

void FixedPoolAllocator::do_deallocate(void* ptr, size_t /*bytes*/,
                                       size_t /*alignment*/) {
  HELIOS_MEMORY_PROFILE_SCOPE_N(
      "helios::mem::FixedPoolAllocator::do_deallocate");

  if (ptr == nullptr) [[unlikely]] {
    return;
  }

  HELIOS_ASSERT(Owns(ptr), "ptr does not belong to fixed pool!");
  free_list_.Push(ptr);
  free_blocks_.fetch_add(1, std::memory_order_relaxed);
  total_deallocations_.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace helios::mem
