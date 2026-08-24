#include <pch.hpp>

#include <helios/memory/stack_allocator.hpp>

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

namespace helios::mem {

StackAllocator::StackAllocator(StackAllocatorOptions options) noexcept
    : initial_capacity_(options.initial_capacity), growth_(options.growth) {
  HELIOS_ASSERT(initial_capacity_ > 0,
                "initial_capacity must be greater than zero!");
  HELIOS_ASSERT(growth_.max_capacity >= initial_capacity_,
                "max_capacity '{}' must be >= initial_capacity '{}'!",
                growth_.max_capacity, initial_capacity_);

  Block* const initial_block = CreateBlock(initial_capacity_);
  HELIOS_VERIFY(initial_block != nullptr, "Failed to allocate initial block!");
  blocks_.Push(initial_block);
  total_capacity_.store(initial_capacity_, std::memory_order_relaxed);
  block_count_.store(1, std::memory_order_relaxed);
}

StackAllocator& StackAllocator::operator=(StackAllocator&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  FreeChain(HeadBlock());
  MoveFrom(other);
  return *this;
}

auto StackAllocator::GetMarker() const noexcept -> Marker {
  Block* const head = HeadBlock();
  if (head == nullptr) {
    return {};
  }

  return {
      .block = head,
      .offset = head->offset.load(std::memory_order_acquire),
  };
}

void StackAllocator::Reset() noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::StackAllocator::Reset");

  Block* const head = HeadBlock();
  if (head == nullptr) {
    return;
  }

  blocks_.Clear();
  FreeChain(head);
  Block* const fresh = CreateBlock(initial_capacity_);
  HELIOS_VERIFY(fresh != nullptr, "Failed to allocate block during reset!");

  blocks_.Push(fresh);
  total_capacity_.store(initial_capacity_, std::memory_order_release);
  total_allocated_.store(0, std::memory_order_release);
  allocation_count_.store(0, std::memory_order_release);
  total_allocations_.store(0, std::memory_order_release);
  total_deallocations_.store(0, std::memory_order_release);
  alignment_waste_.store(0, std::memory_order_release);
  block_count_.store(1, std::memory_order_release);
}

void StackAllocator::RewindToMarker(Marker marker) noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::StackAllocator::RewindToMarker");
  HELIOS_MEMORY_PROFILE_ZONE_VALUE(marker.offset);

  HELIOS_ASSERT(marker.block != nullptr, "marker block cannot be null");
  auto* const target = static_cast<Block*>(marker.block);

  for (;;) {
    Block* const current = HeadBlock();
    HELIOS_ASSERT(current != nullptr, "Marker block not found in stack chain!");
    if (current == target) {
      break;
    }

    Block* const popped = static_cast<Block*>(blocks_.Pop());
    HELIOS_ASSERT(popped == current, "Stack chain mutated during rewind!");
    const size_t capacity = popped->capacity;
    HELIOS_MEMORY_PROFILE_FREE(popped, "StackAllocator");
    std::destroy_at(popped);
    AlignedFree(popped, false);
    total_capacity_.fetch_sub(capacity, std::memory_order_relaxed);
    block_count_.fetch_sub(1, std::memory_order_relaxed);
  }

  target->offset.store(marker.offset, std::memory_order_release);

  allocation_count_.store(0, std::memory_order_relaxed);
  total_allocated_.store(marker.offset, std::memory_order_relaxed);
}

