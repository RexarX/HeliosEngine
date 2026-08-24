#pragma once

#include <atomic>
#include <cstddef>
#include <thread>

namespace helios::mem::test {

/**
 * @brief Yielding barrier for tight multi-round tests.
 * @details Apple's libc++ `std::barrier` waits with a timed nanosleep backoff.
 * Thousands of arrivals in one TEST_CASE exceed the macOS CI per-case timeout
 * even when the work between waits is a single lock-free pop. This barrier
 * parks waiters on `std::this_thread::yield()` so the last arriver can run
 * under oversubscription without sleeping a millisecond per round.
 */
class SpinBarrier final {
public:
  explicit SpinBarrier(ptrdiff_t count) noexcept : count_(count) {}

  void ArriveAndWait() noexcept {
    const ptrdiff_t gen = generation_.load(std::memory_order_acquire);
    if (arrived_.fetch_add(1, std::memory_order_acq_rel) + 1 == count_) {
      arrived_.store(0, std::memory_order_relaxed);
      generation_.store(gen + 1, std::memory_order_release);
      return;
    }

    while (generation_.load(std::memory_order_acquire) == gen) {
      std::this_thread::yield();
    }
  }

private:
  ptrdiff_t count_ = 0;
  std::atomic<ptrdiff_t> arrived_{0};
  std::atomic<ptrdiff_t> generation_{0};
};

}  // namespace helios::mem::test
