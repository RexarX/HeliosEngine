#include <pch.hpp>

#include <helios/memory/fixed_pool_allocator.hpp>

#include <helios/assert.hpp>
#include <helios/memory/aligned_alloc.hpp>
#include <helios/memory/common.hpp>
#include <helios/memory/details/profile.hpp>

#include <atomic>
#include <cstddef>
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
    if (head.compare_exchange_weak(observed, next, std::memory_order_acq_rel,
                                   std::memory_order_acquire)) {
      return observed;
    }
  }
}

}  // namespace

namespace helios::mem {

void FixedPoolAllocator::MoveFrom(FixedPoolAllocator& other) noexcept {
  block_size_ = std::exchange(other.block_size_, 0);
  block_count_ = std::exchange(other.block_count_, 0);
  alignment_ = std::exchange(other.alignment_, 0);
  chunk_capacity_ = std::exchange(other.chunk_capacity_, 0);
  buffer_ = std::exchange(other.buffer_, nullptr);
  free_head_.store(
      other.free_head_.exchange(nullptr, std::memory_order_acq_rel),
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

void FixedPoolAllocator::Rebuild() noexcept {
  free_head_.store(nullptr, std::memory_order_release);
  for (size_t index = 0; index < block_count_; ++index) {
    PushBlock(free_head_, buffer_ + index * block_size_);
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

  void* const block = PopBlock(free_head_);
  HELIOS_VERIFY(block != nullptr, "Fixed pool allocator exhausted!");

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
  PushBlock(free_head_, ptr);
  free_blocks_.fetch_add(1, std::memory_order_relaxed);
  total_deallocations_.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace helios::mem
