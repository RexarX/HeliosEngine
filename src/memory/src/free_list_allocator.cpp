#include <pch.hpp>

#include <helios/memory/free_list_allocator.hpp>

#include <details/accumulate_peak.hpp>
#include <details/deferred_region_ebr.hpp>
#include <helios/memory/aligned_alloc.hpp>
#include <helios/memory/common.hpp>
#include <helios/memory/details/profile.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string_view>
#include <utility>

namespace {

struct AllocationHeader {
  size_t size = 0;
  size_t padding = 0;
};

}  // namespace

namespace helios::mem {

void FreeListAllocator::Reset() noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::FreeListAllocator::Reset");

  const std::scoped_lock lock(mutex_);
  free_list_ = nullptr;
  free_block_count_.store(0, std::memory_order_relaxed);

  RegionHeader* region = regions_.load(std::memory_order_acquire);
  while (region != nullptr) {
    InitializeRegionLocked(*region);
    region = region->next.load(std::memory_order_acquire);
  }

  used_memory_.store(0, std::memory_order_release);
  peak_usage_.store(0, std::memory_order_release);
  allocation_count_.store(0, std::memory_order_release);
  total_allocations_.store(0, std::memory_order_release);
  total_deallocations_.store(0, std::memory_order_release);
  alignment_waste_.store(0, std::memory_order_release);
}

bool FreeListAllocator::Owns(const void* ptr) const noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::FreeListAllocator::Owns");

  if (ptr == nullptr) [[unlikely]] {
    return false;
  }

  const auto addr = reinterpret_cast<uintptr_t>(ptr);
  const details::DeferredRegionEpochGuard guard;
  RegionHeader* region = regions_.load(std::memory_order_acquire);
  while (region != nullptr) {
    const auto begin = reinterpret_cast<uintptr_t>(region->buffer);
    const auto end = begin + region->capacity;
    if (addr >= begin && addr < end) {
      return true;
    }
    region = region->next.load(std::memory_order_acquire);
  }

  return false;
}

