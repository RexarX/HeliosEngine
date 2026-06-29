#include <pch.hpp>

#include <helios/memory/fixed_stack_allocator.hpp>

#include <helios/assert.hpp>
#include <helios/memory/aligned_alloc.hpp>
#include <helios/memory/common.hpp>
#include <helios/memory/details/profile.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>

namespace helios::mem {

void FixedStackAllocator::ClearStats() noexcept {
  offset_.store(0, std::memory_order_release);
  peak_usage_.store(0, std::memory_order_relaxed);
  allocation_count_.store(0, std::memory_order_relaxed);
  total_allocations_.store(0, std::memory_order_relaxed);
  total_deallocations_.store(0, std::memory_order_relaxed);
  alignment_waste_.store(0, std::memory_order_relaxed);
}

void FixedStackAllocator::Release() noexcept {
  if (buffer_ != nullptr) {
    HELIOS_MEMORY_PROFILE_FREE(buffer_, "FixedStackAllocator");
    AlignedFree(buffer_, false);
    buffer_ = nullptr;
  }
}

void FixedStackAllocator::MoveFrom(FixedStackAllocator& other) noexcept {
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

void* FixedStackAllocator::do_allocate(size_t bytes, size_t alignment) {
  HELIOS_MEMORY_PROFILE_SCOPE_N(
      "helios::mem::FixedStackAllocator::do_allocate");
  HELIOS_MEMORY_PROFILE_ZONE_VALUE(bytes);

  if (bytes == 0) [[unlikely]] {
    return nullptr;
  }

  HELIOS_VERIFY(IsPowerOfTwo(alignment),
                "alignment must be a power of two, got {}!", alignment);

  const size_t effective_alignment = std::max(alignment, kMinAlignment);
  const size_t header_alignment =
      std::max(effective_alignment, alignof(AllocationHeader));
  constexpr size_t kHeaderSize = sizeof(AllocationHeader);
  size_t observed = offset_.load(std::memory_order_relaxed);
  for (;;) {
    const size_t padding = CalculatePaddingWithHeader(
        buffer_ + observed, header_alignment, kHeaderSize);
    const size_t user_offset = SaturatingAdd(observed, padding);
    const size_t next = SaturatingAdd(user_offset, bytes);

    HELIOS_VERIFY(next <= capacity_, "Fixed stack allocator exhausted!");

    if (offset_.compare_exchange_weak(observed, next, std::memory_order_acq_rel,
                                      std::memory_order_relaxed)) {
      auto* const user_ptr = buffer_ + user_offset;
      auto* const header = std::launder(
          reinterpret_cast<AllocationHeader*>(user_ptr - kHeaderSize));
      header->previous_offset = observed;
      header->end_offset = next;

      const size_t waste = padding - kHeaderSize;
      alignment_waste_.fetch_add(waste, std::memory_order_relaxed);
      allocation_count_.fetch_add(1, std::memory_order_relaxed);
      total_allocations_.fetch_add(1, std::memory_order_relaxed);

      size_t peak = peak_usage_.load(std::memory_order_relaxed);
      while (next > peak && !peak_usage_.compare_exchange_weak(
                                peak, next, std::memory_order_relaxed)) {
      }
      return user_ptr;
    }
  }
}

void FixedStackAllocator::do_deallocate(void* ptr, size_t /*bytes*/,
                                        size_t /*alignment*/) {
  HELIOS_MEMORY_PROFILE_SCOPE_N(
      "helios::mem::FixedStackAllocator::do_deallocate");

  if (ptr == nullptr) [[unlikely]] {
    return;
  }

  HELIOS_ASSERT(Owns(ptr), "ptr does not belong to fixed stack!");

  constexpr size_t kHeaderSize = sizeof(AllocationHeader);
  auto* const header = std::launder(reinterpret_cast<AllocationHeader*>(
      static_cast<std::byte*>(ptr) - kHeaderSize));
  size_t expected = header->end_offset;
  if (offset_.compare_exchange_strong(expected, header->previous_offset,
                                      std::memory_order_acq_rel)) {
    allocation_count_.fetch_sub(1, std::memory_order_relaxed);
    total_deallocations_.fetch_add(1, std::memory_order_relaxed);
    const size_t user_offset = static_cast<std::byte*>(ptr) - buffer_;
    const size_t waste = user_offset - header->previous_offset - kHeaderSize;
    alignment_waste_.fetch_sub(
        std::min(waste, alignment_waste_.load(std::memory_order_relaxed)),
        std::memory_order_relaxed);
  }
}

}  // namespace helios::mem
