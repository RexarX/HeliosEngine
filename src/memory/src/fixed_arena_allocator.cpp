#include <pch.hpp>

#include <helios/memory/fixed_arena_allocator.hpp>

#include <helios/assert.hpp>
#include <helios/memory/aligned_alloc.hpp>
#include <helios/memory/common.hpp>
#include <helios/memory/details/profile.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>

namespace helios::mem {

void FixedArenaAllocator::MoveFrom(FixedArenaAllocator& other) noexcept {
  capacity_ = std::exchange(other.capacity_, 0);
  buffer_ = std::exchange(other.buffer_, nullptr);
  offset_.store(other.offset_.exchange(0, std::memory_order_acq_rel),
                std::memory_order_release);
  peak_usage_.store(other.peak_usage_.exchange(0, std::memory_order_relaxed),
                    std::memory_order_relaxed);
  allocation_count_.store(
      other.allocation_count_.exchange(0, std::memory_order_relaxed),
      std::memory_order_relaxed);
  total_allocations_.store(
      other.total_allocations_.exchange(0, std::memory_order_relaxed),
      std::memory_order_relaxed);
  total_deallocations_.store(
      other.total_deallocations_.exchange(0, std::memory_order_relaxed),
      std::memory_order_relaxed);
  alignment_waste_.store(
      other.alignment_waste_.exchange(0, std::memory_order_relaxed),
      std::memory_order_relaxed);
}

void FixedArenaAllocator::Release() noexcept {
  if (buffer_ == nullptr) {
    return;
  }

  HELIOS_MEMORY_PROFILE_FREE(buffer_, "FixedArenaAllocator");
  AlignedFree(buffer_, false);
  buffer_ = nullptr;
}

void* FixedArenaAllocator::do_allocate(size_t bytes, size_t alignment) {
  HELIOS_MEMORY_PROFILE_SCOPE_N(
      "helios::mem::FixedArenaAllocator::do_allocate");
  HELIOS_MEMORY_PROFILE_ZONE_VALUE(bytes);

  if (bytes == 0) [[unlikely]] {
    return nullptr;
  }

  HELIOS_VERIFY(IsPowerOfTwo(alignment),
                "alignment must be a power of two, got {}!", alignment);

  const size_t effective_alignment = std::max(alignment, kMinAlignment);
  size_t observed = offset_.load(std::memory_order_relaxed);
  for (;;) {
    const size_t padding =
        CalculatePadding(buffer_ + observed, effective_alignment);
    const size_t next = SaturatingAdd(observed, SaturatingAdd(padding, bytes));
    HELIOS_VERIFY(next <= capacity_, "Fixed arena allocator exhausted!");
    if (offset_.compare_exchange_weak(observed, next, std::memory_order_acq_rel,
                                      std::memory_order_relaxed)) {
      allocation_count_.fetch_add(1, std::memory_order_relaxed);
      total_allocations_.fetch_add(1, std::memory_order_relaxed);
      alignment_waste_.fetch_add(padding, std::memory_order_relaxed);
      size_t peak = peak_usage_.load(std::memory_order_relaxed);
      while (next > peak && !peak_usage_.compare_exchange_weak(
                                peak, next, std::memory_order_relaxed)) {
      }
      return buffer_ + observed + padding;
    }
  }
}

void FixedArenaAllocator::do_deallocate(void* ptr, size_t /*bytes*/,
                                        size_t /*alignment*/) {
  HELIOS_MEMORY_PROFILE_SCOPE_N(
      "helios::mem::FixedArenaAllocator::do_deallocate");

  if (ptr != nullptr) {
    HELIOS_ASSERT(Owns(ptr), "ptr does not belong to fixed arena!");
    total_deallocations_.fetch_add(1, std::memory_order_relaxed);
  }
}

}  // namespace helios::mem