void FreeListAllocator::MoveFrom(FreeListAllocator& other) noexcept {
  free_list_ = std::exchange(other.free_list_, nullptr);
  regions_.store(other.regions_.exchange(nullptr, std::memory_order_acq_rel),
                 std::memory_order_release);
  grow_state_.store(
      other.grow_state_.exchange(GrowState::kIdle, std::memory_order_acq_rel),
      std::memory_order_release);
  initial_capacity_ = std::exchange(other.initial_capacity_, 0);
  growth_ = std::exchange(other.growth_, {});
  capacity_.store(other.capacity_.exchange(0, std::memory_order_acq_rel),
                  std::memory_order_release);
  used_memory_.store(other.used_memory_.exchange(0, std::memory_order_acq_rel),
                     std::memory_order_release);
  peak_usage_.store(other.peak_usage_.exchange(0, std::memory_order_acq_rel),
                    std::memory_order_release);
  free_block_count_.store(
      other.free_block_count_.exchange(0, std::memory_order_acq_rel),
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

void FreeListAllocator::ReleaseRegions() noexcept {
  FreeRegions(regions_.load(std::memory_order_acquire));
  details::FlushDeferredRegions();
}

size_t FreeListAllocator::FreeMemory() const noexcept {
  const size_t cap = capacity_.load(std::memory_order_relaxed);
  const size_t used = used_memory_.load(std::memory_order_relaxed);
  return cap >= used ? cap - used : 0;
}

auto FreeListAllocator::CreateRegion(size_t capacity) noexcept
    -> RegionHeader* {
  constexpr size_t kAlign =
      std::max({alignof(RegionHeader), kDefaultAlignment, kMinAlignment});
  const size_t header_size = AlignUp(sizeof(RegionHeader), kAlign);
  const size_t total_size = SaturatingAdd(header_size, capacity);

  void* const raw = AlignedAlloc(kAlign, total_size, false);
  if (raw == nullptr) [[unlikely]] {
    return nullptr;
  }

  HELIOS_MEMORY_PROFILE_ALLOC(raw, total_size, "FreeListAllocator");

  auto* const region = std::construct_at(static_cast<RegionHeader*>(raw));
  region->buffer = static_cast<std::byte*>(raw) + header_size;
  region->capacity = capacity;
  region->next.store(nullptr, std::memory_order_relaxed);
  return region;
}

void FreeListAllocator::FreeRegions(RegionHeader* region) noexcept {
  if (region == nullptr) [[unlikely]] {
    return;
  }

  while (region != nullptr) {
    RegionHeader* const next = region->next.load(std::memory_order_relaxed);
    details::RetireRegionAllocation(region);
    region = next;
  }
}

void* FreeListAllocator::AllocateLocked(size_t bytes,
                                        size_t alignment) noexcept {
  constexpr size_t kHeaderSize = sizeof(AllocationHeader);
  const size_t effective_alignment = std::max(alignment, kMinAlignment);
  const size_t storage_alignment =
      std::max(effective_alignment, alignof(AllocationHeader));

  FreeBlockHeader* best = nullptr;
  FreeBlockHeader* best_prev = nullptr;
  size_t best_padding = 0;
  size_t best_total = std::numeric_limits<size_t>::max();

  FreeBlockHeader* current = free_list_;
  FreeBlockHeader* prev = nullptr;

  while (current != nullptr) {
    auto* const block_start = reinterpret_cast<std::byte*>(current);
    const size_t padding =
        CalculatePaddingWithHeader(block_start, storage_alignment, kHeaderSize);
    const size_t total = SaturatingAdd(padding, bytes);

    if (current->size >= total && total < best_total) {
      best = current;
      best_prev = prev;
      best_padding = padding;
      best_total = total;
      if (current->size == total) {
        break;
      }
    }

    prev = current;
    current = current->next;
  }

  if (best == nullptr) [[unlikely]] {
    return nullptr;
  }

  if (best_prev != nullptr) {
    best_prev->next = best->next;
  } else {
    free_list_ = best->next;
  }
  free_block_count_.fetch_sub(1, std::memory_order_relaxed);

  const size_t remaining = best->size - best_total;
  constexpr size_t kMinSplit =
      sizeof(FreeBlockHeader) + alignof(FreeBlockHeader);
  if (remaining >= kMinSplit) {
    auto* const split_raw = reinterpret_cast<std::byte*>(best) + best_total;
    const size_t split_pad =
        CalculatePadding(split_raw, alignof(FreeBlockHeader));
    auto* const split_block =
        std::launder(reinterpret_cast<FreeBlockHeader*>(split_raw + split_pad));
    const size_t split_size =
        remaining >= split_pad ? remaining - split_pad : 0;
    if (split_size >= sizeof(FreeBlockHeader) + 1) {
      split_block->size = split_size;
      split_block->next = nullptr;
      InsertAndCoalesceLocked(split_block);
    }
  }

  auto* const user_ptr = reinterpret_cast<std::byte*>(best) + best_padding;
  auto* const header =
      std::launder(reinterpret_cast<AllocationHeader*>(user_ptr - kHeaderSize));
  header->size = best_total;
  header->padding = best_padding;
  alignment_waste_.fetch_add(
      best_padding > kHeaderSize ? best_padding - kHeaderSize : 0,
      std::memory_order_relaxed);
  return user_ptr;
}

void FreeListAllocator::DeallocateLocked(void* ptr) noexcept {
  auto* const header = std::launder(reinterpret_cast<AllocationHeader*>(
      static_cast<std::byte*>(ptr) - sizeof(AllocationHeader)));
  auto* const block_begin = static_cast<std::byte*>(ptr) - header->padding;
  auto* const free_block =
      std::launder(reinterpret_cast<FreeBlockHeader*>(block_begin));
  free_block->size = header->size;
  free_block->next = nullptr;
  InsertAndCoalesceLocked(free_block);
}

bool FreeListAllocator::EnsureCapacity(size_t min_capacity) noexcept {
  const size_t current_capacity = capacity_.load(std::memory_order_relaxed);
  const size_t next_capacity = growth_.NextCapacity(
      current_capacity, SaturatingAdd(current_capacity, min_capacity));
  if (next_capacity <= current_capacity) {
    return false;
  }

  auto expected = GrowState::kIdle;
  if (!grow_state_.compare_exchange_strong(expected, GrowState::kGrowing,
                                           std::memory_order_acq_rel,
                                           std::memory_order_acquire)) {
    grow_state_.wait(GrowState::kGrowing, std::memory_order_acquire);
    return true;
  }

  const size_t region_capacity = next_capacity - current_capacity;
  RegionHeader* const region = CreateRegion(region_capacity);
  if (region == nullptr) [[unlikely]] {
    grow_state_.store(GrowState::kIdle, std::memory_order_release);
    grow_state_.notify_all();
    return false;
  }

  {
    const std::scoped_lock lock(mutex_);
    HELIOS_MEMORY_PROFILE_LOCK_MARK(mutex_);
    RegionHeader* observed = regions_.load(std::memory_order_acquire);
    do {
      region->next.store(observed, std::memory_order_relaxed);
    } while (!regions_.compare_exchange_weak(observed, region,
                                             std::memory_order_release,
                                             std::memory_order_acquire));

    capacity_.fetch_add(region_capacity, std::memory_order_relaxed);
    InitializeRegionLocked(*region);
  }

  grow_state_.store(GrowState::kIdle, std::memory_order_release);
  grow_state_.notify_all();
  return true;
}

void FreeListAllocator::InsertAndCoalesceLocked(
    FreeBlockHeader* block) noexcept {
  const auto block_addr = reinterpret_cast<uintptr_t>(block);
  FreeBlockHeader* prev = nullptr;
  FreeBlockHeader* current = free_list_;

  while (current != nullptr &&
         reinterpret_cast<uintptr_t>(current) < block_addr) {
    prev = current;
    current = current->next;
  }

  block->next = current;
  if (prev != nullptr) {
    prev->next = block;
  } else {
    free_list_ = block;
  }
  free_block_count_.fetch_add(1, std::memory_order_relaxed);

  if (current != nullptr) {
    auto* const block_end = reinterpret_cast<std::byte*>(block) + block->size;
    if (block_end == reinterpret_cast<std::byte*>(current)) {
      block->size += current->size;
      block->next = current->next;
      free_block_count_.fetch_sub(1, std::memory_order_relaxed);
    }
  }

  if (prev != nullptr) {
    auto* const prev_end = reinterpret_cast<std::byte*>(prev) + prev->size;
    if (prev_end == reinterpret_cast<std::byte*>(block)) {
      prev->size += block->size;
      prev->next = block->next;
      free_block_count_.fetch_sub(1, std::memory_order_relaxed);
    }
  }
}

void FreeListAllocator::InitializeRegionLocked(RegionHeader& region) noexcept {
  auto* const block = static_cast<FreeBlockHeader*>(region.buffer);
  block->size = region.capacity;
  block->next = nullptr;
  InsertAndCoalesceLocked(block);
}

void* FreeListAllocator::do_allocate(size_t bytes, size_t alignment) {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::FreeListAllocator::do_allocate");
  HELIOS_MEMORY_PROFILE_ZONE_VALUE(bytes);

  if (bytes == 0) [[unlikely]] {
    return nullptr;
  }

  HELIOS_VERIFY(IsPowerOfTwo(alignment),
                "alignment must be a power of two, got {}!", alignment);

  void* result = nullptr;
  while (result == nullptr) {
    {
      const std::scoped_lock lock(mutex_);
      HELIOS_MEMORY_PROFILE_LOCK_MARK(mutex_);
      result = AllocateLocked(bytes, alignment);
      if (result != nullptr) {
        break;
      }
    }

    HELIOS_VERIFY(EnsureCapacity(SaturatingAdd(bytes, alignment)),
                  "Free-list allocator out of memory and cannot grow!");
  }

  const auto* header = std::launder(reinterpret_cast<const AllocationHeader*>(
      static_cast<const std::byte*>(result) - sizeof(AllocationHeader)));
  const size_t consumed = header->size;

  const size_t used =
      used_memory_.fetch_add(consumed, std::memory_order_relaxed) + consumed;
  details::AccumulatePeak(peak_usage_, used);
  allocation_count_.fetch_add(1, std::memory_order_relaxed);
  total_allocations_.fetch_add(1, std::memory_order_relaxed);

  return result;
}

void FreeListAllocator::do_deallocate(void* ptr, [[maybe_unused]] size_t bytes,
                                      size_t /*alignment*/) {
  HELIOS_MEMORY_PROFILE_SCOPE_N(
      "helios::mem::FreeListAllocator::do_deallocate");
  HELIOS_MEMORY_PROFILE_ZONE_VALUE(bytes);

  if (ptr == nullptr) [[unlikely]] {
    return;
  }

  HELIOS_ASSERT(Owns(ptr), "ptr does not belong to free-list allocator!");

  size_t consumed = 0;
  {
    const std::scoped_lock lock(mutex_);
    HELIOS_MEMORY_PROFILE_LOCK_MARK(mutex_);
    const auto* header = std::launder(reinterpret_cast<const AllocationHeader*>(
        static_cast<const std::byte*>(ptr) - sizeof(AllocationHeader)));
    consumed = header->size;
    DeallocateLocked(ptr);
  }

  used_memory_.fetch_sub(consumed, std::memory_order_relaxed);
  allocation_count_.fetch_sub(1, std::memory_order_relaxed);
  total_deallocations_.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace helios::mem
