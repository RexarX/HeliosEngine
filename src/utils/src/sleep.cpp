#include <pch.hpp>

#include <helios/utils/sleep.hpp>

#include <algorithm>
#include <chrono>
#include <thread>

#ifdef HELIOS_PLATFORM_WINDOWS
#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif
#elif defined(HELIOS_PLATFORM_LINUX) || defined(HELIOS_PLATFORM_MACOS)
#include <cerrno>
#include <ctime>
#endif

#include <helios/platform/platform.hpp>

#ifdef HELIOS_PLATFORM_WINDOWS
#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif
#endif

using namespace std::chrono_literals;

namespace helios::utils {

namespace {

#ifdef HELIOS_PLATFORM_WINDOWS

[[nodiscard]] HANDLE HighResolutionTimer() {
  // Intentionally never closed: thread_local storage duration means the OS
  // reclaims this handle on thread exit. Reused across calls on the same
  // thread to avoid per-call CreateWaitableTimerEx overhead.
  thread_local HANDLE timer = []() -> HANDLE {
    HANDLE handle = CreateWaitableTimerExW(
        nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
        TIMER_ALL_ACCESS);
    if (handle == nullptr) {
      handle = CreateWaitableTimerW(nullptr, TRUE, nullptr);
    }
    return handle;
  }();
  return timer;
}

void WindowsSleepFor(std::chrono::nanoseconds duration) {
  HANDLE timer = HighResolutionTimer();
  if (timer != nullptr) {
    LARGE_INTEGER due{};
    due.QuadPart = -std::max<LONGLONG>(1, duration.count() / 100);
    if (SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE) != FALSE) {
      WaitForSingleObject(timer, INFINITE);
      return;
    }
  }

  std::this_thread::sleep_for(duration);
}

#elif defined(HELIOS_PLATFORM_LINUX) || defined(HELIOS_PLATFORM_MACOS)

void UnixSleepFor(std::chrono::nanoseconds duration) {
  timespec remaining{
      .tv_sec = static_cast<time_t>(duration.count() / 1'000'000'000),
      .tv_nsec = static_cast<long>(duration.count() % 1'000'000'000),
  };

  while (true) {
    timespec requested = remaining;
    if (nanosleep(&requested, &remaining) == 0) {
      return;
    }
    if (errno != EINTR) {
      const auto remaining_ns =
          std::chrono::nanoseconds{remaining.tv_sec} * 1'000'000'000 +
          std::chrono::nanoseconds{remaining.tv_nsec};
      std::this_thread::sleep_for(remaining_ns);
      return;
    }
  }
}

#ifdef HELIOS_PLATFORM_LINUX

void LinuxSleepUntil(std::chrono::steady_clock::time_point deadline) {
  const auto now = std::chrono::steady_clock::now();
  if (now >= deadline) {
    return;
  }

  const auto remaining =
      std::chrono::duration_cast<std::chrono::nanoseconds>(deadline - now);

  timespec ts{};
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
    UnixSleepFor(remaining);
    return;
  }

  ts.tv_sec += static_cast<time_t>(remaining.count() / 1'000'000'000);
  ts.tv_nsec += static_cast<long>(remaining.count() % 1'000'000'000);
  if (ts.tv_nsec >= 1'000'000'000L) {
    ++ts.tv_sec;
    ts.tv_nsec -= 1'000'000'000L;
  }

  while (true) {
    const int err =
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, nullptr);
    if (err == 0) {
      return;
    }
    if (err != EINTR) {
      UnixSleepFor(remaining);
      return;
    }
  }
}

#endif

#endif

}  // namespace

void SleepFor(std::chrono::nanoseconds duration) {
  if (duration.count() <= 0) {
    return;
  }

#ifdef HELIOS_PLATFORM_WINDOWS
  WindowsSleepFor(duration);
#elif defined(HELIOS_PLATFORM_LINUX) || defined(HELIOS_PLATFORM_MACOS)
  UnixSleepFor(duration);
#else
  std::this_thread::sleep_for(duration);
#endif
}

void SleepUntil(std::chrono::steady_clock::time_point deadline) {
#ifdef HELIOS_PLATFORM_LINUX
  LinuxSleepUntil(deadline);
#else
  const auto now = std::chrono::steady_clock::now();
  if (now >= deadline) {
    return;
  }
  SleepFor(
      std::chrono::duration_cast<std::chrono::nanoseconds>(deadline - now));
#endif
}

void PreciseSleepUntil(std::chrono::steady_clock::time_point deadline,
                       bool pinned_thread) {
  using Clock = std::chrono::steady_clock;

  auto current = Clock::now();
  if (current >= deadline) {
    return;
  }

  constexpr auto kShortSleep = 1'000'000ns;
  constexpr auto kMaxSleepCap = 3'000'000ns;
  auto max_sleep = kShortSleep;

  while (current + max_sleep < deadline) {
    SleepFor(kShortSleep);
    const auto now = Clock::now();
    const auto next_sleep = now - current;
    max_sleep = std::clamp(next_sleep, kShortSleep, kMaxSleepCap);
    current = now;
  }

  if (current < deadline) {
    const auto remaining = deadline - current;
    const auto overshoot = max_sleep - kShortSleep;
    if (remaining > overshoot) {
      SleepFor(std::chrono::duration_cast<std::chrono::nanoseconds>(remaining -
                                                                    overshoot));
      current = Clock::now();
    }
  }

  while (current + kShortSleep < deadline) {
    SleepFor(kShortSleep);
    current = Clock::now();
  }

  while (current < deadline) {
    if (pinned_thread) [[likely]] {
      HELIOS_PAUSE_CPU();
    } else {
      std::this_thread::yield();
    }
    current = Clock::now();
  }
}

void PreciseSleep(std::chrono::nanoseconds duration, bool pinned_thread) {
  if (duration.count() <= 0) {
    return;
  }
  PreciseSleepUntil(std::chrono::steady_clock::now() + duration, pinned_thread);
}

}  // namespace helios::utils
