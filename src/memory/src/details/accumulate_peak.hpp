#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>

namespace helios::mem::details {

inline void AccumulatePeak(std::atomic<size_t>& peak,
                           size_t candidate) noexcept {
  size_t observed = peak.load(std::memory_order_relaxed);
  while (candidate > observed) {
    if (peak.compare_exchange_weak(observed, candidate,
                                   std::memory_order_relaxed,
                                   std::memory_order_relaxed)) {
      return;
    }
  }
}

inline void SaturatingFetchSub(std::atomic<size_t>& value,
                               size_t amount) noexcept {
  if (amount == 0) {
    return;
  }

  size_t current = value.load(std::memory_order_relaxed);
  while (true) {
    const size_t next = current - std::min(amount, current);
    if (value.compare_exchange_weak(current, next, std::memory_order_relaxed)) {
      return;
    }
  }
}

}  // namespace helios::mem::details
