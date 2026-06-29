#include <details/deferred_region_ebr.hpp>

#include <helios/memory/aligned_alloc.hpp>
#include <helios/memory/details/profile.hpp>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

namespace helios::mem::details {

namespace {

constexpr size_t kEpochRingSize = 3;
constexpr uint64_t kEpochInactive = 0;

thread_local uint64_t tls_active_epoch = kEpochInactive;

struct DeferredRegionState {
  std::atomic<uint64_t> global_epoch{1};
  std::atomic<size_t> active_readers{0};
  std::mutex retire_mutex;
  std::vector<void*> retire_ring[kEpochRingSize];

  static DeferredRegionState& Instance() noexcept {
    static DeferredRegionState state;
    return state;
  }
};

void FreeRetiredRegion(void* raw_region) noexcept {
  if (raw_region == nullptr) [[unlikely]] {
    return;
  }

  HELIOS_MEMORY_PROFILE_FREE(raw_region, "FreeListAllocator");
  AlignedFree(raw_region, false);
}

void WaitForInactiveReaders() noexcept {
  auto& state = DeferredRegionState::Instance();
  while (state.active_readers.load(std::memory_order_acquire) != 0) {
    std::this_thread::yield();
  }
}

void ReclaimSlot(size_t slot) noexcept {
  auto& state = DeferredRegionState::Instance();
  std::vector<void*> to_free;
  {
    const std::scoped_lock lock(state.retire_mutex);
    to_free.swap(state.retire_ring[slot]);
  }
  for (void* const raw : to_free) {
    FreeRetiredRegion(raw);
  }
}

}  // namespace

DeferredRegionEpochGuard::DeferredRegionEpochGuard() noexcept {
  auto& state = DeferredRegionState::Instance();
  tls_active_epoch = state.global_epoch.load(std::memory_order_acquire);
  state.active_readers.fetch_add(1, std::memory_order_acq_rel);
}

DeferredRegionEpochGuard::~DeferredRegionEpochGuard() noexcept {
  auto& state = DeferredRegionState::Instance();
  state.active_readers.fetch_sub(1, std::memory_order_acq_rel);
  tls_active_epoch = kEpochInactive;
}

void RetireRegionAllocation(void* raw_region) noexcept {
  if (raw_region == nullptr) [[unlikely]] {
    return;
  }

  auto& state = DeferredRegionState::Instance();
  const uint64_t epoch = state.global_epoch.load(std::memory_order_relaxed);
  const std::scoped_lock lock(state.retire_mutex);
  state.retire_ring[static_cast<size_t>(epoch % kEpochRingSize)].push_back(
      raw_region);
}

void SynchronizeDeferredRegions() noexcept {
  WaitForInactiveReaders();

  auto& state = DeferredRegionState::Instance();
  const uint64_t new_epoch =
      state.global_epoch.fetch_add(1, std::memory_order_acq_rel) + 1;
  const auto reclaim_slot = static_cast<size_t>(
      (new_epoch >= 2 ? new_epoch - 2 : 0) % kEpochRingSize);
  ReclaimSlot(reclaim_slot);
}

void FlushDeferredRegions() noexcept {
  WaitForInactiveReaders();

  auto& state = DeferredRegionState::Instance();
  std::vector<void*> to_free;
  {
    const std::scoped_lock lock(state.retire_mutex);
    for (auto& bucket : state.retire_ring) {
      to_free.insert(to_free.end(), bucket.begin(), bucket.end());
      bucket.clear();
    }
  }
  for (void* const raw : to_free) {
    FreeRetiredRegion(raw);
  }
}

}  // namespace helios::mem::details
