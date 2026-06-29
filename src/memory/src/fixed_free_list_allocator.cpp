#include <pch.hpp>

#include <helios/memory/fixed_free_list_allocator.hpp>

#include <helios/assert.hpp>
#include <helios/memory/aligned_alloc.hpp>
#include <helios/memory/common.hpp>
#include <helios/memory/details/profile.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <limits>
#include <mutex>
#include <new>
#include <string_view>
#include <utility>

namespace helios::mem {

void FixedFreeListAllocator::Initialize() noexcept {
  free_list_ = std::launder(reinterpret_cast<FreeBlock*>(buffer_));
  free_list_->size = capacity_;
  free_list_->next = nullptr;
  free_block_count_.store(1, std::memory_order_relaxed);
  used_memory_.store(0, std::memory_order_relaxed);
  allocation_count_.store(0, std::memory_order_relaxed);
  alignment_waste_.store(0, std::memory_order_relaxed);
}

void FixedFreeListAllocator::MoveFrom(FixedFreeListAllocator& other) noexcept {
  capacity_ = std::exchange(other.capacity_, 0);
  buffer_ = std::exchange(other.buffer_, nullptr);
  free_list_ = std::exchange(other.free_list_, nullptr);
  free_block_count_.store(
      other.free_block_count_.exchange(0, std::memory_order_acq_rel),
      std::memory_order_release);
  used_memory_.store(other.used_memory_.exchange(0, std::memory_order_acq_rel),
                     std::memory_order_release);
  peak_usage_.store(other.peak_usage_.exchange(0, std::memory_order_acq_rel),
                    std::memory_order_release);
  allocation_count_.store(
      other.allocation_count_.exchange(0, std::memory_order_acq_rel),
      std::memory_order_release);
  total_allocations_.store(
      other.total_allocations_.exchange(0, std::memory_order_acq_rel),
      std::memory_order_release);
  total_deallocations_.store(
      other.total_deallocations_.exchange(0, std::memory_order_acq_rel),
      std::memory_order_release);
  alignment_waste_.store(
      other.alignment_waste_.exchange(0, std::memory_order_acq_rel),
      std::memory_order_release);
}

void FixedFreeListAllocator::InsertAndCoalesce(FreeBlock* block) noexcept {
  FreeBlock* previous = nullptr;
  FreeBlock* current = free_list_;
  while (current != nullptr && current < block) {
    previous = current;
    current = current->next;
  }

  block->next = current;

  if (previous == nullptr) {
    free_list_ = block;
  } else {
    previous->next = block;
  }

  free_block_count_.fetch_add(1, std::memory_order_relaxed);
  if (current != nullptr && reinterpret_cast<std::byte*>(block) + block->size ==
                                reinterpret_cast<std::byte*>(current)) {
    block->size += current->size;
    block->next = current->next;
    free_block_count_.fetch_sub(1, std::memory_order_relaxed);
  }

  if (previous != nullptr &&
      reinterpret_cast<std::byte*>(previous) + previous->size ==
          reinterpret_cast<std::byte*>(block)) {
    previous->size += block->size;
    previous->next = block->next;
    free_block_count_.fetch_sub(1, std::memory_order_relaxed);
  }
}

void FixedFreeListAllocator::ReleaseUnlocked() noexcept {
  if (buffer_ != nullptr) {
    HELIOS_MEMORY_PROFILE_FREE(buffer_, "FixedFreeListAllocator");
    AlignedFree(buffer_, false);
    buffer_ = nullptr;
    free_list_ = nullptr;
  }
}

void FixedFreeListAllocator::Release() noexcept {
  const std::scoped_lock lock(mutex_);
  ReleaseUnlocked();
}

void* FixedFreeListAllocator::do_allocate(size_t bytes, size_t alignment) {
  HELIOS_MEMORY_PROFILE_SCOPE_N(
      "helios::mem::FixedFreeListAllocator::do_allocate");
  HELIOS_MEMORY_PROFILE_ZONE_VALUE(bytes);

  if (bytes == 0) [[unlikely]] {
    return nullptr;
  }

  HELIOS_VERIFY(IsPowerOfTwo(alignment),
                "alignment must be a power of two, got {}!", alignment);

  const std::scoped_lock lock(mutex_);
  HELIOS_MEMORY_PROFILE_LOCK_MARK(mutex_);

  const size_t effective_alignment = std::max(alignment, kMinAlignment);
  FreeBlock* best = nullptr;
  FreeBlock* best_previous = nullptr;
  size_t best_padding = 0;
  size_t best_total = std::numeric_limits<size_t>::max();
  FreeBlock* previous = nullptr;
  for (FreeBlock* block = free_list_; block != nullptr;
       previous = block, block = block->next) {
    const size_t padding = CalculatePaddingWithHeader(
        block, effective_alignment, sizeof(AllocationHeader));
    const size_t total = SaturatingAdd(padding, bytes);
    if (block->size >= total && total < best_total) {
      best = block;
      best_previous = previous;
      best_padding = padding;
      best_total = total;
    }
  }

  HELIOS_VERIFY(best != nullptr, "Fixed free-list allocator exhausted!");

  if (best_previous != nullptr) {
    best_previous->next = best->next;
  } else {
    free_list_ = best->next;
  }

  free_block_count_.fetch_sub(1, std::memory_order_relaxed);
  size_t remaining = best->size - best_total;
  auto* split_address = reinterpret_cast<std::byte*>(best) + best_total;
  const size_t split_padding =
      CalculatePadding(split_address, alignof(FreeBlock));

  if (remaining >= split_padding + sizeof(FreeBlock) + 1) {
    best_total += split_padding;
    remaining -= split_padding;
    split_address += split_padding;
    auto* const split =
        std::launder(reinterpret_cast<FreeBlock*>(split_address));
    split->size = remaining;
    split->next = nullptr;
    InsertAndCoalesce(split);
  } else {
    best_total = best->size;
  }

  auto* const result =
      std::launder(reinterpret_cast<std::byte*>(best) + best_padding);
  auto* const header =
      std::launder(reinterpret_cast<AllocationHeader*>(result) - 1);
  header->size = best_total;
  header->padding = best_padding;

  const size_t used =
      used_memory_.fetch_add(best_total, std::memory_order_relaxed) +
      best_total;
  size_t peak = peak_usage_.load(std::memory_order_relaxed);
  while (used > peak && !peak_usage_.compare_exchange_weak(
                            peak, used, std::memory_order_relaxed)) {
  }

  allocation_count_.fetch_add(1, std::memory_order_relaxed);
  total_allocations_.fetch_add(1, std::memory_order_relaxed);
  alignment_waste_.fetch_add(best_padding - sizeof(AllocationHeader),
                             std::memory_order_relaxed);
  return result;
}

void FixedFreeListAllocator::do_deallocate(void* ptr, size_t /*bytes*/,
                                           size_t /*alignment*/) {
  HELIOS_MEMORY_PROFILE_SCOPE_N(
      "helios::mem::FixedFreeListAllocator::do_deallocate");

  if (ptr == nullptr) [[unlikely]] {
    return;
  }

  HELIOS_ASSERT(Owns(ptr), "ptr does not belong to fixed free-list!");
  const std::scoped_lock lock(mutex_);
  HELIOS_MEMORY_PROFILE_LOCK_MARK(mutex_);

  auto* const header =
      std::launder(reinterpret_cast<AllocationHeader*>(ptr) - 1);
  auto* const block = std::launder(reinterpret_cast<FreeBlock*>(
      static_cast<std::byte*>(ptr) - header->padding));
  block->size = header->size;
  block->next = nullptr;

  used_memory_.fetch_sub(header->size, std::memory_order_relaxed);
  allocation_count_.fetch_sub(1, std::memory_order_relaxed);
  total_deallocations_.fetch_add(1, std::memory_order_relaxed);

  InsertAndCoalesce(block);
}

}  // namespace helios::mem
