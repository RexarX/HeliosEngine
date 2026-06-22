#pragma once

#include <helios/app/application.hpp>
#include <helios/app/sub_app.hpp>

#include <algorithm>
#include <chrono>
#include <optional>
#include <thread>

namespace helios::app {

/// @brief Configuration for fixed-timestep runners.
struct FixedRunnerConfig {
  static constexpr auto kDefaultInterval =
      std::chrono::nanoseconds{16'666'667};  // ~60 FPS default

  /// Time to wait between the start of consecutive updates.
  std::chrono::nanoseconds update_interval = kDefaultInterval;

  /**
   * @brief Creates a config targeting the given frames per second.
   * @param fps Target frames per second (must be > 0)
   * @return FixedRunnerConfig with the corresponding update interval
   */
  [[nodiscard]] static constexpr auto FromFPS(size_t fps) noexcept
      -> FixedRunnerConfig {
    constexpr size_t kNanosecsInSec = 1'000'000'000;
    return {.update_interval = std::chrono::nanoseconds{kNanosecsInSec / fps}};
  }

  /**
   * @brief Creates a config targeting the given update frequency in Hz.
   * @param hz Target frequency in Hz (must be > 0)
   * @return FixedRunnerConfig with the corresponding update interval
   */
  [[nodiscard]] static constexpr auto FromHz(double hz) noexcept
      -> FixedRunnerConfig {
    auto nanosec = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::duration<double>{1.0 / hz});
    return {.update_interval = nanosec};
  }

  /**
   * @brief Creates a config with the given update interval.
   * @tparam Rep Duration representation type
   * @tparam Period Duration period type
   * @param interval Update interval as a `std::chrono::duration`
   * @return FixedRunnerConfig with the given interval
   */
  template <typename Rep, typename Period>
  [[nodiscard]] static constexpr auto FromInterval(
      std::chrono::duration<Rep, Period> interval) noexcept
      -> FixedRunnerConfig {
    auto nanosec =
        std::chrono::duration_cast<std::chrono::nanoseconds>(interval);
    return {.update_interval = nanosec};
  }
};

/**
 * @brief Runs the application until an `AppExit` message is received.
 * @param app Application to update
 * @return Exit code from the first `AppExit` message
 */
inline ExitCode RunDefault(App& app) {
  std::optional<ExitCode> exit_code;
  while (exit_code = app.ShouldExit(), !exit_code.has_value()) {
    app.Update();
  }
  return *exit_code;
}

/**
 * @brief Runs the application with a fixed timestep.
 * @details Uses `sleep_until` against an advancing absolute target time point
 * so that scheduler wake-up overshoot on one frame is automatically absorbed by
 * a shorter sleep on the next, preventing drift accumulation.
 * @param app Application to update
 * @param config Fixed-runner configuration
 * @return Exit code from the first `AppExit` message
 */
inline ExitCode RunFixed(App& app, const FixedRunnerConfig& config = {}) {
  auto next_tick = std::chrono::steady_clock::now();

  std::optional<ExitCode> exit_code;
  while (exit_code = app.ShouldExit(), !exit_code.has_value()) {
    next_tick += config.update_interval;

    app.Update();

    std::this_thread::sleep_until(next_tick);
    // Prevent unbounded catch-up after a long stall (e.g. debugger pause)
    next_tick = std::max(next_tick, std::chrono::steady_clock::now());
  }
  return *exit_code;
}

/**
 * @brief Runs a single application frame.
 * @param app Application to update
 * @return Exit code from an `AppExit` message, or `ExitCode::kSuccess`
 */
inline ExitCode RunOnce(App& app) {
  app.Update();
  return app.ShouldExit().value_or(ExitCode::kSuccess);
}

/**
 * @brief Runs the sub-app update loop until `SubApp::ShouldExit` is true.
 * @details Intended for async sub-apps via `SubApp::SetRunner`.
 * @param sub_app Sub-app to update
 * @param executor Async executor used to build and run schedules
 */
inline void RunDefaultSubApp(SubApp& sub_app, async::Executor& executor) {
  while (!sub_app.ShouldExit()) {
    sub_app.Update(executor);
  }
}

/**
 * @brief Runs the sub-app with a fixed timestep.
 * @details Intended for async sub-apps via `SubApp::SetRunner`.
 * Uses `sleep_until` against an advancing absolute target time point so that
 * scheduler wake-up overshoot on one frame is automatically absorbed by a
 * shorter sleep on the next, preventing drift accumulation.
 * @param sub_app Sub-app to update
 * @param executor Async executor used to build and run schedules
 * @param config Fixed-runner configuration
 */
inline void RunFixedSubApp(SubApp& sub_app, async::Executor& executor,
                           const FixedRunnerConfig& config = {}) {
  auto next_tick = std::chrono::steady_clock::now();

  while (!sub_app.ShouldExit()) {
    next_tick += config.update_interval;

    sub_app.Update(executor);

    std::this_thread::sleep_until(next_tick);
    // Prevent unbounded catch-up after a long stall (e.g. debugger pause)
    next_tick = std::max(next_tick, std::chrono::steady_clock::now());
  }
}

/**
 * @brief Runs a single sub-app update stage.
 * @details Intended for `SubApp::SetRunner` or blocking per-frame updates.
 * @param sub_app Sub-app to update
 * @param executor Async executor used to build and run schedules
 */
inline void RunOnceSubApp(SubApp& sub_app, async::Executor& executor) {
  sub_app.Update(executor);
}

}  // namespace helios::app
