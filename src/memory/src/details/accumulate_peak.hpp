#pragma once

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

}  // namespace helios::mem::details
