#include <pch.hpp>

#include <helios/memory/arena_allocator.hpp>

#include <details/accumulate_peak.hpp>
#include <helios/assert.hpp>
#include <helios/memory/aligned_alloc.hpp>
#include <helios/memory/common.hpp>
#include <helios/memory/details/profile.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>

namespace helios::mem {

ArenaAllocator::ArenaAllocator(ArenaOptions options) noexcept
    : initial_capacity_(options.initial_capacity), growth_(options.growth) {
  HELIOS_ASSERT(initial_capacity_ > 0,
                "initial_capacity must be greater than zero!");
  HELIOS_ASSERT(growth_.max_capacity >= initial_capacity_,
                "max_capacity '{}' must be >= initial_capacity '{}'!",
                growth_.max_capacity, initial_capacity_);

  Block* const initial_block = CreateBlock(initial_capacity_);
  HELIOS_VERIFY(initial_block != nullptr, "Failed to allocate initial block!");

  head_.store(initial_block, std::memory_order_release);
  total_capacity_.store(initial_capacity_, std::memory_order_relaxed);
  block_count_.store(1, std::memory_order_relaxed);
}

void ArenaAllocator::Reset() noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::ArenaAllocator::Reset");

  Block* current = head_.load(std::memory_order_acquire);
  while (current != nullptr) {
    current->offset.store(0, std::memory_order_release);
    current = current->next.load(std::memory_order_acquire);
  }

  total_allocated_.store(0, std::memory_order_release);
  peak_usage_.store(0, std::memory_order_release);
  allocation_count_.store(0, std::memory_order_release);
  total_allocations_.store(0, std::memory_order_release);
  total_deallocations_.store(0, std::memory_order_release);
  alignment_waste_.store(0, std::memory_order_release);
}

void ArenaAllocator::MoveFrom(ArenaAllocator& other) noexcept {
  head_.store(other.head_.exchange(nullptr, std::memory_order_acq_rel),
              std::memory_order_release);
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

auto ArenaAllocator::CreateBlock(size_t capacity) noexcept -> Block* {
  constexpr size_t kHeaderSize = AlignUp(sizeof(Block), kDefaultAlignment);
  const size_t total_size = SaturatingAdd(kHeaderSize, capacity);

  void* const raw = AlignedAlloc(kDefaultAlignment, total_size, false);
  if (raw == nullptr) [[unlikely]] {
    return nullptr;
  }

  HELIOS_MEMORY_PROFILE_ALLOC(raw, total_size, "ArenaAllocator");

  auto* const block = std::construct_at(static_cast<Block*>(raw));
  block->buffer = static_cast<std::byte*>(raw) + kHeaderSize;
  block->capacity = capacity;
  block->offset.store(0, std::memory_order_relaxed);
  block->next.store(nullptr, std::memory_order_relaxed);
  return block;
}

void ArenaAllocator::PublishBlock(Block* block) noexcept {
  Block* expected_head = head_.load(std::memory_order_acquire);
  do {
    block->next.store(expected_head, std::memory_order_relaxed);
  } while (!head_.compare_exchange_weak(expected_head, block,
                                        std::memory_order_release,
                                        std::memory_order_acquire));

  total_capacity_.fetch_add(block->capacity, std::memory_order_relaxed);
  block_count_.fetch_add(1, std::memory_order_relaxed);
}

void ArenaAllocator::FreeChain(Block* head) noexcept {
  if (head == nullptr) [[unlikely]] {
    return;
  }

  Block* current = head;
  while (current != nullptr) {
    Block* const next = current->next.load(std::memory_order_relaxed);
    HELIOS_MEMORY_PROFILE_FREE(current, "ArenaAllocator");
    std::destroy_at(current);
    AlignedFree(current, false);
    current = next;
  }
}

auto ArenaAllocator::TryReserve(Block& block, size_t size,
                                size_t alignment) noexcept -> Reservation {
  size_t current_offset = block.offset.load(std::memory_order_acquire);
  for (;;) {
    auto* const begin = static_cast<std::byte*>(block.buffer) + current_offset;
    const size_t padding = CalculatePadding(begin, alignment);
    const size_t aligned_offset = SaturatingAdd(current_offset, padding);
    const size_t end_offset = SaturatingAdd(aligned_offset, size);

    if (end_offset > block.capacity) {
      return {};
    }

    if (block.offset.compare_exchange_weak(current_offset, end_offset,
                                           std::memory_order_release,
                                           std::memory_order_acquire)) {
      return {
          .ptr = static_cast<std::byte*>(block.buffer) + aligned_offset,
          .size = size,
          .padding = padding,
      };
    }
  }
}

bool ArenaAllocator::EnsureCapacity(size_t min_capacity) noexcept {
  Block* observed_head = head_.load(std::memory_order_acquire);
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

  Block* new_block = CreateBlock(desired_capacity);
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

void* ArenaAllocator::do_allocate(size_t bytes, size_t alignment) {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::ArenaAllocator::do_allocate");
  HELIOS_MEMORY_PROFILE_ZONE_VALUE(bytes);

  if (bytes == 0) [[unlikely]] {
    return nullptr;
  }

  HELIOS_VERIFY(IsPowerOfTwo(alignment),
                "alignment must be a power of two, got {}!", alignment);

  const size_t effective_alignment = std::max(alignment, kMinAlignment);
  Reservation reservation{};
  for (;;) {
    Block* const head = head_.load(std::memory_order_acquire);
    if (head != nullptr) {
      reservation = TryReserve(*head, bytes, effective_alignment);
      if (reservation.ptr != nullptr) {
        break;
      }
    }

    const size_t min_capacity = SaturatingAdd(bytes, effective_alignment);
    HELIOS_VERIFY(EnsureCapacity(min_capacity),
                  "Unable to grow arena block chain!");
  }

  allocation_count_.fetch_add(1, std::memory_order_relaxed);
  total_allocations_.fetch_add(1, std::memory_order_relaxed);
  alignment_waste_.fetch_add(reservation.padding, std::memory_order_relaxed);
  const size_t total =
      total_allocated_.fetch_add(bytes, std::memory_order_relaxed) + bytes;
  details::AccumulatePeak(peak_usage_, total);

  return reservation.ptr;
}

}  // namespace helios::mem
