#pragma once

#include <chrono>

namespace helios::utils {

/**
 * @brief Performs one OS-level wait for `duration`.
 * @details Zero or negative durations return immediately. Interrupted waits
 * (`EINTR`) are restarted.
 * @param duration Relative time to sleep
 */
void SleepFor(std::chrono::nanoseconds duration);

/**
 * @brief Performs one OS-level wait until `deadline`.
 * @details Past deadlines return immediately.
 * @param deadline Absolute `steady_clock` time to wait for
 */
void SleepUntil(std::chrono::steady_clock::time_point deadline);

/**
 * @brief Sleeps for `duration` with SDL_DelayPrecise-style accuracy.
 * @details Sleeps in ~1 ms chunks while tracking scheduler overshoot, then
 * busy-waits the remainder with a CPU pause. Zero or negative durations
 * return immediately.
 * @param duration Relative time to sleep
 * @param pinned_thread If true (default), the final sub-millisecond busy-wait
 * uses a CPU pause/yield hint (`HELIOS_PAUSE_CPU`), which keeps the calling
 * thread runnable and holding its core the whole time => lowest wakeup
 * latency, but only appropriate when the caller owns a dedicated core (e.g.
 * a pinned render/simulation thread).
 * If false, the tail loop instead calls `std::this_thread::yield()`,
 * which relinquishes the thread's timeslice to the OS scheduler on each spin.
 * Use this when the calling thread may share a core with other runnable work
 * (e.g. a thread-pool worker), so this sleep doesn't starve unrelated threads
 * of scheduler time.
 */
void PreciseSleep(std::chrono::nanoseconds duration, bool pinned_thread = true);

/**
 * @brief Sleeps until `deadline` with SDL_DelayPrecise-style accuracy.
 * @details Past deadlines return immediately.
 * @param deadline Absolute `steady_clock` time to wait for
 * @param pinned_thread If true (default), the final sub-millisecond busy-wait
 * uses a CPU pause/yield hint (`HELIOS_PAUSE_CPU`), which keeps the calling
 * thread runnable and holding its core the whole time => lowest wakeup
 * latency, but only appropriate when the caller owns a dedicated core (e.g.
 * a pinned render/simulation thread).
 * If false, the tail loop instead calls `std::this_thread::yield()`,
 * which relinquishes the thread's timeslice to the OS scheduler on each spin.
 * Use this when the calling thread may share a core with other runnable work
 * (e.g. a thread-pool worker), so this sleep doesn't starve unrelated threads
 * of scheduler time.
 */
void PreciseSleepUntil(std::chrono::steady_clock::time_point deadline,
                       bool pinned_thread = true);

}  // namespace helios::utils
