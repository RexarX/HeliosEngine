#pragma once

namespace helios::mem::details {

/// Pins the calling thread in the current global epoch while scanning regions.
class DeferredRegionEpochGuard {
public:
  DeferredRegionEpochGuard() noexcept;
  DeferredRegionEpochGuard(const DeferredRegionEpochGuard&) = delete;
  DeferredRegionEpochGuard& operator=(const DeferredRegionEpochGuard&) = delete;
  ~DeferredRegionEpochGuard() noexcept;

  DeferredRegionEpochGuard& operator=(DeferredRegionEpochGuard&) = delete;
  DeferredRegionEpochGuard& operator=(DeferredRegionEpochGuard&&) = delete;
};

/// Defers backing-region teardown until a later epoch is reclaimed.
void RetireRegionAllocation(void* raw_region) noexcept;

/// Bumps the global epoch and reclaims regions retired two epochs ago.
/// @details Call at known quiescence points (frame barrier, schedule flush).
void SynchronizeDeferredRegions() noexcept;

/// Reclaims all deferred regions after active region scans finish.
void FlushDeferredRegions() noexcept;

}  // namespace helios::mem::details