void StackAllocator::MoveFrom(StackAllocator& other) noexcept {
  blocks_ = std::move(other.blocks_);
  grow_state_.store(
      other.grow_state_.exchange(GrowState::kIdle, std::memory_order_acq_rel),
      std::memory_order_release);
  initial_capacity_ = std::exchange(other.initial_capacity_, 0);
  growth_ = std::exchange(other.growth_, {});
  total_capacity_.store(
      other.total_capacity_.exchange(0, std::memory_order_acq_rel),
      std::memory_order_release);
  total_allocated_.store(
      other.total_allocated_.exchange(0, std::memory_order_acq_rel),
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
  block_count_.store(other.block_count_.exchange(0, std::memory_order_acq_rel),
                     std::memory_order_release);
}

auto StackAllocator::CreateBlock(size_t capacity) noexcept -> Block* {
  constexpr size_t kHeader = AlignUp(sizeof(Block), kDefaultAlignment);
  const size_t total_size = SaturatingAdd(kHeader, capacity);
  void* const raw = AlignedAlloc(kDefaultAlignment, total_size, false);
  if (raw == nullptr) [[unlikely]] {
    return nullptr;
  }

  HELIOS_MEMORY_PROFILE_ALLOC(raw, total_size, "StackAllocator");

  auto* const block = std::construct_at(static_cast<Block*>(raw));
  block->buffer = static_cast<std::byte*>(raw) + kHeader;
  block->capacity = capacity;
  block->offset.store(0, std::memory_order_relaxed);
  return block;
}

void StackAllocator::FreeChain(Block* head) noexcept {
  if (head == nullptr) [[unlikely]] {
    return;
  }

  Block* current = head;
  while (current != nullptr) {
    Block* const next = NextBlock(current);
    HELIOS_MEMORY_PROFILE_FREE(current, "StackAllocator");
    std::destroy_at(current);
    AlignedFree(current, false);
    current = next;
  }
}

auto StackAllocator::TryReserve(Block& block, size_t size,
                                size_t alignment) noexcept -> Reservation {
  constexpr size_t kHeaderSize = sizeof(AllocationHeader);
  const size_t header_alignment =
      std::max(alignment, alignof(AllocationHeader));

  size_t current_offset = block.offset.load(std::memory_order_acquire);
  for (;;) {
    auto* const start = static_cast<std::byte*>(block.buffer) + current_offset;
    const size_t padding =
        CalculatePaddingWithHeader(start, header_alignment, kHeaderSize);
    const size_t consumed = SaturatingAdd(padding, size);
    const size_t end_offset = SaturatingAdd(current_offset, consumed);

    if (end_offset > block.capacity) {
      return {};
    }

    if (block.offset.compare_exchange_weak(current_offset, end_offset,
                                           std::memory_order_release,
                                           std::memory_order_acquire)) {
      auto* const user_ptr = start + padding;
      auto* const header_ptr = std::launder(
          reinterpret_cast<AllocationHeader*>(user_ptr - kHeaderSize));
      header_ptr->previous_offset = current_offset;
      header_ptr->total_size = consumed;

      return {
          .ptr = user_ptr,
          .allocation_size = size,
          .consumed = consumed,
          .previous_offset = current_offset,
      };
    }
  }
}

bool StackAllocator::EnsureCapacity(size_t min_capacity) noexcept {
  Block* observed_head = HeadBlock();
  const size_t current_capacity =
      observed_head != nullptr ? observed_head->capacity : initial_capacity_;
  const size_t desired_capacity =
      growth_.NextCapacity(current_capacity, min_capacity);
  if (desired_capacity < min_capacity || desired_capacity == 0) {
    return false;
  }

  auto expected = GrowState::kIdle;
  if (!grow_state_.compare_exchange_strong(expected, GrowState::kGrowing,
                                           std::memory_order_acq_rel,
                                           std::memory_order_acquire)) {
    grow_state_.wait(GrowState::kGrowing, std::memory_order_acquire);
    return true;
  }

  Block* const new_block = CreateBlock(desired_capacity);
  if (new_block == nullptr) {
    grow_state_.store(GrowState::kIdle, std::memory_order_release);
    grow_state_.notify_all();
    return false;
  }

  PublishBlock(new_block);
  grow_state_.store(GrowState::kIdle, std::memory_order_release);
  grow_state_.notify_all();
  return true;
}

void StackAllocator::PublishBlock(Block* block) noexcept {
  blocks_.Push(block);

  total_capacity_.fetch_add(block->capacity, std::memory_order_relaxed);
  block_count_.fetch_add(1, std::memory_order_relaxed);
}

void* StackAllocator::do_allocate(size_t bytes, size_t alignment) {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::StackAllocator::do_allocate");
  HELIOS_MEMORY_PROFILE_ZONE_VALUE(bytes);

  if (bytes == 0) [[unlikely]] {
    return nullptr;
  }

  HELIOS_VERIFY(IsPowerOfTwo(alignment),
                "alignment must be a power of two, got {}!", alignment);

  const size_t effective_alignment = std::max(alignment, kMinAlignment);
  Reservation reservation{};
  for (;;) {
    Block* const head = HeadBlock();
    if (head != nullptr) {
      reservation = TryReserve(*head, bytes, effective_alignment);
      if (reservation.ptr != nullptr) {
        break;
      }
    }

    constexpr size_t kHeader = sizeof(AllocationHeader);
    const size_t min_capacity =
        SaturatingAdd(SaturatingAdd(bytes, effective_alignment), kHeader);
    HELIOS_VERIFY(EnsureCapacity(min_capacity),
                  "Unable to grow stack allocator block chain!");
  }

  allocation_count_.fetch_add(1, std::memory_order_relaxed);
  total_allocations_.fetch_add(1, std::memory_order_relaxed);
  total_allocated_.fetch_add(reservation.consumed, std::memory_order_relaxed);
  alignment_waste_.fetch_add(reservation.consumed - reservation.allocation_size,
                             std::memory_order_relaxed);
  const size_t total = total_allocated_.load(std::memory_order_relaxed);
  details::AccumulatePeak(peak_usage_, total);

  return reservation.ptr;
}

void StackAllocator::do_deallocate(void* ptr, size_t bytes,
                                   size_t /*alignment*/) {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::StackAllocator::do_deallocate");
  HELIOS_MEMORY_PROFILE_ZONE_VALUE(bytes);

  if (ptr == nullptr) [[unlikely]] {
    return;
  }

  Block* const head = HeadBlock();
  if (head == nullptr) {
    return;
  }

  const auto addr = reinterpret_cast<uintptr_t>(ptr);
  const auto begin = reinterpret_cast<uintptr_t>(head->buffer);
  const auto end = begin + head->capacity;
  if (addr < begin || addr >= end) {
    return;
  }

  auto* const header = std::launder(reinterpret_cast<AllocationHeader*>(
      static_cast<std::byte*>(ptr) - sizeof(AllocationHeader)));
  size_t expected_end =
      SaturatingAdd(header->previous_offset, header->total_size);
  if (head->offset.compare_exchange_strong(
          expected_end, header->previous_offset, std::memory_order_acq_rel,
          std::memory_order_acquire)) {
    allocation_count_.fetch_sub(1, std::memory_order_relaxed);
    total_deallocations_.fetch_add(1, std::memory_order_relaxed);
    total_allocated_.fetch_sub(header->total_size, std::memory_order_relaxed);

    if (bytes > 0) {
      const size_t waste =
          header->total_size > bytes ? header->total_size - bytes : 0;
      details::SaturatingFetchSub(alignment_waste_, waste);
    }
  }
}

}  // namespace helios::mem
